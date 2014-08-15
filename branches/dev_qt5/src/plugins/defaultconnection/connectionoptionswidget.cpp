#include "connectionoptionswidget.h"

#include <QVBoxLayout>
#include <utils/options.h>

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;
	FOptions = ANode;
	FProxySettings = NULL;

	ui.cmbSslProtocol->addItem(tr("Auto Select"),QSsl::SecureProtocols);
	ui.cmbSslProtocol->addItem(tr("TLSv1.0"),QSsl::TlsV1_0);
	ui.cmbSslProtocol->addItem(tr("TLSv1.1"),QSsl::TlsV1_0);
	ui.cmbSslProtocol->addItem(tr("TLSv1.2"),QSsl::TlsV1_0);
	ui.cmbSslProtocol->addItem(tr("SSLv2"),QSsl::SslV2);
	ui.cmbSslProtocol->addItem(tr("SSLv3"),QSsl::SslV3);

	ui.cmbCertCheckMode->addItem(tr("Disable check"),IDefaultConnection::Disabled);
	ui.cmbCertCheckMode->addItem(tr("Request on errors"),IDefaultConnection::Manual);
	ui.cmbCertCheckMode->addItem(tr("Disconnect on errors"),IDefaultConnection::Forbid);
	ui.cmbCertCheckMode->addItem(tr("Allow only trusted"),IDefaultConnection::TrustedOnly);

	FProxySettings = FManager!=NULL ? FManager->proxySettingsWidget(FOptions.node("proxy"), ui.wdtProxy) : NULL;
	if (FProxySettings)
	{
		QVBoxLayout *layout = new QVBoxLayout(ui.wdtProxy);
		layout->setMargin(0);
		layout->addWidget(FProxySettings->instance());
		connect(FProxySettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
	}
	else
	{
		ui.wdtProxy->setVisible(false);
	}

	connect(ui.lneHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.spbPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbUseLegacySSL,SIGNAL(stateChanged(int)),SLOT(onUseLegacySSLStateChanged(int)));
	connect(ui.cmbSslProtocol,SIGNAL(currentIndexChanged (int)),SIGNAL(modified()));
	connect(ui.cmbCertCheckMode,SIGNAL(currentIndexChanged (int)),SIGNAL(modified()));

	reset();
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply(OptionsNode ANode)
{
	OptionsNode node = !ANode.isNull() ? ANode : FOptions;
	node.setValue(ui.lneHost->text(),"host");
	node.setValue(ui.spbPort->value(),"port");
	node.setValue(ui.chbUseLegacySSL->isChecked(),"use-legacy-ssl");
	node.setValue(ui.cmbSslProtocol->itemData(ui.cmbSslProtocol->currentIndex()),"ssl-protocol");
	node.setValue(ui.cmbCertCheckMode->itemData(ui.cmbCertCheckMode->currentIndex()),"cert-verify-mode");

	if (FProxySettings)
		FManager->saveProxySettings(FProxySettings, node.node("proxy"));

	emit childApply();
}

void ConnectionOptionsWidget::apply()
{
	apply(FOptions);
}

void ConnectionOptionsWidget::reset()
{
	ui.lneHost->setText(FOptions.value("host").toString());
	ui.spbPort->setValue(FOptions.value("port").toInt());
	ui.chbUseLegacySSL->setChecked(FOptions.value("use-legacy-ssl").toBool());
	ui.cmbSslProtocol->setCurrentIndex(ui.cmbSslProtocol->findData(FOptions.value("ssl-protocol").toInt()));
	ui.cmbCertCheckMode->setCurrentIndex(ui.cmbCertCheckMode->findData(FOptions.value("cert-verify-mode").toInt()));

	if (FProxySettings)
		FProxySettings->reset();

	emit childReset();
}

void ConnectionOptionsWidget::onUseLegacySSLStateChanged(int AState)
{
	ui.spbPort->setValue(AState==Qt::Checked ? 5223 : 5222);
	emit modified();
}
