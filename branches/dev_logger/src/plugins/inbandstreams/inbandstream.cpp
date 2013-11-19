#include "inbandstream.h"

#include <QTime>
#include <QEvent>
#include <QReadLocker>
#include <QWriteLocker>
#include <QCoreApplication>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>
#include <utils/jid.h>

#define BUFFER_INCREMENT_SIZE     1024
#define MAX_BUFFER_SIZE           8192

#define DATA_TIMEOUT              60000
#define OPEN_TIMEOUT              30000
#define CLOSE_TIMEOUT             10000

#define SHC_INBAND_OPEN           "/iq[@type='set']/open[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_CLOSE          "/iq[@type='set']/close[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_IQ        "/iq[@type='set']/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_MESSAGE   "/message/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"

class DataEvent :
	public QEvent
{
public:
	DataEvent(bool AFlush) : QEvent(FEventType) { 
		FFlush = AFlush;
	}
	inline bool isFlush() {
		return FFlush;
	}
	static int registeredType() {
		return FEventType; 
	}
private:
	bool FFlush;
	static QEvent::Type FEventType;
};

QEvent::Type DataEvent::FEventType = static_cast<QEvent::Type>(QEvent::registerEventType());


InBandStream::InBandStream(IStanzaProcessor *AProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent)
		: QIODevice(AParent), FReadBuffer(BUFFER_INCREMENT_SIZE), FWriteBuffer(BUFFER_INCREMENT_SIZE,MAX_BUFFER_SIZE)
{
	FStanzaProcessor = AProcessor;

	FStreamId = AStreamId;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FStreamKind = AKind;

	FSHIOpen = -1;
	FSHIClose = -1;
	FSHIData = -1;

	FBlockSize = DEFAULT_BLOCK_SIZE;
	FMaxBlockSize = DEFAULT_MAX_BLOCK_SIZE;
	FStanzaType = DEFAULT_DATA_STANZA_TYPE;
	FStreamState = IDataStreamSocket::Closed;

	LOG_STRM_INFO(AStreamJid,QString("In-band stream created, sid=%1, kind=%2").arg(FStreamId).arg(FStreamKind));
}

InBandStream::~InBandStream()
{
	abort(XmppError(IERR_INBAND_STREAM_DESTROYED));
	LOG_STRM_INFO(FStreamJid,QString("In-band stream destroyed, sid=%1, kind=%2").arg(FStreamId).arg(FStreamKind));
}

bool InBandStream::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	QDomElement elem = AStanza.firstElement(QString::null,NS_INBAND_BYTESTREAMS);
	if (AHandleId==FSHIData && elem.attribute("sid")==FStreamId)
	{
		AAccept = true;
		if (AStanza.firstElement("error").isNull())
		{
			QByteArray data =  QByteArray::fromBase64(elem.text().toUtf8());
			if (FSeqIn==elem.attribute("seq").toInt() && data.size()>0 && data.size()<=FBlockSize)
			{
				if (AStanza.tagName() == "iq")
				{
					Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
					FStanzaProcessor->sendStanzaOut(AStreamJid,result);
				}

				FThreadLock.lockForWrite();
				FReadBuffer.write(data);
				FThreadLock.unlock();

				FSeqIn = FSeqIn<USHRT_MAX ? FSeqIn+1 : 0;
				emit readyRead();
				FReadyReadCondition.wakeAll();
			}
			else
			{
				abort(XmppError(IERR_INBAND_STREAM_INVALID_DATA));
			}
		}
		else
		{
			abort(XmppStanzaError(AStanza));
		}
	}
	else if (AHandleId==FSHIOpen && elem.attribute("sid")==FStreamId)
	{
		AAccept = true;
		removeStanzaHandle(FSHIOpen);
		if (FStreamState == IDataStreamSocket::Opening)
		{
			QDomElement openElem = AStanza.firstElement("open");
			FBlockSize = openElem.attribute("block-size").toInt();
			if (FBlockSize>MINIMUM_BLOCK_SIZE && FBlockSize<=FMaxBlockSize)
			{
				FStanzaType = openElem.attribute("stanza") == "message" ? StanzaMessage : StanzaIq;
				FSHIData = insertStanzaHandle(FStanzaType==StanzaMessage ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
				FSHIClose = insertStanzaHandle(SHC_INBAND_CLOSE);
				if (FSHIData>0 && FSHIClose>0)
				{
					Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
					if (FStanzaProcessor->sendStanzaOut(AStreamJid,result))
						setStreamState(IDataStreamSocket::Opened);
					else
						abort(XmppError(IERR_INBAND_STREAM_NOT_OPENED));
				}
				else
				{
					Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_INTERNAL_SERVER_ERROR);
					FStanzaProcessor->sendStanzaOut(AStreamJid,error);
					abort(XmppError(IERR_INBAND_STREAM_NOT_OPENED));
				}
			}
			else
			{
				Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_RESOURCE_CONSTRAINT);
				FStanzaProcessor->sendStanzaOut(AStreamJid,error);
				abort(XmppError(IERR_INBAND_STREAM_INVALID_BLOCK_SIZE));
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Unexpected open request from=%1, sid=%2: Invalid state").arg(AStanza.from(),FStreamId));
			Stanza error = FStanzaProcessor->makeReplyError(AStanza,XmppStanzaError::EC_UNEXPECTED_REQUEST);
			FStanzaProcessor->sendStanzaOut(AStreamJid,error);
		}
	}
	else if (AHandleId==FSHIClose && elem.attribute("sid")==FStreamId)
	{
		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
		setStreamState(IDataStreamSocket::Closed);
	}
	return false;
}

