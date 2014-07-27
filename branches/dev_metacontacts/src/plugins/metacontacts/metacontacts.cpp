#include "metacontacts.h"

#include <QtDebug>

#include <QDir>
#include <QMouseEvent>
#include <QTextDocument>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
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
#include <utils/logger.h>

#define UPDATE_META_TIMEOUT     0
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
	FStatusIcons = NULL;
	FMessageWidgets = NULL;

	FSortFilterProxyModel = new MetaSortFilterProxyModel(this,this);
	FSortFilterProxyModel->setDynamicSortFilter(true);

	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveContactsToStorageTimerTimeout()));

	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateContactsTimerTimeout()));
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
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRostersModelIndexInserted(IRosterIndex*)));
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRostersModelIndexDestroyed(IRosterIndex*)));
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

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IMessageChatWindow *)),SLOT(onMessageChatWindowCreated(IMessageChatWindow *)));
		}
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
		static const QList<int> roles =	QList<int>(); 
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
				IRosterIndex *proxyIndex = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
				switch (ARole)
				{
				case Qt::DecorationRole:
					return proxyIndex!=NULL ? proxyIndex->data(ARole) : QVariant();
				}
				break;
			}
		case RIK_METACONTACT_ITEM: 
			{
				IRosterIndex *proxyIndex = FMetaIndexItemProxy.value(AIndex);
				switch (ARole)
				{
				case Qt::DisplayRole:
				case Qt::DecorationRole:
					return proxyIndex!=NULL ? proxyIndex->data(ARole) : QVariant();
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
	if (AOrder==RLHO_METACONTACTS && AIndex->kind()==RIK_METACONTACT)
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
		}
	}
	return false;
}

