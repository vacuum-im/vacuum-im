#include "rostersmodel.h"

#include <QTimer>

#define INDEX_CHANGES_FOR_RESET 20

static const QList<int> RosterItemTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;
static const QList<int> RosterGroupTypes = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK << RIT_GROUP_NOT_IN_ROSTER << RIT_GROUP_MY_RESOURCES << RIT_GROUP_AGENTS;

RostersModel::RostersModel()
{
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FAccountManager = NULL;

	FRootIndex = new RosterIndex(RIT_ROOT);
	FRootIndex->setParent(this);
	connect(FRootIndex,SIGNAL(dataChanged(IRosterIndex *, int)),SLOT(onIndexDataChanged(IRosterIndex *, int)));
	connect(FRootIndex,SIGNAL(childAboutToBeInserted(IRosterIndex *)),SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childInserted(IRosterIndex *)),SLOT(onIndexChildInserted(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childAboutToBeRemoved(IRosterIndex *)),SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	connect(FRootIndex,SIGNAL(childRemoved(IRosterIndex *)),SLOT(onIndexChildRemoved(IRosterIndex *)));
}

RostersModel::~RostersModel()
{
	FRootIndex->removeAllChilds();
	delete FRootIndex;
}

void RostersModel::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Model");
	APluginInfo->description = tr("Creates a hierarchical model for display roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool RostersModel::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),
				SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),
				SLOT(onPresenceChanged(IPresence *, int , const QString &, int)));
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
		}
	}

	return true;
}

bool RostersModel::initObjects()
{
	registerSingleGroup(RIT_GROUP_BLANK,tr("Without Groups"));
	registerSingleGroup(RIT_GROUP_AGENTS,tr("Agents"));
	registerSingleGroup(RIT_GROUP_MY_RESOURCES,tr("My Resources"));
	registerSingleGroup(RIT_GROUP_NOT_IN_ROSTER,tr("Not in Roster"));
	return true;
}

QModelIndex RostersModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
	IRosterIndex *pindex = rosterIndexByModelIndex(AParent);

	IRosterIndex *cindex = pindex->child(ARow);
	if (cindex)
		return createIndex(ARow,AColumn,cindex);

	return QModelIndex();
}

QModelIndex RostersModel::parent(const QModelIndex &AIndex) const
{
	if (AIndex.isValid())
		return modelIndexByRosterIndex(rosterIndexByModelIndex(AIndex)->parentIndex());
	return QModelIndex();
}

bool RostersModel::hasChildren(const QModelIndex &AParent) const
{
	IRosterIndex *pindex = rosterIndexByModelIndex(AParent);
	return pindex->childCount() > 0;
}

int RostersModel::rowCount(const QModelIndex &AParent) const
{
	IRosterIndex *pindex = rosterIndexByModelIndex(AParent);
	return pindex->childCount();
}

int RostersModel::columnCount(const QModelIndex &AParent) const
{
	Q_UNUSED(AParent);
	return 1;
}

Qt::ItemFlags RostersModel::flags(const QModelIndex &AIndex) const
{
	IRosterIndex *rindex = rosterIndexByModelIndex(AIndex);
	return rindex->flags();
}

QMap<int, QVariant> RostersModel::itemData(const QModelIndex &AIndex) const
{
	IRosterIndex *rindex = rosterIndexByModelIndex(AIndex);
	return rindex->data();
}

QVariant RostersModel::data(const QModelIndex &AIndex, int ARole) const
{
	IRosterIndex *rindex = rosterIndexByModelIndex(AIndex);
	return rindex->data(ARole);
}

bool RostersModel::setData(const QModelIndex &AIndex, const QVariant &AValue, int ARole)
{
	if (AIndex.isValid())
	{
		IRosterIndex *rindex = rosterIndexByModelIndex(AIndex);
		return rindex->setData(ARole,AValue);
	}
	return false;
}

