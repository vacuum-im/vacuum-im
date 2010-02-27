#include "connectionoptionswidget.h"

#include <QVBoxLayout>

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, ISettings *ASettings, 
                                                 const QString &ASettingsNS, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FManager = AManager;
  FSettings = ASettings;
  FSettingsNS = ASettingsNS;
  FProxySettings = NULL;

  if (FSettings)
  {
    ui.lneHost->setText(FSettings->valueNS(SVN_CONNECTION_HOST,FSettingsNS).toString());
    ui.spbPort->setValue(FSettings->valueNS(SVN_CONNECTION_PORT,FSettingsNS,5222).toInt());
    ui.chbUseSSL->setChecked(FSettings->valueNS(SVN_CONNECTION_USE_SSL,FSettingsNS,false).toBool());
    ui.chbIgnoreSSLWarnings->setChecked(FSettings->valueNS(SVN_CONNECTION_IGNORE_SSLERROR,FSettingsNS,true).toBool());
    FProxySettings = FManager!=NULL ? FManager->proxySettingsWidget(FSettingsNS, ui.wdtProxy) : NULL;
    if (FProxySettings)
    {
      QVBoxLayout *layout = new QVBoxLayout(ui.wdtProxy);
      layout->setMargin(0);
      layout->addWidget(FProxySettings);
    }
  }
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply(const QString &ASettingsNS)
{
  if (FSettings)
  {
    QString settingsNS = ASettingsNS.isEmpty() ? FSettingsNS : ASettingsNS;
    FSettings->setValueNS(SVN_CONNECTION_HOST, settingsNS, ui.lneHost->text());
    FSettings->setValueNS(SVN_CONNECTION_PORT, settingsNS, ui.spbPort->value());
    FSettings->setValueNS(SVN_CONNECTION_USE_SSL, settingsNS, ui.chbUseSSL->isChecked());
    FSettings->setValueNS(SVN_CONNECTION_IGNORE_SSLERROR, settingsNS, ui.chbIgnoreSSLWarnings->isChecked());
    if (FProxySettings)
      FManager->saveProxySettings(FProxySettings, FSettingsNS);
  }
}