bool MetaContacts::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(AEvent);
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
			int removed = 0;
			foreach(const Jid &item, AItems)
				removed += meta.items.removeAll(item);

			if (removed > 0)
			{
				if (updateMetaContact(AStreamJid,meta))
				{
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
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
			if (meta.name != AName)
			{
				meta.name = AName;
				if (updateMetaContact(AStreamJid,meta))
				{
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
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
			if (meta.items != AItems)
			{
				meta.items = AItems;
				if (updateMetaContact(AStreamJid,meta))
				{
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
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
		if (meta.id == AMetaId)
		{
			if (meta.groups != AGroups)
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
			else
			{
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
			return roster->rosterItem(AItem).isValid;
	}
	return false;
}

void MetaContacts::updateMetaIndexes(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot)
	{
		QList<IRosterIndex *> &metaIndexList = FMetaIndexes[AStreamJid][AMetaContact.id];

		QMap<QString, IRosterIndex *> groupMetaIndexMap;
		foreach(IRosterIndex *metaIndex, metaIndexList)
			groupMetaIndexMap.insert(metaIndex->data(RDR_GROUP).toString(),metaIndex);

		QSet<QString> metaGroups = !AMetaContact.groups.isEmpty() ? AMetaContact.groups : QSet<QString>()<<QString::null;
		QSet<QString> curGroups = groupMetaIndexMap.keys().toSet();
		QSet<QString> newGroups = metaGroups - curGroups;
		QSet<QString> oldGroups = curGroups - metaGroups;

		QSet<QString>::iterator oldGroupsIt = oldGroups.begin();
		foreach(const QString &group, newGroups)
		{
			IRosterIndex *groupIndex = FRostersModel->getGroupIndex(!group.isEmpty() ? RIK_GROUP : RIK_GROUP_BLANK,group,sroot);

			IRosterIndex *metaIndex;
			if (oldGroupsIt == oldGroups.end())
			{
				metaIndex = FRostersModel->newRosterIndex(RIK_METACONTACT);
				metaIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
				metaIndex->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);
				metaIndexList.append(metaIndex);
			}
			else
			{
				QString oldGroup = *oldGroupsIt;
				oldGroupsIt = oldGroups.erase(oldGroupsIt);
				metaIndex = groupMetaIndexMap.value(oldGroup);
			}

			metaIndex->setData(group,RDR_GROUP);

			FRostersModel->insertRosterIndex(metaIndex,groupIndex);
		}

		while(oldGroupsIt != oldGroups.end())
		{
			IRosterIndex *metaIndex = groupMetaIndexMap.value(*oldGroupsIt);
			updateMetaIndexItems(metaIndex,QList<Jid>());

			FMetaIndexItems.remove(metaIndex);
			metaIndexList.removeAll(metaIndex);

			FRostersModel->removeRosterIndex(metaIndex);
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

		foreach(IRosterIndex *metaIndex, metaIndexList)
		{
			metaIndex->setData(metaJid.full(),RDR_FULL_JID);
			metaIndex->setData(metaJid.pFull(),RDR_PREP_FULL_JID);
			metaIndex->setData(metaJid.pBare(),RDR_PREP_BARE_JID);
			metaIndex->setData(resources,RDR_RESOURCES);

			metaIndex->setData(AMetaContact.name,RDR_NAME);
			metaIndex->setData(metaPresence.show,RDR_SHOW);
			metaIndex->setData(metaPresence.status,RDR_STATUS);
			metaIndex->setData(metaPresence.priority,RDR_PRIORITY);

			updateMetaIndexItems(metaIndex,AMetaContact.items);

			emit rosterLabelChanged(RLID_METACONTACTS_BRANCH_ITEMS,metaIndex);
		}
	}
}

void MetaContacts::removeMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (FRostersModel)
	{
		foreach(IRosterIndex *metaIndex, FMetaIndexes[AStreamJid].take(AMetaId))
		{
			updateMetaIndexItems(metaIndex,QList<Jid>());
			FMetaIndexItems.remove(metaIndex);

			FRostersModel->removeRosterIndex(metaIndex);
		}
	}
}

void MetaContacts::updateMetaIndexItems(IRosterIndex *AMetaIndex, const QList<Jid> &AItems)
{
	QMap<Jid, IRosterIndex *> &metaItemsMap = FMetaIndexItems[AMetaIndex];

	QMap<Jid, IRosterIndex *> itemIndexMap = metaItemsMap;
	foreach(const Jid &itemJid, AItems)
	{
		QMap<IRosterIndex *, IRosterIndex *> proxyIndexMap;
		foreach(IRosterIndex *index, FRostersModel->findContactIndexes(AMetaIndex->data(RDR_STREAM_JID).toString(),itemJid))
			proxyIndexMap.insert(index->parentIndex(),index);

		IRosterIndex *proxyIndex = !proxyIndexMap.isEmpty() ? proxyIndexMap.value(AMetaIndex->parentIndex(),proxyIndexMap.constBegin().value()) : NULL;
		if (proxyIndex != NULL)
		{
			IRosterIndex *itemIndex = itemIndexMap.take(itemJid);
			if (itemIndex == NULL)
			{
				itemIndex = FRostersModel->newRosterIndex(RIK_METACONTACT_ITEM);
				itemIndex->setData(AMetaIndex->data(RDR_STREAM_JID),RDR_STREAM_JID);
				itemIndex->setData(AMetaIndex->data(RDR_METACONTACT_ID),RDR_METACONTACT_ID);
				metaItemsMap.insert(itemJid,itemIndex);
			}

			static const QList<int> proxyRoles = QList<int>()
				<< RDR_FULL_JID << RDR_PREP_FULL_JID << RDR_PREP_BARE_JID	<< RDR_RESOURCES
				<< RDR_NAME << RDR_SUBSCRIBTION << RDR_ASK
				<< RDR_SHOW << RDR_STATUS;
			
			foreach(int role, proxyRoles)
				itemIndex->setData(proxyIndex->data(role),role);

			static const QList<int> parentRoles = QList<int>()
				<< RDR_GROUP;

			foreach(int role, parentRoles)
				itemIndex->setData(AMetaIndex->data(role),role);

			IRosterIndex *prevProxyIndex = FMetaIndexItemProxy.value(itemIndex);
			if (proxyIndex != prevProxyIndex)
			{
				if (prevProxyIndex)
				{
					FMetaIndexProxyItem.remove(prevProxyIndex,itemIndex);
					if (!FMetaIndexProxyItem.contains(prevProxyIndex))
						prevProxyIndex->setData(QVariant(),RDR_METACONTACT_ID);
				}
				proxyIndex->setData(itemIndex->data(RDR_METACONTACT_ID),RDR_METACONTACT_ID);

				FMetaIndexItemProxy.insert(itemIndex,proxyIndex);
				FMetaIndexProxyItem.insertMulti(proxyIndex,itemIndex);
			}

			FRostersModel->insertRosterIndex(itemIndex,AMetaIndex);
		}
	}

	for(QMap<Jid, IRosterIndex *>::const_iterator it=itemIndexMap.constBegin(); it!=itemIndexMap.constEnd(); ++it)
	{
		IRosterIndex *proxyIndex = FMetaIndexItemProxy.take(it.value());
		if (proxyIndex)
		{
			FMetaIndexProxyItem.remove(proxyIndex,it.value());
			if (!FMetaIndexProxyItem.contains(proxyIndex))
				proxyIndex->setData(QVariant(),RDR_METACONTACT_ID);
		}
		
		metaItemsMap.remove(it.key());
		FRostersModel->removeRosterIndex(it.value());
	}
}

void MetaContacts::updateMetaWindows(const Jid &AStreamJid, const IMetaContact &AMetaContact, const QSet<Jid> ANewItems, const QSet<Jid> AOldItems)
{
	if (FMessageWidgets && (!ANewItems.isEmpty() || !AOldItems.isEmpty()))
	{
		IMessageChatWindow *metaWindow = FMetaChatWindows.value(AStreamJid).value(AMetaContact.id);
		
		foreach(const Jid &itemJid, ANewItems)
		{
			IMessageChatWindow *window = FMessageWidgets->findChatWindow(AStreamJid,itemJid);
			if (window)
				window->address()->removeAddress(AStreamJid,itemJid);
		}

		if (metaWindow)
		{
			foreach(const Jid &itemJid, AOldItems)
				metaWindow->address()->removeAddress(AStreamJid,itemJid);
			foreach(const Jid &itemJid, ANewItems)
				metaWindow->address()->appendAddress(AStreamJid,itemJid);
		}

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
		foreach(const Jid &itemJid, meta.items)
		{
			IRosterItem ritem = roster->rosterItem(itemJid);
			if (ritem.isValid && !itemJid.node().isEmpty())
			{
				if (!before.items.contains(itemJid))
				{
					IMetaContact oldMeta = findMetaContact(AStreamJid,itemJid);
					if (!oldMeta.id.isNull())
						detachMetaContactItems(AStreamJid,oldMeta.id,meta.items);
					itemMetaHash.insert(itemJid,meta.id);
				}

				if (meta.name.isEmpty() && !ritem.name.isEmpty())
					meta.name = ritem.name;

				if (!ritem.groups.isEmpty())
					meta.groups += ritem.groups;

				if (presence)
					meta.presences += presence->findItems(itemJid);
			}
			else
			{
				meta.items.removeAll(itemJid);
			}
		}

		if (meta.name.isEmpty())
			meta.name = meta.items.value(0).uBare();
		if (!meta.presences.isEmpty())
			meta.presences = FPresencePlugin->sortPresenceItems(meta.presences);

		if (meta != before)
		{
			QSet<Jid> newMetaItems = meta.items.toSet()-before.items.toSet();
			QSet<Jid> oldMetaItems = before.items.toSet()-meta.items.toSet();

			foreach(const Jid &itemJid, oldMetaItems)
				itemMetaHash.remove(itemJid);

			if (!meta.isEmpty())
			{
				FMetaContacts[AStreamJid][meta.id] = meta;
				updateMetaIndexes(AStreamJid,meta);
			}
			else
			{
				FMetaContacts[AStreamJid].remove(meta.id);
				removeMetaIndexes(AStreamJid,meta.id);
			}

			updateMetaWindows(AStreamJid,meta,newMetaItems,oldMetaItems);

			emit metaContactChanged(AStreamJid,meta,before);
			return true;
		}
		else
		{
			updateMetaIndexes(AStreamJid,meta);
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
		meta.items.clear();
		updateMetaContact(AStreamJid,meta);
	}
}

void MetaContacts::startUpdateMetaContact(const Jid &AStreamJid, const QUuid &AMetaId)
{
	FUpdateContacts[AStreamJid][AMetaId] += NULL;
	FUpdateTimer.start(UPDATE_META_TIMEOUT);
}

void MetaContacts::startUpdateMetaContactIndex(const Jid &AStreamJid, const QUuid &AMetaId, IRosterIndex *AIndex)
{
	FUpdateContacts[AStreamJid][AMetaId] += AIndex;
	FUpdateTimer.start(UPDATE_META_TIMEOUT);
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

QList<IRosterIndex *> MetaContacts::indexesProxies(const QList<IRosterIndex *> &AIndexes, bool ASelfProxy) const
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
		else if (!ASelfProxy)
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
	FUpdateContacts.remove(ARoster->streamJid());

	QHash<QUuid, QList<IRosterIndex *> > metaIndexes = FMetaIndexes.take(ARoster->streamJid());
	for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt=metaIndexes.constBegin(); metaIt!=metaIndexes.constEnd(); ++metaIt)
	{
		foreach(IRosterIndex *metaIndex, metaIt.value())
			foreach(IRosterIndex *itemIndex, FMetaIndexItems.take(metaIndex))
				FMetaIndexProxyItem.remove(FMetaIndexItemProxy.take(itemIndex),itemIndex);
	}

	FMetaChatWindows.remove(ARoster->streamJid());

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

	FMetaChatWindows[ARoster->streamJid()] = FMetaChatWindows.take(ABefore);

	FMetaContacts[ARoster->streamJid()] = FMetaContacts.take(ABefore);
	FItemMetaContact[ARoster->streamJid()] = FItemMetaContact.take(ABefore);
}

void MetaContacts::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (AItem.subscription!=ABefore.subscription || AItem.groups!=ABefore.groups)
	{
		QUuid metaId = FItemMetaContact.value(ARoster->streamJid()).value(AItem.itemJid);
		if (!metaId.isNull())
			startUpdateMetaContact(ARoster->streamJid(),metaId);
	}
}

void MetaContacts::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.show!=ABefore.show || AItem.priority!=ABefore.priority || AItem.status!=ABefore.status)
	{
		QUuid metaId = FItemMetaContact.value(APresence->streamJid()).value(AItem.itemJid.bare());
		if (!metaId.isNull())
			startUpdateMetaContact(APresence->streamJid(),metaId);
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

void MetaContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->kind()==RIK_CONTACT && !FMetaIndexItemProxy.contains(AIndex))
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid itemJid = AIndex->data(RDR_PREP_BARE_JID).toString();

		QUuid metaId = FItemMetaContact.value(streamJid).value(itemJid);
		if (!metaId.isNull())
		{
			foreach(IRosterIndex *metaIndex, FMetaIndexes.value(streamJid).value(metaId))
			{
				if (AIndex->parentIndex() == metaIndex->parentIndex())
				{
					startUpdateMetaContactIndex(streamJid,metaId,metaIndex);
					break;
				}
			}
		}
	}
}

