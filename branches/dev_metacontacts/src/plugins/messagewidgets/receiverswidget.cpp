#include "receiverswidget.h"

#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDomDocument>
#include <QSortFilterProxyModel>

#define ADR_ITEM_PTR            Action::DR_Parametr1

#define SORT_HANDLER_ORDER      500

ReceiversWidget::ReceiversWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	setWindowIconText(tr("Receivers"));

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;

	FStatusIcons = NULL;
	FRostersModel = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FAccountManager = NULL;
	FMessageProcessor = NULL;

	AdvancedItemDelegate *itemDelegate = new AdvancedItemDelegate(this);
	itemDelegate->setItemsRole(RDR_LABEL_ITEMS);
	ui.trvReceivers->setItemDelegate(itemDelegate);

	FModel = new AdvancedItemModel(this);
	connect(FModel,SIGNAL(itemInserted(QStandardItem *)),SLOT(onModelItemInserted(QStandardItem *)));
	connect(FModel,SIGNAL(itemRemoving(QStandardItem *)),SLOT(onModelItemRemoving(QStandardItem *)));
	connect(FModel,SIGNAL(itemDataChanged(QStandardItem *,int)),SLOT(onModelItemDataChanged(QStandardItem *,int)));
	FModel->insertItemSortHandler(SORT_HANDLER_ORDER,this);
	ui.trvReceivers->setModel(FModel);

	initialize();

	foreach(const Jid &streamJid, FMessageProcessor!=NULL ? FMessageProcessor->activeStreams() : QList<Jid>())
		onActiveStreamAppended(streamJid);

	connect(ui.trvReceivers,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onReceiversContextMenuRequested(const QPoint &)));
}

ReceiversWidget::~ReceiversWidget()
{

}

AdvancedItemSortHandler::SortResult ReceiversWidget::advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const
{
	if (AOrder == SORT_HANDLER_ORDER)
	{
		int leftTypeOrder = ALeft->data(RDR_KIND_ORDER).toInt();
		int rightTypeOrder = ARight->data(RDR_KIND_ORDER).toInt();
		if (leftTypeOrder == rightTypeOrder)
		{
			if (leftTypeOrder != RIKO_STREAM_ROOT)
			{
				int leftShow = ALeft->data(RDR_SHOW).toInt();
				int rightShow = ARight->data(RDR_SHOW).toInt();
				if (leftShow != rightShow)
				{
					static const int showOrders[] = {6,2,1,3,4,5,7,8};
					static const int showOrdersCount = sizeof(showOrders)/sizeof(showOrders[0]);
					if (leftShow<showOrdersCount && rightShow<showOrdersCount)
						return showOrders[leftShow]<showOrders[rightShow] ? AdvancedItemSortHandler::LessThen : AdvancedItemSortHandler::NotLessThen;
				}
			}
			QString leftDisplay = ALeft->data(Qt::DisplayRole).toString().toLower();
			QString rightDisplay = ARight->data(Qt::DisplayRole).toString().toLower();
			return leftDisplay.localeAwareCompare(rightDisplay)<0 ? AdvancedItemSortHandler::LessThen : AdvancedItemSortHandler::NotLessThen;
		}
		return leftTypeOrder<rightTypeOrder ? AdvancedItemSortHandler::LessThen : AdvancedItemSortHandler::NotLessThen;
	}
	return AdvancedItemSortHandler::Undefined;
}

bool ReceiversWidget::isVisibleOnWindow() const
{
	return isVisibleTo(FWindow->instance());
}

IMessageWindow *ReceiversWidget::messageWindow() const
{
	return FWindow;
}

QList<Jid> ReceiversWidget::availStreams() const
{
	return FStreamItems.keys();
}

QTreeView *ReceiversWidget::receiversView() const
{
	return ui.trvReceivers;
}

AdvancedItemModel *ReceiversWidget::receiversModel() const
{
	return FModel;
}

