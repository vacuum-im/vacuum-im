#include "socksoptionswidget.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <definitions/optionvalues.h>
#include <utils/jid.h>

SocksOptionsWidget::SocksOptionsWidget(ISocksStreams *ASocksStreams, IConnectionManager *AConnectionManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FOptionsNode = ANode;
	FSocksStreams = ASocksStreams;
	FConnectionManager = AConnectionManager;

	FProxySettings = FConnectionManager!=NULL ? FConnectionManager->proxySettingsWidget(FOptionsNode.node("network-proxy"),ui.wdtNetworkProxySettings) : NULL;
	if (FProxySettings)
	{
		QVBoxLayout *layout = new QVBoxLayout(ui.wdtNetworkProxySettings);
		layout->setMargin(0);
		layout->addWidget(FProxySettings->instance());
		connect(FProxySettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
		connect(this,SIGNAL(childApply()),FProxySettings->instance(),SLOT(apply()));
		connect(this,SIGNAL(childReset()),FProxySettings->instance(),SLOT(reset()));
	}

	connect(ui.chbEnableDirect,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.spbListenPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbEnableForwardDirect,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.lneForwardDirectAddress,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.chbUseAccountStreamProxy,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbUseUserStreamProxy,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.lneUserStreamProxy,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.chbUseAccountNetworkProxy,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	reset();
}

void SocksOptionsWidget::apply()
{
	Options::node(OPV_DATASTREAMS_SOCKSLISTENPORT).setValue(ui.spbListenPort->value());

	FOptionsNode.setValue(ui.chbEnableDirect->isChecked(),"enable-direct-connections");
	FOptionsNode.setValue(ui.chbEnableForwardDirect->isChecked(),"enable-forward-direct");
	FOptionsNode.setValue(ui.lneForwardDirectAddress->text().trimmed(),"forward-direct-address");
	FOptionsNode.setValue(ui.chbUseAccountStreamProxy->isChecked(),"use-account-stream-proxy");
	FOptionsNode.setValue(ui.chbUseUserStreamProxy->isChecked(),"use-user-stream-proxy");
	FOptionsNode.setValue(ui.lneUserStreamProxy->text().trimmed(),"user-stream-proxy");
	FOptionsNode.setValue(ui.chbUseAccountNetworkProxy->isChecked(),"use-account-network-proxy");

	emit childApply();
}

void SocksOptionsWidget::reset()
{
	ui.spbListenPort->setValue(Options::node(OPV_DATASTREAMS_SOCKSLISTENPORT).value().toInt());

	ui.chbEnableDirect->setChecked(FOptionsNode.value("enable-direct-connections").toBool());
	ui.chbEnableForwardDirect->setChecked(FOptionsNode.value("enable-forward-direct").toBool());
	ui.lneForwardDirectAddress->setText(FOptionsNode.value("forward-direct-address").toString());
	ui.chbUseAccountStreamProxy->setChecked(FOptionsNode.value("use-account-stream-proxy").toBool());
	ui.chbUseUserStreamProxy->setChecked(FOptionsNode.value("use-user-stream-proxy").toBool());
	ui.lneUserStreamProxy->setText(FOptionsNode.value("user-stream-proxy").toString());
	ui.chbUseAccountNetworkProxy->setChecked(FOptionsNode.value("use-account-network-proxy").toBool());

	emit childReset();
}
