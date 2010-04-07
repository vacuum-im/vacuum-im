#include "connectionoptionswidget.h"

#include <QVBoxLayout>

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FManager = AManager;
  FOptions = ANode;
  FProxySettings = NULL;

  reset();
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
  connect(ui.chbUseSSL,SIGNAL(stateChanged(int)),SIGNAL(modified()));
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply(OptionsNode ANode)
{
  OptionsNode node = !ANode.isNull() ? ANode : FOptions;
  node.setValue(ui.lneHost->text(),"host");
  node.setValue(ui.spbPort->value(),"port");
  node.setValue(ui.chbUseSSL->isChecked(),"use-ssl");
  node.setValue(ui.chbIgnoreSSLWarnings->isChecked(),"ignore-ssl-errors");
  if (FProxySettings)
    FManager->saveProxySettings(FProxySettings->instance(), node.node("proxy"));
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
  ui.chbUseSSL->setChecked(FOptions.value("use-ssl").toBool());
  ui.chbIgnoreSSLWarnings->setChecked(FOptions.value("ignore-ssl-errors").toBool());
  if (FProxySettings)
    FProxySettings->reset();
  emit childReset();
}
