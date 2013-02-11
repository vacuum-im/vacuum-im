#include "rostersmodel.h"

#include <QTimer>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterindexkindorders.h>

#define INDEX_CHANGES_FOR_RESET 20

RostersModel::RostersModel()
{
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FAccountManager = NULL;

	FRootIndex = new RootIndex(this);

	setDelayedDataChangedSignals(true);
	setRecursiveParentDataChangedSignals(true);

	connect(this,SIGNAL(itemInserted(QStandardItem *)),SLOT(onAdvancedItemInserted(QStandardItem *)));
	connect(this,SIGNAL(itemRemoving(QStandardItem *)),SLOT(onAdvancedItemRemoving(QStandardItem *)));
	connect(this,SIGNAL(itemDataChanged(QStandardItem *,int)),SLOT(onAdvancedItemDataChanged(QStandardItem *,int)));
}

RostersModel::~RostersModel()
{
	FRootIndex->removeChildren();
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
	registerSingleGroup(RIK_GROUP_BLANK,tr("Without Groups"));
	registerSingleGroup(RIK_GROUP_AGENTS,tr("Agents"));
	registerSingleGroup(RIK_GROUP_MY_RESOURCES,tr("My Resources"));
	registerSingleGroup(RIK_GROUP_NOT_IN_ROSTER,tr("Not in Roster"));
	return true;
}

IRosterIndex *RostersModel::addStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamRoots.value(AStreamJid);
	if (streamIndex == NULL)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;

		if (roster || presence)
		{
			IRosterIndex *streamIndex = newRosterIndex(RIK_STREAM_ROOT,FRootIndex);
			streamIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
			streamIndex->setData(AStreamJid.full(),RDR_FULL_JID);
			streamIndex->setData(AStreamJid.pFull(),RDR_PREP_FULL_JID);
			streamIndex->setData(AStreamJid.pBare(),RDR_PREP_BARE_JID);

			if (presence)
			{
				streamIndex->setData(presence->show(),RDR_SHOW);
				streamIndex->setData(presence->status(),RDR_STATUS);
			}
			if (account)
			{
				streamIndex->setData(account->name(),RDR_NAME);
				connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
			}

			FStreamRoots.insert(AStreamJid,streamIndex);
			FRootIndex->appendChild(streamIndex);
			emit streamAdded(AStreamJid);

			if (roster)
			{
				IRosterItem empty;
				foreach(IRosterItem ritem, roster->rosterItems())
					onRosterItemReceived(roster,ritem,empty);
			}
		}
	}
	return streamIndex;
}

QList<Jid> RostersModel::streams() const
{
	return FStreamRoots.keys();
}

void RostersModel::removeStream(const Jid &AStreamJid)
{
	IRosterIndex *streamIndex = FStreamRoots.take(AStreamJid);
	if (streamIndex)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
		if (account)
			disconnect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),this,SLOT(onAccountOptionsChanged(const OptionsNode &)));

		streamIndex->remove();
		FContactsCache.remove(streamIndex);

		emit streamRemoved(AStreamJid);
	}
}

IRosterIndex *RostersModel::rootIndex() const
{
	return FRootIndex;
}

IRosterIndex *RostersModel::findStreamRoot(const Jid &AStreamJid) const
{
	return FStreamRoots.value(AStreamJid);
}

