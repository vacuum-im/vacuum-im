#include "connectionmanager.h" 

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
  APluginInfo->homePage = "http://jrudevels.org";
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
      FConnectionPlugins.append(cplugin);
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
  }


  return !FConnectionPlugins.isEmpty();
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

IConnectionPlugin *ConnectionManager::pluginById(const QUuid &APluginId) const
{
  foreach (IConnectionPlugin *plugin, FConnectionPlugins)
    if (plugin->pluginUuid() == APluginId)
      return plugin;
  return NULL;
}

IConnectionPlugin *ConnectionManager::defaultPlugin() const
{
  IConnectionPlugin *plugin = pluginById(DEFAULTCONNECTION_UUID);
  return plugin==NULL ? FConnectionPlugins.first() : plugin;
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
  foreach (IConnectionPlugin *plugin, FConnectionPlugins)
    plugin->deleteSettingsNS(AAccount.toString());
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

Q_EXPORT_PLUGIN2(plg_connectionmanager, ConnectionManager)
