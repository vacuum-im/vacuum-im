#include "socksoptions.h"

#include <QVBoxLayout>
#include <QListWidgetItem>
#include <utils/jid.h>

SocksOptions::SocksOptions(ISocksStreams *ASocksStreams, ISocksStream *ASocksStream, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FSocksStream = ASocksStream;
  FProxySettings = NULL;
  FSocksStreams = ASocksStreams;
  FConnectionManager = NULL;
  initialize(AReadOnly);

  ui.spbPort->setVisible(false);

  ui.chbDisableDirectConnect->setChecked(ASocksStream->disableDirectConnection());
  ui.lneForwardHost->setText(ASocksStream->forwardHost());
  ui.spbForwardPort->setValue(ASocksStream->forwardPort());

  ui.chbUseNativeServerProxy->setVisible(false);
  ui.ltwStreamProxy->addItems(ASocksStream->proxyList());

  ui.grbConnectionProxy->setVisible(false);
}

SocksOptions::SocksOptions(ISocksStreams *ASocksStreams, IConnectionManager *AConnectionManager, 
                           const QString &ASettingsNS, bool AReadOnly, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FSocksStream = NULL;
  FSettingsNS = ASettingsNS;
  FProxySettings = NULL;
  FSocksStreams = ASocksStreams;
  FConnectionManager = AConnectionManager;
  initialize(AReadOnly);

  ui.spbPort->setValue(ASocksStreams->serverPort());
  ui.spbPort->setEnabled(ASettingsNS.isEmpty());

  ui.chbDisableDirectConnect->setChecked(ASocksStreams->disableDirectConnections(ASettingsNS));
  ui.lneForwardHost->setText(ASocksStreams->forwardHost(ASettingsNS));
  ui.spbForwardPort->setValue(ASocksStreams->forwardPort(ASettingsNS));
  
  ui.chbUseNativeServerProxy->setChecked(ASocksStreams->useNativeServerProxy(ASettingsNS));
  ui.ltwStreamProxy->addItems(ASocksStreams->proxyList(ASettingsNS));

  //FProxySettings = FConnectionManager!=NULL ? FConnectionManager->proxySettingsWidget(PROXY_NS_PREFIX+FSettingsNS, ui.wdtProxySettings) : NULL;
  //if (FProxySettings)
  //{
  //  QVBoxLayout *layout = new QVBoxLayout(ui.wdtProxySettings);
  //  layout->setMargin(0);
  //  layout->addWidget(FProxySettings);
  //}
  ui.chbUseAccountProxy->setChecked(ASocksStreams->useAccountNetworkProxy(ASettingsNS));
}

SocksOptions::~SocksOptions()
{

}

void SocksOptions::saveSettings(ISocksStream *ASocksStream)
{
  ASocksStream->setDisableDirectConnection(ui.chbDisableDirectConnect->isChecked());
  ASocksStream->setForwardAddress(ui.lneForwardHost->text(), ui.spbForwardPort->value());

  QList<QString> proxyItems;
  for (int row=0; row<ui.ltwStreamProxy->count(); row++)
    proxyItems.append(ui.ltwStreamProxy->item(row)->text());
  ASocksStream->setProxyList(proxyItems);
}

void SocksOptions::saveSettings(const QString &ASettingsNS)
{
  if (ASettingsNS.isEmpty())
    FSocksStreams->setServerPort(ui.spbPort->value());

  FSocksStreams->setDisableDirectConnections(ASettingsNS, ui.chbDisableDirectConnect->isChecked());
  FSocksStreams->setForwardAddress(ASettingsNS, ui.lneForwardHost->text(), ui.spbForwardPort->value());

  QList<QString> proxyItems;
  for (int row=0; row<ui.ltwStreamProxy->count(); row++)
    proxyItems.append(ui.ltwStreamProxy->item(row)->text());
  FSocksStreams->setProxyList(ASettingsNS, proxyItems);
  FSocksStreams->setUseNativeServerProxy(ASettingsNS, ui.chbUseNativeServerProxy->isChecked());

  //if (FProxySettings)
  //{
  //  FConnectionManager->saveProxySettings(FProxySettings);
  //  FSocksStreams->setNetworkProxy(ASettingsNS, FConnectionManager->proxyById(FConnectionManager->loadProxySettings(PROXY_NS_PREFIX+FSettingsNS)).proxy);
  //}
  FSocksStreams->setUseAccountNetworkProxy(ASettingsNS, ui.chbUseAccountProxy->isChecked());
}

void SocksOptions::apply()
{
  if (FSocksStream)
    saveSettings(FSocksStream);
  else
    saveSettings(FSettingsNS);
  emit optionsAccepted();
}

void SocksOptions::initialize(bool AReadOnly)
{
  ui.grbSocksStreams->setTitle(FSocksStreams->methodName());

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

  ui.wdtProxySettings->setEnabled(!AReadOnly);
}

void SocksOptions::onAddStreamProxyClicked(bool)
{
  QString proxy = ui.lneStreamProxy->text().trimmed();
  if (Jid(proxy).isValid() && ui.ltwStreamProxy->findItems(proxy, Qt::MatchExactly).isEmpty())
  {
    ui.ltwStreamProxy->addItem(proxy);
    ui.lneStreamProxy->clear();
  }
}

void SocksOptions::onStreamProxyUpClicked(bool)
{
  if (ui.ltwStreamProxy->currentRow() > 0)
  {
    int row = ui.ltwStreamProxy->currentRow();
    ui.ltwStreamProxy->insertItem(row-1, ui.ltwStreamProxy->takeItem(row));
    ui.ltwStreamProxy->setCurrentRow(row-1);
  }
}

void SocksOptions::onStreamProxyDownClicked(bool)
{
  if (ui.ltwStreamProxy->currentRow() < ui.ltwStreamProxy->count()-1)
  {
    int row = ui.ltwStreamProxy->currentRow();
    ui.ltwStreamProxy->insertItem(row+1, ui.ltwStreamProxy->takeItem(row));
    ui.ltwStreamProxy->setCurrentRow(row+1);
  }
}

void SocksOptions::onDeleteStreamProxyClicked(bool)
{
  if (ui.ltwStreamProxy->currentRow()>=0)
  {
    delete ui.ltwStreamProxy->takeItem(ui.ltwStreamProxy->currentRow());
  }
}
