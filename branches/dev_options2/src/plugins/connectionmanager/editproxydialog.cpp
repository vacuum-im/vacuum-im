#include "editproxydialog.h"

#include <utils/logger.h>

enum ProxyItemDataRoles {
	PDR_UUID = Qt::UserRole,
	PDR_NAME,
	PDR_TYPE,
	PDR_HOST,
	PDR_PORT,
	PDR_USER,
	PDR_PASSWORD
};

EditProxyDialog::EditProxyDialog(IConnectionManager *AManager, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowModality(Qt::WindowModal);

	FManager = AManager;
	IConnectionProxy noproxy = FManager->proxyById(QUuid());

	ui.ltwProxyList->addItem(createProxyItem(QUuid(),noproxy));
	foreach(const QUuid &id, FManager->proxyList())
	{
		IConnectionProxy proxy = FManager->proxyById(id);
		ui.ltwProxyList->addItem(createProxyItem(id, proxy));
	}
	ui.ltwProxyList->sortItems();

	ui.cmbType->addItem(noproxy.name,QNetworkProxy::NoProxy);
	ui.cmbType->addItem(tr("HTTP Proxy"),QNetworkProxy::HttpProxy);
	ui.cmbType->addItem(tr("Socks5 Proxy"),QNetworkProxy::Socks5Proxy);

	connect(ui.pbtAdd, SIGNAL(clicked(bool)),SLOT(onAddButtonClicked(bool)));
	connect(ui.pbtDelete, SIGNAL(clicked(bool)),SLOT(onDeleteButtonClicked(bool)));
	connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogButtonBoxAccepted()));
	connect(ui.btbButtons,SIGNAL(rejected()),SLOT(reject()));

	connect(ui.ltwProxyList, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
		SLOT(onCurrentProxyItemChanged(QListWidgetItem *, QListWidgetItem *)));
	onCurrentProxyItemChanged(ui.ltwProxyList->currentItem(), NULL);
}

EditProxyDialog::~EditProxyDialog()
{

}

QListWidgetItem *EditProxyDialog::createProxyItem(const QUuid &AId, const IConnectionProxy &AProxy) const
{
	QListWidgetItem *item = new QListWidgetItem(AProxy.name);
	item->setData(PDR_UUID, AId.toString());
	item->setData(PDR_NAME, AProxy.name);
	item->setData(PDR_TYPE, AProxy.proxy.type());
	item->setData(PDR_HOST, AProxy.proxy.hostName());
	item->setData(PDR_PORT, AProxy.proxy.port());
	item->setData(PDR_USER, AProxy.proxy.user());
	item->setData(PDR_PASSWORD, AProxy.proxy.password());
	return item;
}

void EditProxyDialog::updateProxyItem(QListWidgetItem *AItem)
{
	if (AItem)
	{
		AItem->setText(ui.lneName->text().trimmed());
		AItem->setData(PDR_NAME, ui.lneName->text().trimmed());
		AItem->setData(PDR_TYPE, ui.cmbType->itemData(ui.cmbType->currentIndex()));
		AItem->setData(PDR_HOST, ui.lneHost->text());
		AItem->setData(PDR_PORT, ui.spbPort->value());
		AItem->setData(PDR_USER, ui.lneUser->text());
		AItem->setData(PDR_PASSWORD, ui.lnePassword->text());
	}
}

void EditProxyDialog::updateProxyWidgets(QListWidgetItem *AItem)
{
	if (AItem)
	{
		ui.lneName->setText(AItem->data(PDR_NAME).toString());
		ui.cmbType->setCurrentIndex(ui.cmbType->findData(AItem->data(PDR_TYPE)));
		ui.lneHost->setText(AItem->data(PDR_HOST).toString());
		ui.spbPort->setValue(AItem->data(PDR_PORT).toInt());
		ui.lneUser->setText(AItem->data(PDR_USER).toString());
		ui.lnePassword->setText(AItem->data(PDR_PASSWORD).toString());
		ui.wdtProperties->setEnabled(!QUuid(AItem->data(PDR_UUID).toString()).isNull());
		ui.pbtDelete->setEnabled(ui.wdtProperties->isEnabled());
	}
}

void EditProxyDialog::onAddButtonClicked(bool)
{
	IConnectionProxy proxy;
	proxy.name = tr("New Proxy");
	proxy.proxy.setType(QNetworkProxy::Socks5Proxy);
	proxy.proxy.setPort(1080);

	QListWidgetItem *item = createProxyItem(QUuid::createUuid(),proxy);
	ui.ltwProxyList->addItem(item);
	ui.ltwProxyList->setCurrentItem(item);
	ui.lneName->setFocus();
}

void EditProxyDialog::onDeleteButtonClicked(bool)
{
	QListWidgetItem *item = ui.ltwProxyList->currentItem();
	if (item)
		delete ui.ltwProxyList->takeItem(ui.ltwProxyList->currentRow());
}

void EditProxyDialog::onCurrentProxyItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious)
{
	updateProxyItem(APrevious);
	updateProxyWidgets(ACurrent);
}

void EditProxyDialog::onDialogButtonBoxAccepted()
{
	updateProxyItem(ui.ltwProxyList->currentItem());

	QSet<QUuid> oldProxy = FManager->proxyList().toSet();
	for (int row = 0; row < ui.ltwProxyList->count(); row++)
	{
		QListWidgetItem *proxyItem = ui.ltwProxyList->item(row);
		QUuid id = proxyItem->data(PDR_UUID).toString();
		if (!id.isNull())
		{
			IConnectionProxy proxy;
			proxy.name = proxyItem->data(PDR_NAME).toString();
			proxy.proxy.setType((QNetworkProxy::ProxyType)proxyItem->data(PDR_TYPE).toInt());
			proxy.proxy.setHostName(proxyItem->data(PDR_HOST).toString());
			proxy.proxy.setPort(proxyItem->data(PDR_PORT).toInt());
			proxy.proxy.setUser(proxyItem->data(PDR_USER).toString());
			proxy.proxy.setPassword(proxyItem->data(PDR_PASSWORD).toString());
			FManager->setProxy(id, proxy);
		}
		oldProxy -= id;
	}

	foreach(const QUuid &id, oldProxy)
		FManager->removeProxy(id);

	accept();
}
