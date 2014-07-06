#include "metacontacts.h"

#include <QDir>
#include <QMouseEvent>
#include <QTextDocument>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
//#include <definitions/optionvalues.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rosterindexkindorders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rostertooltiporders.h>
#include <utils/widgetmanager.h>
//#include <utils/options.h>
#include <utils/logger.h>

#define STORAGE_SAVE_TIMEOUT    100

#define DIR_METACONTACTS        "metacontacts"

#define ADR_STREAM_JID          Action::DR_StreamJid
#define ADR_CONTACT_JID         Action::DR_Parametr1
#define ADR_METACONTACT_ID      Action::DR_Parametr2

static const IMetaContact NullMetaContact = IMetaContact();

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FPrivateStorage = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;

	FSortFilterProxyModel = new MetaSortFilterProxyModel(this,this);
	FSortFilterProxyModel->setDynamicSortFilter(true);

	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveContactsToStorageTimerTimeout()));
}

MetaContacts::~MetaContacts()
{
	delete FSortFilterProxyModel;
}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Meta-Contacts");
	APluginInfo->description = tr("Allows to combine single contacts to meta-contacts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTER_UUID);
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool MetaContacts::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageNotifyAboutToClose(const Jid &)),SLOT(onPrivateStorageNotifyAboutToClose(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRostersModelIndexDataChanged(IRosterIndex*, int)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
			connect(FRostersView->instance(), SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
			connect(FRostersView->instance(), SIGNAL(notifyInserted(int)),SLOT(onRostersViewNotifyInserted(int)));
			connect(FRostersView->instance(), SIGNAL(notifyRemoved(int)),SLOT(onRostersViewNotifyRemoved(int)));
			connect(FRostersView->instance(), SIGNAL(notifyActivated(int)),SLOT(onRostersViewNotifyActivated(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	return FRosterPlugin!=NULL && FPrivateStorage!=NULL;
}

bool MetaContacts::initObjects()
{
	if (FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_METACONTACTS,this);
	}
	if (FRostersView)
	{
		FRostersView->insertLabelHolder(RLHO_METACONTACTS,this);
		FRostersView->insertClickHooker(RCHO_METACONTACTS,this);
		FRostersView->insertProxyModel(FSortFilterProxyModel,RPO_METACONTACTS_FILTER);
		FRostersViewPlugin->registerExpandableRosterIndexKind(RIK_METACONTACT,RDR_METACONTACT_ID);
	}
	return true;
}

bool MetaContacts::initSettings()
{
	return true;
}

bool MetaContacts::startPlugin()
{
	return true;
}

QList<int> MetaContacts::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_METACONTACTS)
	{
		static const QList<int> roles =	QList<int>() 
			<< Qt::DisplayRole << Qt::DecorationRole
			<< RDR_AVATAR_HASH << RDR_AVATAR_IMAGE;
		return roles;
	}
	return QList<int>();
}

QVariant MetaContacts::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder == RDHO_METACONTACTS)
	{
		switch (AIndex->kind())
		{
		case RIK_METACONTACT:
			{
				IRosterIndex *proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
				switch (ARole)
				{
				case Qt::DecorationRole:
				case RDR_AVATAR_HASH:
				case RDR_AVATAR_IMAGE:
					return proxy!=NULL ? proxy->data(ARole) : QVariant();
				}
				break;
			}
		case RIK_METACONTACT_ITEM: 
			{
				IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
				switch (ARole)
				{
				case Qt::DisplayRole:
				case Qt::DecorationRole:
				case RDR_AVATAR_HASH:
				case RDR_AVATAR_IMAGE:
					return proxy!=NULL ? proxy->data(ARole) : QVariant();
				}
				break;
			}
		}
	}
	return QVariant();
}

bool MetaContacts::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AValue); Q_UNUSED(AIndex); Q_UNUSED(ARole);
	return false;
}

QList<quint32> MetaContacts::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder == RLHO_METACONTACTS && AIndex->kind() == RIK_METACONTACT)
	{
		labels.append(RLID_METACONTACTS_BRANCH);
		labels.append(RLID_METACONTACTS_BRANCH_ITEMS);
		labels.append(AdvancedDelegateItem::BranchId);
	}
	return labels;
}

AdvancedDelegateItem MetaContacts::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AIndex);
	if (AOrder==RLHO_METACONTACTS && AIndex->kind()==RIK_METACONTACT)
	{
		if (ALabelId == RLID_METACONTACTS_BRANCH)
		{
			AdvancedDelegateItem label(ALabelId);
			label.d->kind = AdvancedDelegateItem::Branch;
			return label;
		}
		else if (ALabelId == RLID_METACONTACTS_BRANCH_ITEMS)
		{
			AdvancedDelegateItem label(ALabelId);
			label.d->kind = AdvancedDelegateItem::CustomData;
			label.d->data = QString("%1").arg(FMetaIndexItems.value(AIndex).count());
			label.d->hints.insert(AdvancedDelegateItem::FontSizeDelta,-1);
			label.d->hints.insert(AdvancedDelegateItem::Foreground,FRostersView->instance()->palette().color(QPalette::Disabled, QPalette::Text));
			return label;
		}
		else if (ALabelId == AdvancedDelegateItem::BranchId)
		{
			AdvancedDelegateItem label(ALabelId);
			label.d->kind = AdvancedDelegateItem::Branch;
			label.d->flags |= AdvancedDelegateItem::Hidden;
			return label;
		}
	}
	return AdvancedDelegateItem();
}

