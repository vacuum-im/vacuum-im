#include "socksstreams.h"

#include <QCryptographicHash>

#define SVN_SERVER_PORT                     "serverPort"
#define SVN_DISABLE_DIRECT_CONNECT          "settings[]:disableDirectConnect"
#define SVN_USE_NATIVE_SERVER_PROXY         "settings[]:useNativeServerProxy"
#define SVN_USE_ACCOUNT_NETPROXY            "settings[]:useAccountProxy"
#define SVN_FORWARD_HOST                    "settings[]:forwardHost"
#define SVN_FORWARD_PORT                    "settings[]:forwardPort"
#define SVN_NETPROXY_TYPE                   "settings[]:netproxyType"
#define SVN_NETPROXY_HOST                   "settings[]:netproxyHost"
#define SVN_NETPROXY_PORT                   "settings[]:netproxyPort"
#define SVN_NETPROXY_USER                   "settings[]:netproxyUser"
#define SVN_NETPROXY_PASSWORD               "settings[]:netproxyPassword"
#define SVN_PROXY_LIST                      "settings[]:proxyList"

#define DEFAULT_SERVER_PORT                 52227
#define DEFAULT_DIABLE_DIRECT_CONNECT       false
#define DEFAULT_USE_NATIVE_SERVER_PROXY     true
#define DEFAULT_USE_ACCOUNT_NETPROXY        true

SocksStreams::SocksStreams() : FServer(this)
{
  FXmppStreams = NULL;
  FDataManager = NULL;
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FConnectionManager = NULL;
  FSettings = NULL;
  FSettingsPlugin = NULL;

  FForwardPort = 0;
  FServerPort = DEFAULT_SERVER_PORT;
  FDisableDirectConnect = DEFAULT_DIABLE_DIRECT_CONNECT;
  FUseNativeServerProxy = DEFAULT_USE_NATIVE_SERVER_PROXY;
  FUseAccountNetworkProxy = DEFAULT_USE_ACCOUNT_NETPROXY;

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

bool SocksStreams::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
  if (plugin)
    FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
  if (plugin)
    FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());

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

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return FStanzaProcessor!=NULL;
}

bool SocksStreams::initObjects()
{
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
  return tr("Data is transfered out-band over TCP or UDP connection");
}

IDataStreamSocket *SocksStreams::dataStreamSocket(const QString &ASocketId, const Jid &AStreamJid, const Jid &AContactJid, 
                                                  IDataStreamSocket::StreamKind AKind, QObject *AParent)
{
  if (FStanzaProcessor)
  {
    SocksStream *stream = new SocksStream(this,FStanzaProcessor,ASocketId,AStreamJid,AContactJid,AKind,AParent);
    emit socketCreated(stream);
    return stream;
  }
  return NULL;
}

QWidget *SocksStreams::settingsWidget(IDataStreamSocket *ASocket, bool AReadOnly)
{
  ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
  return stream!=NULL ? new SocksOptions(this,stream,AReadOnly,NULL) : NULL;
}

QWidget *SocksStreams::settingsWidget(const QString &ASettingsNS, bool AReadOnly)
{
  return new SocksOptions(this,FConnectionManager,ASettingsNS,AReadOnly,NULL);
}

void SocksStreams::loadSettings(IDataStreamSocket *ASocket, QWidget *AWidget) const
{
  SocksOptions *widget = qobject_cast<SocksOptions *>(AWidget);
  ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
  if (widget && stream)
    widget->saveSettings(stream);
}

void SocksStreams::loadSettings(IDataStreamSocket *ASocket, const QString &ASettingsNS) const
{
  ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
  if (stream)
  {
    QList<QString> proxyItems = proxyList(ASettingsNS);
    if (useNativeServerProxy(ASettingsNS))
    {
      QString nativeProxy = nativeServerProxy(stream->streamJid());
      if (!nativeProxy.isEmpty() && !proxyItems.contains(nativeProxy))
        proxyItems.prepend(nativeProxy);
    }

    stream->setDisableDirectConnection(disableDirectConnections(ASettingsNS));
    stream->setForwardAddress(forwardHost(ASettingsNS), forwardPort(ASettingsNS));
    stream->setNetworkProxy(useAccountNetworkProxy(ASettingsNS) ? accountNetworkProxy(stream->streamJid()) : networkProxy(ASettingsNS));
    stream->setProxyList(proxyItems);
  }
}