void ReceiversWidget::contextMenuForItem(QStandardItem *AItem, Menu *AMenu)
{
	if (AItem->hasChildren())
	{
		Action *selectAll = new Action(AMenu);
		selectAll->setText(tr("Select All Contacts"));
		selectAll->setData(ADR_ITEM_PTR,(qint64)AItem);
		connect(selectAll,SIGNAL(triggered()),SLOT(onSelectAllContacts()));
		AMenu->addAction(selectAll,AG_MWRWCM_MWIDGETS_SELECT_ALL);

		Action *selectOnline = new Action(AMenu);
		selectOnline->setText(tr("Select Online Contact"));
		selectOnline->setData(ADR_ITEM_PTR,(qint64)AItem);
		connect(selectOnline,SIGNAL(triggered()),SLOT(onSelectOnlineContacts()));
		AMenu->addAction(selectOnline,AG_MWRWCM_MWIDGETS_SELECT_ONLINE);

		Action *selectNotBusy = new Action(AMenu);
		selectNotBusy->setText(tr("Select Available Contacts"));
		selectNotBusy->setData(ADR_ITEM_PTR,(qint64)AItem);
		connect(selectNotBusy,SIGNAL(triggered()),SLOT(onSelectNotBusyContacts()));
		AMenu->addAction(selectNotBusy,AG_MWRWCM_MWIDGETS_SELECT_NOTBUSY);

		Action *selectNone = new Action(AMenu);
		selectNone->setText(tr("Clear Selection"));
		selectNone->setData(ADR_ITEM_PTR,(qint64)AItem);
		connect(selectNone,SIGNAL(triggered()),SLOT(onSelectNoneContacts()));
		AMenu->addAction(selectNone,AG_MWRWCM_MWIDGETS_SELECT_CLEAR);

		if (AItem == FModel->invisibleRootItem())
		{
			Action *selectLoad = new Action(AMenu);
			selectLoad->setText(tr("Load Selection"));
			selectLoad->setData(ADR_ITEM_PTR,(qint64)AItem);
			connect(selectLoad,SIGNAL(triggered()),SLOT(onSelectionLoad()));
			AMenu->addAction(selectLoad,AG_MWRWCM_MWIDGETS_SELECT_LOAD);

			Action *selectSave = new Action(AMenu);
			selectSave->setText(tr("Save Selection"));
			selectSave->setData(ADR_ITEM_PTR,(qint64)AItem);
			connect(selectSave,SIGNAL(triggered()),SLOT(onSelectionSave()));
			AMenu->addAction(selectSave,AG_MWRWCM_MWIDGETS_SELECT_SAVE);
		}
	}

	emit contextMenuForItemRequested(AItem,AMenu);
}

QMultiMap<Jid, Jid> ReceiversWidget::selectedAddresses() const
{
	QMultiMap<Jid, Jid> addresses;
	for (QMap<Jid, QMultiHash<Jid, QStandardItem *> >::const_iterator streamIt=FContactItems.constBegin(); streamIt!=FContactItems.constEnd(); ++streamIt)
	{
		for (QMultiHash<Jid, QStandardItem *>::const_iterator contactIt=streamIt->constBegin(); contactIt!=streamIt->constEnd(); ++contactIt)
		{
			if (contactIt.value()->checkState() == Qt::Checked)
			{
				if (!addresses.contains(streamIt.key(),contactIt.key()))
					addresses.insertMulti(streamIt.key(),contactIt.key());
			}
		}
	}
	return addresses;
}

void ReceiversWidget::setGroupSelection(const Jid &AStreamJid, const QString &AGroup, bool ASelected)
{
	QString group = AGroup.isEmpty() ? (FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_BLANK) : tr("Without Groups")) : AGroup;
	QStandardItem *groupItem = FGroupItems.value(AStreamJid).value(group);
	if (groupItem)
		groupItem->setCheckState(ASelected ? Qt::Checked : Qt::Unchecked);
}

void ReceiversWidget::setAddressSelection(const Jid &AStreamJid, const Jid &AContactJid, bool ASelected)
{
	QList<QStandardItem *> contactItems = findContactItems(AStreamJid,AContactJid);
	if (ASelected && contactItems.isEmpty() && FStreamItems.contains(AStreamJid) && AContactJid.isValid())
	{
		QString group = FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_NOT_IN_ROSTER) : tr("Not in Roster");
		QStandardItem *contactItem = getContactItem(AStreamJid,AContactJid,AContactJid.uBare(),group,RIKO_GROUP_NOT_IN_ROSTER);
		updateContactItemsPresence(AStreamJid,AContactJid);
		contactItems.append(contactItem);
	}

	foreach(QStandardItem *contactItem, contactItems)
		contactItem->setCheckState(ASelected ? Qt::Checked : Qt::Unchecked);
}