bool MetaContacts::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (AOrder == RCHO_METACONTACTS)
	{
		if (AIndex->kind() == RIK_METACONTACT)
		{
			QModelIndex modelIndex = FRostersView->instance()->indexAt(AEvent->pos());
			quint32 labelId = FRostersView->labelAt(AEvent->pos(),modelIndex);
			if (labelId==RLID_METACONTACTS_BRANCH || labelId==RLID_METACONTACTS_BRANCH_ITEMS)
			{
				FRostersView->instance()->setExpanded(modelIndex,!FRostersView->instance()->isExpanded(modelIndex));
				return true;
			}
			else
			{
				IRosterIndex *proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
				if (proxy)
					FRostersView->singleClickOnIndex(proxy,AEvent);
			}
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
			if (proxy)
				FRostersView->singleClickOnIndex(proxy,AEvent);
		}
	}
	return false;
}

bool MetaContacts::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (AOrder == RCHO_METACONTACTS)
	{
		if (AIndex->kind() == RIK_METACONTACT)
		{
			IRosterIndex *proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
			if (proxy)
				FRostersView->doubleClickOnIndex(proxy,AEvent);
			return true;
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
			if (proxy)
				FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
	}
	return false;
}

bool MetaContacts::isReady(const Jid &AStreamJid) const
{
	return FPrivateStorage==NULL || FPrivateStorage->isLoaded(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const Jid &AItem) const
{
	QUuid metaId = FItemMetaContact.value(AStreamJid).value(AItem.bare());
	return !metaId.isNull() ? FMetaContacts.value(AStreamJid).value(metaId) : NullMetaContact;
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaContacts.value(AStreamJid).value(AMetaId,NullMetaContact);
}

QList<IRosterIndex *> MetaContacts::findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaIndexes.value(AStreamJid).value(AMetaId);
}

QUuid MetaContacts::createMetaContact(const Jid &AStreamJid, const QList<Jid> &AItems, const QString &AName)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta;
		meta.id = QUuid::createUuid();
		meta.name = AName;
		meta.items = AItems;
		if (updateMetaContact(AStreamJid,meta))
		{
			startSaveContactsToStorage(AStreamJid);
			return meta.id;
		}
	}
	return QUuid();
}

bool MetaContacts::mergeMetaContacts(const Jid &AStreamJid, const QUuid &AMetaId1, const QUuid &AMetaId2)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta1 = findMetaContact(AStreamJid,AMetaId1);
		IMetaContact meta2 = findMetaContact(AStreamJid,AMetaId2);
		if (meta1.id==AMetaId1 && meta2.id==AMetaId2)
		{
			meta1.items += meta2.items;
			if (updateMetaContact(AStreamJid,meta1))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::detachMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			foreach(const Jid &item, AItems)
				meta.items.removeAll(item);
			if (updateMetaContact(AStreamJid,meta))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			meta.name = AName;
			if (updateMetaContact(AStreamJid,meta))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::setMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			meta.items = AItems;
			if (updateMetaContact(AStreamJid,meta))
			{
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id==AMetaId && meta.groups!=AGroups)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
			if (roster && roster->isOpen())
			{
				QSet<QString> newGroups = AGroups - meta.groups;
				QSet<QString> oldGroups = meta.groups - AGroups;
				foreach(const Jid &item, meta.items)
				{
					IRosterItem ritem = roster->rosterItem(item);
					roster->setItem(ritem.itemJid,ritem.name,ritem.groups + newGroups - oldGroups);
				}
				return true;
			}
		}
	}
	return false;
}

bool MetaContacts::isValidItem(const Jid &AStreamJid, const Jid &AItem) const
{
	if (AItem.isValid() && !AItem.node().isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster != NULL)
		{
			IRosterItem ritem = roster->rosterItem(AItem);
			return ritem.isValid;
		}
	}
	return false;
}

