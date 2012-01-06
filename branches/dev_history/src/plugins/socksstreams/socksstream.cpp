#include "socksstream.h"

#include <QEvent>
#include <QReadLocker>
#include <QWriteLocker>
#include <QCoreApplication>
#include <QNetworkInterface>

#define HOST_REQUEST_TIMEOUT      120000
#define PROXY_REQUEST_TIMEOUT     10000
#define ACTIVATE_REQUEST_TIMEOUT  10000

#define BUFFER_INCREMENT_SIZE     5120
#define MAX_BUFFER_SIZE           51200

#define SHC_HOSTS                 "/iq[@type='set']/query[@xmlns='" NS_SOCKS5_BYTESTREAMS "']"

enum NegotiationCommands
{
	NCMD_START_NEGOTIATION,
	NCMD_REQUEST_PROXY_ADDRESS,
	NCMD_SEND_AVAIL_HOSTS,
	NCMD_CONNECT_TO_HOST,
	NCMD_CHECK_NEXT_HOST,
	NCMD_ACTIVATE_STREAM,
	NCMD_START_STREAM
};

class DataEvent :
			public QEvent
{
public:
	DataEvent(bool ARead, bool AWrite, bool AFlush) : QEvent(FEventType) {
		FRead=ARead;
		FWrite=AWrite;
		FFlush = AFlush;
	}
	inline bool isRead() { return FRead; }
	inline bool isWrite() {return FWrite; }
	inline bool isFlush() { return FFlush; }
	static int registeredType() { return FEventType; }
private:
	bool FRead;
	bool FWrite;
	bool FFlush;
	static QEvent::Type FEventType;
};

QEvent::Type DataEvent::FEventType = static_cast<QEvent::Type>(QEvent::registerEventType());

SocksStream::SocksStream(ISocksStreams *ASocksStreams, IStanzaProcessor *AStanzaProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent)
		: QIODevice(AParent), FReadBuffer(BUFFER_INCREMENT_SIZE), FWriteBuffer(BUFFER_INCREMENT_SIZE,MAX_BUFFER_SIZE)
{
	FSocksStreams = ASocksStreams;
	FStanzaProcessor = AStanzaProcessor;

	FStreamId = AStreamId;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FStreamKind = AKind;
	FStreamState = IDataStreamSocket::Closed;

	FTcpSocket = NULL;
	FDisableDirectConnect = false;
	FErrorCode = IDataStreamSocket::NoError;

	FSHIHosts= -1;

	connect(FSocksStreams->instance(),SIGNAL(localConnectionAccepted(const QString &, QTcpSocket *)),SLOT(onLocalConnectionAccepted(const QString &, QTcpSocket *)));
}

SocksStream::~SocksStream()
{
	abort(tr("Stream destroyed"));
	delete FTcpSocket;
}

bool SocksStream::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	QDomElement queryElem = AStanza.firstElement("query",NS_SOCKS5_BYTESTREAMS);
	if (AHandleId==FSHIHosts && queryElem.attribute("sid")==FStreamId)
	{
		AAccept = true;
		if (streamState()==IDataStreamSocket::Opening && queryElem.attribute("mode")!="udp")
		{
			FHosts.clear();
			FHostIndex = 0;
			FHostRequest = AStanza.id();

			if (queryElem.hasAttribute("dstaddr"))
				FConnectKey = queryElem.attribute("dstaddr");

			QDomElement hostElem = queryElem.firstChildElement("streamhost");
			while (!hostElem.isNull())
			{
				HostInfo info;
				info.jid = hostElem.attribute("jid");
				info.name = hostElem.attribute("host");
				info.port = hostElem.attribute("port").toInt();
				if (info.jid.isValid() && !info.name.isEmpty() && info.port>0)
					FHosts.append(info);
				hostElem = hostElem.nextSiblingElement("streamhost");
			}
			negotiateConnection(NCMD_CHECK_NEXT_HOST);
		}
		else
		{
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,ErrorHandler("not-acceptable"));
			error.element().removeChild(error.firstElement("query"));
			FStanzaProcessor->sendStanzaOut(AStreamJid, error);
			abort(tr("Unsupported stream mode"));
		}
		removeStanzaHandle(FSHIHosts);
	}
	return false;
}

