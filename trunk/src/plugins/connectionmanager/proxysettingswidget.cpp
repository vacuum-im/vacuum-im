#include "proxysettingswidget.h"

ProxySettingsWidget::ProxySettingsWidget(IConnectionManager *AManager, const QString &ASettingsNS, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FManager = AManager;
  FSettingsNS = ASettingsNS;

  ui.cmbProxy->addItem(" "+tr("<Default Proxy>"), DEFAULT_PROXY_REF_UUID);
  ui.cmbProxy->addItem(FManager->proxyById(QUuid()).name, QUuid().toString());
  foreach(QUuid id, FManager->proxyList())
    ui.cmbProxy->addItem(FManager->proxyById(id).name, id.toString());
  ui.cmbProxy->setCurrentIndex(ui.cmbProxy->findData(FManager->proxySettings(FSettingsNS).toString()));

  connect(FManager->instance(),SIGNAL(proxyChanged(const QUuid &, const IConnectionProxy &)),
    SLOT(onProxyChanged(const QUuid &, const IConnectionProxy &)));
  connect(FManager->instance(),SIGNAL(proxyRemoved(const QUuid &)),SLOT(onProxyRemoved(const QUuid &)));
  connect(ui.pbtEditProxy,SIGNAL(clicked(bool)),SLOT(onEditButtonClicked(bool)));
}

ProxySettingsWidget::~ProxySettingsWidget()
{

}

void ProxySettingsWidget::apply(const QString &ASettingsNS)
{
  FManager->setProxySettings(ASettingsNS.isEmpty() ? FSettingsNS : ASettingsNS, ui.cmbProxy->itemData(ui.cmbProxy->currentIndex()).toString());
}

void ProxySettingsWidget::onEditButtonClicked(bool)
{
  FManager->showEditProxyDialog(this);
}

void ProxySettingsWidget::onProxyChanged(const QUuid &AProxyId, const IConnectionProxy &AProxy)
{
  int index = ui.cmbProxy->findData(AProxyId.toString());
  if (index < 0)
    ui.cmbProxy->addItem(AProxy.name, AProxyId.toString());
  else
    ui.cmbProxy->setItemText(index, AProxy.name);
}

void ProxySettingsWidget::onProxyRemoved(const QUuid &AProxyId)
{
  ui.cmbProxy->removeItem(ui.cmbProxy->findData(AProxyId.toString()));
}
