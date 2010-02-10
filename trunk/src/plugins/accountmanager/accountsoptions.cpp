#include "accountsoptions.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>

#define COL_NAME                0
#define COL_JID                 1

AccountsOptions::AccountsOptions(AccountManager *AManager, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FManager = AManager;

  foreach(IAccount *account, FManager->accounts())
  {
    QTreeWidgetItem *item = appendAccount(account->accountId(),account->name());
    item->setCheckState(COL_NAME,account->isActive() ? Qt::Checked : Qt::Unchecked);
    item->setText(COL_JID,account->streamJid().full());
  }

  ui.trwAccounts->setHeaderLabels(QStringList() << tr("Name") << tr("Jabber ID"));
  ui.trwAccounts->sortByColumn(COL_NAME,Qt::AscendingOrder);

  ui.trwAccounts->header()->setResizeMode(COL_NAME, QHeaderView::ResizeToContents);
  ui.trwAccounts->header()->setResizeMode(COL_JID, QHeaderView::Stretch);

  connect(ui.pbtAdd,SIGNAL(clicked(bool)),SLOT(onAccountAdd()));
  connect(ui.pbtRemove,SIGNAL(clicked(bool)),SLOT(onAccountRemove()));
  connect(ui.trwAccounts,SIGNAL(itemActivated(QTreeWidgetItem *,int)),SLOT(onItemActivated(QTreeWidgetItem *,int)));
}

AccountsOptions::~AccountsOptions()
{

}

QWidget *AccountsOptions::accountOptions(const QUuid &AAccountId)
{
  AccountOptions *options = FAccountOptions.value(AAccountId);
  if (options == NULL)
  {
    options = new AccountOptions(FManager,AAccountId);
    if (options->name().isEmpty() && FAccountItems.contains(AAccountId))
      options->setName(FAccountItems.value(AAccountId)->text(COL_NAME));
    FAccountOptions.insert(AAccountId,options);
  }
  return options;
}

void AccountsOptions::apply()
{
  bool delayedApply = false;
  QList<IAccount *> curAccounts;
  for (QMap<QUuid, QTreeWidgetItem *>::const_iterator it = FAccountItems.constBegin(); it!=FAccountItems.constEnd(); it++)
  {
    IAccount *account = FManager->appendAccount(it.key());
    if (account)
    {
      if (FAccountOptions.contains(it.key()))
      {
        Jid streamJidBefore = account->streamJid();
        FAccountOptions.value(it.key())->apply();
        if (!account->isValid())
          QMessageBox::warning(NULL,tr("Not valid account"),tr("Account %1 is not valid, change its Jabber ID").arg(Qt::escape(account->name())));
        else if (account->isActive() && account->xmppStream()->isOpen() && account->streamJid()!=streamJidBefore)
          delayedApply = true;
      }
      it.value()->setText(COL_NAME,account->name());
      it.value()->setText(COL_JID,account->streamJid().full());
      account->setActive(it.value()->checkState(COL_NAME) == Qt::Checked);
      it.value()->setCheckState(COL_NAME, account->isActive() ? Qt::Checked : Qt::Unchecked);
      curAccounts.append(account);
    }
  }

  foreach(IAccount *account, FManager->accounts())
    if (!curAccounts.contains(account))
      FManager->destroyAccount(account->accountId());

  if (delayedApply)
  {
    QMessageBox::information(NULL,tr("Account options"),tr("Some accounts changes will be applied after disconnect"));
  }

  emit optionsAccepted();
}

void AccountsOptions::reject()
{
  QList<QUuid> curAccounts;
  foreach(IAccount *account, FManager->accounts())
    curAccounts.append(account->accountId());

  foreach(QUuid account, FAccountItems.keys())
    if (!curAccounts.contains(account))
      removeAccount(account);

  emit optionsRejected();
}

QTreeWidgetItem *AccountsOptions::appendAccount(const QUuid &AAccountId, const QString &AName)
{
  QTreeWidgetItem *item = FAccountItems.value(AAccountId,NULL);
  if (item == NULL)
  {
    item = new QTreeWidgetItem(ui.trwAccounts);
    item->setText(COL_NAME,AName);
    item->setCheckState(COL_NAME,Qt::Checked);
    FAccountItems.insert(AAccountId,item);
    FManager->openAccountOptionsNode(AAccountId,AName);
  }
  return item;
}

void AccountsOptions::removeAccount(const QUuid &AAccountId)
{
  FManager->closeAccountOptionsNode(AAccountId);
  if (FAccountOptions.contains(AAccountId))
    FAccountOptions.take(AAccountId)->deleteLater();
  QTreeWidgetItem *item = FAccountItems.take(AAccountId);
  delete item;
}

void AccountsOptions::onAccountAdd()
{
  QString name = QInputDialog::getText(this,tr("Enter account name"),tr("Account name:")).trimmed();
  if (!name.isEmpty())
  {
    QUuid id = QUuid::createUuid();
    appendAccount(id,name);
    FManager->openAccountOptionsDialog(id);
  }
}

void AccountsOptions::onAccountRemove()
{
  QTreeWidgetItem *item = ui.trwAccounts->currentItem();
  if (item)
  {
    QMessageBox::StandardButton res = QMessageBox::warning(this,
      tr("Confirm removal of an account"),
      tr("You are assured that wish to remove an account <b>%1</b>?<br>All settings will be lost.").arg(Qt::escape(item->text(0))),
      QMessageBox::Ok | QMessageBox::Cancel);
    
    if (res == QMessageBox::Ok)
    {
      removeAccount(FAccountItems.key(item));
    }
  }
}

void AccountsOptions::onItemActivated(QTreeWidgetItem *AItem, int /*AColumn*/)
{
  if (AItem)
    FManager->openAccountOptionsDialog(FAccountItems.key(AItem));
}
