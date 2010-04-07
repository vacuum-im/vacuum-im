#include "connectionmanager.h" 

#define SVN_PROXY             "proxy[]"
#define SVN_PROXY_NAME        SVN_PROXY ":name"
#define SVN_PROXY_TYPE        SVN_PROXY ":type"
#define SVN_PROXY_HOST        SVN_PROXY ":host"
#define SVN_PROXY_PORT        SVN_PROXY ":port"
#define SVN_PROXY_USER        SVN_PROXY ":user"
#define SVN_PROXY_PASS        SVN_PROXY ":pass"

#define SVN_DEFAULT_PROXY     "defaultProxy"

#define SVN_SETTINGS          "settings[]"
#define SVN_SETTINGS_PROXY    SVN_SETTINGS ":proxy"

ConnectionManager::ConnectionManager()
{
  FEncryptedLabelId = -1;
  FAccountManager = NULL;
  FRostersViewPlugin = NULL;
  FSettingsPlugin = NULL;
}

ConnectionManager::~ConnectionManager()
{

}

void ConnectionManager::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Connection Manager");
  APluginInfo->description = tr("Allows to use different types of connections to a Jabber server");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool ConnectionManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  QList<IPlugin *> plugins = APluginManager->pluginInterface("IConnectionPlugin");
  foreach (IPlugin *plugin, plugins)
  {
    IConnectionPlugin *cplugin = qobject_cast<IConnectionPlugin *>(plugin->instance());
    if (cplugin)
    {
      connect(cplugin->instance(),SIGNAL(connectionCreated(IConnection *)),SIGNAL(connectionCreated(IConnection *)));
      connect(cplugin->instance(),SIGNAL(connectionUpdated(IConnection *, const QString &)),
        SIGNAL(connectionUpdated(IConnection *, const QString &)));
      connect(cplugin->instance(),SIGNAL(connectionDestroyed(IConnection *)),SIGNAL(connectionDestroyed(IConnection *)));
      FPlugins.append(cplugin);
    }
  }

  IPlugin *plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(destroyed(const QUuid &)),SLOT(onAccountDestroyed(const QUuid &)));
    }
  }

  plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      connect(xmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(xmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
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


  return !FPlugins.isEmpty();
}

bool ConnectionManager::initObjects()
{
  if (FRostersViewPlugin)
  {
    QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CONNECTION_ENCRYPTED);
    FEncryptedLabelId = FRostersViewPlugin->rostersView()->createIndexLabel(RLO_CONNECTION_ENCRYPTED,icon);
  }
  if (FAccountManager && FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
  }
  return true;
}

QWidget *ConnectionManager::optionsWidget(const QString &ANode, int &AOrder)
{
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    AOrder = OWO_ACCOUNT_CONNECTION;
    ConnectionOptionsWidget *widget = new ConnectionOptionsWidget(this,FAccountManager,nodeTree.at(1));
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FAccountManager->instance(),SIGNAL(optionsAccepted()),widget,SLOT(apply()));
    connect(FAccountManager->instance(),SIGNAL(optionsRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

QList<IConnectionPlugin *> ConnectionManager::pluginList() const
{
  return FPlugins;
}

IConnectionPlugin *ConnectionManager::pluginById(const QUuid &APluginId) const
{
  foreach (IConnectionPlugin *plugin, FPlugins)
    if (plugin->pluginUuid() == APluginId)
      return plugin;
  return NULL;
}

QList<QUuid> ConnectionManager::proxyList() const
{
  return FProxys.keys();
}

IConnectionProxy ConnectionManager::proxyById(const QUuid &AProxyId) const
{
  static const IConnectionProxy noProxy = {" "+tr("<No Proxy>"), QNetworkProxy(QNetworkProxy::NoProxy) };
  return AProxyId!=DEFAULT_PROXY_REF_UUID ? FProxys.value(AProxyId,noProxy) : FProxys.value(FDefaultProxy,noProxy);
}

void ConnectionManager::setProxy(const QUuid &AProxyId, const IConnectionProxy &AProxy)
{
  if (!AProxyId.isNull() && AProxyId!=DEFAULT_PROXY_REF_UUID)
  {
    FProxys.insert(AProxyId, AProxy);
    emit proxyChanged(AProxyId, AProxy);
  }
}

void ConnectionManager::removeProxy(const QUuid &AProxyId)
{
  if (FProxys.contains(AProxyId))
  {
    if (FDefaultProxy == AProxyId)
      setDefaultProxy(QUuid());
    FProxys.remove(AProxyId);
    emit proxyRemoved(AProxyId);
  }
}

QUuid ConnectionManager::defaultProxy() const
{
  return FDefaultProxy;
}

void ConnectionManager::setDefaultProxy(const QUuid &AProxyId)
{
  if (FDefaultProxy!=AProxyId && (AProxyId.isNull() || FProxys.contains(AProxyId)))
  {
    FDefaultProxy = AProxyId;
    QNetworkProxy::setApplicationProxy(proxyById(FDefaultProxy).proxy);
    emit defaultProxyChanged(FDefaultProxy);
  }
}

QDialog *ConnectionManager::showEditProxyDialog(QWidget *AParent)
{
  EditProxyDialog *dialog = new EditProxyDialog(this, AParent);
  dialog->show();
  return dialog;
}

QWidget *ConnectionManager::proxySettingsWidget(const QString &ASettingsNS, QWidget *AParent)
{
  ProxySettingsWidget *widget = new ProxySettingsWidget(this,ASettingsNS,AParent);
  return widget;
}

void ConnectionManager::saveProxySettings(QWidget *AWidget, const QString &ASettingsNS)
{
  ProxySettingsWidget *widget = qobject_cast<ProxySettingsWidget *>(AWidget);
  if (widget)
    widget->apply(ASettingsNS);
}

QUuid ConnectionManager::proxySettings(const QString &ASettingsNS) const
{
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    return settings->valueNS(SVN_SETTINGS_PROXY,ASettingsNS,DEFAULT_PROXY_REF_UUID).toString();
  }
  return DEFAULT_PROXY_REF_UUID;
}

void ConnectionManager::setProxySettings(const QString &ASettingsNS, const QUuid &AProxyId)
{
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    settings->setValueNS(SVN_SETTINGS_PROXY,ASettingsNS,AProxyId.toString());
  }
}

