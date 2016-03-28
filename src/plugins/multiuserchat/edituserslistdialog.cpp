#include "edituserslistdialog.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

#define ADR_USER_JID           Action::DR_Parametr1
#define ADR_USER_AFFILIATON    Action::DR_Parametr2

enum UserItemDataRoles {
	UDR_REAL_JID       = Qt::UserRole,
	UDR_REASON,
	UDR_AFFILIATION,
	UDR_FILTER,
	UDR_REASON_LABEL,
};

static const QStringList Affiliations = QStringList() << MUC_AFFIL_OUTCAST << MUC_AFFIL_MEMBER << MUC_AFFIL_ADMIN << MUC_AFFIL_OWNER;

UsersListProxyModel::UsersListProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent)
{

}

bool UsersListProxyModel::filterAcceptsRow(int ASourceRow, const QModelIndex &ASourceParent) const
{
	if (!ASourceParent.isValid())
		return true;
	return QSortFilterProxyModel::filterAcceptsRow(ASourceRow,ASourceParent);
}

EditUsersListDialog::EditUsersListDialog(IMultiUserChat *AMultiChat, const QString &AAffiliation, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Users Lists - %1").arg(AMultiChat->roomJid().bare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_EDIT_AFFILIATIONS,0,0,"windowIcon");

	FMultiChat = AMultiChat;
	connect(FMultiChat->instance(),SIGNAL(stateChanged(int)),SLOT(reject()));
	connect(FMultiChat->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
		SLOT(onMultiChatRequestFailed(const QString &, const XmppError &)));
	connect(FMultiChat->instance(),SIGNAL(affiliationListLoaded(const QString &, const QList<IMultiUserListItem> &)),
		SLOT(onMultiChatListLoaded(const QString &, const QList<IMultiUserListItem> &)));
	connect(FMultiChat->instance(),SIGNAL(affiliationListUpdated(const QString &, const QList<IMultiUserListItem> &)),
		SLOT(onMultiChatListUpdated(const QString &, const QList<IMultiUserListItem> &)));
	
	FModel = new QStandardItemModel(this);
	FModel->setColumnCount(1);

	FDelegate = new AdvancedItemDelegate(this);
	FDelegate->setContentsMargings(QMargins(5,2,5,2));

	FProxy = new UsersListProxyModel(this);
	FProxy->setSourceModel(FModel);
	FProxy->setFilterRole(UDR_FILTER);
	FProxy->setDynamicSortFilter(true);
	FProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
	FProxy->sort(0);

	foreach (const QString &curAffiliation, Affiliations)
		FAffilTab.insert(curAffiliation,ui.tbrTabs->addTab(affiliatioName(curAffiliation)));

	QString curAffiliation = Options::fileValue("muc.edit-users-list-dialog.affiliation",FMultiChat->roomJid().pBare()).toString();
	curAffiliation = AAffiliation!=MUC_AFFIL_NONE ? AAffiliation : curAffiliation;

	ui.tbrTabs->setDocumentMode(true);
	ui.tbrTabs->setCurrentIndex(FAffilTab.value(curAffiliation));
	connect(ui.tbrTabs,SIGNAL(currentChanged(int)),SLOT(onCurrentAffiliationChanged(int)));

	ui.tbvItems->setModel(FProxy);
	ui.tbvItems->setItemDelegate(FDelegate);
	ui.tbvItems->verticalHeader()->hide();
	ui.tbvItems->horizontalHeader()->hide();
	ui.tbvItems->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui.tbvItems->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	connect(ui.tbvItems,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onItemsTableContextMenuRequested(const QPoint &)));

	connect(ui.sleSearch,SIGNAL(searchStart()),SLOT(onSearchLineEditSearchStart()));

	ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(false);
	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonBoxButtonClicked(QAbstractButton *)));

	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddClicked()));
	connect(ui.pbtDelete,SIGNAL(clicked()),SLOT(onDeleteClicked()));

	restoreGeometry(Options::fileValue("muc.edit-users-list-dialog.geometry").toByteArray());

	onCurrentAffiliationChanged(ui.tbrTabs->currentIndex());
}

EditUsersListDialog::~EditUsersListDialog()
{
	Options::setFileValue(saveGeometry(),"muc.edit-users-list-dialog.geometry");
	Options::setFileValue(currentAffiliation(),"muc.edit-users-list-dialog.affiliation",FMultiChat->roomJid().pBare());
}

QString EditUsersListDialog::currentAffiliation() const
{
	return FAffilTab.key(ui.tbrTabs->currentIndex());
}