void SocksStream::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (FProxyRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QDomElement hostElem = AStanza.firstElement("query",NS_SOCKS5_BYTESTREAMS).firstChildElement("streamhost");
			if (!hostElem.isNull())
			{
				HostInfo info;
				info.jid = hostElem.attribute("jid");
				info.name = hostElem.attribute("host");
				info.port = hostElem.attribute("port").toInt();
				if (info.jid.isValid() && !info.name.isEmpty() && info.port>0)
					FHosts.append(info);
			}
		}
		FProxyRequests.removeAll(AStanza.id());
		if (FProxyRequests.isEmpty())
			negotiateConnection(NCMD_SEND_AVAIL_HOSTS);
	}
	else if (AStanza.id() == FHostRequest)
	{
		if (AStanza.type() == "result")
		{
			QDomElement hostElem = AStanza.firstElement("query",NS_SOCKS5_BYTESTREAMS).firstChildElement("streamhost-used");
			Jid hostJid = hostElem.attribute("jid");
			for (FHostIndex = 0; FHostIndex<FHosts.count() && FHosts.at(FHostIndex).jid!=hostJid; FHostIndex++);
			negotiateConnection(NCMD_CONNECT_TO_HOST);
		}
		else
		{
			abort(tr("Remote client cant connect to given hosts"));
		}
	}
	else if (AStanza.id() == FActivateRequest)
	{
		if (AStanza.type() == "result")
			negotiateConnection(NCMD_START_STREAM);
		else
			abort(tr("Failed to activate stream"));
	}
}

bool SocksStream::isSequential() const
{
	return true;
}

qint64 SocksStream::bytesAvailable() const
{
	QReadLocker locker(&FThreadLock);
	return FReadBuffer.size();
}

qint64 SocksStream::bytesToWrite() const
{
	QReadLocker locker(&FThreadLock);
	return FWriteBuffer.size();
}

bool SocksStream::waitForBytesWritten(int AMsecs)
{
	bool isWritten = false;
	if (streamState()!=IDataStreamSocket::Closed)
	{
		FThreadLock.lockForWrite();
		isWritten = FBytesWrittenCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
		FThreadLock.unlock();
	}
	return isWritten && isOpen();
}

bool SocksStream::waitForReadyRead(int AMsecs)
{
	if (streamState()!=IDataStreamSocket::Closed && bytesAvailable()==0)
	{
		FThreadLock.lockForWrite();
		FReadyReadCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
		FThreadLock.unlock();
	}
	return bytesAvailable()>0;
}

QString SocksStream::methodNS() const
{
	return NS_SOCKS5_BYTESTREAMS;
}

QString SocksStream::streamId() const
{
	return FStreamId;
}

Jid SocksStream::streamJid() const
{
	return FStreamJid;
}

Jid SocksStream::contactJid() const
{
	return FContactJid;
}

int SocksStream::streamKind() const
{
	return FStreamKind;
}

int SocksStream::streamState() const
{
	QReadLocker locker(&FThreadLock);
	return FStreamState;
}

bool SocksStream::isOpen() const
{
	QReadLocker locker(&FThreadLock);
	return FStreamState==IDataStreamSocket::Opened;
}

bool SocksStream::open(QIODevice::OpenMode AMode)
{
	if (streamState() == IDataStreamSocket::Closed)
	{
		setStreamError(QString::null,NoError);
		if (negotiateConnection(NCMD_START_NEGOTIATION))
		{
			setOpenMode(AMode);
			setStreamState(IDataStreamSocket::Opening);
			return true;
		}
	}
	return false;
}

bool SocksStream::flush()
{
	if (isOpen() && bytesToWrite()>0)
	{
		DataEvent *dataEvent = new DataEvent(true,true,true);
		QCoreApplication::postEvent(this,dataEvent);
		return true;
	}
	return false;
}

void SocksStream::close()
{
	int state = streamState();
	if (FTcpSocket && state==IDataStreamSocket::Opened)
	{
		emit aboutToClose();
		writeBufferedData(true);
		setStreamState(IDataStreamSocket::Closing);
		FTcpSocket->disconnectFromHost();
	}
	else if (state != IDataStreamSocket::Closing)
	{
		setStreamState(IDataStreamSocket::Closed);
	}
}