void ReceiversWidget::clearSelection()
{
	for (QMap<Jid, QMultiHash<Jid, QStandardItem *> >::const_iterator streamIt=FContactItems.constBegin(); streamIt!=FContactItems.constEnd(); ++streamIt)
		for (QMultiHash<Jid, QStandardItem *>::const_iterator contactIt=streamIt->constBegin(); contactIt!=streamIt->constEnd(); ++contactIt)
			contactIt.value()->setCheckState(Qt::Unchecked);
}

void ReceiversWidget::initialize()
{
	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceClosed(IPresence *)),SLOT(onPresenceClosed(IPresence *)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
		{
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamAppended(const Jid &)),SLOT(onActiveStreamAppended(const Jid &)));
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveStreamRemoved(const Jid &)));
		}
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
}

void ReceiversWidget::createStreamItems(const Jid &AStreamJid)
{
	if (getStreamItem(AStreamJid))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		foreach(const IRosterItem &ritem, roster!=NULL ? roster->rosterItems() : QList<IRosterItem>())
			onRosterItemReceived(roster,ritem,IRosterItem());
	}
}

void ReceiversWidget::destroyStreamItems(const Jid &AStreamJid)
{
	QStandardItem *streamItem = FStreamItems.value(AStreamJid);
	if (streamItem)
	{
		QMultiHash<Jid, QStandardItem *> contactItems = FContactItems.value(AStreamJid);
		for (QMultiHash<Jid, QStandardItem *>::const_iterator contactIt=contactItems.constBegin(); contactIt!=contactItems.constEnd(); ++contactIt)
		{
			QStandardItem *contactItem = contactIt.value();
			contactItem->setCheckState(Qt::Unchecked);
		}
		FModel->removeRow(streamItem->row());

		FStreamItems.remove(AStreamJid);
		FContactItems.remove(AStreamJid);
		FGroupItems.remove(AStreamJid);
	}
}

QStandardItem *ReceiversWidget::getStreamItem(const Jid &AStreamJid)
{
	QStandardItem *streamItem = FStreamItems.value(AStreamJid);
	if (streamItem == NULL)
	{
		streamItem = new AdvancedItem();
		streamItem->setCheckable(true);
		streamItem->setData(RIK_STREAM_ROOT,RDR_KIND);
		streamItem->setData(RIKO_STREAM_ROOT,RDR_KIND_ORDER);
		streamItem->setData(AStreamJid.pFull(),RDR_STREAM_JID);

		QFont streamFont = streamItem->font();
		streamFont.setBold(true);
		streamItem->setFont(streamFont);

		streamItem->setBackground(ui.trvReceivers->palette().color(QPalette::Active, QPalette::Dark));
		streamItem->setForeground(ui.trvReceivers->palette().color(QPalette::Active, QPalette::BrightText));

		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
		streamItem->setText(account!=NULL ? account->name() : AStreamJid.uBare());

		FModel->invisibleRootItem()->appendRow(streamItem);
		ui.trvReceivers->expand(FModel->indexFromItem(streamItem));
	}
	return streamItem;
}

QStandardItem *ReceiversWidget::getGroupItem(const Jid &AStreamJid, const QString &AGroup, int AGroupOrder)
{
	QStandardItem *groupItem = FGroupItems.value(AStreamJid).value(AGroup);
	if (groupItem == NULL)
	{
		QStringList groupPath = AGroup.split(ROSTER_GROUP_DELIMITER);
		QString groupName = groupPath.takeLast();

		groupItem = new AdvancedItem(groupName);
		groupItem->setCheckable(true);
		groupItem->setData(RIK_GROUP,RDR_KIND);
		groupItem->setData(AGroupOrder,RDR_KIND_ORDER);
		groupItem->setData(AStreamJid.pFull(),RDR_STREAM_JID);
		groupItem->setData(AGroup,RDR_GROUP);
		groupItem->setText(groupName);

		QFont groupFont = groupItem->font();
		groupFont.setBold(true);
		groupItem->setFont(groupFont);

		groupItem->setForeground(ui.trvReceivers->palette().color(QPalette::Active, QPalette::Highlight));

		QStandardItem *parentItem = groupPath.isEmpty() ? getStreamItem(AStreamJid) : getGroupItem(AStreamJid,groupPath.join(ROSTER_GROUP_DELIMITER),AGroupOrder);
		parentItem->appendRow(groupItem);

		ui.trvReceivers->expand(FModel->indexFromItem(groupItem));
	}
	return groupItem;
}

