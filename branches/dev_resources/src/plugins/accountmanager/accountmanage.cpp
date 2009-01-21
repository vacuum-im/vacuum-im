#include "accountmanage.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QTextDocument>

#define ACCOUNT_ID_ROLE Qt::UserRole+1

AccountManage::AccountManage(QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  ui.trwAccounts->setHeaderLabels(QStringList() << tr("Name") << tr("Jabber ID"));
  ui.trwAccounts->sortByColumn(0,Qt::AscendingOrder);
  connect(ui.trwAccounts,SIGNAL(itemActivated(QTreeWidgetItem *,int)),SLOT(onItemActivated(QTreeWidgetItem *,int)));
  connect(ui.pbtAdd,SIGNAL(clicked(bool)),SLOT(onAccountAdd()));
  connect(ui.pbtRemove,SIGNAL(clicked(bool)),SLOT(onAccountRemove()));
}

AccountManage::~AccountManage()
{

}

void AccountManage::setAccount(const QString &AId, const QString &AName, const QString &AStreamJid, bool AShown)
{
  QTreeWidgetItem *item = FAccountItems.value(AId,NULL);
  if (!item)
  {
    item = new QTreeWidgetItem(ui.trwAccounts);
    FAccountItems.insert(AId,item);
  }
  item->setText(0,AName);
  item->setText(1,AStreamJid);
  item->setCheckState(0,AShown ? Qt::Checked : Qt::Unchecked);
  item->setData(0,ACCOUNT_ID_ROLE,AId);
}

bool AccountManage::isAccountActive(const QString &AId) const
{
  QTreeWidgetItem *item = FAccountItems.value(AId,NULL);
  if (item)
  {
    return item->checkState(0) == Qt::Checked;
  }
  return false;
}

QString AccountManage::accountName(const QString &AId) const
{
  QTreeWidgetItem *item = FAccountItems.value(AId,NULL);
  if (item)
  {
    return item->text(0);
  }
  return QString();
}

void AccountManage::removeAccount(const QString &AId)
{
  QTreeWidgetItem *item = FAccountItems.value(AId,NULL);
  if (item)
  {
    FAccountItems.remove(AId);
    delete item;
  }
}

void AccountManage::onAccountAdd()
{
  QString name = QInputDialog::getText(this,tr("Enter account name"),tr("Account name:")); 
  if (!name.isEmpty())
    emit accountAdded(name);
}

void AccountManage::onAccountRemove()
{
  QTreeWidgetItem *item = ui.trwAccounts->currentItem();
  if (item)
  {
    QMessageBox::StandardButton res = QMessageBox::warning(this,
      tr("Confirm removal of an account."),
      tr("You are assured that wish to remove an account <b>%1</b>?<br>All settings will be lost.").arg(Qt::escape(item->text(0))),
      QMessageBox::Ok | QMessageBox::Cancel);
    
    if (res == QMessageBox::Ok)
      emit accountRemoved(item->data(0,ACCOUNT_ID_ROLE).toString());
  }
}

void AccountManage::onItemActivated(QTreeWidgetItem *AItem, int /*AColumn*/)
{
  if (AItem)
    emit accountShow(AItem->data(0,ACCOUNT_ID_ROLE).toString());
}