void SocksStream::abort(const QString &AError, int ACode)
{
	if (streamState() != IDataStreamSocket::Closed)
	{
		setStreamError(AError, ACode);
		close();
		setStreamState(IDataStreamSocket::Closed);
	}
}

int SocksStream::errorCode() const
{
	QReadLocker locker(&FThreadLock);
	return FErrorCode;
}

QString SocksStream::errorString() const
{
	QReadLocker locker(&FThreadLock);
	return QIODevice::errorString();
}

bool SocksStream::disableDirectConnection() const
{
	return FDisableDirectConnect;
}

void SocksStream::setDisableDirectConnection(bool ADisable)
{
	if (FDisableDirectConnect != ADisable)
	{
		FDisableDirectConnect = ADisable;
		emit propertiesChanged();
	}
}

QString SocksStream::forwardHost() const
{
	return FForwardHost;
}

quint16 SocksStream::forwardPort() const
{
	return FForwardPort;
}

void SocksStream::setForwardAddress(const QString &AHost, quint16 APort)
{
	if (FForwardHost!=AHost || FForwardPort!=APort)
	{
		FForwardHost = AHost;
		FForwardPort = APort;
		emit propertiesChanged();
	}
}

QNetworkProxy SocksStream::networkProxy() const
{
	return FNetworkProxy;
}

void SocksStream::setNetworkProxy(const QNetworkProxy &AProxy)
{
	if (FNetworkProxy != AProxy)
	{
		FNetworkProxy = AProxy;
		emit propertiesChanged();
	}
}

QList<QString> SocksStream::proxyList() const
{
	return FProxyList;
}

void SocksStream::setProxyList(const QList<QString> &AProxyList)
{
	if (FProxyList != AProxyList)
	{
		FProxyList = AProxyList;
		emit propertiesChanged();
	}
}

qint64 SocksStream::readData(char *AData, qint64 AMaxSize)
{
	FThreadLock.lockForWrite();
	qint64 bytes = FTcpSocket!=NULL || FReadBuffer.size()>0 ? FReadBuffer.read(AData,AMaxSize) : -1;
	FThreadLock.unlock();

	if (bytes > 0)
	{
		DataEvent *dataEvent = new DataEvent(true,false,false);
		QCoreApplication::postEvent(this,dataEvent);
	}

	return bytes;
}

qint64 SocksStream::writeData(const char *AData, qint64 AMaxSize)
{
	FThreadLock.lockForWrite();
	qint64 bytes = FTcpSocket!=NULL ? FWriteBuffer.write(AData,AMaxSize) : -1;
	FThreadLock.unlock();

	if (bytes > 0)
	{
		DataEvent *dataEvent = new DataEvent(false,true,false);
		QCoreApplication::postEvent(this,dataEvent);
	}

	return bytes;
}

void SocksStream::setOpenMode(OpenMode AMode)
{
	QWriteLocker locker(&FThreadLock);
	QIODevice::setOpenMode(AMode);
}

bool SocksStream::event(QEvent *AEvent)
{
	if (AEvent->type() == DataEvent::registeredType())
	{
		DataEvent *dataEvent = static_cast<DataEvent *>(AEvent);
		if (dataEvent->isRead())
			readBufferedData(dataEvent->isFlush());
		if (dataEvent->isWrite())
			writeBufferedData(dataEvent->isFlush());
		return true;
	}
	return QIODevice::event(AEvent);
}

void SocksStream::setStreamState(int AState)
{
	if (streamState() != AState)
	{
		if (AState == IDataStreamSocket::Opened)
		{
			FThreadLock.lockForWrite();
			QIODevice::open(openMode());
			FThreadLock.unlock();
		}
		else if (AState == IDataStreamSocket::Closed)
		{
			removeStanzaHandle(FSHIHosts);
			FSocksStreams->removeLocalConnection(FConnectKey);
			emit readChannelFinished();

			FThreadLock.lockForWrite();
			QString saveError = QIODevice::errorString();
			QIODevice::close();
			QIODevice::setErrorString(saveError);
			FReadBuffer.clear();
			FWriteBuffer.clear();
			FThreadLock.unlock();

			FReadyReadCondition.wakeAll();
			FBytesWrittenCondition.wakeAll();
		}

		FThreadLock.lockForWrite();
		FStreamState = AState;
		FThreadLock.unlock();

		emit stateChanged(AState);
	}
}