QList<IMultiUserListItem> EditUsersListDialog::deltaList() const
{
	QList<IMultiUserListItem> result;

	QSet<Jid> listItems = FUserListItem.keys().toSet();
	QSet<Jid> modelItems = FUserModelItem.keys().toSet();

	QSet<Jid> newItems =  modelItems - listItems;
	QSet<Jid> oldItems = listItems - modelItems;

	// Created Items
	foreach(const Jid &realJid, newItems)
	{
		QStandardItem *modelItem = FUserModelItem.value(realJid);

		IMultiUserListItem listItem;
		listItem.realJid = realJid;
		listItem.reason = modelItem->data(UDR_REASON).toString();
		listItem.affiliation = modelItem->data(UDR_AFFILIATION).toString();

		result.append(listItem);
	}

	// Deleted Items
	foreach (const Jid &realJid, oldItems)
	{
		IMultiUserListItem listItem;
		listItem.realJid = realJid;
		listItem.affiliation = MUC_AFFIL_NONE;

		result.append(listItem);
	}

	// Changed Items
	for (QHash<Jid, QStandardItem*>::const_iterator it=FUserModelItem.constBegin(); it!=FUserModelItem.constEnd(); ++it)
	{
		IMultiUserListItem listItem = FUserListItem.value(it.key());
		QString modelAffiliation = it.value()->data(UDR_AFFILIATION).toString();
		if (listItem.realJid==it.key() && listItem.affiliation!=modelAffiliation)
		{
			listItem.affiliation = modelAffiliation;
			result.append(listItem);
		}
	}

	return result;
}

QString EditUsersListDialog::affiliatioName(const QString &AAffiliation) const
{
	if (AAffiliation == MUC_AFFIL_OWNER)
		return tr("Owners");
	else if (AAffiliation == MUC_AFFIL_ADMIN)
		return tr("Administrators");
	else if (AAffiliation == MUC_AFFIL_MEMBER)
		return tr("Members");
	else if (AAffiliation == MUC_AFFIL_OUTCAST)
		return tr("Outcasts");
	return tr("None");
}

void EditUsersListDialog::updateAffiliationTabNames() const
{
	foreach(const QString &affiliation, Affiliations)
	{
		QString text;
		if (!FAffilUpdateId.isEmpty() || FAffilLoadId.values().contains(affiliation))
			text = QString("%1 (...)").arg(affiliatioName(affiliation));
		else if (FAffilRoot.contains(affiliation))
			text = QString("%1 (%2)").arg(affiliatioName(affiliation)).arg(FAffilRoot.value(affiliation)->rowCount());
		else
			text = affiliatioName(affiliation);
		ui.tbrTabs->setTabText(FAffilTab.value(affiliation),text);
	}
}

QList<QStandardItem *> EditUsersListDialog::selectedModelItems() const
{
	QList<QStandardItem *> modelItems;
	QStandardItem *affilRoot = FAffilRoot.value(currentAffiliation());
	foreach(const QModelIndex &index, ui.tbvItems->selectionModel()->selectedIndexes())
	{
		QStandardItem *modelItem = FModel->itemFromIndex(FProxy->mapToSource(index));
		if (modelItem!=NULL && modelItem->parent()==affilRoot)
			modelItems.append(modelItem);
	}
	return modelItems;
}

QStandardItem *EditUsersListDialog::createModelItem(const Jid &ARealJid) const
{
	QStandardItem *modelItem = new QStandardItem(ARealJid.uFull());
	modelItem->setData(ARealJid.full(),UDR_REAL_JID);

	AdvancedDelegateItem nameLabel(AdvancedDelegateItem::DisplayId);
	nameLabel.d->kind = AdvancedDelegateItem::Display;
	nameLabel.d->data = Qt::DisplayRole;
	nameLabel.d->hints.insert(AdvancedDelegateItem::FontWeight,QFont::Bold);

	AdvancedDelegateItem reasonLabel(AdvancedDelegateItem::DisplayId+1);
	reasonLabel.d->kind = AdvancedDelegateItem::CustomData;
	reasonLabel.d->data = UDR_REASON_LABEL;

	AdvancedDelegateItems labels;
	labels.insert(nameLabel.d->id, nameLabel);
	labels.insert(reasonLabel.d->id, reasonLabel);
	modelItem->setData(QVariant::fromValue<AdvancedDelegateItems>(labels), FDelegate->itemsRole());

	return modelItem;
}