void InBandStream::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
	if (AStanza.id() == FDataIqRequestId)
	{
		if (AStanza.type() == "result")
		{
			FDataIqRequestId.clear();
			sendNextPaket();
		}
		else
		{
			abort(XmppStanzaError(AStanza));
		}
	}
	else if (AStanza.id() == FOpenRequestId)
	{
		if (AStanza.type() == "result")
		{
			FSHIData = insertStanzaHandle(FStanzaType==StanzaMessage ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
			FSHIClose = insertStanzaHandle(SHC_INBAND_CLOSE);
			if (FSHIData>0 && FSHIClose>0)
				setStreamState(IDataStreamSocket::Opened);
			else
				abort(XmppError(IERR_INBAND_STREAM_NOT_OPENED));
		}
		else
		{
			abort(XmppStanzaError(AStanza));
		}
	}
	else if (AStanza.id() == FCloseRequestId)
	{
		setStreamState(IDataStreamSocket::Closed);
	}
}

bool InBandStream::isSequential() const
{
	return true;
}

qint64 InBandStream::bytesAvailable() const
{
	QReadLocker locker(&FThreadLock);
	return FReadBuffer.size();
}

qint64 InBandStream::bytesToWrite() const
{
	QReadLocker locker(&FThreadLock);
	return FWriteBuffer.size();
}

bool InBandStream::waitForBytesWritten(int AMsecs)
{
	bool isWritten = false;
	if (streamState() != IDataStreamSocket::Closed)
	{
		FThreadLock.lockForWrite();
		isWritten = FBytesWrittenCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
		FThreadLock.unlock();
	}
	return isWritten && isOpen();
}

bool InBandStream::waitForReadyRead(int AMsecs)
{
	if (streamState()!=IDataStreamSocket::Closed && bytesAvailable()==0)
	{
		FThreadLock.lockForWrite();
		FReadyReadCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
		FThreadLock.unlock();
	}
	return bytesAvailable()>0;
}

QString InBandStream::methodNS() const
{
	return NS_INBAND_BYTESTREAMS;
}

QString InBandStream::streamId() const
{
	return FStreamId;
}

Jid InBandStream::streamJid() const
{
	return FStreamJid;
}

Jid InBandStream::contactJid() const
{
	return FContactJid;
}

int InBandStream::streamKind() const
{
	return FStreamKind;
}

int InBandStream::streamState() const
{
	QReadLocker locker(&FThreadLock);
	return FStreamState;
}

XmppError InBandStream::error() const
{
	QReadLocker locker(&FThreadLock);
	return FError;
}

bool InBandStream::isOpen() const
{
	QReadLocker locker(&FThreadLock);
	return FStreamState==IDataStreamSocket::Opened;
}