void SocksStream::setStreamError(const QString &AError, int ACode)
{
	if (ACode==NoError || errorCode()==NoError)
	{
		QWriteLocker locker(&FThreadLock);
		FErrorCode = ACode;
		setErrorString(AError);
	}
}

void SocksStream::setTcpSocket(QTcpSocket *ASocket)
{
	if (ASocket)
	{
		connect(ASocket,SIGNAL(readyRead()), SLOT(onTcpSocketReadyRead()));
		connect(ASocket,SIGNAL(bytesWritten(qint64)),SLOT(onTcpSocketBytesWritten(qint64)));
		connect(ASocket,SIGNAL(disconnected()),SLOT(onTcpSocketDisconnected()));
		QWriteLocker locker(&FThreadLock);
		FTcpSocket = ASocket;
	}
}

void SocksStream::writeBufferedData(bool AFlush)
{
	if (FTcpSocket && isOpen())
	{
		FThreadLock.lockForRead();
		qint64 dataSize = AFlush ? FWriteBuffer.size() : qMin((qint64)FWriteBuffer.size(), qint64(MAX_BUFFER_SIZE)-FTcpSocket->bytesToWrite());
		FThreadLock.unlock();

		if (dataSize > 0)
		{
			FThreadLock.lockForWrite();
			QByteArray data = FWriteBuffer.read(dataSize);
			FThreadLock.unlock();
			FBytesWrittenCondition.wakeAll();

			if (FTcpSocket->write(data) != data.size())
				abort("Failed to send data to socket");
			else if (AFlush)
				FTcpSocket->flush();

			emit bytesWritten(data.size());
		}
	}
}

void SocksStream::readBufferedData(bool AFlush)
{
	if (FTcpSocket && isOpen())
	{
		FThreadLock.lockForRead();
		qint64 dataSize = AFlush ? FTcpSocket->bytesAvailable() : qMin(FTcpSocket->bytesAvailable(), qint64(MAX_BUFFER_SIZE)-FReadBuffer.size());
		FThreadLock.unlock();

		if (dataSize > 0)
		{
			FThreadLock.lockForWrite();
			FReadBuffer.write(FTcpSocket->read(dataSize));
			FThreadLock.unlock();

			FReadyReadCondition.wakeAll();
			emit readyRead();
		}
	}
}

int SocksStream::insertStanzaHandle(const QString &ACondition)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = FStreamJid;
		shandle.conditions.append(ACondition);
		return FStanzaProcessor->insertStanzaHandle(shandle);
	}
	return -1;
}

void SocksStream::removeStanzaHandle(int &AHandleId)
{
	if (FStanzaProcessor && AHandleId>0)
	{
		FStanzaProcessor->removeStanzaHandle(AHandleId);
		AHandleId = -1;
	}
}

