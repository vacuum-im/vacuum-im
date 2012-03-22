#include "receiverswidget.h"

#include <QHeaderView>
#include <QInputDialog>

ReceiversWidget::ReceiversWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FMessageWidgets = AMessageWidgets;
	FStreamJid = AStreamJid;

	FPresence = NULL;
	FStatusIcons = NULL;
	FRoster = NULL;

	setWindowIconText(tr("Receivers"));

	connect(ui.pbtSelectAll,SIGNAL(clicked()),SLOT(onSelectAllClicked()));
	connect(ui.pbtSelectAllOnLine,SIGNAL(clicked()),SLOT(onSelectAllOnlineClicked()));
	connect(ui.pbtSelectNone,SIGNAL(clicked()),SLOT(onSelectNoneClicked()));
	connect(ui.pbtAdd,SIGNAL(clicked()),SLOT(onAddClicked()));
	connect(ui.pbtUpdate,SIGNAL(clicked()),SLOT(onUpdateClicked()));

	initialize();
}

ReceiversWidget::~ReceiversWidget()
{

}

void ReceiversWidget::setStreamJid(const Jid &AStreamJid)
{
	Jid before = FStreamJid;
	FStreamJid = AStreamJid;
	initialize();
	emit streamJidChanged(before);
}

QString ReceiversWidget::receiverName(const Jid &AReceiver) const
{
	QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
	if (contactItem)
		return contactItem->data(0,RDR_NAME).toString();
	return QString::null;
}

void ReceiversWidget::addReceiversGroup(const QString &AGroup)
{
	QTreeWidgetItem *groupItem = FGroupItems.value(AGroup,NULL);
	if (groupItem)
		groupItem->setCheckState(0,Qt::Checked);
}