void MetaContacts::updateMetaIndex(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot)
	{
		QList<IRosterIndex *> &metaIndexList = FMetaIndexes[AStreamJid][AMetaContact.id];

		QMap<QString, IRosterIndex *> indexGroup;
		foreach(IRosterIndex *index, metaIndexList)
		{
			QString group = index->data(RDR_GROUP).toString();
			indexGroup.insert(group,index);
		}

		QSet<QString> metaGroups = !AMetaContact.groups.isEmpty() ? AMetaContact.groups : QSet<QString>()<<QString::null;
		QSet<QString> curGroups = indexGroup.keys().toSet();
		QSet<QString> newGroups = metaGroups - curGroups;
		QSet<QString> oldGroups = curGroups - metaGroups;

		QSet<QString>::iterator oldGroupsIt = oldGroups.begin();
		foreach(const QString &group, newGroups)
		{
			IRosterIndex *groupIndex = FRostersModel->getGroupIndex(!group.isEmpty() ? RIK_GROUP : RIK_GROUP_BLANK,group,sroot);

			IRosterIndex *index;
			if (oldGroupsIt == oldGroups.end())
			{
				index = FRostersModel->newRosterIndex(RIK_METACONTACT);
				index->setData(AStreamJid.pFull(),RDR_STREAM_JID);
				index->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);
				metaIndexList.append(index);
			}
			else
			{
				QString oldGroup = *oldGroupsIt;
				oldGroupsIt = oldGroups.erase(oldGroupsIt);
				index = indexGroup.value(oldGroup);
			}

			index->setData(group,RDR_GROUP);
			FRostersModel->insertRosterIndex(index,groupIndex);
		}

		while(oldGroupsIt != oldGroups.end())
		{
			IRosterIndex *index = indexGroup.value(*oldGroupsIt);

			updateMetaIndexItems(index,QList<Jid>());
			FMetaIndexItems.remove(index);

			metaIndexList.removeAll(index);

			FRostersModel->removeRosterIndex(index);
			oldGroupsIt = oldGroups.erase(oldGroupsIt);
		}

		QStringList resources;
		foreach(const IPresenceItem &pitem, AMetaContact.presences)
		{
			if (pitem.show != IPresence::Offline)
				resources.append(pitem.itemJid.pFull());
		}

		IPresenceItem metaPresence = AMetaContact.presences.value(0);
		Jid metaJid = metaPresence.itemJid.isValid() ? metaPresence.itemJid : AMetaContact.items.value(0);

		foreach(IRosterIndex *index, metaIndexList)
		{
			index->setData(metaJid.full(),RDR_FULL_JID);
			index->setData(metaJid.pFull(),RDR_PREP_FULL_JID);
			index->setData(metaJid.pBare(),RDR_PREP_BARE_JID);
			index->setData(resources,RDR_RESOURCES);

			index->setData(AMetaContact.name,RDR_NAME);
			index->setData(metaPresence.show,RDR_SHOW);
			index->setData(metaPresence.status,RDR_STATUS);
			index->setData(metaPresence.priority,RDR_PRIORITY);

			updateMetaIndexItems(index,AMetaContact.items);

			emit rosterLabelChanged(RLID_METACONTACTS_BRANCH_ITEMS,index);
		}
	}
}

void MetaContacts::removeMetaIndex(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (FRostersModel)
	{
		foreach(IRosterIndex *index, FMetaIndexes[AStreamJid].take(AMetaId))
		{
			updateMetaIndexItems(index,QList<Jid>());
			FMetaIndexItems.remove(index);

			FRostersModel->removeRosterIndex(index);
		}
	}
}

void MetaContacts::updateMetaIndexItems(IRosterIndex *AMetaIndex, const QList<Jid> &AItems)
{
	QMap<Jid, IRosterIndex *> &metaItemsMap = FMetaIndexItems[AMetaIndex];

	QMap<Jid, IRosterIndex *> itemIndex = metaItemsMap;
	foreach(const Jid &itemJid, AItems)
	{
		QMap<IRosterIndex *, IRosterIndex *> proxyMap;
		foreach(IRosterIndex *index, FRostersModel->findContactIndexes(AMetaIndex->data(RDR_STREAM_JID).toString(),itemJid))
			proxyMap.insert(index->parentIndex(),index);

		IRosterIndex *proxy = !proxyMap.isEmpty() ? proxyMap.value(AMetaIndex,proxyMap.constBegin().value()) : NULL;
		if (proxy != NULL)
		{
			IRosterIndex *item = itemIndex.take(itemJid);
			if (item == NULL)
			{
				item = FRostersModel->newRosterIndex(RIK_METACONTACT_ITEM);
				item->setData(AMetaIndex->data(RDR_STREAM_JID),RDR_STREAM_JID);
				item->setData(AMetaIndex->data(RDR_METACONTACT_ID),RDR_METACONTACT_ID);
				metaItemsMap.insert(itemJid,item);
			}

			static const QList<int> proxyRoles = QList<int>()
				<< RDR_FULL_JID << RDR_PREP_FULL_JID << RDR_PREP_BARE_JID	<< RDR_RESOURCES
				<< RDR_NAME << RDR_GROUP << RDR_SUBSCRIBTION << RDR_ASK
				<< RDR_SHOW << RDR_STATUS;

			foreach(int role, proxyRoles)
				item->setData(proxy->data(role),role);

			IRosterIndex *prevProxy = FMetaIndexItemProxy.value(item);
			if (proxy != prevProxy)
			{
				FMetaIndexItemProxy.insert(item,proxy);

				if (prevProxy)
					prevProxy->setData(QVariant(),RDR_METACONTACT_ID);
				proxy->setData(item->data(RDR_METACONTACT_ID),RDR_METACONTACT_ID);
			}

			FRostersModel->insertRosterIndex(item,AMetaIndex);
		}
	}

	for(QMap<Jid, IRosterIndex *>::const_iterator it=itemIndex.constBegin(); it!=itemIndex.constEnd(); ++it)
	{
		IRosterIndex *proxy = FMetaIndexItemProxy.take(it.value());
		proxy->setData(QVariant(),RDR_METACONTACT_ID);

		metaItemsMap.remove(it.key());
		FRostersModel->removeRosterIndex(it.value());
	}
}