bool InBandStream::open(QIODevice::OpenMode AMode)
{
	if (FStanzaProcessor && streamState()==IDataStreamSocket::Closed)
	{
		setStreamError(XmppError::null);
		if (streamKind() == IDataStreamSocket::Initiator)
		{
			Stanza openRequest("iq");
			openRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.full());
			QDomElement elem = openRequest.addElement("open",NS_INBAND_BYTESTREAMS);
			elem.setAttribute("sid",FStreamId);
			elem.setAttribute("block-size",FBlockSize);
			elem.setAttribute("stanza",FStanzaType==StanzaMessage ? "message" : "iq");
			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,openRequest,OPEN_TIMEOUT))
			{
				LOG_STRM_INFO(FStreamJid,QString("Open stream request sent, sid=%1").arg(FStreamId));
				FOpenRequestId = openRequest.id();
				setOpenMode(AMode);
				setStreamState(IDataStreamSocket::Opening);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send open stream request, sid=%1").arg(FStreamId));
			}
		}
		else
		{
			FSHIOpen = insertStanzaHandle(SHC_INBAND_OPEN);
			if (FSHIOpen != -1)
			{
				LOG_STRM_INFO(FStreamJid,QString("Open stanza handler inserted, sid=%1").arg(FStreamId));
				setOpenMode(AMode);
				setStreamState(IDataStreamSocket::Opening);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to insert open stanza handler, sid=%1").arg(FStreamId));
			}
		}
	}
	return false;
}

bool InBandStream::flush()
{
	if (isOpen() && bytesToWrite()>0)
	{
		DataEvent *dataEvent = new DataEvent(true);
		QCoreApplication::postEvent(this,dataEvent);
		return true;
	}
	return false;
}

void InBandStream::close()
{
	int state = streamState();
	if (state==IDataStreamSocket::Opened || state==IDataStreamSocket::Opening)
	{
		emit aboutToClose();
		if (FStanzaProcessor)
		{
			Stanza closeRequest("iq");
			closeRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.full());
			closeRequest.addElement("close",NS_INBAND_BYTESTREAMS).setAttribute("sid",FStreamId);
			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,closeRequest,CLOSE_TIMEOUT))
			{
				LOG_STRM_INFO(FStreamJid,QString("Close stream request sent, sid=%1").arg(FStreamId));
				FCloseRequestId = closeRequest.id();
				setStreamState(IDataStreamSocket::Closing);
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send close stream request, sid=%1").arg(FStreamId));
				setStreamState(IDataStreamSocket::Closed);
			}
		}
		else
		{
			setStreamState(IDataStreamSocket::Closed);
		}
	}
}

void InBandStream::abort(const XmppError &AError)
{
	if (streamState() != IDataStreamSocket::Closed)
	{
		LOG_STRM_INFO(FStreamJid,QString("Aborting stream, sid=%1: %2").arg(FStreamId,AError.errorMessage()));
		setStreamError(AError);
		close();
		setStreamState(IDataStreamSocket::Closed);
	}
}

int InBandStream::blockSize() const
{
	return FBlockSize;
}

void InBandStream::setBlockSize(int ASize)
{
	if (streamState()==IDataStreamSocket::Closed && ASize>=MINIMUM_BLOCK_SIZE && ASize<=maximumBlockSize())
	{
		FBlockSize = ASize;
		emit propertiesChanged();
	}
}

int InBandStream::maximumBlockSize() const
{
	return FMaxBlockSize;
}

void InBandStream::setMaximumBlockSize(int ASize)
{
	if (ASize>=MINIMUM_BLOCK_SIZE && ASize<=USHRT_MAX)
	{
		FMaxBlockSize = ASize;
		emit propertiesChanged();
	}
}

int InBandStream::dataStanzaType() const
{
	return FStanzaType;
}

void InBandStream::setDataStanzaType(int AType)
{
	if (streamState() == IDataStreamSocket::Closed)
	{
		FStanzaType = AType;
		emit propertiesChanged();
	}
}

qint64 InBandStream::readData(char *AData, qint64 AMaxSize)
{
	QWriteLocker locker(&FThreadLock);
	return FReadBuffer.read(AData,AMaxSize);
}

qint64 InBandStream::writeData(const char *AData, qint64 AMaxSize)
{
	DataEvent *dataEvent = new DataEvent(false);
	QCoreApplication::postEvent(this,dataEvent);
	QWriteLocker locker(&FThreadLock);
	return FWriteBuffer.write(AData,AMaxSize);
}

void InBandStream::setOpenMode(OpenMode AMode)
{
	QWriteLocker locker(&FThreadLock);
	QIODevice::setOpenMode(AMode);
}

bool InBandStream::event(QEvent *AEvent)
{
	if (AEvent->type() == DataEvent::registeredType())
	{
		DataEvent *dataEvent = static_cast<DataEvent *>(AEvent);
		sendNextPaket(dataEvent->isFlush());
		return true;
	}
	return QIODevice::event(AEvent);
}

