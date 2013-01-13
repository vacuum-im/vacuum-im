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
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Managing TCP connections");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Connection Manager";
  APluginInfo->uid = CONNECTIONMANAGER_UUID;
  APluginInfo->version = "0.1";
}

bool ConnectionManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  QList<IPlugin *> plugins = APluginManager->getPlugins("IConnectionPlugin");
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

  IPlugin *plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(destroyed(const QString &)),SLOT(onAccountDestroyed(const QString &)));
      connect(FAccountManager->instance(),SIGNAL(optionsAccepted()),SLOT(onOptionsAccepted()));
      connect(FAccountManager->instance(),SIGNAL(optionsRejected()),SLOT(onOptionsRejected()));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),SLOT(onOptionsDialogClosed()));
    }
  }

  return !plugins.isEmpty();
}

bool ConnectionManager::initObjects()
{
  if (FRostersViewPlugin)
  {
    QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CONNECTION_ENCRYPTED);
    FEncryptedLabelId = FRostersViewPlugin->rostersView()->createIndexLabel(RLO_CONNECTION_ENCRYPTED,icon);
  }
  if (FSettingsPlugin)
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
    AOrder = OO_ACCOUNT_CONNECTION;
    QUuid pluginId = defaultPlugin()->pluginUuid();
    QString accountId = nodeTree.at(1);
    IAccount *account = FAccountManager->accountById(accountId);
    if (account)
      pluginId = account->value(AVN_CONNECTION_ID,pluginId.toString()).toString();
    ConnectionOptionsWidget *widget = new ConnectionOptionsWidget(this,accountId,pluginId);
    FOptionsWidgets.append(widget);
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
      connection = plugin->newConnection(AAccount->accountId(),AAccount->xmppStream()->instance());
      AAccount->xmppStream()->setConnection(connection);
      connect(AAccount->xmppStream()->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(AAccount->xmppStream()->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
    }
    return connection;
  }
  return NULL;
}

void ConnectionManager::onAccountShown(IAccount *AAccount)
{
  insertConnection(AAccount);
}

void ConnectionManager::onAccountDestroyed(const QString &AAccount)
{
  foreach (IConnectionPlugin *plugin, FConnectionPlugins)
    plugin->deleteSettingsNS(AAccount);
}


void ConnectionManager::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FRostersViewPlugin && AXmppStream->connection() && AXmppStream->connection()->isEncrypted())
  {
    IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
    IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->jid()) : NULL;
    if (index!=NULL)
      FRostersViewPlugin->rostersView()->insertIndexLabel(FEncryptedLabelId,index);
  }
}

void ConnectionManager::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FRostersViewPlugin)
  {
    IRostersModel *model = FRostersViewPlugin->rostersView()->rostersModel();
    IRosterIndex *index = model!=NULL ? model->streamRoot(AXmppStream->jid()) : NULL;
    if (index!=NULL)
      FRostersViewPlugin->rostersView()->removeIndexLabel(FEncryptedLabelId,index);
  }
}

void ConnectionManager::onOptionsAccepted()
{
  foreach (ConnectionOptionsWidget *widget, FOptionsWidgets)
  {
    IAccount *account = FAccountManager->accountById(widget->accountId());
    if (account)
    {
      account->setValue(AVN_CONNECTION_ID,widget->pluginId().toString());
      IConnectionPlugin *plugin = pluginById(widget->pluginId());
      if (plugin)
      {
        plugin->saveOptions(account->accountId());
        IConnection *connection = insertConnection(account);
        if (connection)
          plugin->loadSettings(connection, account->accountId());
      }
    }
  }
  emit optionsAccepted();
}

void ConnectionManager::onOptionsRejected()
{
  emit optionsRejected();
}

void ConnectionManager::onOptionsDialogClosed()
{
  FOptionsWidgets.clear();
}

Q_EXPORT_PLUGIN2(ConnectionManagerPlugin, ConnectionManager)