bool MetaContacts::updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster!=NULL && !AMetaContact.id.isNull())
	{
		IMetaContact meta = AMetaContact;
		IMetaContact before = findMetaContact(AStreamJid,meta.id);
		QHash<Jid, QUuid> &itemMetaHash = FItemMetaContact[AStreamJid];
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;

		meta.groups.clear();
		meta.presences.clear();
		foreach(const Jid &item, meta.items)
		{
			IRosterItem ritem = roster->rosterItem(item);
			if (ritem.isValid && !item.node().isEmpty())
			{
				if (!before.items.contains(item))
				{
					IMetaContact oldMeta = findMetaContact(AStreamJid,item);
					if (!oldMeta.id.isNull())
						detachMetaContactItems(AStreamJid,oldMeta.id,meta.items);
					itemMetaHash.insert(item,meta.id);
				}

				if (meta.name.isEmpty())
					meta.name = !ritem.name.isEmpty() ? ritem.name : ritem.itemJid.uBare();
				meta.groups += ritem.groups;

				if (presence)
					meta.presences += presence->findItems(item);
			}
			else
			{
				meta.items.removeAll(item);
				itemMetaHash.remove(item);
			}
		}

		foreach(const Jid &item, before.items.toSet()-meta.items.toSet())
			itemMetaHash.remove(item);
		meta.presences = !meta.presences.isEmpty() ? FPresencePlugin->sortPresenceItems(meta.presences) : meta.presences;

		if (meta != before)
		{
			if (!meta.isEmpty())
			{
				FMetaContacts[AStreamJid][meta.id] = meta;
				updateMetaIndex(AStreamJid,meta);
			}
			else
			{
				FMetaContacts[AStreamJid].remove(meta.id);
				removeMetaIndex(AStreamJid,meta.id);
			}

			emit metaContactChanged(AStreamJid,meta,before);
			return true;
		}
	}
	return false;
}

void MetaContacts::updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts)
{
	QSet<QUuid> oldMetaId = FMetaContacts.value(AStreamJid).keys().toSet();

	foreach(const IMetaContact &meta, AMetaContacts)
	{
		updateMetaContact(AStreamJid,meta);
		oldMetaId -= meta.id;
	}

	foreach(const QUuid &metaId, oldMetaId)
	{
		IMetaContact meta = findMetaContact(AStreamJid,metaId);
		detachMetaContactItems(AStreamJid,metaId,meta.items);
	}
}

bool MetaContacts::isReadyStreams(const QStringList &AStreams) const
{
	foreach(const Jid &streamJid, AStreams)
		if (!isReady(streamJid))
			return false;
	return !AStreams.isEmpty();
}

bool MetaContacts::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	foreach(IRosterIndex *index, ASelected)
	{
		int kind = index->data(RDR_KIND).toInt();
		if (kind!=RIK_CONTACT && kind!=RIK_METACONTACT && kind!=RIK_METACONTACT_ITEM)
			return false;
		else if (kind==RIK_CONTACT && !isValidItem(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString()))
			return false;
	}
	return !ASelected.isEmpty();
}

QList<IRosterIndex *> MetaContacts::indexesProxies(const QList<IRosterIndex *> &AIndexes, bool AExclusive) const
{
	QList<IRosterIndex *> proxies;
	foreach(IRosterIndex *index, AIndexes)
	{
		if (FMetaIndexItems.contains(index))
		{
			foreach(IRosterIndex *metaItem, FMetaIndexItems.value(index))
				proxies.append(FMetaIndexItemProxy.value(metaItem));
		}
		else if (FMetaIndexItemProxy.contains(index))
		{
			proxies.append(FMetaIndexItemProxy.value(index));
		}
		else if (!AExclusive)
		{
			proxies.append(index);
		}
	}
	proxies.removeAll(NULL);
	return proxies.toSet().toList();
}

