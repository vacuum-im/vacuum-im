#include "accountoptionswidget.h"

AccountOptionsWidget::AccountOptionsWidget(const QString &AAccountId)
{
  ui.setupUi(this);
  FAccountId = AAccountId;
}

AccountOptionsWidget::~AccountOptionsWidget()
{

}

bool AccountOptionsWidget::autoConnect() const
{
  return ui.chbAutoConnect->isChecked();
}

void AccountOptionsWidget::setAutoConnect(bool AAutoConnect)
{
  ui.chbAutoConnect->setCheckState(AAutoConnect ? Qt::Checked : Qt::Unchecked);
}

bool AccountOptionsWidget::autoReconnect() const
{
  return ui.chbAutoReconnect->isChecked();

}

void AccountOptionsWidget::setAutoReconnect(bool AAutoReconnect)
{
  ui.chbAutoReconnect->setCheckState(AAutoReconnect ? Qt::Checked : Qt::Unchecked);
}