IRosterIndex *RostersModel::newRosterIndex(int AKind, IRosterIndex *AParent)
{
	static const struct { int kind; int order; }	DefKindOrders[] = {
		{RIK_STREAM_ROOT,         RIKO_STREAM_ROOT},
		{RIK_GROUP,               RIKO_GROUP},
		{RIK_GROUP_BLANK,         RIKO_GROUP_BLANK},
		{RIK_GROUP_NOT_IN_ROSTER, RIKO_GROUP_NOT_IN_ROSTER},
		{RIK_GROUP_MY_RESOURCES,  RIKO_GROUP_MY_RESOURCES},
		{RIK_GROUP_AGENTS,        RIKO_GROUP_AGENTS},
		{-1,                      -1}
	};

	IRosterIndex *rindex = new RosterIndex(AKind,this);

	if (AParent)
		rindex->setData(AParent->data(RDR_STREAM_JID),RDR_STREAM_JID);

	int typeOrder = RIKO_DEFAULT;
	for (int i=0; DefKindOrders[i].kind>=0; i++)
	{
		if (AKind == DefKindOrders[i].kind)
		{
			typeOrder = DefKindOrders[i].order;
			break;
		}
	}
	rindex->setData(typeOrder,RDR_KIND_ORDER);

	emit indexCreated(rindex);

	return rindex;
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex, bool ADestroy)
{
	IRosterIndex *groupIndex = AIndex->parentIndex();
	if (groupIndex)
	{
		if (ADestroy)
			AIndex->remove();
		else
			groupIndex->takeIndex(AIndex->row());
		removeEmptyGroup(groupIndex);
	}
}

IRosterIndex *RostersModel::findGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const
{
	QString groupPath = getGroupName(AKind,AGroup);
	QList<QString> groupTree = groupPath.split(AGroupDelim,QString::SkipEmptyParts);

	IRosterIndex *groupIndex = AParent;
	do 
	{
		QList<IRosterIndex *> indexes = FGroupsCache.value(groupIndex).values(groupTree.takeFirst());

		groupIndex = NULL;
		for(QList<IRosterIndex *>::const_iterator it = indexes.constBegin(); !groupIndex && it!=indexes.constEnd(); ++it)
			if ((*it)->kind() == AKind)
				groupIndex = *it;

	} while (groupIndex && !groupTree.isEmpty());

	return groupIndex;
}

IRosterIndex *RostersModel::getGroupIndex(int AKind, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent)
{
	IRosterIndex *groupIndex = findGroupIndex(AKind,AGroup,AGroupDelim,AParent);
	if (!groupIndex)
	{
		QString groupPath = getGroupName(AKind,AGroup);
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

			childGroupIndex = findGroupIndex(AKind, groupTree.at(i), AGroupDelim, groupIndex);
			if (childGroupIndex)
			{
				groupIndex = childGroupIndex;
				i++;
			}
		}

		while (i < groupTree.count())
		{
			childGroupIndex = newRosterIndex(AKind, groupIndex);
			childGroupIndex->setData(!FSingleGroups.contains(AKind) ? group : QString::null,RDR_GROUP);
			childGroupIndex->setData(groupTree.at(i),RDR_NAME);
			groupIndex->appendChild(childGroupIndex);
			groupIndex = childGroupIndex;
			group += AGroupDelim + groupTree.value(++i);
		}
	}
	return groupIndex;
}

QList<IRosterIndex *> RostersModel::getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate)
{
	QList<IRosterIndex *> indexes;
	IRosterIndex *streamIndex = FStreamRoots.value(AStreamJid);
	if (streamIndex)
	{
		indexes = findContactIndexes(AStreamJid,AContactJid,AContactJid.resource().isEmpty());
		if (indexes.isEmpty() && !AContactJid.resource().isEmpty())
			indexes = findContactIndexes(AStreamJid,AContactJid,true);

		if (ACreate && indexes.isEmpty())
		{
			int type = RIK_CONTACT;
			if (AContactJid.node().isEmpty())
				type = RIK_AGENT;
			else if (AContactJid && AStreamJid)
				type = RIK_MY_RESOURCE;

			IRosterIndex *groupIndex;
			if (type == RIK_MY_RESOURCE)
				groupIndex = getGroupIndex(RIK_GROUP_MY_RESOURCES,QString::null,"::",streamIndex);
			else
				groupIndex = getGroupIndex(RIK_GROUP_NOT_IN_ROSTER,QString::null,"::",streamIndex);

			IRosterIndex *itemIndex = newRosterIndex(type,groupIndex);
			itemIndex->setData(AContactJid.full(),RDR_FULL_JID);
			itemIndex->setData(AContactJid.pFull(),RDR_PREP_FULL_JID);
			itemIndex->setData(AContactJid.pBare(),RDR_PREP_BARE_JID);
			itemIndex->setData(groupIndex->data(RDR_GROUP),RDR_GROUP);
			itemIndex->setData(IPresence::Offline,RDR_SHOW);
			groupIndex->appendChild(itemIndex);
			indexes.append(itemIndex);
		}
	}
	return indexes;
}