void SocksStreams::saveSettings(const QString &ASettingsNS, QWidget *AWidget)
{
  SocksOptions *widget = qobject_cast<SocksOptions *>(AWidget);
  if (widget)
    widget->saveSettings(ASettingsNS);
}

void SocksStreams::saveSettings(const QString &ASettingsNS, IDataStreamSocket *ASocket)
{
  ISocksStream *stream = qobject_cast<ISocksStream *>(ASocket->instance());
  if (stream)
  {
    setDisableDirectConnections(ASettingsNS, stream->disableDirectConnection());
    setForwardAddress(ASettingsNS, stream->forwardHost(), stream->forwardPort());
    setNetworkProxy(ASettingsNS, stream->networkProxy());
    setProxyList(ASettingsNS, stream->proxyList());
  }
}

void SocksStreams::deleteSettings(const QString &ASettingsNS)
{
  if (ASettingsNS.isEmpty())
  {
    FDisableDirectConnect = DEFAULT_DIABLE_DIRECT_CONNECT;
    FUseNativeServerProxy = DEFAULT_USE_NATIVE_SERVER_PROXY;
    FUseAccountNetworkProxy = DEFAULT_USE_ACCOUNT_NETPROXY;
  }
  else if (FSettings)
  {
    FSettings->deleteNS(ASettingsNS);
  }
  if (FConnectionManager)
  {
    FConnectionManager->deleteProxySettings(PROXY_NS_PREFIX+ASettingsNS);
  }
}

quint16 SocksStreams::serverPort() const
{
  return FServerPort;
}

void SocksStreams::setServerPort(quint16 APort)
{
  if (FServerPort != APort)
  {
    FServerPort = APort;
    if (FServer.isListening() && FServer.serverPort()!=FServerPort)
    {
      FServer.close();
      FServer.listen(QHostAddress::Any, FServerPort);
    }
  }
}

bool SocksStreams::disableDirectConnections(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_DISABLE_DIRECT_CONNECT,ASettingsNS,FDisableDirectConnect).toBool();
  return FDisableDirectConnect;
}

void SocksStreams::setDisableDirectConnections(const QString &ASettingsNS, bool ADisable)
{
  if (ASettingsNS.isEmpty())
    FDisableDirectConnect = ADisable;
  else if (FSettings && FDisableDirectConnect == ADisable)
    FSettings->deleteValueNS(SVN_DISABLE_DIRECT_CONNECT, ASettingsNS);
  else if (FSettings)
    FSettings->setValueNS(SVN_DISABLE_DIRECT_CONNECT, ASettingsNS, ADisable);
}

bool SocksStreams::useAccountNetworkProxy(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_USE_ACCOUNT_NETPROXY, ASettingsNS, FUseAccountNetworkProxy).toBool();
  return FUseAccountNetworkProxy;
}

void SocksStreams::setUseAccountNetworkProxy(const QString &ASettingsNS, bool AUse)
{
  if (ASettingsNS.isEmpty())
    FUseAccountNetworkProxy = AUse;
  else if (FSettings && FUseAccountNetworkProxy == AUse)
    FSettings->deleteValueNS(SVN_USE_ACCOUNT_NETPROXY, ASettingsNS);
  else if (FSettings)
    FSettings->setValueNS(SVN_USE_ACCOUNT_NETPROXY, ASettingsNS, AUse);
}

bool SocksStreams::useNativeServerProxy(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_USE_NATIVE_SERVER_PROXY,ASettingsNS,FUseNativeServerProxy).toBool();
  return FUseNativeServerProxy;
}