IRosterIndex *RostersModel::addStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
	if (streamIndex == NULL)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;

		if (roster || presence)
		{
			IRosterIndex *streamIndex = createRosterIndex(RIT_STREAM_ROOT,FRootIndex);
			streamIndex->setRemoveOnLastChildRemoved(false);
			streamIndex->setData(RDR_STREAM_JID,AStreamJid.pFull());
			streamIndex->setData(RDR_FULL_JID,AStreamJid.full());
			streamIndex->setData(RDR_PREP_FULL_JID,AStreamJid.pFull());
			streamIndex->setData(RDR_PREP_BARE_JID,AStreamJid.pBare());

			if (presence)
			{
				streamIndex->setData(RDR_SHOW, presence->show());
				streamIndex->setData(RDR_STATUS,presence->status());
			}
			if (account)
			{
				streamIndex->setData(RDR_NAME,account->name());
				connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
			}

			FStreamsRoot.insert(AStreamJid,streamIndex);
			insertRosterIndex(streamIndex,FRootIndex);
			emit streamAdded(AStreamJid);

			if (roster)
			{
				IRosterItem empty;
				foreach(IRosterItem ritem, roster->rosterItems()) {
					onRosterItemReceived(roster,ritem,empty); }
			}
		}
	}
	return streamIndex;
}

QList<Jid> RostersModel::streams() const
{
	return FStreamsRoot.keys();
}

void RostersModel::removeStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamsRoot.take(AStreamJid);
	if (streamIndex)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
		if (account)
			connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),this,SLOT(onAccountOptionsChanged(const OptionsNode &)));
		removeRosterIndex(streamIndex);
		FContactsCache.remove(streamIndex);
		emit streamRemoved(AStreamJid);
	}
}

IRosterIndex *RostersModel::rootIndex() const
{
	return FRootIndex;
}

IRosterIndex *RostersModel::streamRoot(const Jid &AStreamJid) const
{
	return FStreamsRoot.value(AStreamJid);
}

IRosterIndex *RostersModel::createRosterIndex(int AType, IRosterIndex *AParent)
{
	IRosterIndex *rindex = new RosterIndex(AType);
	connect(rindex->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onIndexDestroyed(IRosterIndex *)));

	if (AParent)
		rindex->setData(RDR_STREAM_JID,AParent->data(RDR_STREAM_JID));

	emit indexCreated(rindex,AParent);
	insertDefaultDataHolders(rindex);

	return rindex;
}

IRosterIndex *RostersModel::findGroupIndex(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const
{
	QString groupPath = getGroupName(AType,AGroup);
	QList<QString> groupTree = groupPath.split(AGroupDelim,QString::SkipEmptyParts);

	IRosterIndex *groupIndex = AParent;
	do
	{
		QList<IRosterIndex *> indexes = FGroupsCache.value(groupIndex).values(groupTree.takeFirst());

		groupIndex = NULL;
		for(QList<IRosterIndex *>::const_iterator it = indexes.constBegin(); !groupIndex && it!=indexes.constEnd(); ++it)
			if ((*it)->type() == AType)
				groupIndex = *it;

	} while (groupIndex && !groupTree.isEmpty());

	return groupIndex;
}

IRosterIndex *RostersModel::createGroupIndex(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent)
{
	IRosterIndex *groupIndex = findGroupIndex(AType,AGroup,AGroupDelim,AParent);
	if (!groupIndex)
	{
		QString groupPath = getGroupName(AType,AGroup);
		QList<QString> groupTree = groupPath.split(AGroupDelim,QString::SkipEmptyParts);

		int i = 0;
		groupIndex = AParent;
		IRosterIndex *childGroupIndex = groupIndex;
		QString group = AParent->data(RDR_GROUP).toString();
		while (childGroupIndex && i<groupTree.count())
		{
			if (group.isEmpty())
				group = groupTree.at(i);
			else
				group += AGroupDelim + groupTree.at(i);

			childGroupIndex = findGroupIndex(AType, groupTree.at(i), AGroupDelim, groupIndex);
			if (childGroupIndex)
			{
				groupIndex = childGroupIndex;
				i++;
			}
		}

		while (i < groupTree.count())
		{
			childGroupIndex = createRosterIndex(AType, groupIndex);
			childGroupIndex->setData(RDR_GROUP, !FSingleGroups.contains(AType) ? group : QString::null);
			childGroupIndex->setData(RDR_NAME, groupTree.at(i));
			insertRosterIndex(childGroupIndex, groupIndex);
			groupIndex = childGroupIndex;
			group += AGroupDelim + groupTree.value(++i);
		}
	}
	return groupIndex;
}

void RostersModel::insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent)
{
	if (AIndex)
		AIndex->setParentIndex(AParent);
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex)
{
	AIndex->setParentIndex(NULL);
}