void MetaContacts::detachMetaItems(const QStringList &AStreams, const QStringList &AContacts)
{
	if (isReadyStreams(AStreams) && !AStreams.isEmpty() && AStreams.count()==AContacts.count())
	{
		QMap<Jid, QMap<QUuid, QList<Jid> > > detachMap;
		for (int i=0; i<AStreams.count(); i++)
		{
			Jid streamJid = AStreams.at(i);
			Jid contactJid = AContacts.at(i);

			IMetaContact meta = findMetaContact(streamJid,contactJid);
			if (!meta.isNull())
				detachMap[streamJid][meta.id].append(contactJid);
		}

		for(QMap<Jid, QMap<QUuid, QList<Jid> > >::const_iterator streamIt=detachMap.constBegin(); streamIt!=detachMap.constEnd(); ++streamIt)
		{
			for(QMap<QUuid, QList<Jid> >::const_iterator metaIt=streamIt->constBegin(); metaIt!=streamIt->constEnd(); ++metaIt)
				detachMetaContactItems(streamIt.key(),metaIt.key(),metaIt.value());
		}
	}
}

void MetaContacts::combineMetaItems(const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas)
{
	if (isReadyStreams(AStreams))
	{
		CombineContactsDialog *dialog = new CombineContactsDialog(this,AStreams,AContacts,AMetas);
		WidgetManager::showActivateRaiseWindow(dialog);
	}
}

void MetaContacts::destroyMetaContacts(const QStringList &AStreams, const QStringList &AMetas)
{
	if (isReadyStreams(AStreams) && !AStreams.isEmpty() && AStreams.count()==AMetas.count())
	{
		for (int i=0; i<AStreams.count(); i++)
		{
			IMetaContact meta = findMetaContact(AStreams.at(i),QUuid(AMetas.at(i)));
			if (!meta.isNull())
				detachMetaContactItems(AStreams.at(i),meta.id,meta.items);
		}
	}
}

void MetaContacts::startSaveContactsToStorage(const Jid &AStreamJid)
{
	if (FPrivateStorage)
	{
		FSaveStreams += AStreamJid;
		FSaveTimer.start(STORAGE_SAVE_TIMEOUT);
	}
}

bool MetaContacts::saveContactsToStorage(const Jid &AStreamJid) const
{
	if (FPrivateStorage && isReady(AStreamJid))
	{
		QDomDocument doc;
		QDomElement storageElem = doc.appendChild(doc.createElementNS(NS_STORAGE_METACONTACTS,"storage")).toElement();
		saveMetaContactsToXML(storageElem,FMetaContacts.value(AStreamJid).values());
		if (!FPrivateStorage->saveData(AStreamJid,storageElem).isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,"Save meta-contacts to storage request sent");
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send save meta-contacts to storage request");
		}
	}
	else if (FPrivateStorage)
	{
		REPORT_ERROR("Failed to save meta-contacts to storage: Stream not ready");
	}
	return false;
}

QString MetaContacts::metaContactsFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_METACONTACTS))
		dir.mkdir(DIR_METACONTACTS);
	dir.cd(DIR_METACONTACTS);
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

QList<IMetaContact> MetaContacts::loadMetaContactsFromXML(const QDomElement &AElement) const
{
	QList<IMetaContact> contacts;
	QDomElement metaElem = AElement.firstChildElement("meta");
	while (!metaElem.isNull())
	{
		IMetaContact meta;
		meta.id = metaElem.attribute("id");
		meta.name = metaElem.attribute("name");

		QDomElement itemElem = metaElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			meta.items.append(itemElem.text());
			itemElem = itemElem.nextSiblingElement("item");
		}

		contacts.append(meta);
		metaElem = metaElem.nextSiblingElement("meta");
	}
	return contacts;
}

void MetaContacts::saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const
{
	for (QList<IMetaContact>::const_iterator metaIt=AContacts.constBegin(); metaIt!=AContacts.constEnd(); ++metaIt)
	{
		QDomElement metaElem = AElement.ownerDocument().createElement("meta");
		metaElem.setAttribute("id",metaIt->id.toString());
		metaElem.setAttribute("name",metaIt->name);

		for (QList<Jid>::const_iterator itemIt=metaIt->items.constBegin(); itemIt!=metaIt->items.constEnd(); ++itemIt)
		{
			QDomElement itemElem = AElement.ownerDocument().createElement("item");
			itemElem.appendChild(AElement.ownerDocument().createTextNode(itemIt->pBare()));
			metaElem.appendChild(itemElem);
		}

		AElement.appendChild(metaElem);
	}
}

