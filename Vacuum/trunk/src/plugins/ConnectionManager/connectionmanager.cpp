#include "connectionmanager.h" 

#include <QVBoxLayout>

#define AVN_CONNECTION_ID          "connectionId"

ConnectionManager::ConnectionManager()
{
  FAccountManager = NULL;
  FSettingsPlugin = NULL;
}

ConnectionManager::~ConnectionManager()
{

}

void ConnectionManager::pluginInfo(PluginInfo *APluginInfo)
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
  PluginList plugins = APluginManager->getPlugins("IConnectionPlugin");
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
      connect(FAccountManager->instance(),SIGNAL(added(IAccount *)),SLOT(onAccountAdded(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(destroyed(IAccount *)),SLOT(onAccountDestroyed(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(optionsAccepted()),SLOT(onOptionsAccepted()));
      connect(FAccountManager->instance(),SIGNAL(optionsRejected()),SLOT(onOptionsRejected()));
    }
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
  FSettingsPlugin->appendOptionsHolder(this);
  return true;
}

QWidget *ConnectionManager::optionsWidget(const QString &ANode, int &AOrder)
{
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    AOrder = OO_ACCOUNT_CONNECTION;
    QUuid pluginId;
    QString accountId = nodeTree.at(1);
    IAccount *account = FAccountManager->accountById(accountId);
    if (account && account->xmppStream()->connection())
      pluginId = account->xmppStream()->connection()->ownerPlugin()->pluginUuid();
    else
      pluginId = defaultPlugin()->pluginUuid();
    ConnectionOptionsWidget *widget = new ConnectionOptionsWidget(this,accountId,pluginId);
    FOptionsWidgets.append(widget);
    return widget;
  }
  return NULL;
}

IConnectionPlugin *ConnectionManager::pluginById(const QUuid &APluginId)
{
  if (!APluginId.isNull())
  {
    foreach (IConnectionPlugin *plugin, FConnectionPlugins)
      if (plugin->pluginUuid() == APluginId)
        return plugin;
  }
  return NULL;
}

IConnectionPlugin * ConnectionManager::defaultPlugin() const
{
  foreach (IConnectionPlugin *plugin, FConnectionPlugins)
    if (plugin->pluginUuid() == DEFAULTCONNECTION_UUID)
      return plugin;

  return FConnectionPlugins.first();
}

void ConnectionManager::onAccountAdded(IAccount *AAccount)
{
  QUuid pluginId = AAccount->value(AVN_CONNECTION_ID).toString();
  IConnectionPlugin *plugin = pluginById(pluginId);
  if (plugin)
  {
    IConnection *connection = plugin->newConnection(AAccount->accountId(),AAccount->xmppStream()->instance());
    AAccount->xmppStream()->setConnection(connection);
  }
}

void ConnectionManager::onAccountDestroyed(IAccount *AAccount)
{
  foreach (IConnectionPlugin *plugin, FConnectionPlugins)
    plugin->deleteSettingsNS(AAccount->accountId());
}

void ConnectionManager::onOptionsAccepted()
{
  foreach (ConnectionOptionsWidget *widget, FOptionsWidgets)
  {
    IAccount *account = FAccountManager->accountById(widget->accountId());
    if (account)
    {
      IConnection *connection = account->xmppStream()->connection();
      IConnectionPlugin *plugin = pluginById(widget->pluginId());
      if (plugin)
      {
        plugin->saveOptions(account->accountId());
        if (!connection || connection->ownerPlugin()->pluginUuid() != widget->pluginId())
        {
          if (connection)
          {
            account->xmppStream()->setConnection(NULL);
            connection->ownerPlugin()->destroyConnection(connection);
          }
          IConnection *newConnection = plugin->newConnection(account->accountId(),account->xmppStream()->instance());
          account->xmppStream()->setConnection(newConnection);
        }
        else
        {
          plugin->loadSettings(connection, account->accountId());
        }
      }
      else if (connection)
      {
        connection->ownerPlugin()->destroyConnection(connection);
        account->xmppStream()->setConnection(NULL);
      }
      account->setValue(AVN_CONNECTION_ID,widget->pluginId().toString());
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
