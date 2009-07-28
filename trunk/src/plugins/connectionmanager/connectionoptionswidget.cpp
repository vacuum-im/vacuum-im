#include "connectionoptionswidget.h"

ConnectionOptionsWidget::ConnectionOptionsWidget(ConnectionManager *ACManager, IAccountManager *AAManager, const QUuid &AAccountId)
{
  ui.setupUi(this);
  FAccountManager = AAManager;
  FConnectionManager = ACManager;

  FAccountId = AAccountId;
  FOptionsWidget = NULL;

  QList<IConnectionPlugin *> plugins = FConnectionManager->pluginList();
  foreach (IConnectionPlugin *plugin, plugins)
    ui.cmbConnections->addItem(plugin->displayName(),plugin->pluginUuid().toString());
  connect(ui.cmbConnections, SIGNAL(currentIndexChanged(int)),SLOT(onComboConnectionsChanged(int)));

  if (plugins.count() < 2)
    ui.wdtSelectConnection->setVisible(false);

  QUuid pluginId = FConnectionManager->defaultPlugin()->pluginUuid();
  IAccount *account = FAccountManager->accountById(FAccountId);
  if (account != NULL)
    pluginId = account->value(AVN_CONNECTION_ID,pluginId.toString()).toString();

  setPluginById(pluginId);
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply()
{
  IAccount *account = FAccountManager->accountById(FAccountId);
  if (account)
  {
    account->setValue(AVN_CONNECTION_ID,FPluginId.toString());
    IConnectionPlugin *plugin = FConnectionManager->pluginById(FPluginId);
    if (plugin)
    {
      plugin->saveOptions(FAccountId.toString());
      IConnection *connection = FConnectionManager->insertConnection(account);
      if (connection)
        plugin->loadSettings(connection, FAccountId.toString());
    }
  }
  emit optionsAccepted();
}

void ConnectionOptionsWidget::setPluginById(const QUuid &APluginId)
{
  if (FPluginId != APluginId)
  {
    if (FOptionsWidget)
    {
      ui.grbOptions->layout()->removeWidget(FOptionsWidget);
      FOptionsWidget->deleteLater();
      FOptionsWidget = NULL;
      FPluginId = QUuid();
    }

    IConnectionPlugin *plugin = FConnectionManager->pluginById(APluginId);
    if (plugin)
    {
      FOptionsWidget = plugin->optionsWidget(FAccountId.toString());
      if (FOptionsWidget)
      {
        ui.grbOptions->layout()->addWidget(FOptionsWidget);
        FPluginId = APluginId;
      }
    }

    if (ui.cmbConnections->itemData(ui.cmbConnections->currentIndex()).toString() != APluginId)
      ui.cmbConnections->setCurrentIndex(ui.cmbConnections->findData(APluginId.toString()));
  }
}

void ConnectionOptionsWidget::onComboConnectionsChanged(int AIndex)
{
  if (AIndex != -1)
    setPluginById(ui.cmbConnections->itemData(AIndex).toString());
  else
    setPluginById(QUuid());
}