QModelIndex RostersModel::modelIndexFromRosterIndex(IRosterIndex *AIndex) const
{
	return AIndex!=NULL && AIndex!=FRootIndex ? indexFromItem(AIndex->instance()) : QModelIndex();
}

IRosterIndex *RostersModel::rosterIndexFromModelIndex(const QModelIndex &AIndex) const
{
	if (AIndex.isValid())
		return static_cast<RosterIndex *>(itemFromIndex(AIndex));
	return FRootIndex;
}

bool RostersModel::isGroupKind(int AKind) const
{
	return AKind==RIK_GROUP || FSingleGroups.contains(AKind);
}

QList<int> RostersModel::singleGroupKinds() const
{
	return FSingleGroups.keys();
}

QString RostersModel::singleGroupName(int AKind) const
{
	return FSingleGroups.value(AKind);
}

void RostersModel::registerSingleGroup(int AKind, const QString &AName)
{
	if (!FSingleGroups.contains(AKind) && !AName.trimmed().isEmpty())
		FSingleGroups.insert(AKind,AName);
}

QMultiMap<int, IRosterDataHolder *> RostersModel::rosterDataHolders() const
{
	return FRosterDataHolders;
}

void RostersModel::insertRosterDataHolder(int AOrder, IRosterDataHolder *AHolder)
{
	if (AHolder && !FRosterDataHolders.contains(AOrder,AHolder))
	{
		FRosterDataHolders.insertMulti(AOrder,AHolder);
		DataHolder *proxyHolder = FAdvancedDataHolders.value(AHolder);
		if (proxyHolder == NULL)
		{
			proxyHolder = new DataHolder(AHolder,this);
			FAdvancedDataHolders.insert(AHolder,proxyHolder);
		}
		AdvancedItemModel::insertItemDataHolder(AOrder,proxyHolder);
	}
}

void RostersModel::removeRosterDataHolder(int AOrder, IRosterDataHolder *AHolder)
{
	if (FRosterDataHolders.contains(AOrder,AHolder))
	{
		FRosterDataHolders.remove(AOrder,AHolder);
		DataHolder *proxyHolder = FRosterDataHolders.values().contains(AHolder) ? FAdvancedDataHolders.value(AHolder) : FAdvancedDataHolders.take(AHolder);
		AdvancedItemModel::removeItemDataHolder(AOrder,proxyHolder);
	}
}

void RostersModel::emitIndexDestroyed(IRosterIndex *AIndex)
{
	emit indexDestroyed(AIndex);
}

void RostersModel::removeEmptyGroup(IRosterIndex *AGroupIndex)
{
	if (AGroupIndex && AGroupIndex->childCount()==0 && isGroupKind(AGroupIndex->kind()))
	{
		IRosterIndex *parentGroup = AGroupIndex->parentIndex();
		AGroupIndex->remove();
		removeEmptyGroup(parentGroup);
	}
}

QString RostersModel::getGroupName(int AKind, const QString &AGroup) const
{
	if (FSingleGroups.contains(AKind))
		return singleGroupName(AKind);
	else if (AGroup.isEmpty())
		return singleGroupName(RIK_GROUP_BLANK);
	return AGroup;
}

bool RostersModel::isChildIndex(IRosterIndex *AIndex, IRosterIndex *AParent) const
{
	IRosterIndex *pindex = AIndex->parentIndex();
	while (pindex!=NULL && pindex!=AParent)
		pindex = pindex->parentIndex();
	return pindex==AParent;
}

