#include "socksoptions.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <utils/jid.h>

SocksOptions::SocksOptions(ISocksStreams *ASocksStreams, ISocksStream *ASocksStream, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FSocksStreams = ASocksStreams;
	FSocksStream = ASocksStream;
	FProxySettings = NULL;
	FConnectionManager = NULL;
	initialize(AReadOnly);

	ui.spbPort->setVisible(false);
	ui.chbUseAccountNetworkProxy->setVisible(false);
	ui.grbConnectionProxy->setVisible(false);

	reset();
}

SocksOptions::SocksOptions(ISocksStreams *ASocksStreams, IConnectionManager *AConnectionManager, const OptionsNode &ANode, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FSocksStreams = ASocksStreams;
	FSocksStream = NULL;
	FProxySettings = NULL;
	FOptions = ANode;
	FConnectionManager = AConnectionManager;
	initialize(AReadOnly);

	FProxySettings = FConnectionManager!=NULL ? FConnectionManager->proxySettingsWidget(FOptions.node("network-proxy"),ui.wdtProxySettings) : NULL;
	if (FProxySettings)
	{
		QVBoxLayout *layout = new QVBoxLayout(ui.wdtProxySettings);
		layout->setMargin(0);
		layout->addWidget(FProxySettings->instance());
		connect(FProxySettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
	}

	reset();
}

SocksOptions::~SocksOptions()
{

}

void SocksOptions::apply(OptionsNode ANode)
{
	OptionsNode node = ANode.isNull() ? FOptions : ANode;

	Options::node(OPV_DATASTREAMS_SOCKSLISTENPORT).setValue(ui.spbPort->value());

	node.setValue(ui.spbConnectTimeout->value()*1000,"connect-timeout");
	node.setValue(ui.chbDisableDirectConnect->isChecked(),"disable-direct-connections");
	node.setValue(ui.lneForwardHost->text(),"forward-host");
	node.setValue(ui.spbForwardPort->value(),"forward-port");

	QStringList proxyItems;
	for (int row=0; row<ui.ltwStreamProxy->count(); row++)
	{
		QString proxyItem = Jid(ui.ltwStreamProxy->item(row)->text()).pBare();
		if (!proxyItems.contains(proxyItem))
			proxyItems.append(proxyItem);
	}
	node.setValue(proxyItems,"stream-proxy-list");

	node.setValue(ui.chbUseAccountStreamProxy->isChecked(),"use-account-stream-proxy");
	node.setValue(ui.chbUseAccountNetworkProxy->isChecked(),"use-account-network-proxy");

	if (FProxySettings)
		FConnectionManager->saveProxySettings(FProxySettings);

	emit childApply();
}

void SocksOptions::apply(ISocksStream *ASocksStream)
{
	ASocksStream->setConnectTimeout(ui.spbConnectTimeout->value()*1000);
	ASocksStream->setDirectConnectionsDisabled(ui.chbDisableDirectConnect->isChecked());
	ASocksStream->setForwardAddress(ui.lneForwardHost->text(), ui.spbForwardPort->value());

	QList<QString> proxyItems;
	for (int row=0; row<ui.ltwStreamProxy->count(); row++)
		proxyItems.append(ui.ltwStreamProxy->item(row)->text());
	ASocksStream->setProxyList(proxyItems);

	emit childApply();
}

void SocksOptions::apply()
{
	if (FSocksStream)
		apply(FSocksStream);
	else
		apply(FOptions);
}

void SocksOptions::reset()
{
	if (FSocksStream)
	{
		ui.spbConnectTimeout->setValue(FSocksStream->connectTimeout()/1000);
		ui.chbDisableDirectConnect->setChecked(FSocksStream->isDirectConnectionsDisabled());
		ui.lneForwardHost->setText(FSocksStream->forwardHost());
		ui.spbForwardPort->setValue(FSocksStream->forwardPort());
		ui.ltwStreamProxy->addItems(FSocksStream->proxyList());
	}
	else
	{
		ui.spbPort->setValue(Options::node(OPV_DATASTREAMS_SOCKSLISTENPORT).value().toInt());
		ui.spbConnectTimeout->setValue(FOptions.value("connect-timeout").toInt()/1000);
		ui.chbDisableDirectConnect->setChecked(FOptions.value("disable-direct-connections").toBool());
		ui.lneForwardHost->setText(FOptions.value("forward-host").toString());
		ui.spbForwardPort->setValue(FOptions.value("forward-port").toInt());
		ui.ltwStreamProxy->clear();
		ui.ltwStreamProxy->addItems(FOptions.value("stream-proxy-list").toStringList());
		ui.chbUseAccountStreamProxy->setChecked(FOptions.value("use-account-stream-proxy").toBool());
		ui.chbUseAccountNetworkProxy->setChecked(FOptions.value("use-account-network-proxy").toBool());
		if (FProxySettings)
			FProxySettings->reset();
	}
	emit childReset();
}

void SocksOptions::initialize(bool AReadOnly)
{
	ui.grbSocksStreams->setTitle(FSocksStreams->methodName());

	ui.spbConnectTimeout->setReadOnly(AReadOnly);
	ui.spbPort->setReadOnly(AReadOnly);
	ui.lneForwardHost->setReadOnly(AReadOnly);
	ui.spbForwardPort->setReadOnly(AReadOnly);

	ui.lneStreamProxy->setReadOnly(AReadOnly);
	ui.pbtAddStreamProxy->setEnabled(!AReadOnly);
	ui.pbtStreamProxyUp->setEnabled(!AReadOnly);
	ui.pbtStreamProxyDown->setEnabled(!AReadOnly);
	ui.pbtDeleteStreamProxy->setEnabled(!AReadOnly);
	connect(ui.pbtAddStreamProxy,SIGNAL(clicked(bool)),SLOT(onAddStreamProxyClicked(bool)));
	connect(ui.pbtStreamProxyUp,SIGNAL(clicked(bool)),SLOT(onStreamProxyUpClicked(bool)));
	connect(ui.pbtStreamProxyDown,SIGNAL(clicked(bool)),SLOT(onStreamProxyDownClicked(bool)));
	connect(ui.pbtDeleteStreamProxy,SIGNAL(clicked(bool)),SLOT(onDeleteStreamProxyClicked(bool)));

	connect(ui.spbPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.spbConnectTimeout,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbDisableDirectConnect,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.lneForwardHost,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.spbForwardPort,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.chbUseAccountStreamProxy,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.chbUseAccountNetworkProxy,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	ui.wdtProxySettings->setEnabled(!AReadOnly);
}

void SocksOptions::onAddStreamProxyClicked(bool)
{
	QString proxy = ui.lneStreamProxy->text().trimmed();
	if (Jid(proxy).isValid() && ui.ltwStreamProxy->findItems(proxy, Qt::MatchExactly).isEmpty())
	{
		ui.ltwStreamProxy->addItem(proxy);
		ui.lneStreamProxy->clear();
		emit modified();
	}
}

void SocksOptions::onStreamProxyUpClicked(bool)
{
	if (ui.ltwStreamProxy->currentRow() > 0)
	{
		int row = ui.ltwStreamProxy->currentRow();
		ui.ltwStreamProxy->insertItem(row-1, ui.ltwStreamProxy->takeItem(row));
		ui.ltwStreamProxy->setCurrentRow(row-1);
		emit modified();
	}
}

void SocksOptions::onStreamProxyDownClicked(bool)
{
	if (ui.ltwStreamProxy->currentRow() < ui.ltwStreamProxy->count()-1)
	{
		int row = ui.ltwStreamProxy->currentRow();
		ui.ltwStreamProxy->insertItem(row+1, ui.ltwStreamProxy->takeItem(row));
		ui.ltwStreamProxy->setCurrentRow(row+1);
		emit modified();
	}
}

void SocksOptions::onDeleteStreamProxyClicked(bool)
{
	if (ui.ltwStreamProxy->currentRow()>=0)
	{
		delete ui.ltwStreamProxy->takeItem(ui.ltwStreamProxy->currentRow());
		emit modified();
	}
}
