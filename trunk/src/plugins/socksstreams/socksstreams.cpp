#include "socksstreams.h"

#include <QCryptographicHash>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/options.h>
#include <utils/logger.h>

SocksStreams::SocksStreams() : FServer(this)
{
	FXmppStreams = NULL;
	FDataManager = NULL;
	FStanzaProcessor = NULL;
	FDiscovery = NULL;
	FConnectionManager = NULL;

	FServer.setProxy(QNetworkProxy::NoProxy);
	connect(&FServer,SIGNAL(newConnection()),SLOT(onNewServerConnection()));
}

SocksStreams::~SocksStreams()
{

}

void SocksStreams::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SOCKS5 Data Stream");
	APluginInfo->description = tr("Allows to initiate SOCKS5 stream of data between two XMPP entities");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SocksStreams::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
	if (plugin)
	{
		FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
	{
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsReceived(const IDiscoItems &)));
		}
	}

	return FStanzaProcessor!=NULL;
}

bool SocksStreams::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_DESTROYED,tr("Stream destroyed"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_INVALID_MODE,tr("Unsupported stream mode"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_HOSTS_REJECTED,tr("Remote client cant connect to given hosts"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_HOSTS_UNREACHABLE,tr("Cant connect to given hosts"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_HOSTS_NOT_CREATED,tr("Failed to create hosts"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_NOT_ACTIVATED,tr("Failed to activate stream"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_DATA_NOT_SENT,tr("Failed to send data to socket"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_NO_DIRECT_CONNECTION,tr("Direct connection not established"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_INVALID_HOST,tr("Invalid host"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_INVALID_HOST_ADDRESS,tr("Invalid host address"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_HOST_NOT_CONNECTED,tr("Failed to connect to host"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SOCKS5_STREAM_HOST_DISCONNECTED,tr("Host disconnected"));

	if (FDataManager)
	{
		FDataManager->insertMethod(this);
	}
	if (FDiscovery)
	{
		IDiscoFeature feature;
		feature.var = NS_SOCKS5_BYTESTREAMS;
		feature.active = true;
		feature.name = tr("SOCKS5 Data Stream");
		feature.description = tr("Supports the initiating of the SOCKS5 stream of data between two XMPP entities");
		FDiscovery->insertDiscoFeature(feature);
	}
	return true;
}

bool SocksStreams::initSettings()
{
	Options::setDefaultValue(OPV_DATASTREAMS_SOCKSLISTENPORT,5277);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_DISABLEDIRECT,false);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_FORWARDHOST,QString());
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_FORWARDPORT,0);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_USEACCOUNTSTREAMPROXY,true);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_USEACCOUNTNETPROXY,true);
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_NETWORKPROXY,QString(APPLICATION_PROXY_REF_UUID));
	Options::setDefaultValue(OPV_DATASTREAMS_METHOD_CONNECTTIMEOUT,10000);
	return true;
}

QString SocksStreams::methodNS() const
{
	return NS_SOCKS5_BYTESTREAMS;
}

QString SocksStreams::methodName() const
{
	return tr("SOCKS5 Data Stream");
}

QString SocksStreams::methodDescription() const
{
	return tr("Data is transferred out-band over TCP or UDP connection");
}

IDataStreamSocket *SocksStreams::dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, IDataStreamSocket::StreamKind AKind, QObject *AParent)
{
	if (FStanzaProcessor)
	{
		SocksStream *stream = new SocksStream(this,FStanzaProcessor,ASocketId,AStreamJid,AContactJid,AKind,AParent);
		emit socketCreated(stream);
		return stream;
	}
	return NULL;
}

IOptionsWidget *SocksStreams::methodSettingsWidget(const OptionsNode &ANode, bool AReadOnly, QWidget *AParent)
{
	return new SocksOptions(this,FConnectionManager,ANode,AReadOnly,AParent);
}

IOptionsWidget *SocksStreams::methodSettingsWidget(IDataStreamSocket *ASocket, bool AReadOnly, QWidget *AParent)
{
	ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
	return stream!=NULL ? new SocksOptions(this,stream,AReadOnly,AParent) : NULL;
}

void SocksStreams::saveMethodSettings(IOptionsWidget *AWidget, OptionsNode ANode)
{
	SocksOptions *widget = qobject_cast<SocksOptions *>(AWidget->instance());
	if (widget)
		widget->apply(ANode);
	else
		REPORT_ERROR("Failed to save socks stream settings: Invalid options widget");
}

void SocksStreams::loadMethodSettings(IDataStreamSocket *ASocket, IOptionsWidget *AWidget)
{
	SocksOptions *widget = qobject_cast<SocksOptions *>(AWidget->instance());
	ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
	if (widget && stream)
		widget->apply(stream);
	else if (widget == NULL)
		REPORT_ERROR("Failed to load socks stream settings: Invalid options widget");
	else if (stream == NULL)
		REPORT_ERROR("Failed to load socks stream settings: Invalid socket");
}

void SocksStreams::loadMethodSettings(IDataStreamSocket *ASocket, const OptionsNode &ANode)
{
	ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
	if (stream)
	{
		QStringList proxyItems = ANode.value("stream-proxy-list").toStringList();
		if (ANode.value("use-account-stream-proxy").toBool())
		{
			QString streamProxy = accountStreamProxy(stream->streamJid());
			if (!streamProxy.isEmpty() && !proxyItems.contains(streamProxy))
				proxyItems.prepend(streamProxy);
		}
		stream->setProxyList(proxyItems);

		stream->setConnectTimeout(ANode.value("connect-timeout").toInt());
		stream->setDirectConnectionsDisabled(ANode.value("disable-direct-connections").toBool());
		stream->setForwardAddress(ANode.value("forward-host").toString(), ANode.value("forward-port").toInt());
		if (ANode.value("use-account-network-proxy").toBool())
			stream->setNetworkProxy(accountNetworkProxy(stream->streamJid()));
		else if (FConnectionManager)
			stream->setNetworkProxy(FConnectionManager->proxyById(ANode.value("network-proxy").toString()).proxy);
	}
	else
	{
		REPORT_ERROR("Failed to load socks stream settings: Invalid socket");
	}
}

quint16 SocksStreams::listeningPort() const
{
	return FServer.isListening() ? FServer.serverPort() : Options::node(OPV_DATASTREAMS_SOCKSLISTENPORT).value().toInt();
}

QString SocksStreams::accountStreamProxy(const Jid &AStreamJid) const
{
	return FStreamProxy.value(AStreamJid);
}

QNetworkProxy SocksStreams::accountNetworkProxy(const Jid &AStreamJid) const
{
	QNetworkProxy proxy(QNetworkProxy::NoProxy);
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	IDefaultConnection *connection = stream!=NULL ? qobject_cast<IDefaultConnection *>(stream->connection()->instance()) : NULL;
	return connection!=NULL ? connection->proxy() : QNetworkProxy(QNetworkProxy::NoProxy);
}

QString SocksStreams::connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const
{
	QString keyString = ASessionId + AInitiator.pFull() + ATarget.pFull();
	QByteArray keyData = QCryptographicHash::hash(keyString.toUtf8(), QCryptographicHash::Sha1).toHex();
	return QString::fromUtf8(keyData.constData(), keyData.size()).toLower();
}

bool SocksStreams::appendLocalConnection(const QString &AKey)
{
	if (!AKey.isEmpty() && !FLocalKeys.contains(AKey))
	{
		if (FServer.isListening() || FServer.listen(QHostAddress::Any, listeningPort()))
		{
			FLocalKeys.append(AKey);
			return true;
		}
		else if (!FServer.isListening())
		{
			LOG_ERROR(QString("Failed to append local socks connection, port=%1: %2").arg(listeningPort()).arg(FServer.errorString()));
		}
	}
	else if (AKey.isEmpty())
	{
		REPORT_ERROR("Failed to append local socks connection: Key is empty");
	}
	return false;
}

void SocksStreams::removeLocalConnection(const QString &AKey)
{
	FLocalKeys.removeAll(AKey);
	if (FLocalKeys.isEmpty())
		FServer.close();
}

void SocksStreams::onXmppStreamOpened(IXmppStream *AStream)
{
	if (FDiscovery)
		FDiscovery->requestDiscoItems(AStream->streamJid(), AStream->streamJid().domain());
}

void SocksStreams::onXmppStreamClosed(IXmppStream *AStream)
{
	FStreamProxy.remove(AStream->streamJid());
}

void SocksStreams::onDiscoItemsReceived(const IDiscoItems &AItems)
{
	if (AItems.contactJid==AItems.streamJid.domain() && AItems.node.isEmpty())
	{
		FStreamProxy.remove(AItems.streamJid);
		foreach(const IDiscoItem &item, AItems.items)
		{
			QString itemBareJid = item.itemJid.pBare();
			if (itemBareJid.startsWith("proxy.") || itemBareJid.startsWith("proxy65."))
			{
				LOG_STRM_INFO(AItems.streamJid,QString("Found socks proxy on server, jid=%1").arg(itemBareJid));
				FStreamProxy.insert(AItems.streamJid,itemBareJid);
				break;
			}
		}
	}
}

void SocksStreams::onNewServerConnection()
{
	QTcpSocket *tcpsocket = FServer.nextPendingConnection();
	connect(tcpsocket, SIGNAL(readyRead()), SLOT(onServerConnectionReadyRead()));
	connect(tcpsocket, SIGNAL(disconnected()), SLOT(onServerConnectionDisconnected()));
	LOG_INFO(QString("Socks local connection appended, address=%1").arg(tcpsocket->peerAddress().toString()));
}

void SocksStreams::onServerConnectionReadyRead()
{
	QTcpSocket *tcpsocket = qobject_cast<QTcpSocket *>(sender());
	if (tcpsocket)
	{
		QByteArray inData = tcpsocket->read(tcpsocket->bytesAvailable());
		if (inData.size() < 10)
		{
			if (inData.startsWith(5))
			{
				QByteArray outData;
				outData[0] = 5;   // Socks version
				outData[1] = 0;   // Auth method - no auth
				if (tcpsocket->write(outData) == outData.length())
				{
					LOG_INFO(QString("Socks local connection authentication request sent to=%1").arg(tcpsocket->peerAddress().toString()));
				}
				else
				{
					LOG_WARNING(QString("Failed to send socks local connection authentication request to=%1: %2").arg(tcpsocket->peerAddress().toString(),tcpsocket->errorString()));
					tcpsocket->disconnectFromHost();
				}
			}
			else
			{
				LOG_WARNING(QString("Failed to accept socks local connection from=%1: Invalid socket version=%2").arg(tcpsocket->peerAddress().toString()).arg((quint8)inData.at(0)));
				tcpsocket->disconnectFromHost();
			}
		}
		else
		{
			unsigned char keyLen = inData.size()>4 ? inData.at(4) : 0;
			if (inData.size() >= 5+keyLen+2)
			{
				QString key = QString::fromUtf8(inData.constData()+5, keyLen).toLower();
				if (FLocalKeys.contains(key))
				{
					QByteArray outData;
					outData += (char)5;         // Socks version
					outData += (char)0;         // Connect accepted
					outData += (char)0;         // Reserved
					outData += (char)3;         // Domain name
					outData += keyLen;          // Domain size;
					outData += key.toLatin1();  // Domain
					outData += (char)0;         // port
					outData += (char)0;         // port
					tcpsocket->write(outData);

					tcpsocket->disconnect(this);
					removeLocalConnection(key);

					LOG_INFO(QString("Authenticated socks local connection from=%1").arg(tcpsocket->peerAddress().toString()));
					emit localConnectionAccepted(key,tcpsocket);
				}
				else
				{
					LOG_WARNING(QString("Failed to authenticate socks local connection from=%1: Invalid key=%2").arg(tcpsocket->peerAddress().toString(),key));
					tcpsocket->disconnectFromHost();
				}
			}
			else
			{
				LOG_WARNING(QString("Failed to authenticate socks local connection from=%1: Invalid response size=%2").arg(tcpsocket->peerAddress().toString()).arg(inData.size()));
				tcpsocket->disconnectFromHost();
			}
		}
	}
}

void SocksStreams::onServerConnectionDisconnected()
{
	QTcpSocket *tcpsocket = qobject_cast<QTcpSocket *>(sender());
	if (tcpsocket)
	{
		tcpsocket->deleteLater();
		LOG_INFO(QString("Socks local connection disconnected, address=%1").arg(tcpsocket->peerAddress().toString()));
	}
}

Q_EXPORT_PLUGIN2(plg_socksstreams, SocksStreams);