void SocksStreams::setUseNativeServerProxy(const QString &ASettingsNS, bool AAppend)
{
  if (ASettingsNS.isEmpty())
    FUseNativeServerProxy = AAppend;
  else if (FSettings && FUseNativeServerProxy == AAppend)
    FSettings->deleteValueNS(SVN_USE_NATIVE_SERVER_PROXY, ASettingsNS);
  else if (FSettings)
    FSettings->setValueNS(SVN_USE_NATIVE_SERVER_PROXY, ASettingsNS, AAppend);
}

QString SocksStreams::forwardHost(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_FORWARD_HOST,ASettingsNS,FForwardHost).toString();
  return FForwardHost;
}

quint16 SocksStreams::forwardPort(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_FORWARD_PORT,ASettingsNS,FForwardPort).toInt();
  return FForwardPort;
}

void SocksStreams::setForwardAddress(const QString &ASettingsNS, const QString &AHost, quint16 APort)
{
  if (ASettingsNS.isEmpty())
  {
    FForwardHost = AHost;
    FForwardPort = APort;
  }
  else if (FSettings && FForwardHost==AHost && FForwardPort==APort)
  {
    FSettings->deleteValueNS(SVN_FORWARD_HOST, ASettingsNS);
    FSettings->deleteValueNS(SVN_FORWARD_PORT, ASettingsNS);
  }
  else if (FSettings)
  {
    FSettings->setValueNS(SVN_FORWARD_HOST, ASettingsNS, AHost);
    FSettings->setValueNS(SVN_FORWARD_PORT, ASettingsNS, APort);
  }
}

QNetworkProxy SocksStreams::accountNetworkProxy(const Jid &AStreamJid) const
{
  QNetworkProxy proxy(QNetworkProxy::NoProxy);
  IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
  IDefaultConnection *connection = stream!=NULL ? qobject_cast<IDefaultConnection *>(stream->connection()->instance()) : NULL;
  return connection!=NULL ? connection->proxy() : QNetworkProxy(QNetworkProxy::NoProxy);
}

QNetworkProxy SocksStreams::networkProxy(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
  {
    QNetworkProxy proxy;
    proxy.setType((QNetworkProxy::ProxyType)FSettings->valueNS(SVN_NETPROXY_TYPE, ASettingsNS, FNetworkProxy.type()).toInt());
    proxy.setHostName(FSettings->valueNS(SVN_NETPROXY_HOST, ASettingsNS, FNetworkProxy.hostName()).toString());
    proxy.setPort(FSettings->valueNS(SVN_NETPROXY_PORT, ASettingsNS, FNetworkProxy.port()).toInt());
    proxy.setUser(FSettings->valueNS(SVN_NETPROXY_USER, ASettingsNS, FNetworkProxy.user()).toString());
    QString defPass = FSettings->encript(FNetworkProxy.password(), ASettingsNS.toUtf8());
    proxy.setPassword(FSettings->decript(FSettings->valueNS(SVN_NETPROXY_PASSWORD, ASettingsNS, defPass).toByteArray(), ASettingsNS.toUtf8()));
    return proxy;
  }
  return FNetworkProxy;
}

void SocksStreams::setNetworkProxy(const QString &ASettingsNS, const QNetworkProxy &AProxy)
{
  if (ASettingsNS.isEmpty())
  {
    FNetworkProxy = AProxy;
  }
  else if (FSettings && FNetworkProxy == AProxy)
  {
    FSettings->deleteValueNS(SVN_NETPROXY_TYPE, ASettingsNS);
    FSettings->deleteValueNS(SVN_NETPROXY_HOST, ASettingsNS);
    FSettings->deleteValueNS(SVN_NETPROXY_PORT, ASettingsNS);
    FSettings->deleteValueNS(SVN_NETPROXY_USER, ASettingsNS);
    FSettings->deleteValueNS(SVN_NETPROXY_PASSWORD, ASettingsNS);
  }
  else if (FSettings)
  {
    FSettings->setValueNS(SVN_NETPROXY_TYPE, ASettingsNS, AProxy.type());
    FSettings->setValueNS(SVN_NETPROXY_HOST, ASettingsNS, AProxy.hostName());
    FSettings->setValueNS(SVN_NETPROXY_PORT, ASettingsNS, AProxy.port());
    FSettings->setValueNS(SVN_NETPROXY_USER, ASettingsNS, AProxy.user());
    FSettings->setValueNS(SVN_NETPROXY_PASSWORD, ASettingsNS, FSettings->encript(AProxy.password(), ASettingsNS.toUtf8()));
  }
}