void EditUsersListDialog::updateModelItem(QStandardItem *AModelItem, const IMultiUserListItem &AListItem) const
{
	AModelItem->setData(AListItem.reason,UDR_REASON);
	AModelItem->setData(AListItem.affiliation,UDR_AFFILIATION);

	AModelItem->setData(AListItem.realJid.uFull() + " " + AListItem.reason,UDR_FILTER);
	AModelItem->setData(!AListItem.reason.isEmpty() ? QString(" - %1").arg(AListItem.reason) : QString::null,UDR_REASON_LABEL);
}

void EditUsersListDialog::applyListItems(const QList<IMultiUserListItem> &AListItems)
{
	foreach(const IMultiUserListItem &listItem, AListItems)
	{
		QStandardItem *affilRoot = FAffilRoot.value(listItem.affiliation);
		QStandardItem *modelItem = FUserModelItem.value(listItem.realJid);

		// User deleted from all lists, or user affiliation does not loaded
		if (listItem.affiliation==MUC_AFFIL_NONE || affilRoot==NULL)
		{
			if (modelItem != NULL)
			{
				FUserModelItem.remove(listItem.realJid);
				qDeleteAll(modelItem->parent()->takeRow(modelItem->row()));
			}
			FUserListItem.remove(listItem.realJid);
		}
		else
		{
			// User is not created yet
			if (modelItem == NULL)
			{
				modelItem = createModelItem(listItem.realJid);
				FUserModelItem.insert(listItem.realJid,modelItem);
				affilRoot->appendRow(modelItem);
			}
			// User affiliation changed
			else if (modelItem->parent() != affilRoot)
			{
				modelItem->parent()->takeRow(modelItem->row());
				affilRoot->appendRow(modelItem);
			}
			updateModelItem(modelItem,listItem);
			FUserListItem.insert(listItem.realJid,listItem);
		}
	}
}

void EditUsersListDialog::onAddClicked()
{
	QString affiliation = currentAffiliation();
	QStandardItem *affilRoot = FAffilRoot.value(affiliation);
	if (affilRoot!=NULL && FAffilLoadId.isEmpty())
	{
		Jid userJid = Jid::fromUserInput(QInputDialog::getText(this,tr("Add User"),tr("Enter user Jabber ID:"))).bare();
		if (userJid.isValid())
		{
			if (!FUserModelItem.contains(userJid))
			{
				IMultiUserListItem listItem;
				listItem.realJid = userJid;
				listItem.affiliation = affiliation;
				listItem.reason = QInputDialog::getText(this,tr("Add User"),tr("Enter note:"));

				QStandardItem *modelItem = createModelItem(listItem.realJid);
				updateModelItem(modelItem,listItem);
				FUserModelItem.insert(userJid,modelItem);
				affilRoot->appendRow(modelItem);

				ui.tbvItems->setCurrentIndex(FProxy->mapFromSource(FModel->indexFromItem(modelItem)));

				updateAffiliationTabNames();
				ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(true);
			}
			else
			{
				QStandardItem *modelItem = FUserModelItem.value(userJid);
				QMessageBox::warning(this,tr("Warning"),tr("User %1 already present in list of '%2'").arg(userJid.uBare(),affiliatioName(modelItem->data(UDR_AFFILIATION).toString())));
			}
		}
	}
}

void EditUsersListDialog::onDeleteClicked()
{
	foreach(QStandardItem *modelItem, selectedModelItems())
	{
		FUserModelItem.remove(modelItem->data(UDR_REAL_JID).toString());
		qDeleteAll(modelItem->parent()->takeRow(modelItem->row()));
		ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(true);
	}
	updateAffiliationTabNames();
}

void EditUsersListDialog::onMoveUserActionTriggered()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString affiliation = action->data(ADR_USER_AFFILIATON).toString();
		QStandardItem *affilRoot = FAffilRoot.value(affiliation);

		foreach(const Jid userJid, action->data(ADR_USER_JID).toStringList())
		{
			QStandardItem *modelItem = FUserModelItem.value(userJid);
			if (modelItem)
			{
				if (affiliation == MUC_AFFIL_NONE)
				{
					FUserModelItem.remove(userJid);
					qDeleteAll(modelItem->parent()->takeRow(modelItem->row()));
				}
				else if (affilRoot)
				{
					modelItem->parent()->takeRow(modelItem->row());

					IMultiUserListItem listItem;
					listItem.realJid = userJid;
					listItem.affiliation = affiliation;
					updateModelItem(modelItem,listItem);

					affilRoot->appendRow(modelItem);
				}
			}
		}
		
		updateAffiliationTabNames();
		ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(true);
	}
}

