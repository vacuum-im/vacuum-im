#include "defaultconnectionplugin.h"

#define SVN_CONNECTION_HOST                 "connection[]:host"
#define SVN_CONNECTION_PORT                 "connection[]:port"
#define SVN_CONNECTION_PROXY_TYPE           "connection[]:proxyType"
#define SVN_CONNECTION_PROXY_HOST           "connection[]:proxyHost"
#define SVN_CONNECTION_PROXY_PORT           "connection[]:proxyPort"
#define SVN_CONNECTION_PROXY_USER           "connection[]:proxyUser"
#define SVN_CONNECTION_PROXY_PSWD           "connection[]:proxyPassword"

DefaultConnectionPlugin::DefaultConnectionPlugin()
{
  FSettings = NULL;
  FSettingsPlugin = NULL;
}

DefaultConnectionPlugin::~DefaultConnectionPlugin()
{

}

void DefaultConnectionPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Creating standard connection to jabber server");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Default connection";
  APluginInfo->uid = DEFAULTCONNECTION_UUID;
  APluginInfo->version = "0.1";
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
  AConnection->setOption(IDefaultConnection::CO_Host,FSettings->valueNS(SVN_CONNECTION_HOST,ASettingsNS,QString()));
  AConnection->setOption(IDefaultConnection::CO_Port,FSettings->valueNS(SVN_CONNECTION_PORT,ASettingsNS,5222));
  AConnection->setOption(IDefaultConnection::CO_ProxyType,FSettings->valueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,0));
  AConnection->setOption(IDefaultConnection::CO_ProxyHost,FSettings->valueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,QString()));
  AConnection->setOption(IDefaultConnection::CO_ProxyPort,FSettings->valueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,0));
  AConnection->setOption(IDefaultConnection::CO_ProxyUserName,FSettings->valueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,QString()));
  AConnection->setOption(IDefaultConnection::CO_ProxyPassword, 
    FSettings->decript(FSettings->valueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,QString()).toByteArray(),ASettingsNS.toUtf8()));
  emit connectionUpdated(AConnection,ASettingsNS);
}

void DefaultConnectionPlugin::saveSettings(IConnection *AConnection, const QString &ASettingsNS)
{
  FSettings->setValueNS(SVN_CONNECTION_HOST,ASettingsNS,AConnection->option(IDefaultConnection::CO_Host));
  FSettings->setValueNS(SVN_CONNECTION_PORT,ASettingsNS,AConnection->option(IDefaultConnection::CO_Port));
  FSettings->setValueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,AConnection->option(IDefaultConnection::CO_ProxyType));
  FSettings->setValueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,AConnection->option(IDefaultConnection::CO_ProxyHost));
  FSettings->setValueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,AConnection->option(IDefaultConnection::CO_ProxyPort));
  FSettings->setValueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,AConnection->option(IDefaultConnection::CO_ProxyUserName));
  FSettings->setValueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,
    FSettings->encript(AConnection->option(IDefaultConnection::CO_ProxyPassword).toString(),ASettingsNS.toUtf8()));
}

void DefaultConnectionPlugin::deleteSettingsNS(const QString &ASettingsNS)
{
  FSettings->deleteNS(ASettingsNS);
}

QWidget *DefaultConnectionPlugin::optionsWidget(const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = new ConnectionOptionsWidget;
  widget->setHost(FSettings->valueNS(SVN_CONNECTION_HOST,ASettingsNS,QString()).toString());
  widget->setPort(FSettings->valueNS(SVN_CONNECTION_PORT,ASettingsNS,5222).toInt());
  widget->setProxyTypes(QStringList() << tr("Direct connection") << tr("Socket5 proxy") << tr("HTTPS proxy"));
  widget->setProxyType(FSettings->valueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,0).toInt());
  widget->setProxyHost(FSettings->valueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,QString()).toString());
  widget->setProxyPort(FSettings->valueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,0).toInt());
  widget->setProxyUserName(FSettings->valueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,QString()).toString());
  widget->setProxyPassword(FSettings->decript(FSettings->valueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,QString()).toByteArray(),ASettingsNS.toUtf8())); 
  FWidgetsByNS.insert(ASettingsNS,widget);
  return widget;
}

void DefaultConnectionPlugin::saveOptions(const QString &ASettingsNS)
{
  ConnectionOptionsWidget *widget = FWidgetsByNS.value(ASettingsNS,NULL);
  if (widget)
  {
    FSettings->setValueNS(SVN_CONNECTION_HOST,ASettingsNS,widget->host());
    FSettings->setValueNS(SVN_CONNECTION_PORT,ASettingsNS,widget->port());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_TYPE,ASettingsNS,widget->proxyType());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_HOST,ASettingsNS,widget->proxyHost());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PORT,ASettingsNS,widget->proxyPort());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_USER,ASettingsNS,widget->proxyUserName());
    FSettings->setValueNS(SVN_CONNECTION_PROXY_PSWD,ASettingsNS,
      FSettings->encript(widget->proxyPassword(),ASettingsNS.toUtf8()));
  }
}

void DefaultConnectionPlugin::onOptionsDialogClosed()
{
  FWidgetsByNS.clear();
}

Q_EXPORT_PLUGIN2(DefaultConnectionPlugin, DefaultConnectionPlugin)
