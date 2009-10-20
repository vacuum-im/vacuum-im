#include "defaultconnectionplugin.h"

#define SVN_CONNECTION_HOST                 "connection[]:host"
#define SVN_CONNECTION_PORT                 "connection[]:port"
#define SVN_CONNECTION_USE_SSL              "connection[]:useSSL"
#define SVN_CONNECTION_IGNORE_SSLERROR      "connection[]:ingnoreSSLErrors"
#define SVN_CONNECTION_PROXY_TYPE           "connection[]:proxyType"
#define SVN_CONNECTION_PROXY_HOST           "connection[]:proxyHost"
#define SVN_CONNECTION_PROXY_PORT           "connection[]:proxyPort"
#define SVN_CONNECTION_PROXY_USER           "connection[]:proxyUser"
#define SVN_CONNECTION_PROXY_PSWD           "connection[]:proxyPassword"

DefaultConnectionPlugin::DefaultConnectionPlugin()
{
  FSettings = NULL;
  FSettingsPlugin = NULL;
  FXmppStreams = NULL;
}

DefaultConnectionPlugin::~DefaultConnectionPlugin()
{

}

void DefaultConnectionPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Default connection");
  APluginInfo->description = tr("Creating standard connection to jabber server");
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->version = "1.0";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(SETTINGS_UUID);
}

bool DefaultConnectionPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),SLOT(onOptionsDialogClosed()));
    }
  }
  
  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FSettingsPlugin!=NULL;
}

bool DefaultConnectionPlugin::initObjects()
{
  if (FSettingsPlugin)
    FSettings = FSettingsPlugin->settingsForPlugin(DEFAULTCONNECTION_UUID);
  return true;
}

QString DefaultConnectionPlugin::displayName() const
{
  return tr("Default connection");
}

IConnection *DefaultConnectionPlugin::newConnection(const QString &ASettingsNS, QObject *AParent)
{
  DefaultConnection *connection = new DefaultConnection(this,AParent);
  connect(connection,SIGNAL(aboutToConnect()),SLOT(onConnectionAboutToConnect()));
  emit connectionCreated(connection);
  loadSettings(connection,ASettingsNS);
  FCleanupHandler.add(connection);
  return connection;
}

void DefaultConnectionPlugin::destroyConnection(IConnection *AConnection)
{
  DefaultConnection *connection = qobject_cast<DefaultConnection *>(AConnection->instance());
  if (connection)
  {
    emit connectionDestroyed(connection);
    delete connection;
  }
}

void DefaultConnectionPlugin::loadSettings(IConnection *AConnection, const QString &ASettingsNS)
{
  if (FSettings)
  {
    AConnection->setOption(IDefaultConnection::CO_HOST,FSettings->valueNS(SVN_CONNECTION_HOST,ASettingsNS,QString()));
    AConnection->setOption(IDefaultConnection::CO_PORT,FSettings->valueNS(SVN_CONNECTION_PORT,ASettingsNS,5222));
    AConnection->setOption(IDefaultConnection::CO_USE_SSL,FSettings->valueNS(SVN_CONNECTION_USE_SSL,ASettingsNS,false));
    AConnection->setOption(IDefaultConnection::CO_IGNORE_SSL_ERRORS,FSettings->valueNS(SVN_CONNECTION_IGNORE_SSLERROR,ASettingsNS,true));
    AConnection->setOption(IDefaultConnection::CO_PROXY_TYPE,FSettings->valueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,0));
    AConnection->setOption(IDefaultConnection::CO_PROXY_HOST,FSettings->valueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,QString()));
    AConnection->setOption(IDefaultConnection::CO_PROXY_PORT,FSettings->valueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,0));
    AConnection->setOption(IDefaultConnection::CO_PROXY_USER_NAME,FSettings->valueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,QString()));
    AConnection->setOption(IDefaultConnection::CO_PROXY_PASSWORD, 
      FSettings->decript(FSettings->valueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,QString()).toByteArray(),ASettingsNS.toUtf8()));
    emit connectionUpdated(AConnection,ASettingsNS);
  }
}