QList<IRosterIndex *> RostersModel::findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, bool ABare, IRosterIndex *AParent) const
{
	QList<IRosterIndex *> indexes = FContactsCache.value(FStreamRoots.value(AStreamJid)).values(AContactJid.bare());

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

void RostersModel::onAdvancedItemInserted(QStandardItem *AItem)
{
	if (AItem->type() == IRosterIndex::StandardItemTypeValue)
	{
		IRosterIndex *rindex = static_cast<RosterIndex *>(AItem);
		if (isGroupKind(rindex->kind()))
		{
			if (rindex->parentIndex())
				FGroupsCache[rindex->parentIndex()].insertMulti(rindex->data(RDR_NAME).toString(),rindex);
		}
		else
		{
			QString bareJid = rindex->data(RDR_PREP_BARE_JID).toString();
			IRosterIndex *streamIndex = !bareJid.isEmpty() ? FStreamRoots.value(rindex->data(RDR_STREAM_JID).toString()) : NULL;
			if (streamIndex && isChildIndex(rindex,streamIndex))
				FContactsCache[streamIndex].insertMulti(bareJid,rindex);
		}
		emit indexInserted(rindex);
	}
}

void RostersModel::onAdvancedItemRemoving(QStandardItem *AItem)
{
	if (AItem->type() == IRosterIndex::StandardItemTypeValue)
	{
		IRosterIndex *rindex = static_cast<RosterIndex *>(AItem);
		if (isGroupKind(rindex->kind()))
		{
			IRosterIndex *pindex = rindex->parentIndex();
			if (pindex)
				FGroupsCache[pindex].remove(rindex->data(RDR_NAME).toString(),rindex);
		}
		else
		{
			QString bareJid = rindex->data(RDR_PREP_BARE_JID).toString();
			IRosterIndex *streamIndex = !bareJid.isEmpty() ? FStreamRoots.value(rindex->data(RDR_STREAM_JID).toString()) : NULL;
			if (streamIndex && isChildIndex(rindex,streamIndex))
				FContactsCache[streamIndex].remove(bareJid,rindex);
		}
		emit indexRemoving(rindex);
	}
}

void RostersModel::onAdvancedItemDataChanged(QStandardItem *AItem, int ARole)
{
	if (AItem->type() == IRosterIndex::StandardItemTypeValue)
		emit indexDataChanged(static_cast<RosterIndex *>(AItem),ARole);
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
		IRosterIndex *streamIndex = FStreamRoots.value(account->xmppStream()->streamJid());
		if (streamIndex)
			streamIndex->setData(account->name(),RDR_NAME);
	}
}

