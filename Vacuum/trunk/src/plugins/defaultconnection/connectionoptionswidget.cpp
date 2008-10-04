#include "connectionoptionswidget.h"

ConnectionOptionsWidget::ConnectionOptionsWidget()
{
  ui.setupUi(this);
  connect(ui.chbUseSSL,SIGNAL(stateChanged(int)),SLOT(onUseSSLStateChanged(int)));
  connect(ui.cmbProxyType,SIGNAL(currentIndexChanged(int)),SLOT(onProxyTypeChanged(int)));
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

QString ConnectionOptionsWidget::host() const
{
  return ui.lneHost->text();  
}

void ConnectionOptionsWidget::setHost(const QString &AHost)
{
  ui.lneHost->setText(AHost);
}

int ConnectionOptionsWidget::port() const
{
  return ui.spbPort->value();
}

void ConnectionOptionsWidget::setPort( int APort )
{
  ui.spbPort->setValue(APort);
}

bool ConnectionOptionsWidget::useSSL() const
{
  return ui.chbUseSSL->isChecked();
}

void ConnectionOptionsWidget::setUseSSL(bool AUseSSL)
{
  ui.chbUseSSL->setCheckState(AUseSSL ? Qt::Checked : Qt::Unchecked);
}

bool ConnectionOptionsWidget::ignoreSSLErrors() const
{
  return ui.chbIgnoreSSLWarnings->isChecked();
}

void ConnectionOptionsWidget::setIgnoreSSLError(bool AIgnore)
{
  ui.chbIgnoreSSLWarnings->setCheckState(AIgnore ? Qt::Checked : Qt::Unchecked);
}

int ConnectionOptionsWidget::proxyType() const
{
  return ui.cmbProxyType->itemData(ui.cmbProxyType->currentIndex()).toInt();
}

void ConnectionOptionsWidget::setProxyTypes(const QStringList &AProxyTypes)
{
  for (int i = 0; i < AProxyTypes.count(); i++)
    ui.cmbProxyType->addItem(AProxyTypes.at(i),i);
}

void ConnectionOptionsWidget::setProxyType(int AProxyType)
{
  ui.cmbProxyType->setCurrentIndex(AProxyType);
}

QString ConnectionOptionsWidget::proxyHost() const
{
  return ui.lneProxyHost->text();
}

void ConnectionOptionsWidget::setProxyHost(const QString &AProxyHost)
{
  ui.lneProxyHost->setText(AProxyHost);
}

int ConnectionOptionsWidget::proxyPort() const
{
  return ui.spbProxyPort->value();
}

void ConnectionOptionsWidget::setProxyPort(int AProxyPort)
{
  ui.spbProxyPort->setValue(AProxyPort);
}

QString ConnectionOptionsWidget::proxyUserName() const
{
  return ui.lneProxyUser->text();
}

void ConnectionOptionsWidget::setProxyUserName(const QString &AProxyUser)
{
  ui.lneProxyUser->setText(AProxyUser);
}

QString ConnectionOptionsWidget::proxyPassword() const
{
  return ui.lneProxyPassword->text();
}

void ConnectionOptionsWidget::setProxyPassword(const QString &APassword)
{
  ui.lneProxyPassword->setText(APassword);
}

void ConnectionOptionsWidget::onUseSSLStateChanged(int AState)
{
  if (AState == Qt::Checked && port()==5222)
    setPort(5223);
  else if (AState == Qt::Unchecked && port() == 5223)
    setPort(5222);
 }

void ConnectionOptionsWidget::onProxyTypeChanged(int AIndex)
{
  ui.lneProxyHost->setEnabled(AIndex != 0);
  ui.spbProxyPort->setEnabled(AIndex != 0);
  ui.lneProxyUser->setEnabled(AIndex != 0);
  ui.lneProxyPassword->setEnabled(AIndex != 0);
}