QList<QStandardItem *> ReceiversWidget::findContactItems(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FContactItems.value(AStreamJid).values(AContactJid.bare());
}

QStandardItem *ReceiversWidget::findContactItem(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const
{
	foreach(QStandardItem *item, findContactItems(AStreamJid,AContactJid))
		if (item->data(RDR_GROUP).toString() == AGroup)
			return item;
	return NULL;
}

QStandardItem *ReceiversWidget::getContactItem(const Jid &AStreamJid, const Jid &AContactJid, const QString &AName, const QString &AGroup, int AGroupOrder)
{
	QStandardItem *contactItem = findContactItem(AStreamJid,AContactJid,AGroup);
	if (contactItem == NULL)
	{
		contactItem = new AdvancedItem();
		contactItem->setCheckable(true);
		contactItem->setData(RIK_CONTACT,RDR_KIND);
		contactItem->setData(RIKO_DEFAULT,RDR_KIND_ORDER);
		contactItem->setData(AStreamJid.pFull(),RDR_STREAM_JID);
		contactItem->setData(AContactJid.pBare(),RDR_PREP_BARE_JID);
		contactItem->setData(AGroup,RDR_GROUP);

		contactItem->setToolTip(Qt::escape(AContactJid.uBare()));

		QStandardItem *parentItem = getGroupItem(AStreamJid,AGroup,AGroupOrder);
		parentItem->appendRow(contactItem);
	}
	contactItem->setText(AName);
	return contactItem;
}

void ReceiversWidget::deleteItemLater(QStandardItem *AItem)
{
	if (AItem && !FDeleteDelayed.contains(AItem))
	{
		FDeleteDelayed.append(AItem);
		QTimer::singleShot(0,this,SIGNAL(onDeleteDelayedItems()));
	}
}

void ReceiversWidget::updateCheckState(QStandardItem *AItem)
{
	if (AItem && AItem->hasChildren() && AItem!=FModel->invisibleRootItem())
	{
		bool allChecked = true;
		bool allUnchecked = true;
		for (int row=0; row<AItem->rowCount(); row++)
		{
			QStandardItem *childItem = AItem->child(row);
			if (!FModel->isRemovedItem(childItem))
			{
				allChecked = allChecked && (childItem->checkState()==Qt::Checked);
				allUnchecked = allUnchecked && (childItem->checkState()==Qt::Unchecked);
			}
		}

		if (allChecked && !allUnchecked)
			AItem->setCheckState(Qt::Checked);
		else if (!allChecked && allUnchecked)
			AItem->setCheckState(Qt::Unchecked);
		else if (!allChecked && !allUnchecked)
			AItem->setCheckState(Qt::PartiallyChecked);
	}
}

void ReceiversWidget::updateContactItemsPresence(const Jid &AStreamJid, const Jid &AContactJid)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	IPresenceItem pitem = presence!=NULL ? FPresencePlugin->sortPresenceItems(presence->findItems(AContactJid.bare())).value(0) : IPresenceItem();
	foreach(QStandardItem *contactItem, findContactItems(AStreamJid,AContactJid))
	{
		contactItem->setData(pitem.show,RDR_SHOW);
		contactItem->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(pitem.itemJid,pitem.show,SUBSCRIPTION_BOTH,false) : QIcon());
	}
}

Jid ReceiversWidget::findAvailStream(const Jid &AStreamJid) const
{
	foreach(const Jid streamJid, FStreamItems.keys())
		if (streamJid.pBare() == AStreamJid.pBare())
			return streamJid;
	return Jid::null;
}

void ReceiversWidget::selectAllContacts(QStandardItem *AParent)
{
	for (int row=0; row<AParent->rowCount(); row++)
	{
		QStandardItem *item = AParent->child(row);
		if (item->data(RDR_KIND).toInt() == RIK_CONTACT)
			item->setCheckState(Qt::Checked);
		else if (item->hasChildren())
			selectAllContacts(item);
	}
}

