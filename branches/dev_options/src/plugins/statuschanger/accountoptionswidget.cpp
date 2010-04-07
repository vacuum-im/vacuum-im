#include "accountoptionswidget.h"

AccountOptionsWidget::AccountOptionsWidget(IAccountManager *AAccountManager, const QUuid &AAccountId)
{
  ui.setupUi(this);
  FAccountId = AAccountId;
  FAccountManager = AAccountManager;

  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(FAccountId) : NULL;
  ui.chbAutoConnect->setCheckState(account!=NULL ? (account->value(AVN_AUTO_CONNECT,false).toBool() ? Qt::Checked : Qt::Unchecked) : Qt::Unchecked);
  ui.chbAutoReconnect->setCheckState(account!=NULL ? (account->value(AVN_AUTO_RECONNECT,true).toBool() ? Qt::Checked : Qt::Unchecked) : Qt::Checked);
}

AccountOptionsWidget::~AccountOptionsWidget()
{

}

void AccountOptionsWidget::apply()
{
  IAccount *account = FAccountManager->accountById(FAccountId);
  if (account)
  {
    account->setValue(AVN_AUTO_CONNECT,ui.chbAutoConnect->isChecked());
    account->setValue(AVN_AUTO_RECONNECT,ui.chbAutoReconnect->isChecked());
  }
  emit optionsAccepted();
}