QList<IMetaContact> MetaContacts::loadMetaContactsFromFile(const QString &AFileName) const
{
	QList<IMetaContact> contacts;

	QFile file(AFileName);
	if (file.open(QIODevice::ReadOnly))
	{
		QString xmlError;
		QDomDocument doc;
		if (doc.setContent(&file,true,&xmlError))
		{
			QDomElement storageElem = doc.firstChildElement("storage");
			contacts = loadMetaContactsFromXML(storageElem);
		}
		else
		{
			REPORT_ERROR(QString("Failed to load meta-contacts from file content: %1").arg(xmlError));
			file.remove();
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load meta-contacts from file: %1").arg(file.errorString()));
	}

	return contacts;
}

void MetaContacts::saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const
{
	QFile file(AFileName);
	if (file.open(QIODevice::WriteOnly|QIODevice::Truncate))
	{
		QDomDocument doc;
		QDomElement storageElem = doc.appendChild(doc.createElementNS(NS_STORAGE_METACONTACTS,"storage")).toElement();
		saveMetaContactsToXML(storageElem,AContacts);
		file.write(doc.toByteArray());
		file.flush();
	}
	else
	{
		REPORT_ERROR(QString("Failed to save meta-contacts to file: %1").arg(file.errorString()));
	}
}

void MetaContacts::onRosterAdded(IRoster *ARoster)
{
	FLoadStreams += ARoster->streamJid();
	FMetaContacts[ARoster->streamJid()].clear();
	QTimer::singleShot(0,this,SLOT(onLoadContactsFromFileTimerTimeout()));
}

void MetaContacts::onRosterRemoved(IRoster *ARoster)
{
	FSaveStreams -= ARoster->streamJid();
	FLoadStreams -= ARoster->streamJid();

	QHash<QUuid, QList<IRosterIndex *> > metaIndexes = FMetaIndexes.take(ARoster->streamJid());
	for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt=metaIndexes.constBegin(); metaIt!=metaIndexes.constEnd(); ++metaIt)
	{
		foreach(IRosterIndex *metaIndex, metaIt.value())
			foreach(IRosterIndex *itemIndex, FMetaIndexItems.take(metaIndex))
				FMetaIndexItemProxy.remove(itemIndex);
	}

	FItemMetaContact.remove(ARoster->streamJid());
	QHash<QUuid, IMetaContact> metas = FMetaContacts.take(ARoster->streamJid());
	saveMetaContactsToFile(metaContactsFileName(ARoster->streamJid()),metas.values());
}

void MetaContacts::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	if (FSaveStreams.contains(ABefore))
	{
		FSaveStreams -= ABefore;
		FSaveStreams += ARoster->streamJid();
	}
	if (FLoadStreams.contains(ABefore))
	{
		FLoadStreams -= ABefore;
		FLoadStreams += ARoster->streamJid();
	}

	FMetaContacts[ARoster->streamJid()] = FMetaContacts.take(ABefore);
	FItemMetaContact[ARoster->streamJid()] = FItemMetaContact.take(ABefore);
	
	QString afterStreamJid = ARoster->streamJid().pFull();
	QHash<QUuid, QList<IRosterIndex *> > metaIndexes = FMetaIndexes.take(ABefore);
	for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt = metaIndexes.constBegin(); metaIt!=metaIndexes.constEnd(); ++metaIt)
	{
		for (QList<IRosterIndex *>::const_iterator metaIndexIt=metaIt->constBegin(); metaIndexIt!=metaIt->constEnd(); ++metaIndexIt)
		{
			IRosterIndex *metaIndex = *metaIndexIt;
			foreach(IRosterIndex *itemIndex, FMetaIndexItems.value(metaIndex))
				itemIndex->setData(afterStreamJid,RDR_STREAM_JID);
			metaIndex->setData(afterStreamJid,RDR_STREAM_JID);
		}
	}
	FMetaIndexes.insert(ARoster->streamJid(),metaIndexes);
}

void MetaContacts::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (AItem.subscription!=ABefore.subscription || AItem.groups!=ABefore.groups)
	{
		IMetaContact meta = findMetaContact(ARoster->streamJid(),AItem.itemJid);
		if (!meta.isNull())
			updateMetaContact(ARoster->streamJid(),meta);
	}
}

void MetaContacts::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.show!=ABefore.show || AItem.priority!=ABefore.priority || AItem.status!=ABefore.status)
	{
		IMetaContact meta = findMetaContact(APresence->streamJid(),AItem.itemJid);
		if (!meta.isNull())
			updateMetaContact(APresence->streamJid(),meta);
	}
}

void MetaContacts::onPrivateStorageOpened(const Jid &AStreamJid)
{
	if (!FPrivateStorage->loadData(AStreamJid,"storage",NS_STORAGE_METACONTACTS).isEmpty())
		LOG_STRM_INFO(AStreamJid,"Load meta-contacts from storage request sent");
	else
		LOG_STRM_WARNING(AStreamJid,"Failed to send load meta-contacts from storage request");
}

void MetaContacts::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.namespaceURI() == NS_STORAGE_METACONTACTS)
	{
		LOG_STRM_INFO(AStreamJid,"Meta-contacts loaded from storage");
		updateMetaContacts(AStreamJid,loadMetaContactsFromXML(AElement));
	}
}

void MetaContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	Q_UNUSED(ATagName);
	if (ANamespace == NS_STORAGE_METACONTACTS)
	{
		if (!FPrivateStorage->loadData(AStreamJid,ATagName,NS_STORAGE_METACONTACTS).isEmpty())
			LOG_STRM_INFO(AStreamJid,"Reload meta-contacts from storage request sent");
		else
			LOG_STRM_WARNING(AStreamJid,"Failed to send reload meta-contacts from storage request");
	}
}