bool InBandStream::sendNextPaket(bool AFlush)
{
	bool sent = false;
	if (isOpen() && FDataIqRequestId.isEmpty() && (bytesToWrite()>=FBlockSize || AFlush))
	{
		FThreadLock.lockForWrite();
		QByteArray data = FWriteBuffer.read(FBlockSize);
		FThreadLock.unlock();

		if (!data.isEmpty())
		{
			if (FStanzaProcessor)
			{
				Stanza paket(FStanzaType==StanzaMessage ? "message" : "iq");
				paket.setTo(FContactJid.full()).setId(FStanzaProcessor->newId());
				QDomElement dataElem = paket.addElement("data",NS_INBAND_BYTESTREAMS);
				dataElem.setAttribute("sid",FStreamId);
				dataElem.setAttribute("seq",FSeqOut);
				dataElem.appendChild(paket.createTextNode(QString::fromUtf8(data.toBase64())));

				if (FStanzaType == StanzaMessage)
				{
					QDomElement ampElem = paket.addElement("amp","http://jabber.org/protocol/amp");
					QDomElement ruleElem = ampElem.appendChild(paket.createElement("rule")).toElement();
					ruleElem.setAttribute("condition","deliver");
					ruleElem.setAttribute("value","stored");
					ruleElem.setAttribute("action","error");
					ruleElem = ampElem.appendChild(paket.createElement("rule")).toElement();
					ruleElem.setAttribute("condition","match-resource");
					ruleElem.setAttribute("value","exact");
					ruleElem.setAttribute("action","error");

					DataEvent *dataEvent = new DataEvent(AFlush);
					QCoreApplication::postEvent(this, dataEvent);

					sent = FStanzaProcessor->sendStanzaOut(FStreamJid,paket);
				}
				else
				{
					paket.setType("set");
					FDataIqRequestId = paket.id();
					sent = FStanzaProcessor->sendStanzaRequest(this,FStreamJid,paket,DATA_TIMEOUT);
				}
			}

			if (sent)
			{
				FSeqOut = FSeqOut<USHRT_MAX ? FSeqOut+1 : 0;
				emit bytesWritten(data.size());
				FBytesWrittenCondition.wakeAll();
			}
			else
			{
				abort(XmppError(IERR_INBAND_STREAM_DATA_NOT_SENT));
			}
		}
	}
	return sent;
}

void InBandStream::setStreamState(int AState)
{
	if (streamState() != AState)
	{
		if (AState == IDataStreamSocket::Opened)
		{
			FSeqIn = 0;
			FSeqOut = 0;
			FDataIqRequestId.clear();
			FThreadLock.lockForWrite();
			QIODevice::open(openMode());
			FThreadLock.unlock();
			LOG_STRM_INFO(FStreamJid,QString("In-band stream opened, sid=%1, stanzaType=%2").arg(FStreamId).arg(FStanzaType));
		}
		else if (AState == IDataStreamSocket::Closed)
		{
			removeStanzaHandle(FSHIOpen);
			removeStanzaHandle(FSHIClose);
			removeStanzaHandle(FSHIData);
			emit readChannelFinished();

			FThreadLock.lockForWrite();
			FStreamState = AState;
			QString saveError = QIODevice::errorString();
			QIODevice::close();
			QIODevice::setErrorString(saveError);
			FReadBuffer.clear();
			FWriteBuffer.clear();
			FThreadLock.unlock();

			FReadyReadCondition.wakeAll();
			FBytesWrittenCondition.wakeAll();
			LOG_STRM_INFO(FStreamJid,QString("In-band stream closed, sid=%1").arg(FStreamId));
		}

		FThreadLock.lockForWrite();
		FStreamState = AState;
		FThreadLock.unlock();

		emit stateChanged(AState);
	}
}

void InBandStream::setStreamError(const XmppError &AError)
{
	if (AError.isNull() != FError.isNull())
	{
		QWriteLocker locker(&FThreadLock);
		FError = AError;
		setErrorString(!FError.isNull() ? FError.errorString() : QString::null);
	}
}

int InBandStream::insertStanzaHandle(const QString &ACondition)
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

void InBandStream::removeStanzaHandle(int &AHandleId)
{
	if (FStanzaProcessor && AHandleId>0)
	{
		FStanzaProcessor->removeStanzaHandle(AHandleId);
		AHandleId = -1;
	}
}
