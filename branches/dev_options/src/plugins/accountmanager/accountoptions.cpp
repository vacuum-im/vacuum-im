#include "accountoptions.h"

AccountOptions::AccountOptions(IAccountManager *AManager, const QUuid &AAccountId, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FAccountId = AAccountId;
  FManager = AManager;
  
  connect(ui.lneName,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
  connect(ui.lneJabberId,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
  connect(ui.lneResource,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
  connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

  reset();
}

AccountOptions::~AccountOptions()
{

}

void AccountOptions::apply()
{
  IAccount *account = FManager->accountById(FAccountId);
  if (account)
  {
    QString name = ui.lneName->text().trimmed();
    if (name.isEmpty())
      name = ui.lneJabberId->text().trimmed();
    if (name.isEmpty())
      name = tr("New Account");
    Jid streamJid = ui.lneJabberId->text();
    streamJid.setResource(ui.lneResource->text());

    account->setName(name);
    account->setStreamJid(streamJid);
    account->setPassword(ui.lnePassword->text());
  }
  emit childApply();
}

void AccountOptions::reset()
{
  IAccount *account = FManager->accountById(FAccountId);
  if (account)
  {
    ui.lneName->setText(account->name());
    ui.lneJabberId->setText(account->streamJid().bare());
    ui.lneResource->setText(account->streamJid().resource());
    ui.lnePassword->setText(account->password());
  }
  else
  {
    ui.lneResource->setText(CLIENT_NAME);
  }
  emit childReset();
}

QString AccountOptions::name() const
{
  return ui.lneName->text();
}

void AccountOptions::setName(const QString &AName)
{
  ui.lneName->setText(AName);
}