QList<IRosterIndex *> RostersModel::getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate)
{
	QList<IRosterIndex *> indexes;
	IRosterIndex *streamIndex = FStreamsRoot.value(AStreamJid);
	if (streamIndex)
	{
		indexes = findContactIndexes(AStreamJid,AContactJid,AContactJid.resource().isEmpty());
		if (indexes.isEmpty() && !AContactJid.resource().isEmpty())
			indexes = findContactIndexes(AStreamJid,AContactJid,true);

		if (ACreate && indexes.isEmpty())
		{
			int type = RIT_CONTACT;
			if (AContactJid.node().isEmpty())
				type = RIT_AGENT;
			else if (AContactJid && AStreamJid)
				type = RIT_MY_RESOURCE;

			IRosterIndex *groupIndex;
			if (type == RIT_MY_RESOURCE)
				groupIndex = createGroupIndex(RIT_GROUP_MY_RESOURCES,QString::null,"::",streamIndex);
			else
				groupIndex = createGroupIndex(RIT_GROUP_NOT_IN_ROSTER,QString::null,"::",streamIndex);

			IRosterIndex *itemIndex = createRosterIndex(type,groupIndex);
			itemIndex->setData(RDR_FULL_JID,AContactJid.full());
			itemIndex->setData(RDR_PREP_FULL_JID,AContactJid.pFull());
			itemIndex->setData(RDR_PREP_BARE_JID,AContactJid.pBare());
			itemIndex->setData(RDR_GROUP,groupIndex->data(RDR_GROUP));
			insertRosterIndex(itemIndex,groupIndex);
			indexes.append(itemIndex);
		}
	}
	return indexes;
}

QModelIndex RostersModel::modelIndexByRosterIndex(IRosterIndex *AIndex) const
{
	return AIndex!=NULL && AIndex!=FRootIndex ? createIndex(AIndex->row(),0,AIndex) : QModelIndex();
}

IRosterIndex *RostersModel::rosterIndexByModelIndex(const QModelIndex &AIndex) const
{
	return AIndex.isValid() ? reinterpret_cast<IRosterIndex *>(AIndex.internalPointer()) : FRootIndex;
}

QString RostersModel::singleGroupName(int AType) const
{
	return FSingleGroups.value(AType);
}

void RostersModel::registerSingleGroup(int AType, const QString &AName)
{
	if (!FSingleGroups.contains(AType) && !AName.trimmed().isEmpty())
		FSingleGroups.insert(AType,AName);
}

void RostersModel::insertDefaultDataHolder(IRosterDataHolder *ADataHolder)
{
	if (ADataHolder && !FDataHolders.contains(ADataHolder))
	{
		QMultiMap<int,QVariant> findData;
		foreach(int type, ADataHolder->rosterDataTypes())
			findData.insertMulti(RDR_TYPE, type);

		foreach(IRosterIndex *rindex, FRootIndex->findChilds(findData,true))
			rindex->insertDataHolder(ADataHolder);

		FDataHolders.append(ADataHolder);
		emit defaultDataHolderInserted(ADataHolder);
	}
}

void RostersModel::removeDefaultDataHolder(IRosterDataHolder *ADataHolder)
{
	if (FDataHolders.contains(ADataHolder))
	{
		QMultiMap<int,QVariant> findData;
		foreach(int type, ADataHolder->rosterDataTypes())
			findData.insertMulti(RDR_TYPE, type);

		QList<IRosterIndex *> indexes = FRootIndex->findChilds(findData,true);
		foreach(IRosterIndex *rindex, indexes)
			rindex->removeDataHolder(ADataHolder);

		FDataHolders.removeAll(ADataHolder);
		emit defaultDataHolderRemoved(ADataHolder);
	}
}

void RostersModel::emitDelayedDataChanged(IRosterIndex *AIndex)
{
	removeChangedIndex(AIndex);

	if (AIndex != FRootIndex)
	{
		QModelIndex modelIndex = modelIndexByRosterIndex(AIndex);
		emit dataChanged(modelIndex,modelIndex);
	}

	QList<IRosterIndex *> childs;
	foreach(IRosterIndex *rindex, FChangedIndexes)
		if (rindex->parentIndex() == AIndex)
			childs.append(rindex);

	foreach(IRosterIndex *rindex, childs)
		emitDelayedDataChanged(rindex);
}

