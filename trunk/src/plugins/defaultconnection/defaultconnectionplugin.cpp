#include "defaultconnectionplugin.h"

DefaultConnectionPlugin::DefaultConnectionPlugin()
{
  FSettings = NULL;
  FSettingsPlugin = NULL;
  FXmppStreams = NULL;
  FConnectionManager = NULL;
}

DefaultConnectionPlugin::~DefaultConnectionPlugin()
{

}

void DefaultConnectionPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Default connection");
  APluginInfo->description = tr("Allows to set a standard TCP connection to Jabber server");
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->version = "1.0";
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool DefaultConnectionPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
  
  plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
  if (plugin)
    FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());

  return true;
}

bool DefaultConnectionPlugin::initObjects()
{
  if (FSettingsPlugin)
    FSettings = FSettingsPlugin->settingsForPlugin(pluginUuid());
  return true;
}

QString DefaultConnectionPlugin::displayName() const
{
  return tr("Default connection");
}

IConnection *DefaultConnectionPlugin::newConnection(const QString &ASettingsNS, QObject *AParent)
{
  DefaultConnection *connection = new DefaultConnection(this,AParent);
  loadSettings(connection,ASettingsNS);
  FCleanupHandler.add(connection);
  connect(connection,SIGNAL(aboutToConnect()),SLOT(onConnectionAboutToConnect()));
  emit connectionCreated(connection);
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

QWidget *DefaultConnectionPlugin::settingsWidget(const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = new ConnectionOptionsWidget(FConnectionManager, FSettings, ASettingsNS);
  return widget;
}

void DefaultConnectionPlugin::saveSettings(QWidget *AWidget, const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = qobject_cast<ConnectionOptionsWidget *>(AWidget);
  if (widget)
    widget->apply(ASettingsNS);
}

void DefaultConnectionPlugin::loadSettings(IConnection *AConnection, const QString &ASettingsNS)
{
  DefaultConnection *connection = qobject_cast<DefaultConnection *>(AConnection->instance());
  if (FSettings && connection)
  {
    connection->setOption(IDefaultConnection::CO_HOST,FSettings->valueNS(SVN_CONNECTION_HOST,ASettingsNS,QString()));
    connection->setOption(IDefaultConnection::CO_PORT,FSettings->valueNS(SVN_CONNECTION_PORT,ASettingsNS,5222));
    connection->setOption(IDefaultConnection::CO_USE_SSL,FSettings->valueNS(SVN_CONNECTION_USE_SSL,ASettingsNS,false));
    connection->setOption(IDefaultConnection::CO_IGNORE_SSL_ERRORS,FSettings->valueNS(SVN_CONNECTION_IGNORE_SSLERROR,ASettingsNS,true));
    if (FConnectionManager)
      connection->setProxy(FConnectionManager->proxyById(FConnectionManager->proxySettings(ASettingsNS)).proxy);
    emit connectionUpdated(AConnection,ASettingsNS);
  }
}

void DefaultConnectionPlugin::deleteSettings(const QString &ASettingsNS)
{
  if (FSettings)
    FSettings->deleteNS(ASettingsNS);
  if (FConnectionManager)
    FConnectionManager->deleteProxySettings(ASettingsNS);
}

void DefaultConnectionPlugin::onConnectionAboutToConnect()
{
  DefaultConnection *connection = qobject_cast<DefaultConnection*>(sender());
  if (FXmppStreams && connection)
  {
    foreach(IXmppStream *stream, FXmppStreams->xmppStreams())
      if (stream->connection() == connection)
        connection->setOption(IDefaultConnection::CO_DOMAINE,stream->streamJid().pDomain());
  }
}

Q_EXPORT_PLUGIN2(plg_defaultconnection, DefaultConnectionPlugin)