bool SocksStream::negotiateConnection(int ACommand)
{
	if (ACommand == NCMD_START_NEGOTIATION)
	{
		FHosts.clear();
		FHostIndex = INT_MAX;
		if (streamKind() == IDataStreamSocket::Initiator)
		{
			FConnectKey = FSocksStreams->connectionKey(FStreamId, FStreamJid, FContactJid);
			if (requestProxyAddress() || sendAvailHosts())
				return true;
		}
		else
		{
			FSHIHosts = insertStanzaHandle(SHC_HOSTS);
			if (FSHIHosts >= 0)
			{
				FConnectKey = FSocksStreams->connectionKey(FStreamId, FContactJid, FStreamJid);
				return true;
			}
		}
	}
	else if (streamState() == IDataStreamSocket::Opening)
	{
		if (ACommand == NCMD_SEND_AVAIL_HOSTS)
		{
			if (sendAvailHosts())
				return true;
			abort(tr("Failed to create hosts"));
		}
		else if (ACommand == NCMD_CONNECT_TO_HOST)
		{
			if (FHostIndex < FHosts.count())
			{
				HostInfo info = FHosts.value(FHostIndex);
				if (info.jid == FStreamJid)
				{
					if (FTcpSocket!=NULL)
					{
						setStreamState(IDataStreamSocket::Opened);
						return true;
					}
					abort(tr("Direct connection not established"));
				}
				else
				{
					if (connectToHost())
						return true;

					abort("Invalid host address");
					FSocksStreams->removeLocalConnection(FConnectKey);
				}
			}
			abort(tr("Invalid host"));
		}
		else if (ACommand == NCMD_CHECK_NEXT_HOST)
		{
			if (connectToHost())
				return true;

			sendFailedHosts();
			abort(tr("Cant connect to given hosts"));
		}
		else if (ACommand == NCMD_ACTIVATE_STREAM)
		{
			if (streamKind() == IDataStreamSocket::Initiator)
			{
				if (activateStream())
					return true;
				abort(tr("Failed to activate stream"));
			}
			else
			{
				if (sendUsedHost())
				{
					setStreamState(IDataStreamSocket::Opened);
					return true;
				}
				abort(tr("Failed to activate stream"));
			}
		}
		else if (ACommand == NCMD_START_STREAM)
		{
			setStreamState(IDataStreamSocket::Opened);
			return true;
		}
	}
	return false;
}

bool SocksStream::requestProxyAddress()
{
	bool requested = false;
	foreach(Jid proxy, FProxyList)
	{
		Stanza request("iq");
		request.setType("get").setTo(proxy.eFull()).setId(FStanzaProcessor->newId());
		request.addElement("query",NS_SOCKS5_BYTESTREAMS);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,PROXY_REQUEST_TIMEOUT))
		{
			FProxyRequests.append(request.id());
			requested = true;
		}
	}
	return requested;
}

bool SocksStream::sendAvailHosts()
{
	Stanza request("iq");
	request.setType("set").setTo(FContactJid.eFull()).setId(FStanzaProcessor->newId());

	QDomElement queryElem = request.addElement("query",NS_SOCKS5_BYTESTREAMS);
	queryElem.setAttribute("sid",FStreamId);
	queryElem.setAttribute("mode","tcp");
	queryElem.setAttribute("dstaddr",FConnectKey);

	if (!disableDirectConnection() && FSocksStreams->appendLocalConnection(FConnectKey))
	{
		if (!FForwardHost.isEmpty() && FForwardPort>0)
		{
			HostInfo info;
			info.jid = FStreamJid;
			info.name = FForwardHost;
			info.port = FForwardPort;
			FHosts.prepend(info);
		}
		else foreach(QHostAddress address, QNetworkInterface::allAddresses())
		{
			if (address.protocol()!=QAbstractSocket::IPv6Protocol && address!=QHostAddress::LocalHost)
			{
				HostInfo info;
				info.jid = FStreamJid;
				info.name = address.toString();
				info.port = FSocksStreams->listeningPort();
				FHosts.prepend(info);
			}
		}
	}

	foreach(HostInfo info, FHosts)
	{
		QDomElement hostElem = queryElem.appendChild(request.createElement("streamhost")).toElement();
		hostElem.setAttribute("jid",info.jid.eFull());
		hostElem.setAttribute("host",info.name);
		hostElem.setAttribute("port",info.port);
	}

	if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,HOST_REQUEST_TIMEOUT))
	{
		FHostRequest = request.id();
		return !FHosts.isEmpty();
	}

	return false;
}

bool SocksStream::connectToHost()
{
	if (FHostIndex < FHosts.count())
	{
		HostInfo info = FHosts.value(FHostIndex);
		if (!FTcpSocket)
		{
			FTcpSocket = new QTcpSocket(this);
			connect(FTcpSocket,SIGNAL(connected()),SLOT(onHostSocketConnected()));
			connect(FTcpSocket,SIGNAL(readyRead()), SLOT(onHostSocketReadyRead()));
			connect(FTcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), SLOT(onHostSocketError(QAbstractSocket::SocketError)));
			connect(FTcpSocket,SIGNAL(disconnected()),SLOT(onHostSocketDisconnected()));
			FTcpSocket->setProxy(FNetworkProxy);
		}
		FTcpSocket->connectToHost(info.name, info.port);
		return true;
	}
	return false;
}