void RostersModel::insertDefaultDataHolders(IRosterIndex *AIndex)
{
	foreach(IRosterDataHolder *dataHolder, FDataHolders)
		if (dataHolder->rosterDataTypes().contains(RIT_ANY_TYPE) || dataHolder->rosterDataTypes().contains(AIndex->type()))
			AIndex->insertDataHolder(dataHolder);
}

void RostersModel::insertChangedIndex(IRosterIndex *AIndex)
{
	if (AIndex)
	{
		if (FChangedIndexes.isEmpty())
			QTimer::singleShot(0,this,SLOT(onDelayedDataChanged()));
		FChangedIndexes += AIndex;
	}
}

void RostersModel::removeChangedIndex(IRosterIndex *AIndex)
{
	FChangedIndexes -= AIndex;
}

QString RostersModel::getGroupName(int AType, const QString &AGroup) const
{
	if (FSingleGroups.contains(AType))
		return singleGroupName(AType);
	else if (AGroup.isEmpty())
		return singleGroupName(RIT_GROUP_BLANK);
	return AGroup;
}

QList<IRosterIndex *> RostersModel::findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, bool ABare, IRosterIndex *AParent) const
{
	QList<IRosterIndex *> indexes = FContactsCache.value(FStreamsRoot.value(AStreamJid)).values(AContactJid.bare());

	if (AParent)
	{
		for(QList<IRosterIndex *>::iterator it=indexes.begin(); it!=indexes.end(); )
		{
			if ((*it)->parentIndex() != AParent)
				it = indexes.erase(it);
			else
				++it;
		}
	}

	if (!ABare)
	{
		for(QList<IRosterIndex *>::iterator it=indexes.begin(); it!=indexes.end(); )
		{
			if (AContactJid != (*it)->data(RDR_FULL_JID).toString())
				it = indexes.erase(it);
			else
				++it;
		}
	}

	return indexes;
}

void RostersModel::onAccountShown(IAccount *AAccount)
{
	if (AAccount->isActive())
		addStream(AAccount->xmppStream()->streamJid());
}

void RostersModel::onAccountHidden(IAccount *AAccount)
{
	if (AAccount->isActive())
		removeStream(AAccount->xmppStream()->streamJid());
}

void RostersModel::onAccountOptionsChanged(const OptionsNode &ANode)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account && account->isActive() && account->optionsNode().childPath(ANode)=="name")
	{
		IRosterIndex *streamIndex = FStreamsRoot.value(account->xmppStream()->streamJid());
		if (streamIndex)
			streamIndex->setData(RDR_NAME,account->name());
	}
}

