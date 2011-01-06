#include "connectionoptionswidget.h"

#include <QVBoxLayout>

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;
	FOptions = ANode;
	FProxySettings = NULL;

	FProxySettings = FManager!=NULL ? FManager->proxySettingsWidget(FOptions.node("proxy"), ui.wdtProxy) : NULL;
	if (FProxySettings)
	{
		QVBoxLayout *layout = new QVBoxLayout(ui.wdtProxy);
		layout->setMargin(0);
		layout->addWidget(FProxySettings->instance());
		connect(FProxySettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
	}
	else
		ui.wdtProxy->setVisible(false);

	connect(ui.lneHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.spbPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbUseLegacySSL,SIGNAL(stateChanged(int)),SLOT(onUseLegacySSLStateChanged(int)));

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
	if (FProxySettings)
		FProxySettings->reset();
	emit childReset();
}

void ConnectionOptionsWidget::onUseLegacySSLStateChanged(int AState)
{
	ui.spbPort->setValue(AState==Qt::Checked ? 5223 : 5222);
	emit modified();
}