bool SocksStream::sendUsedHost()
{
	if (FHostIndex < FHosts.count())
	{
		Stanza reply("iq");
		reply.setType("result").setId(FHostRequest).setTo(FContactJid.eFull());

		QDomElement query =  reply.addElement("query",NS_SOCKS5_BYTESTREAMS);
		query.setAttribute("sid",FStreamId);

		QDomElement hostElem = query.appendChild(reply.addElement("streamhost-used")).toElement();
		hostElem.setAttribute("jid",FHosts.at(FHostIndex).jid.eFull());

		if (FStanzaProcessor->sendStanzaOut(FStreamJid, reply))
			return true;
	}
	return false;
}

bool SocksStream::sendFailedHosts()
{
	Stanza reply("iq");
	reply.setType("error").setTo(FContactJid.eFull()).setId(FHostRequest);

	QDomElement errElem = reply.addElement("error");
	errElem.setAttribute("code", 404);
	errElem.setAttribute("type","cancel");
	errElem.appendChild(reply.createElement("item-not-found", EHN_DEFAULT));

	return FStanzaProcessor->sendStanzaOut(FStreamJid, reply);
}

bool SocksStream::activateStream()
{
	if (FHostIndex < FHosts.count())
	{
		Stanza request("iq");
		request.setType("set").setTo(FHosts.at(FHostIndex).jid.eFull()).setId(FStanzaProcessor->newId());
		QDomElement queryElem = request.addElement("query",NS_SOCKS5_BYTESTREAMS);
		queryElem.setAttribute("sid",FStreamId);
		queryElem.appendChild(request.createElement("activate")).appendChild(request.createTextNode(FContactJid.eFull()));
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,ACTIVATE_REQUEST_TIMEOUT))
		{
			FActivateRequest = request.id();
			return true;
		}
	}
	return false;
}

void SocksStream::onHostSocketConnected()
{
	QByteArray outData;
	outData += (char)5;   // Socks version
	outData += (char)1;   // Number of possible authentication methods
	outData += (char)0;   // No-auth
	FTcpSocket->write(outData);
}

void SocksStream::onHostSocketReadyRead()
{
	QByteArray inData = FTcpSocket->read(FTcpSocket->bytesAvailable());
	if (inData.size() < 10)
	{
		QByteArray outData;
		outData += (char)5;                     // socks version
		outData += (char)1;                     // connect method
		outData += (char)0;                     // reserved
		outData += (char)3;                     // address type (domain)
		outData += (char)FConnectKey.length();  // domain length
		outData += FConnectKey.toLatin1();      // domain
		outData += (char)0;                     // port
		outData += (char)0;                     // port
		FTcpSocket->write(outData);
	}
	else if (inData.at(0)==5 && inData.at(1)==0)
	{
		FTcpSocket->disconnect(this);
		setTcpSocket(FTcpSocket);
		negotiateConnection(NCMD_ACTIVATE_STREAM);
	}
	else
		FTcpSocket->disconnectFromHost();
}

void SocksStream::onHostSocketError(QAbstractSocket::SocketError)
{
	if (FTcpSocket->state() != QAbstractSocket::ConnectedState)
		onHostSocketDisconnected();
}

void SocksStream::onHostSocketDisconnected()
{
	FHostIndex++;
	if (streamKind() == IDataStreamSocket::Initiator)
		abort(tr("Failed to connect to host"));
	else
		negotiateConnection(NCMD_CHECK_NEXT_HOST);
}

void SocksStream::onTcpSocketReadyRead()
{
	readBufferedData(false);
}

void SocksStream::onTcpSocketBytesWritten(qint64 ABytes)
{
	Q_UNUSED(ABytes);
	writeBufferedData(false);
}

void SocksStream::onTcpSocketDisconnected()
{
	readBufferedData(true);

	if (streamState() == IDataStreamSocket::Closing)
		setStreamState(IDataStreamSocket::Closed);

	QWriteLocker locker(&FThreadLock);
	FTcpSocket->deleteLater();
	FTcpSocket = NULL;
}

void SocksStream::onLocalConnectionAccepted(const QString &AKey, QTcpSocket *ATcpSocket)
{
	if (FConnectKey == AKey)
		setTcpSocket(ATcpSocket);
}
