#include "edituserslistdialog.h"

#include <QMessageBox>
#include <QHeaderView>
#include <QInputDialog>

#define TIDR_ITEMJID    Qt::UserRole+1

EditUsersListDialog::EditUsersListDialog(const QString &AAffiliation, const QList<IMultiUserListItem> &AList, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FAffiliation = AAffiliation;

	if (AAffiliation == MUC_AFFIL_OUTCAST)
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_EDIT_BAN_LIST,0,0,"windowIcon");
	else if (AAffiliation == MUC_AFFIL_MEMBER)
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_EDIT_MEMBERS_LIST,0,0,"windowIcon");
	else if (AAffiliation == MUC_AFFIL_ADMIN)
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_EDIT_ADMINS_LIST,0,0,"windowIcon");
	else if (AAffiliation == MUC_AFFIL_OWNER)
		IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_EDIT_OWNERS_LIST,0,0,"windowIcon");

	int row = 0;
	ui.tbwTable->setRowCount(AList.count());
	foreach(IMultiUserListItem listItem, AList)
	{
		QTableWidgetItem *jidItem = new QTableWidgetItem();
		jidItem->setText(Jid(listItem.jid).full());
		jidItem->setData(TIDR_ITEMJID, listItem.jid);
		jidItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui.tbwTable->setItem(row,0,jidItem);
		if (FAffiliation == MUC_AFFIL_OUTCAST)
		{
			QTableWidgetItem *reasonItem = new QTableWidgetItem(listItem.notes);
			reasonItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			ui.tbwTable->setItem(jidItem->row(),1,reasonItem);
		}
		row++;
		FCurrentItems.insert(listItem.jid,jidItem);
	}
	ui.tbwTable->horizontalHeader()->setHighlightSections(false);

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

	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddClicked()));
	connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onDeleteClicked()));
}

EditUsersListDialog::~EditUsersListDialog()
{

}

QString EditUsersListDialog::affiliation() const
{
	return FAffiliation;
}

QList<IMultiUserListItem> EditUsersListDialog::deltaList() const
{
	QList<IMultiUserListItem> result;
	foreach(QTableWidgetItem *jidItem, FAddedItems)
	{
		IMultiUserListItem listItem;
		listItem.jid = jidItem->data(TIDR_ITEMJID).toString();
		listItem.affiliation = FAffiliation;
		if (FAffiliation == MUC_AFFIL_OUTCAST)
			listItem.notes = ui.tbwTable->item(jidItem->row(),1)->text();
		result.append(listItem);
	}
	foreach(QString userJid, FDeletedItems)
	{
		IMultiUserListItem listItem;
		listItem.jid = userJid;
		listItem.affiliation = MUC_AFFIL_NONE;
		result.append(listItem);
	}
	return result;
}

void EditUsersListDialog::setTitle(const QString &ATitle)
{
	setWindowTitle(ATitle);
}

void EditUsersListDialog::onAddClicked()
{
	Jid userJid = QInputDialog::getText(this,tr("Add new item"),tr("Enter new item JID:"));
	if (userJid.isValid() && !FCurrentItems.contains(userJid.eFull()))
	{
		int row = ui.tbwTable->rowCount();
		ui.tbwTable->setRowCount(row+1);
		QTableWidgetItem *jidItem = new QTableWidgetItem();
		jidItem->setText(userJid.full());
		jidItem->setData(TIDR_ITEMJID,userJid.eFull());
		jidItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		ui.tbwTable->setItem(row,0,jidItem);
		if (FAffiliation == MUC_AFFIL_OUTCAST)
		{
			QTableWidgetItem *reasonItem = new QTableWidgetItem;
			reasonItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
			ui.tbwTable->setItem(jidItem->row(),1,reasonItem);
			ui.tbwTable->horizontalHeader()->resizeSection(0,QHeaderView::ResizeToContents);
		}
		ui.tbwTable->setCurrentItem(jidItem);
		FDeletedItems.removeAll(userJid.eFull());
		FAddedItems.insert(userJid.eFull(),jidItem);
		FCurrentItems.insert(userJid.eFull(),jidItem);
	}
	else if (!userJid.isEmpty())
	{
		QMessageBox::warning(this,tr("Wrong item JID"),tr("Entered item JID is not valid or already exists."));
	}
}

void EditUsersListDialog::onDeleteClicked()
{
	QTableWidgetItem *tableItem = ui.tbwTable->currentItem();
	if (tableItem)
	{
		QString userJid = ui.tbwTable->item(tableItem->row(),0)->data(TIDR_ITEMJID).toString();
		if (!FAddedItems.contains(userJid))
			FDeletedItems.append(userJid);
		else
			FAddedItems.remove(userJid);
		FCurrentItems.remove(userJid);
		ui.tbwTable->removeRow(tableItem->row());
	}
}