void ReceiversWidget::selectOnlineContacts(QStandardItem *AParent)
{
	for (int row=0; row<AParent->rowCount(); row++)
	{
		QStandardItem *item = AParent->child(row);
		if (item->data(RDR_KIND).toInt() == RIK_CONTACT)
		{
			int show = item->data(RDR_SHOW).toInt();
			if (show!=IPresence::Offline && show!=IPresence::Error)
				item->setCheckState(Qt::Checked);
			else
				item->setCheckState(Qt::Unchecked);
		}
		else if (item->hasChildren())
		{
			selectOnlineContacts(item);
		}
	}
}

void ReceiversWidget::selectNotBusyContacts(QStandardItem *AParent)
{
	for (int row=0; row<AParent->rowCount(); row++)
	{
		QStandardItem *item = AParent->child(row);
		if (item->data(RDR_KIND).toInt() == RIK_CONTACT)
		{
			int show = item->data(RDR_SHOW).toInt();
			if (show!=IPresence::Offline && show!=IPresence::Error && show!=IPresence::DoNotDisturb)
				item->setCheckState(Qt::Checked);
			else
				item->setCheckState(Qt::Unchecked);
		}
		else if (item->hasChildren())
		{
			selectNotBusyContacts(item);
		}
	}
}

void ReceiversWidget::selectNoneContacts(QStandardItem *AParent)
{
	for (int row=0; row<AParent->rowCount(); row++)
	{
		QStandardItem *item = AParent->child(row);
		if (item->data(RDR_KIND).toInt() == RIK_CONTACT)
			item->setCheckState(Qt::Unchecked);
		else if (item->hasChildren())
			selectNoneContacts(item);
	}
}

void ReceiversWidget::onModelItemInserted(QStandardItem *AItem)
{
	int itemKind = AItem->data(RDR_KIND).toInt();
	Jid streamJid = AItem->data(RDR_STREAM_JID).toString();
	if (itemKind==RIK_STREAM_ROOT || FStreamItems.contains(streamJid))
	{
		if (itemKind == RIK_STREAM_ROOT)
			FStreamItems.insert(streamJid,AItem);
		else if (itemKind == RIK_GROUP)
			FGroupItems[streamJid].insert(AItem->data(RDR_GROUP).toString(),AItem);
		else if (itemKind == RIK_CONTACT)
			FContactItems[streamJid].insertMulti(AItem->data(RDR_PREP_BARE_JID).toString(),AItem);
	}

	if (AItem->parent())
		AItem->parent()->sortChildren(0);
	else
		FModel->sort(0);
	updateCheckState(AItem->parent());
}

void ReceiversWidget::onModelItemRemoving(QStandardItem *AItem)
{
	int itemKind = AItem->data(RDR_KIND).toInt();
	Jid streamJid = AItem->data(RDR_STREAM_JID).toString();
	if (FStreamItems.contains(streamJid))
	{
		AItem->setCheckState(Qt::Unchecked);
		if (itemKind == RIK_STREAM_ROOT)
			FStreamItems.remove(streamJid);
		else if (itemKind == RIK_GROUP)
			FGroupItems[streamJid].remove(AItem->data(RDR_GROUP).toString());
		else if (itemKind == RIK_CONTACT)
			FContactItems[streamJid].remove(AItem->data(RDR_PREP_BARE_JID).toString(),AItem);
	}
	updateCheckState(AItem->parent());

	if (AItem->parent() && AItem->parent()->rowCount()<2 && AItem->parent()->data(RDR_KIND).toInt()!=RIK_STREAM_ROOT)
		deleteItemLater(AItem->parent());
	FDeleteDelayed.removeAll(AItem);
}

