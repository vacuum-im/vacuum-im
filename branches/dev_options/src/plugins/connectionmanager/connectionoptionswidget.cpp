#include "connectionoptionswidget.h"

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FManager = AManager;

  FOptions = ANode;
  FPluginSettings = NULL;

  foreach (QString pluginId, FManager->pluginList())
    ui.cmbConnections->addItem(FManager->pluginById(pluginId)->pluginName(),pluginId);
  connect(ui.cmbConnections, SIGNAL(currentIndexChanged(int)),SLOT(onComboConnectionsChanged(int)));
  ui.wdtSelectConnection->setVisible(ui.cmbConnections->count() > 1);

  reset();
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply()
{
  IConnectionPlugin *plugin = FManager->pluginById(FPluginId);
  if (plugin)
  {
    FOptions.node("connection-type").setValue(FPluginId);
    if (FPluginSettings)
      plugin->saveConnectionSettings(FPluginSettings->instance());
  }
  emit childApply();
}

void ConnectionOptionsWidget::reset()
{
  QString pluginId = FOptions.value("connection-type").toString();
  if (!FManager->pluginList().isEmpty())
    setPluginById(FManager->pluginById(pluginId) ? pluginId : FManager->pluginList().first());
  if (FPluginSettings)
    FPluginSettings->reset();
  emit childReset();
}

void ConnectionOptionsWidget::setPluginById(const QString &APluginId)
{
  if (FPluginId != APluginId)
  {
    if (FPluginSettings)
    {
      ui.grbOptions->layout()->removeWidget(FPluginSettings->instance());
      delete FPluginSettings->instance();
      FPluginSettings = NULL;
      FPluginId = QUuid();
    }

    IConnectionPlugin *plugin = FManager->pluginById(APluginId);
    if (plugin)
    {
      FPluginSettings = plugin->connectionSettingsWidget(FOptions.node("connection",APluginId), ui.grbOptions);
      if (FPluginSettings)
      {
        FPluginId = APluginId;
        ui.grbOptions->layout()->addWidget(FPluginSettings->instance());
        connect(FPluginSettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
      }
    }

    if (ui.cmbConnections->itemData(ui.cmbConnections->currentIndex()).toString() != APluginId)
      ui.cmbConnections->setCurrentIndex(ui.cmbConnections->findData(APluginId));

    emit modified();
  }
}

void ConnectionOptionsWidget::onComboConnectionsChanged(int AIndex)
{
  if (AIndex != -1)
    setPluginById(ui.cmbConnections->itemData(AIndex).toString());
  else
    setPluginById(QUuid());
}