QString SocksStreams::nativeServerProxy(const Jid &AStreamJid) const
{
  return FNativeProxy.value(AStreamJid);
}

QList<QString> SocksStreams::proxyList(const QString &ASettingsNS) const
{
  if (FSettings && !ASettingsNS.isEmpty())
    return FSettings->valueNS(SVN_PROXY_LIST, ASettingsNS, QStringList(FProxyList).join("||")).toString().split("||",QString::SkipEmptyParts);
  return FProxyList;
}

void SocksStreams::setProxyList(const QString &ASettingsNS, const QList<QString> &AProxyList)
{
  QList<QString> nsProxyList = proxyList(ASettingsNS);
  if (nsProxyList != AProxyList)
  {
    if (ASettingsNS.isEmpty())
      FProxyList = AProxyList;
    else
      FSettings->setValueNS(SVN_PROXY_LIST, ASettingsNS, QStringList(AProxyList).join("||"));
  }
}

QString SocksStreams::connectionKey(const QString &ASessionId, const Jid &AInitiator, const Jid &ATarget) const
{
  QString keyString = ASessionId + AInitiator.prepared().eFull() + ATarget.prepared().eFull();
  QByteArray keyData = QCryptographicHash::hash(keyString.toUtf8(), QCryptographicHash::Sha1).toHex();
  return QString::fromUtf8(keyData.constData(), keyData.size()).toLower();
}

bool SocksStreams::appendLocalConnection(const QString &AKey)
{
  if (!AKey.isEmpty() && !FLocalKeys.contains(AKey))
  {
    if (FServer.isListening() || FServer.listen(QHostAddress::Any, serverPort()))
    {
      FLocalKeys.append(AKey);
      return true;
    }
  }
  return false;
}

void SocksStreams::removeLocalConnection(const QString &AKey)
{
  if (FLocalKeys.contains(AKey))
    FLocalKeys.removeAll(AKey);
  if (FLocalKeys.isEmpty())
    FServer.close();
}

void SocksStreams::onSettingsOpened()
{
  FSettings = FSettingsPlugin->settingsForPlugin(SOCKSSTREAMS_UUID);

  FServerPort = FSettings->valueNS(SVN_SERVER_PORT, QString::null, DEFAULT_SERVER_PORT).toInt();
  FDisableDirectConnect = FSettings->valueNS(SVN_DISABLE_DIRECT_CONNECT, QString::null, DEFAULT_DIABLE_DIRECT_CONNECT).toBool();
  FUseNativeServerProxy = FSettings->valueNS(SVN_USE_NATIVE_SERVER_PROXY, QString::null, DEFAULT_USE_NATIVE_SERVER_PROXY).toBool();
  FUseAccountNetworkProxy = FSettings->valueNS(SVN_USE_ACCOUNT_NETPROXY, QString::null, DEFAULT_USE_ACCOUNT_NETPROXY).toBool();
  FForwardHost = FSettings->valueNS(SVN_FORWARD_HOST, QString::null).toString();
  FForwardPort = FSettings->valueNS(SVN_FORWARD_PORT, QString::null).toInt();

  FNetworkProxy.setType((QNetworkProxy::ProxyType)FSettings->valueNS(SVN_NETPROXY_TYPE, QString::null, QNetworkProxy::NoProxy).toInt());
  FNetworkProxy.setHostName(FSettings->valueNS(SVN_NETPROXY_HOST, QString::null, FNetworkProxy.hostName()).toString());
  FNetworkProxy.setPort(FSettings->valueNS(SVN_NETPROXY_PORT, QString::null, FNetworkProxy.port()).toInt());
  FNetworkProxy.setUser(FSettings->valueNS(SVN_NETPROXY_USER, QString::null, FNetworkProxy.user()).toString());
  FNetworkProxy.setPassword(FSettings->decript(FSettings->valueNS(SVN_NETPROXY_PASSWORD, QString::null).toByteArray(), QString(SVN_NETPROXY_PASSWORD).toUtf8()));
  
  FProxyList = FSettings->valueNS(SVN_PROXY_LIST, QString::null).toString().split("||", QString::SkipEmptyParts);
}