void ConnectionManager::deleteProxySettings(const QString &ASettingsNS)
{
  if (FSettingsPlugin)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
    settings->deleteValueNS(SVN_SETTINGS,ASettingsNS);
  }
}

IConnectionPlugin *ConnectionManager::defaultPlugin() const
{
  IConnectionPlugin *plugin = pluginById(DEFAULTCONNECTION_UUID);
  return plugin==NULL ? FPlugins.value(0) : plugin;
}

IConnection *ConnectionManager::insertConnection(IAccount *AAccount) const
{
  if (AAccount->isActive())
  {
    QUuid pluginId = AAccount->value(AVN_CONNECTION_ID).toString();
    IConnectionPlugin *plugin = pluginById(pluginId);
    IConnection *connection = AAccount->xmppStream()->connection();
    if (connection && connection->ownerPlugin()!=plugin)
    {
      AAccount->xmppStream()->setConnection(NULL);
      connection->ownerPlugin()->destroyConnection(connection);
      connection = NULL;
    }
    if (plugin!=NULL && connection==NULL)
    {
      connection = plugin->newConnection(AAccount->accountId().toString(),AAccount->xmppStream()->instance());
      AAccount->xmppStream()->setConnection(connection);
    }
    return connection;
  }
  return NULL;
}

void ConnectionManager::onAccountShown(IAccount *AAccount)
{
  insertConnection(AAccount);
}

void ConnectionManager::onAccountDestroyed(const QUuid &AAccount)
{
  foreach (IConnectionPlugin *plugin, FPlugins)
    plugin->deleteSettings(AAccount.toString());
}


void ConnectionManager::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FRostersViewPlugin && AXmppStream->connection() && AXmppStream->connection()->isEncrypted())
  {
    IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
    IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->streamJid()) : NULL;
    if (index!=NULL)
      FRostersViewPlugin->rostersView()->insertIndexLabel(FEncryptedLabelId,index);
  }
}

void ConnectionManager::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FRostersViewPlugin)
  {
    IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
    IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->streamJid()) : NULL;
    if (index!=NULL)
      FRostersViewPlugin->rostersView()->removeIndexLabel(FEncryptedLabelId,index);
  }
}

void ConnectionManager::onSettingsOpened()
{
  FProxys.clear();

  ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
  foreach(QString ns, settings->values(SVN_PROXY).keys())
  {
    IConnectionProxy proxy;
    proxy.name = settings->valueNS(SVN_PROXY_NAME, ns).toString();
    proxy.proxy.setType((QNetworkProxy::ProxyType)settings->valueNS(SVN_PROXY_TYPE, ns).toInt());
    proxy.proxy.setHostName(settings->valueNS(SVN_PROXY_HOST, ns).toString());
    proxy.proxy.setPort(settings->valueNS(SVN_PROXY_PORT, ns).toInt());
    proxy.proxy.setUser(settings->valueNS(SVN_PROXY_USER, ns).toString());
    proxy.proxy.setPassword(settings->decript(settings->valueNS(SVN_PROXY_PASS, ns).toByteArray(),ns.toUtf8()));
    setProxy(ns, proxy);
  }

  setDefaultProxy(settings->value(SVN_DEFAULT_PROXY,QUuid().toString()).toString());
}

void ConnectionManager::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
  
  settings->setValue(SVN_DEFAULT_PROXY, FDefaultProxy.toString());

  QSet<QString> oldProxy = settings->values(SVN_PROXY).keys().toSet();
  foreach(QUuid id, FProxys.keys())
  {
    IConnectionProxy proxy = FProxys.value(id);
    settings->setValueNS(SVN_PROXY_NAME, id.toString(), proxy.name);
    settings->setValueNS(SVN_PROXY_TYPE, id.toString(), proxy.proxy.type());
    settings->setValueNS(SVN_PROXY_HOST, id.toString(), proxy.proxy.hostName());
    settings->setValueNS(SVN_PROXY_PORT, id.toString(), proxy.proxy.port());
    settings->setValueNS(SVN_PROXY_USER, id.toString(), proxy.proxy.user());
    settings->setValueNS(SVN_PROXY_PASS, id.toString(), settings->encript(proxy.proxy.password(), id.toString().toUtf8()));
    oldProxy -= id.toString();
    removeProxy(id);
  }

  foreach(QString ns, oldProxy)
    settings->deleteValueNS(SVN_PROXY, ns);
}

Q_EXPORT_PLUGIN2(plg_connectionmanager, ConnectionManager)