void MetaContacts::onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid)
{
	if (FSaveStreams.contains(AStreamJid))
	{
		saveContactsToStorage(AStreamJid);
		FSaveStreams -= AStreamJid;
	}
}

void MetaContacts::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	static const QList<int> updateDataRoles = QList<int>() << Qt::DecorationRole << Qt::DisplayRole;
	if (updateDataRoles.contains(ARole))
	{
		foreach(const IRosterIndex *index, FMetaIndexItemProxy.keys(AIndex))
			emit rosterDataChanged(const_cast<IRosterIndex *>(index),ARole);
	}
}

void MetaContacts::onRostersViewIndexContextMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	if (menu)
	{
		QSet<Action *> proxyActions = FProxyContextMenuActions.take(menu);
		foreach(Action *action, menu->groupActions())
			if (!proxyActions.contains(action))
				action->setVisible(false);
	}
	FProxyContextMenuActions.clear();
}

void MetaContacts::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MetaContacts::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	static bool blocked = false;
	if (!blocked && ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		QSet<Action *> metaActions;
		QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_KIND<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_METACONTACT_ID);

		if (isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			QStringList uniqueMetas = rolesMap.value(RDR_METACONTACT_ID).toSet().toList();
			if (uniqueMetas.count()>1 || uniqueMetas.value(0).isEmpty())
			{
				Action *combineAction = new Action(AMenu);
				combineAction->setText(tr("Combine to Metacontact..."));
				combineAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				combineAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				combineAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				connect(combineAction,SIGNAL(triggered()),SLOT(onCombineMetaItemsByAction()));
				AMenu->addAction(combineAction,AG_RVCM_METACONTACTS);
				metaActions += combineAction;
			}

			QStringList uniqueKinds = rolesMap.value(RDR_KIND).toSet().toList();
			if (uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT_ITEM)
			{
				Action *detachAction = new Action(AMenu);
				detachAction->setText(tr("Detach from Metacontact"));
				detachAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				detachAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				connect(detachAction,SIGNAL(triggered()),SLOT(onDetachMetaItemsByAction()));
				AMenu->addAction(detachAction,AG_RVCM_METACONTACTS);
				metaActions += detachAction;
			}

			if (uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT)
			{
				Action *destroyAction = new Action(AMenu);
				destroyAction->setText(tr("Destroy Metacontact"));
				destroyAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				destroyAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				connect(destroyAction,SIGNAL(triggered()),SLOT(onDestroyMetaContactsByAction()));
				AMenu->addAction(destroyAction,AG_RVCM_METACONTACTS);
				metaActions += destroyAction;
			}
		}

		QList<IRosterIndex *> proxies = indexesProxies(AIndexes,false);
		if (!proxies.isEmpty())
		{
			blocked = true;

			QSet<Action *> oldActions = AMenu->groupActions().toSet();
			FRostersView->contextMenuForIndex(proxies,NULL,AMenu);
			connect(AMenu,SIGNAL(aboutToShow()),SLOT(onRostersViewIndexContextMenuAboutToShow()));
			FProxyContextMenuActions[AMenu] = AMenu->groupActions().toSet() - oldActions + metaActions;

			blocked = false;
		}
	}
}