void DefaultConnectionPlugin::saveSettings(IConnection *AConnection, const QString &ASettingsNS)
{
  if (FSettings)
  {
    FSettings->setValueNS(SVN_CONNECTION_HOST,ASettingsNS,AConnection->option(IDefaultConnection::CO_HOST));
    FSettings->setValueNS(SVN_CONNECTION_PORT,ASettingsNS,AConnection->option(IDefaultConnection::CO_PORT));
    FSettings->setValueNS(SVN_CONNECTION_USE_SSL,ASettingsNS,AConnection->option(IDefaultConnection::CO_USE_SSL));
    FSettings->setValueNS(SVN_CONNECTION_IGNORE_SSLERROR,ASettingsNS,AConnection->option(IDefaultConnection::CO_IGNORE_SSL_ERRORS));
    FSettings->setValueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,AConnection->option(IDefaultConnection::CO_PROXY_TYPE));
    FSettings->setValueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,AConnection->option(IDefaultConnection::CO_PROXY_HOST));
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,AConnection->option(IDefaultConnection::CO_PROXY_PORT));
    FSettings->setValueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,AConnection->option(IDefaultConnection::CO_PROXY_USER_NAME));
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,
      FSettings->encript(AConnection->option(IDefaultConnection::CO_PROXY_PASSWORD).toString(),ASettingsNS.toUtf8()));
  }
}

void DefaultConnectionPlugin::deleteSettingsNS(const QString &ASettingsNS)
{
  if (FSettings)
    FSettings->deleteNS(ASettingsNS);
}

QWidget *DefaultConnectionPlugin::optionsWidget(const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = new ConnectionOptionsWidget;
  if (FSettings)
  {
    widget->setHost(FSettings->valueNS(SVN_CONNECTION_HOST,ASettingsNS,QString()).toString());
    widget->setPort(FSettings->valueNS(SVN_CONNECTION_PORT,ASettingsNS,5222).toInt());
    widget->setUseSSL(FSettings->valueNS(SVN_CONNECTION_USE_SSL,ASettingsNS,false).toBool());
    widget->setIgnoreSSLError(FSettings->valueNS(SVN_CONNECTION_IGNORE_SSLERROR,ASettingsNS,false).toBool());
    widget->setProxyTypes(proxyTypeNames());
    widget->setProxyType(FSettings->valueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,0).toInt());
    widget->setProxyHost(FSettings->valueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,QString()).toString());
    widget->setProxyPort(FSettings->valueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,0).toInt());
    widget->setProxyUserName(FSettings->valueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,QString()).toString());
    widget->setProxyPassword(FSettings->decript(FSettings->valueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,QString()).toByteArray(),ASettingsNS.toUtf8())); 
  }
  FWidgetsByNS.insert(ASettingsNS,widget);
  return widget;
}

void DefaultConnectionPlugin::saveOptions(const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = FWidgetsByNS.value(ASettingsNS,NULL);
  if (FSettings && widget)
  {
    FSettings->setValueNS(SVN_CONNECTION_HOST,ASettingsNS,widget->host());
    FSettings->setValueNS(SVN_CONNECTION_PORT,ASettingsNS,widget->port());
    FSettings->setValueNS(SVN_CONNECTION_USE_SSL,ASettingsNS,widget->useSSL());
    FSettings->setValueNS(SVN_CONNECTION_IGNORE_SSLERROR,ASettingsNS,widget->ignoreSSLErrors());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,widget->proxyType());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,widget->proxyHost());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,widget->proxyPort());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,widget->proxyUserName());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,
      FSettings->encript(widget->proxyPassword(),ASettingsNS.toUtf8()));
  }
}

QStringList DefaultConnectionPlugin::proxyTypeNames() const
{
  return QStringList() << tr("Default proxy") << tr("Without proxy") << tr("Socket5 proxy") << tr("HTTP proxy");
}

void DefaultConnectionPlugin::onConnectionAboutToConnect()
{
  DefaultConnection *connection = qobject_cast<DefaultConnection*>(sender());
  if (FXmppStreams && connection && connection->option(IDefaultConnection::CO_HOST).toString().isEmpty())
  {
    foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
      if (stream->connection() == connection)
      {
        connection->setOption(IDefaultConnection::CO_HOST,stream->streamJid().pDomain());
        break;
      }
  }
}

void DefaultConnectionPlugin::onOptionsDialogClosed()
{
  FWidgetsByNS.clear();
}

Q_EXPORT_PLUGIN2(DefaultConnectionPlugin, DefaultConnectionPlugin)