void ReceiversWidget::removeReceiversGroup(const QString &AGroup)
{
	QTreeWidgetItem *groupItem = FGroupItems.value(AGroup,NULL);
	if (groupItem)
		groupItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::addReceiver(const Jid &AReceiver)
{
	QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
	if (!contactItem)
	{
		if (AReceiver.isValid())
		{
			QTreeWidgetItem *groupItem = getReceiversGroup(tr("Not in Roster"));
			groupItem->setExpanded(true);
			QString name = AReceiver.node().isEmpty() ? AReceiver.domain() : AReceiver.node();
			contactItem = getReceiver(AReceiver,name,groupItem);
			contactItem->setCheckState(0,Qt::Checked);
		}
	}
	else
	{
		contactItem->setCheckState(0,Qt::Checked);
	}
}

void ReceiversWidget::removeReceiver(const Jid &AReceiver)
{
	QTreeWidgetItem *contactItem = FContactItems.value(AReceiver,NULL);
	if (contactItem)
		contactItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::clear()
{
	onSelectNoneClicked();
}

void ReceiversWidget::initialize()
{
	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (presencePlugin)
			FPresence = presencePlugin->findPresence(FStreamJid);
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (rosterPlugin)
			FRoster = rosterPlugin->findRoster(FStreamJid);
	}

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	if (FRoster && FPresence)
		createRosterTree();
}

QTreeWidgetItem *ReceiversWidget::getReceiversGroup(const QString &AGroup)
{
	QString curGroup;
	QString groupDelim = FRoster->groupDelimiter();
	QTreeWidgetItem *parentGroupItem = ui.trwReceivers->invisibleRootItem();
	QStringList subGroups = AGroup.split(groupDelim,QString::SkipEmptyParts);
	foreach(QString subGroup,subGroups)
	{
		curGroup = curGroup.isEmpty() ? subGroup : curGroup+groupDelim+subGroup;
		QTreeWidgetItem *groupItem = FGroupItems.value(curGroup,NULL);
		if (groupItem == NULL)
		{
			QStringList columns = QStringList() << ' '+subGroup << QString::null;
			groupItem = new QTreeWidgetItem(parentGroupItem,columns);
			groupItem->setCheckState(0,parentGroupItem->checkState(0));
			groupItem->setForeground(0,palette().color(QPalette::Active, QPalette::Highlight));
			groupItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
			groupItem->setData(0,RDR_TYPE,RIT_GROUP);
			groupItem->setData(0,RDR_GROUP,curGroup);
			FGroupItems.insert(curGroup,groupItem);
		}
		parentGroupItem = groupItem;
	}
	return parentGroupItem;
}

QTreeWidgetItem *ReceiversWidget::getReceiver(const Jid &AReceiver, const QString &AName, QTreeWidgetItem *AParent)
{
	QTreeWidgetItem *contactItem = NULL;
	QList<QTreeWidgetItem *> contactItems = FContactItems.values(AReceiver);
	for (int i = 0; contactItem==NULL && i<contactItems.count(); i++)
		if (contactItems.at(i)->parent() == AParent)
			contactItem = contactItems.at(i);

	if (!contactItem)
	{
		QStringList columns = QStringList() << AName << AReceiver.full();
		contactItem = new QTreeWidgetItem(AParent,columns);
		contactItem->setIcon(0,FStatusIcons->iconByJid(FStreamJid,AReceiver));
		contactItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
		contactItem->setData(0,RDR_TYPE,RIT_CONTACT);
		contactItem->setData(0,RDR_FULL_JID,AReceiver.full());
		contactItem->setData(0,RDR_NAME,AName);
		FContactItems.insert(AReceiver,contactItem);
	}
	return contactItem;
}

void ReceiversWidget::createRosterTree()
{
	onSelectNoneClicked();
	disconnect(ui.trwReceivers,SIGNAL(itemChanged(QTreeWidgetItem *,int)),this,SLOT(onReceiversItemChanged(QTreeWidgetItem *,int)));

	ui.trwReceivers->clear();
	FGroupItems.clear();
	FContactItems.clear();
	ui.trwReceivers->setColumnCount(2);
	ui.trwReceivers->setIndentation(10);
	ui.trwReceivers->setHeaderLabels(QStringList() << tr("Contact") << tr("Jid"));

	QList<IRosterItem> ritems = FRoster->rosterItems();
	foreach (IRosterItem ritem, ritems)
	{
		QSet<QString> groups;
		if (ritem.itemJid.node().isEmpty())
			groups.insert(FRostersModel!=NULL ? FRostersModel->singleGroupName(RIT_GROUP_AGENTS) : tr("Agents"));
		else if (ritem.groups.isEmpty())
			groups.insert(FRostersModel!=NULL ? FRostersModel->singleGroupName(RIT_GROUP_BLANK) : tr("Without Groups"));
		else
			groups = ritem.groups;

		foreach(QString group,groups)
		{
			QTreeWidgetItem *groupItem = getReceiversGroup(group);

			QMap<Jid,int> itemJids;
			QList<IPresenceItem> pitems = FPresence->presenceItems(ritem.itemJid);
			foreach(IPresenceItem pitem,pitems)
				itemJids.insert(pitem.itemJid, pitem.show);

			if (itemJids.isEmpty())
				itemJids.insert(ritem.itemJid, IPresence::Offline);

			foreach(Jid itemJid, itemJids.keys())
			{
				QString bareName = !ritem.name.isEmpty() ? ritem.name : ritem.itemJid.bare();
				QString fullName = itemJid.resource().isEmpty() ? bareName : bareName+"/"+itemJid.resource();
				QTreeWidgetItem *contactItem = getReceiver(itemJid,fullName,groupItem);
				contactItem->setCheckState(0, Qt::Unchecked);
				contactItem->setData(0,RDR_SHOW,itemJids.value(itemJid));
			}
		}
	}

	QList<IPresenceItem> myResources = FPresence->presenceItems(FStreamJid);
	foreach(IPresenceItem pitem, myResources)
	{
		QTreeWidgetItem *groupItem = getReceiversGroup(FRostersModel!=NULL ? FRostersModel->singleGroupName(RIT_GROUP_MY_RESOURCES) : tr("My Resources"));
		QString name = pitem.itemJid.resource();
		QTreeWidgetItem *contactItem = getReceiver(pitem.itemJid,name,groupItem);
		contactItem->setCheckState(0, Qt::Unchecked);
		contactItem->setData(0,RDR_TYPE,RIT_MY_RESOURCE);
		contactItem->setData(0,RDR_SHOW,pitem.show);
	}

	ui.trwReceivers->expandAll();
	ui.trwReceivers->sortItems(0,Qt::AscendingOrder);
	ui.trwReceivers->header()->setResizeMode(0,QHeaderView::ResizeToContents);
	ui.trwReceivers->setSelectionMode(QAbstractItemView::NoSelection);
	ui.trwReceivers->setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(ui.trwReceivers,SIGNAL(itemChanged(QTreeWidgetItem *,int)),SLOT(onReceiversItemChanged(QTreeWidgetItem *,int)));
}

void ReceiversWidget::onReceiversItemChanged(QTreeWidgetItem *AItem, int AColumn)
{
	Q_UNUSED(AColumn);
	static int blockUpdateChilds = 0;

	if (AItem->data(0,RDR_TYPE).toInt() == RIT_CONTACT)
	{
		Jid contactJid = AItem->data(0,RDR_FULL_JID).toString();
		if (AItem->checkState(0) == Qt::Checked && !FReceivers.contains(contactJid))
		{
			FReceivers.append(contactJid);
			emit receiverAdded(contactJid);
		}
		else if (AItem->checkState(0) == Qt::Unchecked && FReceivers.contains(contactJid))
		{
			FReceivers.removeAt(FReceivers.indexOf(contactJid));
			emit receiverRemoved(contactJid);
		}

		QList<QTreeWidgetItem *> contactItems = FContactItems.values(contactJid);
		foreach(QTreeWidgetItem *contactItem,contactItems)
			contactItem->setCheckState(0,AItem->checkState(0));
	}
	else if (blockUpdateChilds == 0 && AItem->data(0,RDR_TYPE).toInt() == RIT_GROUP)
	{
		for (int i =0; i< AItem->childCount(); i++)
			AItem->child(i)->setCheckState(0,AItem->checkState(0));
	}

	if (AItem->parent() != NULL)
	{
		blockUpdateChilds++;
		if (AItem->checkState(0) == Qt::Checked)
		{
			QTreeWidgetItem *parentItem = AItem->parent();
			Qt::CheckState state = Qt::Checked;
			for (int i =0; state == Qt::Checked && i < parentItem->childCount(); i++)
				state = parentItem->child(i)->checkState(0);
			if (state == Qt::Checked)
				parentItem->setCheckState(0,Qt::Checked);
		}
		else
			AItem->parent()->setCheckState(0,Qt::Unchecked);
		blockUpdateChilds--;
	}
}

void ReceiversWidget::onSelectAllClicked()
{
	foreach(QTreeWidgetItem *treeItem,FGroupItems)
		treeItem->setCheckState(0,Qt::Checked);
}

void ReceiversWidget::onSelectAllOnlineClicked()
{
	foreach(QTreeWidgetItem *treeItem, FContactItems) 
	{
		if (treeItem->data(0,RDR_TYPE).toInt() == RIT_CONTACT)
		{
			int status = treeItem->data(0,RDR_SHOW).toInt();
			if (status!=IPresence::Offline && status!=IPresence::Error)
				treeItem->setCheckState(0,Qt::Checked);
			else
				treeItem->setCheckState(0,Qt::Unchecked);
		}
	}
}

void ReceiversWidget::onSelectNoneClicked()
{
	foreach(QTreeWidgetItem *treeItem,FContactItems)
		treeItem->setCheckState(0,Qt::Unchecked);
}

void ReceiversWidget::onAddClicked()
{
	Jid contactJid = QInputDialog::getText(this,tr("Add receiver"),tr("Enter valid contact jid:"));
	if (contactJid.isValid())
		addReceiver(contactJid);
}

void ReceiversWidget::onUpdateClicked()
{
	QList<Jid> savedReceivers = FReceivers;
	createRosterTree();
	foreach(Jid receiver, savedReceivers)
		addReceiver(receiver);
}