void MetaContacts::onRostersModelIndexDestroyed(IRosterIndex *AIndex)
{
	for (QMultiHash<const IRosterIndex *, IRosterIndex *>::iterator it=FMetaIndexProxyItem.find(AIndex); it!=FMetaIndexProxyItem.end() && it.key()==AIndex; it=FMetaIndexProxyItem.erase(it))
		FMetaIndexItemProxy.remove(it.value());
	
	if (!FMetaIndexItems.take(AIndex).isEmpty())
		FMetaIndexes[AIndex->data(RDR_STREAM_JID).toString()][AIndex->data(RDR_METACONTACT_ID).toString()].removeAll(AIndex);
}

void MetaContacts::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AIndex); Q_UNUSED(ARole);
	//static const QList<int> updateDataRoles = QList<int>() << Qt::DecorationRole << Qt::DisplayRole;
	//if (updateDataRoles.contains(ARole))
	//{
	//	foreach(const IRosterIndex *index, FMetaIndexProxyItem.values(AIndex))
	//		emit rosterDataChanged(const_cast<IRosterIndex *>(index),ARole);
	//}
}

void MetaContacts::onRostersViewIndexContextMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	Menu *proxyMenu = FProxyContextMenu.value(menu);
	if (proxyMenu != NULL)
	{
		QStringList proxyActions;
		foreach(Action *action, proxyMenu->groupActions())
		{
			proxyActions.append(action->text());
			menu->addAction(action,proxyMenu->actionGroup(action),true);
		}

		foreach(Action *action, menu->groupActions())
		{
			if (proxyActions.contains(action->text()) && proxyMenu->actionGroup(action)==AG_NULL)
				menu->removeAction(action);
		}
	}
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
		
		QStringList uniqueKinds = rolesMap.value(RDR_KIND).toSet().toList();
		QStringList uniqueMetas = rolesMap.value(RDR_METACONTACT_ID).toSet().toList();

		if (isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			if (FRostersView->hasMultiSelection() && (uniqueMetas.count()>1 || uniqueMetas.value(0).isEmpty()))
			{
				Action *combineAction = new Action(AMenu);
				combineAction->setText(tr("Combine to Metacontact..."));
				combineAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				combineAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				combineAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				connect(combineAction,SIGNAL(triggered()),SLOT(onCombineMetaItemsByAction()));
				AMenu->addAction(combineAction,AG_RVCM_METACONTACTS,true);
				metaActions += combineAction;
			}

			if (uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT_ITEM)
			{
				Action *detachAction = new Action(AMenu);
				detachAction->setText(tr("Detach from Metacontact"));
				detachAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				detachAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				connect(detachAction,SIGNAL(triggered()),SLOT(onDetachMetaItemsByAction()));
				AMenu->addAction(detachAction,AG_RVCM_METACONTACTS,true);
				metaActions += detachAction;
			}

			if (uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT)
			{
				Action *destroyAction = new Action(AMenu);
				destroyAction->setText(tr("Destroy Metacontact"));
				destroyAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				destroyAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				connect(destroyAction,SIGNAL(triggered()),SLOT(onDestroyMetaContactsByAction()));
				AMenu->addAction(destroyAction,AG_RVCM_METACONTACTS,true);
				metaActions += destroyAction;
			}
		}

		if (uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT)
		{
			blocked = true;

			QList<IRosterIndex *> proxyIndexes;
			foreach(IRosterIndex *metaIndex, AIndexes)
				proxyIndexes += FMetaIndexItems.value(metaIndex).values();

			Menu *proxyMenu = new Menu(AMenu);
			FProxyContextMenu.insert(AMenu,proxyMenu);
			FRostersView->contextMenuForIndex(proxyIndexes,NULL,proxyMenu);
			connect(AMenu,SIGNAL(aboutToShow()),SLOT(onRostersViewIndexContextMenuAboutToShow()),Qt::UniqueConnection);

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
			foreach(const IRosterIndex *itemIndex, FMetaIndexProxyItem.values(proxy))
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

void MetaContacts::onMessageChatWindowCreated(IMessageChatWindow *AWindow)
{
	IMetaContact meta = findMetaContact(AWindow->streamJid(),AWindow->contactJid());
	if (!meta.isNull())
	{
		foreach(Jid itemJid, meta.items)
			AWindow->address()->appendAddress(AWindow->streamJid(),itemJid);

		FMetaChatWindows[AWindow->streamJid()].insert(meta.id,AWindow);
		connect(AWindow->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMessageChatWindowDestroyed()));
	}
}