void EditUsersListDialog::onSearchLineEditSearchStart()
{
	FProxy->setFilterFixedString(ui.sleSearch->text());
}

void EditUsersListDialog::onCurrentAffiliationChanged(int ATabIndex)
{
	QString affiliation = FAffilTab.key(ATabIndex);
	if (!FAffilRoot.contains(affiliation))
	{
		QString id = FMultiChat->loadAffiliationList(affiliation);
		if (!id.isEmpty())
		{
			QStandardItem *affilRoot = new QStandardItem(affiliation);
			FAffilRoot.insert(affiliation,affilRoot);
			FModel->appendRow(affilRoot);

			FAffilLoadId.insert(id,affiliation);
		}
		else
		{
			QMessageBox::warning(this,tr("Warning"), tr("Failed to load list of '%1'").arg(affiliatioName(affiliation)));
		}
		updateAffiliationTabNames();
	}
	ui.tbvItems->setRootIndex(FProxy->mapFromSource(FModel->indexFromItem(FAffilRoot.value(affiliation))));
}

void EditUsersListDialog::onDialogButtonBoxButtonClicked(QAbstractButton *AButton)
{
	if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Save)
	{
		QList<IMultiUserListItem> listItems = deltaList();
		if (!listItems.isEmpty())
		{
			FAffilUpdateId = FMultiChat->updateAffiliationList(listItems);
			if (!FAffilUpdateId.isEmpty())
			{
				updateAffiliationTabNames();
				ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(false);
			}
			else
			{
				QMessageBox::warning(this,tr("Warning"), tr("Failed to update users affiliation lists"));
			}
		}
		else
		{
			ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(false);
		}
	}
	else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Close)
	{
		reject();
	}
}

void EditUsersListDialog::onItemsTableContextMenuRequested(const QPoint &APos)
{
	QList<QStandardItem *> modelItems = selectedModelItems();
	if (!modelItems.isEmpty())
	{
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose, true);

		QStringList users;
		foreach(QStandardItem *modelItem, modelItems)
			users.append(modelItem->data(UDR_REAL_JID).toString());

		foreach(const QString &affiliation, Affiliations)
		{
			if (affiliation != currentAffiliation())
			{
				Action *action = new Action(menu);
				action->setData(ADR_USER_JID,users);
				action->setData(ADR_USER_AFFILIATON, affiliation);
				action->setEnabled(FAffilRoot.contains(affiliation));
				action->setText(tr("Move %n user(s) to '%1'","",users.count()).arg(affiliatioName(affiliation)));
				connect(action,SIGNAL(triggered()),SLOT(onMoveUserActionTriggered()));
				menu->addAction(action);
			}
		}

		Action *action = new Action(menu);
		action->setData(ADR_USER_JID,users);
		action->setData(ADR_USER_AFFILIATON, MUC_AFFIL_NONE);
		action->setText(tr("Delete %n user(s)","",users.count()));
		connect(action,SIGNAL(triggered()),SLOT(onMoveUserActionTriggered()));
		menu->addAction(action);

		menu->popup(ui.tbvItems->mapToGlobal(APos));
	}
}

void EditUsersListDialog::onMultiChatRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FAffilLoadId.contains(AId))
	{
		QString affiliation = FAffilLoadId.take(AId);
		FModel->removeRow(FAffilRoot.take(affiliation)->row());
		QMessageBox::warning(this,tr("Warning"), tr("Failed to load list of '%1': %2").arg(affiliatioName(affiliation),AError.errorMessage()));
		updateAffiliationTabNames();
	}
	else if (AId == FAffilUpdateId)
	{
		FAffilUpdateId.clear();
		QMessageBox::warning(this,tr("Warning"), tr("Failed to update users affiliation lists: %1").arg(AError.errorMessage()));
		ui.dbbButtons->button(QDialogButtonBox::Save)->setEnabled(true);
		updateAffiliationTabNames();
	}
}

void EditUsersListDialog::onMultiChatListLoaded(const QString &AId, const QList<IMultiUserListItem> &AItems)
{
	if (FAffilLoadId.contains(AId))
	{
		FAffilLoadId.remove(AId);
		applyListItems(AItems);
		updateAffiliationTabNames();
	}
}

void EditUsersListDialog::onMultiChatListUpdated(const QString &AId, const QList<IMultiUserListItem> &AItems)
{
	if (AId == FAffilUpdateId)
	{
		FAffilUpdateId.clear();
		applyListItems(AItems);
		updateAffiliationTabNames();
	}
}