void ReceiversWidget::onModelItemDataChanged(QStandardItem *AItem, int ARole)
{
	if (ARole == Qt::CheckStateRole)
	{
		if (AItem->data(RDR_KIND).toInt() == RIK_CONTACT)
		{
			static bool block = false;
			if (!block)
			{
				block = true;
				Jid streamJid = AItem->data(RDR_STREAM_JID).toString();
				Jid contactJid = AItem->data(RDR_PREP_BARE_JID).toString();
				QList<QStandardItem *> contactItems = findContactItems(streamJid,contactJid);
				if (!FModel->isRemovedItem(AItem))
				{
					foreach(QStandardItem *contactItem, contactItems)
						contactItem->setCheckState(AItem->checkState());
					emit addressSelectionChanged(streamJid,contactJid,AItem->checkState()==Qt::Checked);
				}
				else if (contactItems.count() < 2)
				{
					emit addressSelectionChanged(streamJid,contactJid,false);
				}
				block = false;
			}
		}

		Qt::CheckState state = AItem->checkState();
		if (state != Qt::PartiallyChecked)
			for (int row=0; row<AItem->rowCount(); row++)
				AItem->child(row)->setCheckState(state);
		updateCheckState(AItem->parent());
	}
	else if (ARole==Qt::DisplayRole || ARole==RDR_KIND_ORDER || ARole==RDR_SHOW)
	{
		AItem->parent()->sortChildren(0);
	}
}

void ReceiversWidget::onActiveStreamAppended(const Jid &AStreamJid)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen())
		onPresenceOpened(presence);
}

void ReceiversWidget::onActiveStreamRemoved(const Jid &AStreamJid)
{
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence)
		onPresenceClosed(presence);
}

void ReceiversWidget::onPresenceOpened(IPresence *APresence)
{
	if (!FStreamItems.contains(APresence->streamJid()))
	{
		if (FMessageProcessor==NULL || FMessageProcessor->activeStreams().contains(APresence->streamJid()))
		{
			createStreamItems(APresence->streamJid());
			emit availStreamsChanged();
		}
	}
}

void ReceiversWidget::onPresenceClosed(IPresence *APresence)
{
	if (FStreamItems.contains(APresence->streamJid()))
	{
		destroyStreamItems(APresence->streamJid());
		emit availStreamsChanged();
	}
}

void ReceiversWidget::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (FStreamItems.contains(APresence->streamJid()))
		if (AItem.show!=ABefore.show || AItem.priority!=ABefore.priority)
			updateContactItemsPresence(APresence->streamJid(),AItem.itemJid);
}

void ReceiversWidget::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (FStreamItems.contains(ARoster->streamJid()))
	{
		QList<QStandardItem *> contactItems = findContactItems(ARoster->streamJid(),AItem.itemJid);
		if (AItem.subscription == SUBSCRIPTION_REMOVE)
		{
			foreach(QStandardItem *contactItem, contactItems)
				contactItem->parent()->removeRow(contactItem->row());
		}
		else
		{
			bool updatePresence = false;
			QString name = AItem.name.isEmpty() ? AItem.itemJid.uBare() : AItem.name;

			if (contactItems.isEmpty() || AItem.groups!=ABefore.groups)
			{
				QSet<QString> oldGroups;
				foreach(QStandardItem *contactItem, contactItems)
					oldGroups += contactItem->data(RDR_GROUP).toString();

				int groupOrder;
				QSet<QString> newGroups;
				if (AItem.itemJid.node().isEmpty())
				{
					groupOrder = RIKO_GROUP_AGENTS;
					newGroups << (FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_AGENTS) : tr("Agents"));
				}
				else if (AItem.groups.isEmpty())
				{
					groupOrder = RIKO_GROUP_BLANK;
					newGroups << (FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_BLANK) : tr("Without Groups"));
				}
				else
				{
					groupOrder = RIKO_GROUP;
					newGroups = AItem.groups;
				}

				foreach(const QString &group, newGroups-oldGroups)
				{
					QStandardItem *contactItem = getContactItem(ARoster->streamJid(),AItem.itemJid,name,group,groupOrder);
					if (!contactItems.isEmpty())
						contactItem->setCheckState(contactItems.first()->checkState());
					contactItems.append(contactItem);
					updatePresence = true;
				}

				foreach(const QString &group, oldGroups-newGroups)
				{
					QStandardItem *contactItem = findContactItem(ARoster->streamJid(),AItem.itemJid,group);
					if (contactItem)
						contactItem->parent()->removeRow(contactItem->row());
				}
			}

			if (AItem.name != ABefore.name)
			{
				foreach(QStandardItem *contactItem, contactItems)
					contactItem->setText(name);
			}

			if (updatePresence)
				updateContactItemsPresence(ARoster->streamJid(),AItem.itemJid);
		}
	}
}

