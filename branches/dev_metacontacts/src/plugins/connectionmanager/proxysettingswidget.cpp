#include "proxysettingswidget.h"

#include <definitions/optionvalues.h>
#include <utils/options.h>

ProxySettingsWidget::ProxySettingsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;
	FOptions = ANode;

	if (ANode.path() == OPV_PROXY_DEFAULT)
		ui.lblProxy->setText(tr("Default proxy:"));
	else
		ui.cmbProxy->addItem(" "+tr("<Default Proxy>"), APPLICATION_PROXY_REF_UUID);
	ui.cmbProxy->addItem(FManager->proxyById(QUuid()).name, QUuid().toString());

	foreach(const QUuid &id, FManager->proxyList())
		ui.cmbProxy->addItem(FManager->proxyById(id).name, id.toString());

	connect(FManager->instance(),SIGNAL(proxyChanged(const QUuid &, const IConnectionProxy &)),
		SLOT(onProxyChanged(const QUuid &, const IConnectionProxy &)));
	connect(FManager->instance(),SIGNAL(proxyRemoved(const QUuid &)),SLOT(onProxyRemoved(const QUuid &)));

	connect(ui.cmbProxy,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.pbtEditProxy,SIGNAL(clicked(bool)),SLOT(onEditButtonClicked(bool)));

	reset();
}

ProxySettingsWidget::~ProxySettingsWidget()
{

}

void ProxySettingsWidget::apply(OptionsNode ANode)
{
	if (!ANode.isNull())
		ANode.setValue(ui.cmbProxy->itemData(ui.cmbProxy->currentIndex()).toString());
	else
		FOptions.setValue(ui.cmbProxy->itemData(ui.cmbProxy->currentIndex()).toString());
	emit childApply();
}

void ProxySettingsWidget::apply()
{
	apply(FOptions);
}

void ProxySettingsWidget::reset()
{
	ui.cmbProxy->setCurrentIndex(ui.cmbProxy->findData(FManager->loadProxySettings(FOptions).toString()));
	emit childReset();
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