void SocksStreams::onSettingsClosed()
{
  FSettings->setValueNS(SVN_SERVER_PORT, QString::null, FServerPort);
  FSettings->setValueNS(SVN_DISABLE_DIRECT_CONNECT, QString::null, FDisableDirectConnect);
  FSettings->setValueNS(SVN_USE_NATIVE_SERVER_PROXY, QString::null, FUseNativeServerProxy);
  FSettings->setValueNS(SVN_USE_ACCOUNT_NETPROXY, QString::null, FUseAccountNetworkProxy);
  FSettings->setValueNS(SVN_FORWARD_HOST, QString::null, FForwardHost);
  FSettings->setValueNS(SVN_FORWARD_PORT, QString::null, FForwardPort);

  FSettings->setValueNS(SVN_NETPROXY_TYPE, QString::null, FNetworkProxy.type());
  FSettings->setValueNS(SVN_NETPROXY_HOST, QString::null, FNetworkProxy.hostName());
  FSettings->setValueNS(SVN_NETPROXY_PORT, QString::null, FNetworkProxy.port());
  FSettings->setValueNS(SVN_NETPROXY_USER, QString::null, FNetworkProxy.user());
  FSettings->setValueNS(SVN_NETPROXY_PASSWORD, QString::null, FSettings->encript(FNetworkProxy.password(), QString(SVN_NETPROXY_PASSWORD).toUtf8()));

  FSettings->setValueNS(SVN_PROXY_LIST, QString::null, QStringList(FProxyList).join("||"));

  FSettings = NULL;
}

void SocksStreams::onXmppStreamOpened(IXmppStream *AStream)
{
  if (FDiscovery)
    FDiscovery->requestDiscoItems(AStream->streamJid(), AStream->streamJid().domain());
}

void SocksStreams::onXmppStreamClosed(IXmppStream *AStream)
{
  FNativeProxy.remove(AStream->streamJid());
}

void SocksStreams::onDiscoItemsReceived(const IDiscoItems &AItems)
{
  if (AItems.contactJid==AItems.streamJid.domain() && AItems.node.isEmpty())
  {
    FNativeProxy.remove(AItems.streamJid);
    Jid proxyJid = "proxy." + AItems.streamJid.domain();
    foreach(IDiscoItem item, AItems.items)
    {
      if (item.itemJid == proxyJid)
      {
        FNativeProxy.insert(AItems.streamJid, item.itemJid.full());
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
        tcpsocket->write(outData);
      }
      else
        tcpsocket->disconnectFromHost();
    }
    else
    {
      unsigned char keyLen = inData.size()>4 ? inData.at(4) : 0;
      if (inData.size()>=5+keyLen+2)
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
          emit localConnectionAccepted(key,tcpsocket);
        }
        else
          tcpsocket->disconnectFromHost();
      }
      else
        tcpsocket->disconnectFromHost();
    }
  }
}

void SocksStreams::onServerConnectionDisconnected()
{
  QTcpSocket *tcpsocket = qobject_cast<QTcpSocket *>(sender());
  if (tcpsocket)
    tcpsocket->deleteLater();
}

Q_EXPORT_PLUGIN2(plg_socksstreams, SocksStreams);
