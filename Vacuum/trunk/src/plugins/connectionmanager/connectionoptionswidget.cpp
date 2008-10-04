#include "connectionoptionswidget.h"

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const QString &AAccountId, const QUuid &APluginId)
{
  ui.setupUi(this);
  FOptionsWidget = NULL;
  FManager = AManager;
  FAccountId = AAccountId;

  QList<IConnectionPlugin *> plugins = FManager->pluginList();
  foreach (IConnectionPlugin *plugin, plugins)
    ui.cmbConnections->addItem(plugin->displayName(),plugin->pluginUuid().toString());
  connect(ui.cmbConnections, SIGNAL(currentIndexChanged(int)),SLOT(onComboConnectionsChanged(int)));

  if (plugins.count() < 2)
    ui.wdtSelectConnection->setVisible(false);

  setPluginById(APluginId);
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::setPluginById(const QUuid &APluginId)
{
  if (FCurPluginId != APluginId)
  {
    if (FOptionsWidget)
    {
      ui.grbOptions->layout()->removeWidget(FOptionsWidget);
      FOptionsWidget->deleteLater();
      FOptionsWidget = NULL;
      FCurPluginId = QUuid();
    }
    
    IConnectionPlugin *plugin = FManager->pluginById(APluginId);
    if (plugin)
    {
      FOptionsWidget = plugin->optionsWidget(FAccountId);
      if (FOptionsWidget)
      {
        ui.grbOptions->layout()->addWidget(FOptionsWidget);
        FCurPluginId = APluginId;
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