void RostersModel::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamRoots.value(ARoster->streamJid());
	if (streamIndex)
	{
		QList<IRosterIndex *> curItemList = findContactIndexes(ARoster->streamJid(),AItem.itemJid,true);
		
		int groupKind;
		QSet<QString> itemGroups;
		int itemKind = !AItem.itemJid.node().isEmpty() ? RIK_CONTACT : RIK_AGENT;
		if (itemKind == RIK_AGENT)
		{
			groupKind = RIK_GROUP_AGENTS;
			itemGroups += QString::null;
		}
		else if (AItem.groups.isEmpty())
		{
			groupKind = RIK_GROUP_BLANK;
			itemGroups += QString::null;
		}
		else
		{
			groupKind = RIK_GROUP;
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
				IRosterIndex *groupIndex = getGroupIndex(groupKind,group,groupDelim,streamIndex);

				QList<IRosterIndex *> groupItemList;
				if (newGroups.contains(group) && !oldGroups.isEmpty())
				{
					IRosterIndex *oldGroupIndex;
					QString oldGroup = oldGroups.values().value(0);
					if (oldGroup.isEmpty())
						oldGroupIndex = findGroupIndex(RIK_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						oldGroupIndex = findGroupIndex(RIK_GROUP,oldGroup,groupDelim,streamIndex);
					if (oldGroupIndex)
					{
						groupItemList = findContactIndexes(ARoster->streamJid(),AItem.itemJid,true,oldGroupIndex);
						foreach(IRosterIndex *itemIndex, groupItemList)
						{
							oldGroupIndex->takeIndex(itemIndex->row());
							itemIndex->setData(group,RDR_GROUP);
							groupIndex->appendChild(itemIndex);
							removeEmptyGroup(oldGroupIndex);
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
							itemIndex = newRosterIndex(itemKind,groupIndex);
							itemIndex->setData(pitem.itemJid.full(),RDR_FULL_JID);
							itemIndex->setData(pitem.itemJid.pFull(),RDR_PREP_FULL_JID);
							itemIndex->setData(pitem.priority,RDR_PRIORITY);
						}
						else
						{
							itemIndex = newRosterIndex(itemKind,groupIndex);
							itemIndex->setData(AItem.itemJid.bare(),RDR_FULL_JID);
							itemIndex->setData(AItem.itemJid.pBare(),RDR_PREP_FULL_JID);
						}

						itemIndex->setData(AItem.itemJid.pBare(),RDR_PREP_BARE_JID);
						itemIndex->setData(AItem.name,RDR_NAME);
						itemIndex->setData(AItem.subscription,RDR_SUBSCRIBTION);
						itemIndex->setData(AItem.ask,RDR_ASK);
						itemIndex->setData(group,RDR_GROUP);
						itemIndex->setData(pitem.show,RDR_SHOW);
						itemIndex->setData(pitem.status,RDR_STATUS);
						groupIndex->appendChild(itemIndex);
						itemList.append(itemIndex);
					}
					while (presIndex < pitems.count());
				}
				else foreach(IRosterIndex *itemIndex, groupItemList)
				{
					itemIndex->setData(AItem.name,RDR_NAME);
					itemIndex->setData(AItem.subscription,RDR_SUBSCRIBTION);
					itemIndex->setData(AItem.ask,RDR_ASK);
					itemList.append(itemIndex);
				}
			}
		}

		foreach(IRosterIndex *itemIndex, curItemList)
		{
			if (!itemList.contains(itemIndex))
				removeRosterIndex(itemIndex);
		}
	}
}

void RostersModel::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	IRosterIndex *streamIndex = FStreamRoots.value(ABefore);
	if (streamIndex)
	{
		Jid after = ARoster->streamJid();

		QMultiMap<int,QVariant> findData;
		findData.insert(RDR_STREAM_JID,ABefore.pFull());

		QList<IRosterIndex *> itemList = FRootIndex->findChilds(findData,true);
		foreach(IRosterIndex *itemIndex, itemList)
			itemIndex->setData(after.pFull(),RDR_STREAM_JID);

		streamIndex->setData(after.full(),RDR_FULL_JID);
		streamIndex->setData(after.pFull(),RDR_PREP_FULL_JID);

		FStreamRoots.remove(ABefore);
		FStreamRoots.insert(after,streamIndex);

		emit streamJidChanged(ABefore,after);
	}
}

void RostersModel::onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority)
{
	IRosterIndex *streamIndex = FStreamRoots.value(APresence->streamJid());
	if (streamIndex)
	{
		streamIndex->setData(AShow,RDR_SHOW);
		streamIndex->setData(AStatus,RDR_STATUS);
		if (AShow!=IPresence::Offline && AShow!=IPresence::Error)
			streamIndex->setData(APriority,RDR_PRIORITY);
		else
			streamIndex->setData(QVariant(),RDR_PRIORITY);
	}
}