void MetaContacts::onMessageChatWindowDestroyed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		QHash<QUuid, IMessageChatWindow *> &metaWindowHash = FMetaChatWindows[window->streamJid()];
		for (QHash<QUuid, IMessageChatWindow *>::iterator it=metaWindowHash.begin(); it!=metaWindowHash.end(); )
		{
			if (it.value() == window)
				it = metaWindowHash.erase(it);
			else
				++it;
		}
	}
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

void MetaContacts::onUpdateContactsTimerTimeout()
{
	for (QMap<Jid, QMap<QUuid, QSet<IRosterIndex *> > >::iterator streamIt=FUpdateContacts.begin(); streamIt!=FUpdateContacts.end(); streamIt=FUpdateContacts.erase(streamIt))
	{
		for(QMap<QUuid, QSet<IRosterIndex *> >::const_iterator metaIt=streamIt->constBegin(); metaIt!=streamIt->constEnd(); ++metaIt)
		{
			IMetaContact meta = findMetaContact(streamIt.key(),metaIt.key());
			if (!meta.isNull())
			{
				QList<IRosterIndex *> metaIndexList = FMetaIndexes.value(streamIt.key()).value(metaIt.key());
				if (metaIt->contains(NULL))
				{
					updateMetaContact(streamIt.key(),meta);
				}
				else foreach(IRosterIndex *metaIndex, metaIt.value())
				{
					if (metaIndexList.contains(metaIndex))
						updateMetaIndexItems(metaIndex,meta.items);
				}
			}
		}
	}
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