void ReceiversWidget::onSelectionLoad()
{
	QString fileName = QFileDialog::getOpenFileName(this,tr("Load Contacts from File"),QString::null,"*.cts");
	if (!fileName.isEmpty())
	{
		QFile file(fileName);
		if (file.open(QFile::ReadOnly))
		{
			QString errorMsg;
			QDomDocument doc;
			if (doc.setContent(&file,true,&errorMsg))
			{
				if (doc.documentElement().namespaceURI() == "vacuum:messagewidgets:receiverswidget:selection")
				{
					clearSelection();
					QDomElement streamElem = doc.documentElement().firstChildElement("stream");
					while(!streamElem.isNull())
					{
						Jid streamJid = findAvailStream(streamElem.attribute("jid"));
						if (streamJid.isValid())
						{
							QDomElement itemElem = streamElem.firstChildElement("item");
							while(!itemElem.isNull())
							{
								setAddressSelection(streamJid,itemElem.text(),true);
								itemElem = itemElem.nextSiblingElement("item");
							}
						}
						streamElem = streamElem.nextSiblingElement("stream");
					}
				}
				else
				{
					QMessageBox::critical(this,tr("Failed to Load Contacts"),tr("Incorrect file format"),QMessageBox::Ok);
				}
			}
			else
			{
				QMessageBox::critical(this,tr("Failed to Load Contacts"),tr("Failed to read file: %1").arg(errorMsg),QMessageBox::Ok);
			}
		}
		else
		{
			QMessageBox::critical(this,tr("Failed to Load Contacts"),tr("Failed to open file: %1").arg(file.errorString()),QMessageBox::Ok);
		}
	}
}

void ReceiversWidget::onSelectionSave()
{
	QMultiMap<Jid, Jid> addresses = selectedAddresses();
	if (!addresses.isEmpty())
	{
		QString fileName = QFileDialog::getSaveFileName(this,tr("Save Contacts to File"),QString::null,"*.cts");
		if (!fileName.isEmpty())
		{
			QFile file(fileName);
			if (file.open(QFile::WriteOnly))
			{
				QDomDocument doc;
				doc.appendChild(doc.createElementNS("vacuum:messagewidgets:receiverswidget:selection","addresses"));

				Jid streamJid;
				QDomElement streamElem;
				for (QMultiMap<Jid,Jid>::const_iterator it=addresses.constBegin(); it!=addresses.constEnd(); ++it)
				{
					if (streamJid != it.key())
					{
						streamJid = it.key();
						streamElem = doc.documentElement().appendChild(doc.createElement("stream")).toElement();
						streamElem.setAttribute("jid",streamJid.bare());
					}

					QDomElement itemElem = streamElem.appendChild(doc.createElement("item")).toElement();
					itemElem.appendChild(doc.createTextNode(it->bare()));
				}

				file.write(doc.toByteArray());
				file.close();
			}
			else
			{
				QMessageBox::critical(this,tr("Failed to Save Contacts"),tr("Failed to create file: %1").arg(file.errorString()),QMessageBox::Ok);
			}
		}
	}
}

void ReceiversWidget::onSelectAllContacts()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		selectAllContacts((QStandardItem *)action->data(ADR_ITEM_PTR).toLongLong());
}

void ReceiversWidget::onSelectOnlineContacts()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		selectOnlineContacts((QStandardItem *)action->data(ADR_ITEM_PTR).toLongLong());
}

void ReceiversWidget::onSelectNotBusyContacts()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		selectNotBusyContacts((QStandardItem *)action->data(ADR_ITEM_PTR).toLongLong());
}

void ReceiversWidget::onSelectNoneContacts()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		selectNoneContacts((QStandardItem *)action->data(ADR_ITEM_PTR).toLongLong());
}

void ReceiversWidget::onReceiversContextMenuRequested(const QPoint &APos)
{
	QModelIndex index = ui.trvReceivers->indexAt(APos);
	if (index.isValid())
	{
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);
		contextMenuForItem(FModel->itemFromIndex(index),menu);

		if (!menu->isEmpty())
			menu->popup(ui.trvReceivers->mapToGlobal(APos));
		else
			delete menu;
	}
}

void ReceiversWidget::onDeleteDelayedItems()
{
	QList<QStandardItem *> deleteNow = FDeleteDelayed;
	foreach(QStandardItem *item, deleteNow)
	{
		if (FDeleteDelayed.contains(item))
			item->parent()->removeRow(item->row());
	}
}