void RostersModel::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamsRoot.value(ARoster->streamJid());
	if (streamIndex)
	{
		QList<IRosterIndex *> curItemList = findContactIndexes(ARoster->streamJid(),AItem.itemJid,true);
		
		int groupType;
		QSet<QString> itemGroups;
		int itemType = !AItem.itemJid.node().isEmpty() ? RIT_CONTACT : RIT_AGENT;
		if (itemType == RIT_AGENT)
		{
			groupType = RIT_GROUP_AGENTS;
			itemGroups += QString::null;
		}
		else if (AItem.groups.isEmpty())
		{
			groupType = RIT_GROUP_BLANK;
			itemGroups += QString::null;
		}
		else
		{
			groupType = RIT_GROUP;
			itemGroups = AItem.groups;
		}

		QList<IRosterIndex *> itemList;
		if (AItem.subscription != SUBSCRIPTION_REMOVE)
		{
			QSet<QString> curGroups;
			foreach(IRosterIndex *itemIndex, curItemList)
				curGroups.insert(itemIndex->data(RDR_GROUP).toString());

			QSet<QString> newGroups = itemGroups - curGroups;
			QSet<QString> oldGroups = curGroups - itemGroups;

			QString groupDelim = ARoster->groupDelimiter();
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(ARoster->streamJid()) : NULL;
			foreach(QString group, itemGroups)
			{
				IRosterIndex *groupIndex = createGroupIndex(groupType,group,groupDelim,streamIndex);

				QList<IRosterIndex *> groupItemList;
				if (newGroups.contains(group) && !oldGroups.isEmpty())
				{
					IRosterIndex *oldGroupIndex;
					QString oldGroup = oldGroups.values().value(0);
					if (oldGroup.isEmpty())
						oldGroupIndex = findGroupIndex(RIT_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						oldGroupIndex = findGroupIndex(RIT_GROUP,oldGroup,groupDelim,streamIndex);
					if (oldGroupIndex)
					{
						groupItemList = findContactIndexes(ARoster->streamJid(),AItem.itemJid,true,oldGroupIndex);
						foreach(IRosterIndex *itemIndex, groupItemList)
						{
							itemIndex->setData(RDR_GROUP,group);
							itemIndex->setParentIndex(groupIndex);
						}
					}
					oldGroups -= group;
				}
				else
				{
					groupItemList = findContactIndexes(ARoster->streamJid(),AItem.itemJid,true,groupIndex);
				}

				if (groupItemList.isEmpty())
				{
					int presIndex = 0;
					QList<IPresenceItem> pitems = presence!=NULL ? presence->presenceItems(AItem.itemJid) : QList<IPresenceItem>();
					do
					{
						IRosterIndex *itemIndex;
						IPresenceItem pitem = pitems.value(presIndex++);
						if (pitem.isValid)
						{
							itemIndex = createRosterIndex(itemType,groupIndex);
							itemIndex->setData(RDR_FULL_JID,pitem.itemJid.full());
							itemIndex->setData(RDR_PREP_FULL_JID,pitem.itemJid.pFull());
							itemIndex->setData(RDR_SHOW,pitem.show);
							itemIndex->setData(RDR_STATUS,pitem.status);
							itemIndex->setData(RDR_PRIORITY,pitem.priority);
						}
						else
						{
							itemIndex = createRosterIndex(itemType,groupIndex);
							itemIndex->setData(RDR_FULL_JID,AItem.itemJid.bare());
							itemIndex->setData(RDR_PREP_FULL_JID,AItem.itemJid.pBare());
						}

						itemIndex->setData(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
						itemIndex->setData(RDR_NAME,AItem.name);
						itemIndex->setData(RDR_SUBSCRIBTION,AItem.subscription);
						itemIndex->setData(RDR_ASK,AItem.ask);
						itemIndex->setData(RDR_GROUP,group);
						insertRosterIndex(itemIndex,groupIndex);
						itemList.append(itemIndex);
					}
					while (presIndex < pitems.count());
				}
				else foreach(IRosterIndex *itemIndex, groupItemList)
				{
					itemIndex->setData(RDR_NAME,AItem.name);
					itemIndex->setData(RDR_SUBSCRIBTION,AItem.subscription);
					itemIndex->setData(RDR_ASK,AItem.ask);
					itemList.append(itemIndex);
				}
			}
		}

		foreach(IRosterIndex *itemIndex, curItemList)
			if (!itemList.contains(itemIndex))
				removeRosterIndex(itemIndex);
	}
}

void RostersModel::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(ABefore);
	if (streamIndex)
	{
		Jid after = ARoster->streamJid();

		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_STREAM_JID,ABefore.pFull());

		QList<IRosterIndex *> itemList = FRootIndex->findChilds(findData,true);
		foreach(IRosterIndex *itemIndex, itemList)
			itemIndex->setData(RDR_STREAM_JID,after.pFull());

		streamIndex->setData(RDR_FULL_JID,after.full());
		streamIndex->setData(RDR_PREP_FULL_JID,after.pFull());

		FStreamsRoot.remove(ABefore);
		FStreamsRoot.insert(after,streamIndex);

		emit streamJidChanged(ABefore,after);
	}
}

void RostersModel::onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority)
{
	IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
	if (streamIndex)
	{
		streamIndex->setData(RDR_SHOW, AShow);
		streamIndex->setData(RDR_STATUS, AStatus);
		if (AShow != IPresence::Offline && AShow != IPresence::Error)
			streamIndex->setData(RDR_PRIORITY, APriority);
		else
			streamIndex->setData(RDR_PRIORITY, QVariant());
	}
}

