#include "edituserslistdialog.h"

#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>

EditUsersListDialog::EditUsersListDialog(const QString &AAffiliation, const QList<IMultiUserListItem> &AList,
                                         QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FAffiliation = AAffiliation;

  int row = 0;
  ui.tbwTable->setRowCount(AList.count());
  foreach(IMultiUserListItem listItem, AList)
  {
    QTableWidgetItem *jidItem = new QTableWidgetItem(listItem.userJid.full());
    jidItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    ui.tbwTable->setItem(row,0,jidItem);
    if (FAffiliation == MUC_AFFIL_OUTCAST)
    {
      QTableWidgetItem *reasonItem = new QTableWidgetItem(listItem.notes);
      reasonItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      ui.tbwTable->setItem(jidItem->row(),1,reasonItem);
    }
    row++;
    FCurrentItems.insert(listItem.userJid,jidItem);
  }

  if (AAffiliation == MUC_AFFIL_OUTCAST)
  {
    ui.tbwTable->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    ui.tbwTable->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
  }
  else
  {
    ui.tbwTable->hideColumn(1);
    ui.tbwTable->horizontalHeader()->setResizeMode(0,QHeaderView::Stretch);
  }
  ui.tbwTable->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);

  connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddClicked()));
  connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onDeleteClicked()));
}

EditUsersListDialog::~EditUsersListDialog()
{

}

QList<IMultiUserListItem> EditUsersListDialog::deltaList() const
{
  QList<IMultiUserListItem> result;
  foreach(QTableWidgetItem *tableItem, FAddedItems)
  {
    IMultiUserListItem listItem;
    listItem.userJid = tableItem->text();
    listItem.affiliation = FAffiliation;
    if (FAffiliation == MUC_AFFIL_OUTCAST)
      listItem.notes = ui.tbwTable->item(tableItem->row(),1)->text();
    result.append(listItem);
  }
  foreach(Jid userJid, FDeletedItems)
  {
    IMultiUserListItem listItem;
    listItem.userJid = userJid;
    listItem.affiliation = MUC_AFFIL_NONE;
    listItem.notes = "";
    result.append(listItem);
  }
  return result;
}

void EditUsersListDialog::onAddClicked()
{
  Jid  userJid = QInputDialog::getText(this,tr("Add new item"),tr("Enter new item JID:"));
  if (userJid.isValid() && !FCurrentItems.contains(userJid))
  {
    int row = ui.tbwTable->rowCount();
    ui.tbwTable->setRowCount(row+1);
    QTableWidgetItem *jidItem = new QTableWidgetItem(userJid.full());
    jidItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    ui.tbwTable->setItem(row,0,jidItem);
    if (FAffiliation == MUC_AFFIL_OUTCAST)
    {
      QTableWidgetItem *reasonItem = new QTableWidgetItem;
      reasonItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
      ui.tbwTable->setItem(jidItem->row(),1,reasonItem);
      ui.tbwTable->horizontalHeader()->resizeSection(0,QHeaderView::ResizeToContents);
    }
    ui.tbwTable->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
    ui.tbwTable->setCurrentItem(jidItem);
    FCurrentItems.insert(userJid,jidItem);
    FAddedItems.insert(userJid,jidItem);
    FDeletedItems.removeAt(FDeletedItems.indexOf(userJid));
  }
  else
  {
    QMessageBox::warning(this,tr("Wring item JID"),tr("Entered item JID is not valid or already exists."));
  }
}

void EditUsersListDialog::onDeleteClicked()
{
  QTableWidgetItem *tableItem = ui.tbwTable->currentItem();
  if (tableItem)
  {
    Jid userJid = ui.tbwTable->item(tableItem->row(),0)->text();
    ui.tbwTable->removeRow(tableItem->row());
    FCurrentItems.remove(userJid);
    FAddedItems.remove(userJid);
    FDeletedItems.append(userJid);
  }
}

void EditUsersListDialog::setTitle(const QString &ATitle)
{
  setWindowTitle(ATitle);
}