void RostersModel::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Q_UNUSED(ABefore);
	IRosterIndex *streamIndex = FStreamRoots.value(APresence->streamJid());
	if (streamIndex)
	{
		int itemKind = RIK_CONTACT;
		if (AItem.itemJid.node().isEmpty())
			itemKind = RIK_AGENT;
		else if (AItem.itemJid && APresence->streamJid())
			itemKind = RIK_MY_RESOURCE;

		if (AItem.show == IPresence::Offline)
		{
			int pitemsCount = APresence->presenceItems(AItem.itemJid).count();
			QList<IRosterIndex *> itemList = findContactIndexes(APresence->streamJid(),AItem.itemJid,false);
			foreach (IRosterIndex *itemIndex, itemList)
			{
				if (itemKind==RIK_MY_RESOURCE || pitemsCount>1)
				{
					removeRosterIndex(itemIndex);
				}
				else
				{
					itemIndex->setData(AItem.itemJid.bare(),RDR_FULL_JID);
					itemIndex->setData(AItem.itemJid.pBare(),RDR_PREP_FULL_JID);
					itemIndex->setData(AItem.show,RDR_SHOW);
					itemIndex->setData(AItem.status,RDR_STATUS);
					itemIndex->setData(QVariant(),RDR_PRIORITY);
				}
			}
		}
		else if (AItem.show == IPresence::Error)
		{
			QList<IRosterIndex *> itemList = findContactIndexes(APresence->streamJid(),AItem.itemJid,true);
			foreach(IRosterIndex *itemIndex, itemList)
			{
				itemIndex->setData(AItem.show,RDR_SHOW);
				itemIndex->setData(AItem.status,RDR_STATUS);
				itemIndex->setData(QVariant(),RDR_PRIORITY);
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
				else if (itemKind == RIK_MY_RESOURCE)
				{
					itemGroups += QString::null;
				}

				foreach(QString group, itemGroups)
				{
					IRosterIndex *groupIndex = NULL;
					if (itemKind == RIK_MY_RESOURCE)
						groupIndex = getGroupIndex(RIK_GROUP_MY_RESOURCES,QString::null,groupDelim,streamIndex);
					else if (!ritem.isValid)
						groupIndex = getGroupIndex(RIK_GROUP_NOT_IN_ROSTER,QString::null,groupDelim,streamIndex);
					else if (itemKind == RIK_AGENT)
						groupIndex = findGroupIndex(RIK_GROUP_AGENTS,QString::null,groupDelim,streamIndex);
					else if (group.isEmpty())
						groupIndex = findGroupIndex(RIK_GROUP_BLANK,QString::null,groupDelim,streamIndex);
					else
						groupIndex = findGroupIndex(RIK_GROUP,group,groupDelim,streamIndex);

					if (groupIndex)
					{
						IRosterIndex *itemIndex = findContactIndexes(APresence->streamJid(),AItem.itemJid.bare(),false).value(0);
						if (!itemIndex)
						{
							itemIndex = newRosterIndex(itemKind,groupIndex);
							itemIndex->setData(AItem.itemJid.pBare(),RDR_PREP_BARE_JID);
							itemIndex->setData(group,RDR_GROUP);
							if (ritem.isValid)
							{
								itemIndex->setData(ritem.name,RDR_NAME);
								itemIndex->setData(ritem.subscription,RDR_SUBSCRIBTION);
								itemIndex->setData(ritem.ask,RDR_ASK);
							}
						}

						itemIndex->setData(AItem.itemJid.full(),RDR_FULL_JID);
						itemIndex->setData(AItem.itemJid.pFull(),RDR_PREP_FULL_JID);
						itemIndex->setData(AItem.show,RDR_SHOW);
						itemIndex->setData(AItem.status,RDR_STATUS);
						itemIndex->setData(AItem.priority,RDR_PRIORITY);
						groupIndex->appendChild(itemIndex);
					}
				}
			}
			else foreach(IRosterIndex *itemIndex, itemList)
			{
				itemIndex->setData(AItem.show,RDR_SHOW);
				itemIndex->setData(AItem.status,RDR_STATUS);
				itemIndex->setData(AItem.priority,RDR_PRIORITY);
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_rostersmodel, RostersModel)