void MetaContacts::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		if (AIndex->kind() == RIK_METACONTACT)
		{
			QStringList metaItems;
			QStringList metaAvatars;
			QStringList metaToolTips;

			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AIndex->data(RDR_STREAM_JID).toString()) : NULL;

			QMap<Jid, IRosterIndex *> metaItemsMap = FMetaIndexItems.value(AIndex);
			for(QMap<Jid, IRosterIndex *>::const_iterator it = metaItemsMap.constBegin(); it!=metaItemsMap.constEnd(); ++it)
			{
				QMap<int, QString> toolTips;
				FRostersView->toolTipsForIndex(it.value(),NULL,toolTips);
				if (!toolTips.isEmpty())
				{
					static const QList<int> requiredToolTipOrders = QList<int>();

					QString avatarToolTip = toolTips.value(RTTO_AVATAR_IMAGE);
					if (!avatarToolTip.isEmpty() && metaAvatars.count()<10 && !metaAvatars.contains(avatarToolTip))
						metaAvatars += avatarToolTip;

					for (QMap<int, QString>::iterator it=toolTips.begin(); it!=toolTips.end(); )
					{
						if (!requiredToolTipOrders.contains(it.key()))
							it = toolTips.erase(it);
						else
							++it;
					}

					if (!toolTips.isEmpty())
					{
						QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTips.values()).join("<p/><nbsp>"));
						metaToolTips.append(tooltip);
					}
				}

				if (FStatusIcons && presence)
				{
					IPresenceItem pitem = FPresencePlugin->sortPresenceItems(presence->findItems(it.key())).value(0);
					QString iconset = FStatusIcons->iconsetByJid(pitem.isValid ? pitem.itemJid : it.key());
					QString iconkey = FStatusIcons->iconKeyByJid(presence->streamJid(),pitem.isValid ? pitem.itemJid : it.key());
					metaItems.append(QString("<img src='%1'><nbsp>%2").arg(FStatusIcons->iconFileName(iconset,iconkey),Qt::escape(it.key().uBare())));
				}
				else
				{
					metaItems.append(Qt::escape(it.key().uBare()));
				}
			}

			if (!metaAvatars.isEmpty())
				AToolTips.insert(RTTO_AVATAR_IMAGE,QString("<span>%1</span>").arg(metaAvatars.join("<nbsp>")));

			if (!metaItems.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_JABBERID,QString("<span>%1</span>").arg(metaItems.join("<p/><nbsp>")));
			else
				AToolTips.remove(RTTO_ROSTERSVIEW_INFO_JABBERID);

			if (!metaToolTips.isEmpty())
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_STREAMS,QString("<span><hr>%1</span>").arg(metaToolTips.join("<hr><p/><nbsp>")));

			QStringList resources = AIndex->data(RDR_RESOURCES).toStringList();
			if (!resources.isEmpty())
			{
				for(int resIndex=0; resIndex<10 && resIndex<resources.count(); resIndex++)
				{
					int orderShift = resIndex*100;
					IPresenceItem pitem = presence!=NULL ? presence->findItem(resources.at(resIndex)) : IPresenceItem();
					if (pitem.isValid)
						AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_NAME+orderShift,tr("<b>Resource:</b> %1 (%2)").arg(Qt::escape(pitem.itemJid.uFull())).arg(pitem.priority));
				}
			}

			if (!AToolTips.contains(RTTO_ROSTERSVIEW_RESOURCE_TOPLINE))
			{
				AToolTips.remove(RTTO_ROSTERSVIEW_RESOURCE_STATUS_NAME);
				AToolTips.remove(RTTO_ROSTERSVIEW_RESOURCE_STATUS_TEXT);
			}
			AToolTips.remove(RTTO_ROSTERSVIEW_INFO_SUBCRIPTION);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
			if (proxy != NULL)
				FRostersView->toolTipsForIndex(proxy,NULL,AToolTips);
		}
	}
}

void MetaContacts::onRostersViewNotifyInserted(int ANotifyId)
{
	QList<IRosterIndex *> metaIndexes;
	QList<IRosterIndex *> itemIndexes;
	foreach(IRosterIndex *proxy, FRostersView->notifyIndexes(ANotifyId))
	{
		if (proxy->kind() == RIK_CONTACT)
		{
			foreach(const IRosterIndex *itemIndex, FMetaIndexItemProxy.keys(proxy))
			{
				IRosterIndex *metaIndex = itemIndex->parentIndex();
				if (!metaIndexes.contains(metaIndex))
					metaIndexes.append(metaIndex);
				itemIndexes.append(const_cast<IRosterIndex *>(itemIndex));
			}
		}
	}

	if (!itemIndexes.isEmpty())
	{
		IRostersNotify notify = FRostersView->notifyById(ANotifyId);

		int metaNotifyId = FRostersView->insertNotify(notify,metaIndexes);
		FProxyToIndexNotify.insert(ANotifyId,metaNotifyId);

		notify.flags &= ~(IRostersNotify::ExpandParents);
		int itemNotifyId = FRostersView->insertNotify(notify,itemIndexes);
		FProxyToIndexNotify.insert(metaNotifyId,itemNotifyId);
	}
}

void MetaContacts::onRostersViewNotifyRemoved(int ANotifyId)
{
	if (FProxyToIndexNotify.contains(ANotifyId))
		FRostersView->removeNotify(FProxyToIndexNotify.take(ANotifyId));
}

void MetaContacts::onRostersViewNotifyActivated(int ANotifyId)
{
	int notifyId = FProxyToIndexNotify.key(ANotifyId);
	if (notifyId > 0)
		FRostersView->activateNotify(notifyId);
}

void MetaContacts::onDetachMetaItemsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		detachMetaItems(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList());
}

void MetaContacts::onCombineMetaItemsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		combineMetaItems(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_METACONTACT_ID).toStringList());
}

void MetaContacts::onDestroyMetaContactsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		destroyMetaContacts(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_METACONTACT_ID).toStringList());
}

void MetaContacts::onLoadContactsFromFileTimerTimeout()
{
	for (QSet<Jid>::iterator it=FLoadStreams.begin(); it!=FLoadStreams.end(); it=FLoadStreams.erase(it))
		updateMetaContacts(*it,loadMetaContactsFromFile(metaContactsFileName(*it)));
}

void MetaContacts::onSaveContactsToStorageTimerTimeout()
{
	for (QSet<Jid>::iterator it=FSaveStreams.begin(); it!=FSaveStreams.end(); it=FSaveStreams.erase(it))
		saveContactsToStorage(*it);
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