void RostersModel::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamsRoot.value(APresence->streamJid());
	if (streamIndex)
	{
		int itemType = RIT_CONTACT;
		if (AItem.itemJid.node().isEmpty())
			itemType = RIT_AGENT;
		else if (AItem.itemJid && APresence->streamJid())
			itemType = RIT_MY_RESOURCE;

		if (AItem.show == IPresence::Offline)
		{
			int pitemsCount = APresence->presenceItems(AItem.itemJid).count();
			QList<IRosterIndex *> itemList = findContactIndexes(APresence->streamJid(),AItem.itemJid,false);
			foreach (IRosterIndex *itemIndex, itemList)
			{
				if (itemType == RIT_MY_RESOURCE || pitemsCount > 1)
				{
					removeRosterIndex(itemIndex);
				}
				else
				{
					itemIndex->setData(RDR_FULL_JID,AItem.itemJid.bare());
					itemIndex->setData(RDR_PREP_FULL_JID,AItem.itemJid.pBare());
					itemIndex->setData(RDR_SHOW,AItem.show);
					itemIndex->setData(RDR_STATUS,AItem.status);
					itemIndex->setData(RDR_PRIORITY,QVariant());
				}
			}
		}
		else if (AItem.show == IPresence::Error)
		{
			QList<IRosterIndex *> itemList = findContactIndexes(APresence->streamJid(),AItem.itemJid,true);
			foreach(IRosterIndex *itemIndex, itemList)
			{
				itemIndex->setData(RDR_SHOW,AItem.show);
				itemIndex->setData(RDR_STATUS,AItem.status);
				itemIndex->setData(RDR_PRIORITY,QVariant());
			}
		}
		else
		{
			QList<IRosterIndex *> itemList = findContactIndexes(APresence->streamJid(),AItem.itemJid,false);
			if (itemList.isEmpty())
			{
				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(APresence->streamJid()) : NULL;
				IRosterItem ritem = roster!=NULL ? roster->rosterItem(AItem.itemJid) : IRosterItem();
				QString groupDelim = roster!=NULL ? roster->groupDelimiter() : "::";

				QSet<QString> itemGroups;
				if (ritem.isValid)
				{
					if (!ritem.groups.isEmpty())
						itemGroups = ritem.groups;
					else
						itemGroups += QString::null;
				}
				else if (itemType == RIT_MY_RESOURCE)
				{
					itemGroups += QString::null;
				}

				foreach(QString group, itemGroups)
				{
					IRosterIndex *groupIndex = NULL;
					if (itemType == RIT_MY_RESOURCE)
						groupIndex = createGroupIndex(RIT_GROUP_MY_RESOURCES,QString::null,groupDelim,streamIndex);
					else if (!ritem.isValid)
						groupIndex = createGroupIndex(RIT_GROUP_NOT_IN_ROSTER,QString::null,groupDelim,streamIndex);
					else if (itemType == RIT_AGENT)
						groupIndex = findGroupIndex(RIT_GROUP_AGENTS,QString::null,groupDelim,streamIndex);
					else if (group.isEmpty())
						groupIndex = findGroupIndex(RIT_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						groupIndex = findGroupIndex(RIT_GROUP,group,groupDelim,streamIndex);

					if (groupIndex)
					{
						IRosterIndex *itemIndex = findContactIndexes(APresence->streamJid(),AItem.itemJid.bare(),false).value(0);
						if (!itemIndex)
						{
							itemIndex = createRosterIndex(itemType,groupIndex);
							itemIndex->setData(RDR_PREP_BARE_JID,AItem.itemJid.pBare());
							itemIndex->setData(RDR_GROUP,group);
							if (ritem.isValid)
							{
								itemIndex->setData(RDR_NAME,ritem.name);
								itemIndex->setData(RDR_SUBSCRIBTION,ritem.subscription);
								itemIndex->setData(RDR_ASK,ritem.ask);
							}
						}

						itemIndex->setData(RDR_FULL_JID,AItem.itemJid.full());
						itemIndex->setData(RDR_PREP_FULL_JID,AItem.itemJid.pFull());
						itemIndex->setData(RDR_SHOW,AItem.show);
						itemIndex->setData(RDR_STATUS,AItem.status);
						itemIndex->setData(RDR_PRIORITY,AItem.priority);
						insertRosterIndex(itemIndex,groupIndex);
					}
				}
			}
			else foreach(IRosterIndex *itemIndex, itemList)
			{
				itemIndex->setData(RDR_SHOW,AItem.show);
				itemIndex->setData(RDR_STATUS,AItem.status);
				itemIndex->setData(RDR_PRIORITY,AItem.priority);
			}
		}
	}
}

void RostersModel::onIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	insertChangedIndex(AIndex);
	emit indexDataChanged(AIndex, ARole);
}

void RostersModel::onIndexChildAboutToBeInserted(IRosterIndex *AIndex)
{
	emit indexAboutToBeInserted(AIndex);
	beginInsertRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->parentIndex()->childCount(),AIndex->parentIndex()->childCount());
	connect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
		SLOT(onIndexDataChanged(IRosterIndex *, int)));
	connect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
		SLOT(onIndexChildInserted(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
		SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	connect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
		SLOT(onIndexChildRemoved(IRosterIndex *)));
}

void RostersModel::onIndexChildInserted(IRosterIndex *AIndex)
{
	insertChangedIndex(AIndex);
	if (RosterItemTypes.contains(AIndex->type()))
	{
		IRosterIndex *streamIndex = FStreamsRoot.value(AIndex->data(RDR_STREAM_JID).toString());
		if (streamIndex)
			FContactsCache[streamIndex].insertMulti(AIndex->data(RDR_PREP_BARE_JID).toString(),AIndex);
	}
	else if (AIndex->parentIndex() && RosterGroupTypes.contains(AIndex->type()))
	{
		FGroupsCache[AIndex->parentIndex()].insertMulti(AIndex->data(RDR_NAME).toString(),AIndex);
	}
	endInsertRows();
	emit indexInserted(AIndex);
}

void RostersModel::onIndexChildAboutToBeRemoved(IRosterIndex *AIndex)
{
	insertChangedIndex(AIndex->parentIndex());
	emit indexAboutToBeRemoved(AIndex);
	beginRemoveRows(modelIndexByRosterIndex(AIndex->parentIndex()),AIndex->row(),AIndex->row());
	if (RosterItemTypes.contains(AIndex->type()))
	{
		IRosterIndex *streamIndex = FStreamsRoot.value(AIndex->data(RDR_STREAM_JID).toString());
		if (streamIndex)
			FContactsCache[streamIndex].remove(AIndex->data(RDR_PREP_BARE_JID).toString(),AIndex);
	}
	else if (AIndex->parentIndex() && RosterGroupTypes.contains(AIndex->type()))
	{
		FGroupsCache[AIndex->parentIndex()].remove(AIndex->data(RDR_NAME).toString(),AIndex);
	}
}

void RostersModel::onIndexChildRemoved(IRosterIndex *AIndex)
{
	removeChangedIndex(AIndex);
	disconnect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
		this,SLOT(onIndexDataChanged(IRosterIndex *, int)));
	disconnect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
		this,SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
		this,SLOT(onIndexChildInserted(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
		this,SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
	disconnect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
		this,SLOT(onIndexChildRemoved(IRosterIndex *)));
	endRemoveRows();
	emit indexRemoved(AIndex);
}

void RostersModel::onIndexDestroyed(IRosterIndex *AIndex)
{
	emit indexDestroyed(AIndex);
}

void RostersModel::onDelayedDataChanged()
{
	//Замена сигналов изменения большого числа индексов на сброс модели
	//из-за тормозов в сортировке в QSortFilterProxyModel
	if (FChangedIndexes.count() < INDEX_CHANGES_FOR_RESET)
	{
		//Вызывает dataChanged у всех родителей для поддержки SortFilterProxyModel
		QSet<IRosterIndex *> childIndexes = FChangedIndexes;
		foreach(IRosterIndex *rindex, childIndexes)
		{
			IRosterIndex *pindex = rindex->parentIndex();
			while (pindex && !FChangedIndexes.contains(pindex))
			{
				FChangedIndexes += pindex;
				pindex = pindex->parentIndex();
			}
			QModelIndex modelIndex = modelIndexByRosterIndex(rindex);
			emit dataChanged(modelIndex,modelIndex);
		}
		emitDelayedDataChanged(FRootIndex);
	}
	else
	{
		reset();
	}
	FChangedIndexes.clear();
}

Q_EXPORT_PLUGIN2(plg_rostersmodel, RostersModel)
