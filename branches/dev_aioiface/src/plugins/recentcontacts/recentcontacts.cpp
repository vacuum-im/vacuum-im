#include "recentcontacts.h"

#include <QDir>
#include <QFile>
#include <QStyle>
#include <QPalette>
#include <QMouseEvent>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/recentitemtypes.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/datetime.h>

#define DIR_RECENT                   "recent"

#define PST_RECENTCONTACTS           "recent"
#define PSN_RECENTCONTACTS           "vacuum:recent-contacts"

#define MIN_VISIBLE_ITEMS            5

bool recentItemLessThen(const IRecentItem &AItem1, const IRecentItem &AItem2) 
{
	return AItem1.favorite==AItem2.favorite ? AItem1.dateTime>AItem2.dateTime : AItem1.favorite>AItem2.favorite;
}

RecentContacts::RecentContacts()
{
	FPrivateStorage = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FStatusIcons = NULL;
	FRostersViewPlugin = NULL;

	FRootIndex = NULL;
	FInsertFavariteLabelId = 0;
	FRemoveFavoriteLabelId = 0;
	
	FMaxVisibleItems = 20;
}

RecentContacts::~RecentContacts()
{

}

void RecentContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Recent Contacts");
	APluginInfo->description = tr("Displays a recently used contacts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool RecentContacts::initConnections(IPluginManager *APluginManager, int &AInitOrder)
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
			connect(FPrivateStorage->instance(),SIGNAL(storageAboutToClose(const Jid &)),SLOT(onPrivateStorageAboutToClose(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(streamAdded(const Jid &)),SLOT(onRostersModelStreamAdded(const Jid &)));
			connect(FRostersModel->instance(),SIGNAL(streamRemoved(const Jid &)),SLOT(onRostersModelStreamRemoved(const Jid &)));
			connect(FRostersModel->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),SLOT(onRostersModelStreamJidChanged(const Jid &, const Jid &)));
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRostersModelIndexInserted(IRosterIndex *)));
			connect(FRostersModel->instance(),SIGNAL(indexRemoved(IRosterIndex *)),SLOT(onRostersModelIndexRemoved(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
			connect(FRostersView->instance(), SIGNAL(indexToolTips(IRosterIndex*,quint32,QMultiMap<int,QString>&)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex*,quint32,QMultiMap<int,QString>&)));
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

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	return FPrivateStorage!=NULL;
}

bool RecentContacts::initObjects()
{
	if (FRostersView)
	{
		AdvancedDelegateItem insertFavorite(RLID_RECENT_FAVORITE);
		insertFavorite.d->kind = AdvancedDelegateItem::CustomData;
		insertFavorite.d->showStates = QStyle::State_MouseOver;
		insertFavorite.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENTCONTACTS_INSERT_FAV);
		FInsertFavariteLabelId = FRostersView->registerLabel(insertFavorite);

		AdvancedDelegateItem removeFavorive(RLID_RECENT_FAVORITE+1);
		removeFavorive.d->kind = AdvancedDelegateItem::CustomData;
		removeFavorive.d->showStates = QStyle::State_MouseOver;
		removeFavorive.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENTCONTACTS_REMOVE_FAV);
		FRemoveFavoriteLabelId = FRostersView->registerLabel(removeFavorive);

		FRostersView->insertClickHooker(RCHO_RECENTCONTACTS,this);
		FRostersViewPlugin->registerExpandableRosterIndexType(RIT_RECENT_ROOT,RDR_TYPE);
	}
	if (FRostersModel)
	{
		FRootIndex = FRostersModel->createRosterIndex(RIT_RECENT_ROOT,FRostersModel->rootIndex());
		FRootIndex->setData(RDR_TYPE_ORDER,RITO_RECENT_ROOT);
		FRootIndex->setData(Qt::DisplayRole,tr("Recent Contacts"));
		FRootIndex->setData(Qt::DecorationRole,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENTCONTACTS_RECENT));
		FRootIndex->setData(RDR_STATES_FORCE_ON,QStyle::State_Children);
		FRootIndex->setRemoveOnLastChildRemoved(false);
		FRootIndex->setRemoveChildsOnRemoved(false);
		FRootIndex->setDestroyOnParentRemoved(false);
		FRootIndex->insertDataHolder(this);
	}
	registerItemHandler(REIT_CONTACT,this);
	return true;
}

bool RecentContacts::initSettings()
{
	return true;
}

bool RecentContacts::startPlugin()
{
	return true;
}

int RecentContacts::rosterDataOrder() const
{
	return RDHO_DEFAULT;
}

QList<int> RecentContacts::rosterDataRoles() const
{
	static const QList<int> roles = QList<int>() << Qt::DisplayRole << Qt::DecorationRole << Qt::ForegroundRole << Qt::BackgroundColorRole;
	return roles;
}

QList<int> RecentContacts::rosterDataTypes() const
{
	static const QList<int> types = QList<int>() << RIT_RECENT_ROOT << RIT_RECENT_ITEM;
	return types;
}

QVariant RecentContacts::rosterData(const IRosterIndex *AIndex, int ARole) const
{
	switch (AIndex->type())
	{
	case RIT_RECENT_ROOT:
		{
			QPalette palette = FRostersView!=NULL ? FRostersView->instance()->palette() : QPalette();
			switch (ARole)
			{
			case Qt::ForegroundRole:
				return palette.color(QPalette::Active, QPalette::BrightText);
			case Qt::BackgroundColorRole:
				return palette.color(QPalette::Active, QPalette::Dark);
			}
			break;
		}
	case RIT_RECENT_ITEM:
		{
			IRosterIndex *proxy = FIndexToProxy.value(AIndex);
			if (proxy)
				return proxy->data(ARole);
			break;
		}
	}
	return QVariant();
}

bool RecentContacts::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
	Q_UNUSED(AIndex);
	Q_UNUSED(ARole);
	Q_UNUSED(AValue);
	return false;
}

bool RecentContacts::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (AOrder == RCHO_RECENTCONTACTS && AIndex->type()==RIT_RECENT_ITEM)
	{
		QModelIndex modelIndex = FRostersView->mapFromModel(FRostersModel->modelIndexByRosterIndex(AIndex));
		quint32 labelId = FRostersView->labelAt(AEvent->pos(),modelIndex);
		if (labelId == FInsertFavariteLabelId)
		{
			setItemFavorite(rosterIndexItem(AIndex),true);
			return true;
		}
		else if (labelId == FRemoveFavoriteLabelId)
		{
			setItemFavorite(rosterIndexItem(AIndex),false);
			return true;
		}
		
		IRosterIndex *proxy = FIndexToProxy.value(AIndex);
		if (proxy)
		{
			return FRostersView->singleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->data(RDR_RECENT_TYPE)==REIT_CONTACT && Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		{
			Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
			Jid contactJid = AIndex->data(RDR_RECENT_REFERENCE).toString();
			return FMessageProcessor->createMessageWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
		}
	}
	return false;
}

bool RecentContacts::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	if (AOrder == RCHO_RECENTCONTACTS && AIndex->type()==RIT_RECENT_ITEM)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AIndex);
		if (proxy)
		{
			return FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->data(RDR_RECENT_TYPE) == REIT_CONTACT)
		{
			Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
			Jid contactJid = AIndex->data(RDR_RECENT_REFERENCE).toString();
			return FMessageProcessor->createMessageWindow(streamJid,contactJid,Message::Chat,IMessageHandler::SM_SHOW);
		}
	}
	return false;
}

bool RecentContacts::recentItemCanShow(const IRecentItem &AItem) const
{
	return AItem.streamJid.pBare() != AItem.reference;
}

QIcon RecentContacts::recentItemIcon(const IRecentItem &AItem) const
{
	if (FStatusIcons)
		return FStatusIcons->iconByJid(AItem.streamJid,AItem.reference);
	return QIcon();
}

QString RecentContacts::recentItemName(const IRecentItem &AItem) const
{
	return AItem.reference;
}

QList<IRosterIndex *> RecentContacts::recentItemProxyIndexes(const IRecentItem &AItem) const
{
	QList<IRosterIndex *> proxies;
	IRosterIndex *root = FRostersModel!=NULL ? FRostersModel->streamRoot(AItem.streamJid) : NULL;
	if (root)
	{
		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_TYPE,RIT_CONTACT);
		findData.insertMulti(RDR_TYPE,RIT_AGENT);
		findData.insertMulti(RDR_PREP_BARE_JID,AItem.reference);
		proxies =  sortItemProxies(root->findChilds(findData,true));
	}
	return proxies;
}

bool RecentContacts::isItemValid(const IRecentItem &AItem) const
{
	if (AItem.type.isEmpty())
		return false;
	if (!FStreamItems.contains(AItem.streamJid))
		return false;
	return true;
}

QList<IRecentItem> RecentContacts::streamItems(const Jid &AStreamJid) const
{
	return FStreamItems.value(AStreamJid);
}

QList<IRecentItem> RecentContacts::favoriteItems(const Jid &AStreamJid) const
{
	QList<IRecentItem> items = FStreamItems.value(AStreamJid);
	for(QList<IRecentItem>::iterator it=items.begin(); it!=items.end(); )
	{
		if (!it->favorite)
			it = items.erase(it);
		else
			++it;
	}
	return items;
}

void RecentContacts::setItemFavorite(const IRecentItem &AItem, bool AFavorite)
{
	IRecentItem &item = findRealItem(AItem);
	if (!item.type.isEmpty() && item.favorite!=AFavorite)
	{
		item.favorite = AFavorite;
		emit recentItemChanged(item);
		updateVisibleItems();
		updateItemIndex(AItem);
	}
}

void RecentContacts::setItemDateTime(const IRecentItem &AItem, const QDateTime &ATime)
{
	IRecentItem item = findRealItem(AItem);
	if (item.type.isEmpty())
	{
		item = AItem;
		item.favorite = false;
		item.dateTime = ATime;
		mergeRecentItems(QList<IRecentItem>() << item);
	}
	else if (item.dateTime < ATime)
	{
		item.dateTime = ATime;
		mergeRecentItems(QList<IRecentItem>() << item);
		updateItemIndex(AItem);
	}
}

QList<IRecentItem> RecentContacts::visibleItems() const
{
	return FVisibleItems.keys();
}

quint8 RecentContacts::maximumVisibleItems() const
{
	return FMaxVisibleItems;
}

void RecentContacts::setMaximumVisibleItems(quint8 ACount)
{
	if (FMaxVisibleItems!=ACount && ACount>=MIN_VISIBLE_ITEMS)
	{
		FMaxVisibleItems = ACount;
		updateVisibleItems();
	}
}

IRosterIndex *RecentContacts::itemRosterIndex(const IRecentItem &AItem) const
{
	return FVisibleItems.value(AItem);
}

IRosterIndex *RecentContacts::itemRosterProxyIndex(const IRecentItem &AItem) const
{
	return FIndexToProxy.value(FVisibleItems.value(AItem));
}

QList<QString> RecentContacts::itemHandlerTypes() const
{
	return FItemHandlers.keys();
}

IRecentItemHandler *RecentContacts::itemTypeHandler(const QString &AType) const
{
	return FItemHandlers.value(AType);
}

void RecentContacts::registerItemHandler(const QString &AType, IRecentItemHandler *AHandler)
{
	if (AHandler != NULL)
	{
		if (!FItemHandlers.values().contains(AHandler))
			connect(AHandler->instance(),SIGNAL(recentItemUpdated(const IRecentItem &)),SLOT(onHandlerRecentItemUpdated(const IRecentItem &)));
		FItemHandlers.insert(AType,AHandler);
		emit itemHandlerRegistered(AType,AHandler);
	}
}

void RecentContacts::updateVisibleItems()
{
	if (FRostersModel)
	{
		QList<IRecentItem> common;
		QList<IRecentItem> favorite;

		for (QMap<Jid, QList<IRecentItem> >::const_iterator stream_it=FStreamItems.constBegin(); stream_it!=FStreamItems.constEnd(); ++stream_it)
		{
			for (QList<IRecentItem>::const_iterator it = stream_it->constBegin(); it!=stream_it->constEnd(); ++it)
			{
				IRecentItemHandler *handler = FItemHandlers.value(it->type);
				if (handler!=NULL && handler->recentItemCanShow(*it))
					common.append(*it);
			}
		}
		qSort(common.begin(),common.end(),recentItemLessThen);

		QSet<IRecentItem> curVisible = FVisibleItems.keys().toSet();
		QSet<IRecentItem> newVisible = common.mid(0,FMaxVisibleItems).toSet();

		QSet<IRecentItem> addItems = newVisible - curVisible;
		QSet<IRecentItem> removeItems = curVisible - newVisible;

		foreach(IRecentItem item, removeItems)
			removeItemIndex(item);

		foreach(IRecentItem item, addItems)
			createItemIndex(item);

		if (!addItems.isEmpty() || !removeItems.isEmpty())
			emit visibleItemsChanged();
	}
}

void RecentContacts::createItemIndex(const IRecentItem &AItem)
{
	IRosterIndex *index = FVisibleItems.value(AItem);
	if (index == NULL)
	{
		IRecentItemHandler *handler = FItemHandlers.value(AItem.type);
		if (handler)
		{
			index = FRostersModel->createRosterIndex(RIT_RECENT_ITEM,FRootIndex);
			index->setData(RDR_RECENT_TYPE,AItem.type);
			index->setData(RDR_STREAM_JID,AItem.streamJid.pFull());
			index->setData(RDR_RECENT_REFERENCE,AItem.reference);
			index->insertDataHolder(this);
			FRostersModel->insertRosterIndex(index,FRootIndex);

			FVisibleItems.insert(AItem,index);
			emit recentItemIndexCreated(AItem,index);

			updateItemProxy(AItem);
			updateItemIndex(AItem);
		}
	}
}

void RecentContacts::updateItemProxy(const IRecentItem &AItem)
{
	IRosterIndex *index = FVisibleItems.value(AItem);
	if (index)
	{
		IRecentItemHandler *handler = FItemHandlers.value(AItem.type);
		if (handler)
		{
			QList<IRosterIndex *> proxies = handler->recentItemProxyIndexes(AItem);
			FIndexProxies.insert(index,proxies);
			
			IRosterIndex *proxy = proxies.value(0);
			IRosterIndex *oldProxy = FIndexToProxy.value(index);
			if (oldProxy != proxy)
			{
				if (oldProxy)
					disconnect(oldProxy->instance());

				if (proxy)
				{
					FIndexToProxy.insert(index,proxy);
					FProxyToIndex.insert(proxy,index);
					connect(proxy->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),SLOT(onProxyIndexDataChanged(IRosterIndex *, int)));
				}
				else
				{
					FProxyToIndex.remove(FIndexToProxy.take(index));
				}
			}
		}
	}
}

void RecentContacts::updateItemIndex(const IRecentItem &AItem)
{
	static const QDateTime zero = QDateTime::fromTime_t(0);

	IRosterIndex *index = FVisibleItems.value(AItem);
	if (index)
	{
		IRecentItem item = findRealItem(AItem);
		IRosterIndex *proxy = FIndexToProxy.value(index);
		if (proxy == NULL)
		{
			IRecentItemHandler *handler = FItemHandlers.value(item.type);
			if (handler)
			{
				index->setData(Qt::DecorationRole, handler->recentItemIcon(item));
				index->setData(Qt::DisplayRole, handler->recentItemName(item));
			}
		}
		index->setData(RDR_RECENT_DATETIME,item.dateTime);
		index->setData(RDR_SORT_ORDER, (int)(item.favorite ? 0x80000000 : item.dateTime.secsTo(zero)));

		if (FRostersView)
		{
			if (item.favorite)
			{
				FRostersView->removeLabel(FInsertFavariteLabelId,index);
				FRostersView->insertLabel(FRemoveFavoriteLabelId,index);
			}
			else
			{
				FRostersView->removeLabel(FRemoveFavoriteLabelId,index);
				FRostersView->insertLabel(FInsertFavariteLabelId,index);
			}
		}
	}
}

void RecentContacts::removeItemIndex(const IRecentItem &AItem)
{
	IRosterIndex *index = FVisibleItems.take(AItem);
	if (index)
	{
		FIndexProxies.remove(index);
		FProxyToIndex.remove(FIndexToProxy.take(index));
		FRostersModel->removeRosterIndex(index);
		delete index->instance();
	}
}

void RecentContacts::mergeRecentItems(const QList<IRecentItem> &AItems)
{
	QSet<Jid> changedStreams;

	for (QList<IRecentItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		if (isItemValid(*it))
		{
			QList<IRecentItem> &curItems = FStreamItems[it->streamJid];
			int index = curItems.indexOf(*it);
			if (index >= 0)
			{
				IRecentItem &curItem = curItems[index];
				if (curItem.dateTime < it->dateTime)
				{
					curItem.dateTime = qMin(it->dateTime,QDateTime::currentDateTime());
					changedStreams += it->streamJid;
					emit recentItemChanged(curItem);
				}
			}
			else
			{
				IRecentItem curItem = *it;
				curItem.dateTime = qMin(curItem.dateTime,QDateTime::currentDateTime());
				curItems.append(curItem);
				changedStreams += it->streamJid;
				emit recentItemAdded(curItem);
			}
		}
	}

	foreach(Jid streamJid, changedStreams)
	{
		QList<IRecentItem> &curItems = FStreamItems[streamJid];
		qSort(curItems.begin(),curItems.end(),recentItemLessThen);

		int removeCount = curItems.count() - FMaxVisibleItems;
		for(int index = curItems.count()-1; removeCount>0 && index>=0; index--)
		{
			if (!curItems.at(index).favorite)
			{
				IRecentItem item = curItems.takeAt(index);
				emit recentItemRemoved(item);
				removeCount--;
			}
		}
	}

	if (!changedStreams.isEmpty())
		updateVisibleItems();
}

QString RecentContacts::recentFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_RECENT))
		dir.mkdir(DIR_RECENT);
	dir.cd(DIR_RECENT);
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

IRecentItem &RecentContacts::findRealItem(const IRecentItem &AItem)
{
	static IRecentItem nullItem;
	int index = FStreamItems.value(AItem.streamJid).indexOf(AItem);
	return index>=0 ? FStreamItems[AItem.streamJid][index] : nullItem;
}

IRecentItem RecentContacts::rosterIndexItem(const IRosterIndex *AIndex) const
{
	IRecentItem item;
	if (AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_AGENT)
	{
		item.type = REIT_CONTACT;
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_PREP_BARE_JID).toString();
	}
	else if (AIndex->type()==RIT_RECENT_ITEM)
	{
		item.type = AIndex->data(RDR_RECENT_TYPE).toString();
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_RECENT_REFERENCE).toString();
	}
	return item;
}

QList<IRosterIndex *> RecentContacts::sortItemProxies(const QList<IRosterIndex *> &AIndexes) const
{
	QList<IRosterIndex *> proxies;
	
	QMap<int, QMultiMap<int, IRosterIndex *> > order;
	for (int i=0; i<AIndexes.count(); i++)
	{
		const static int showOrders[] = {6,2,1,3,4,5,7,8};
		IRosterIndex *index = AIndexes.at(i);
		int showOrder = showOrders[index->data(RDR_SHOW).toInt()];
		int priority = index->data(RDR_PRIORITY).toInt();
		order[showOrder].insertMulti(-priority, index);
	}
	
	for(QMap<int, QMultiMap<int, IRosterIndex *> >::const_iterator it=order.constBegin(); it!=order.constEnd(); ++it)
		proxies += it->values();
	
	return proxies;
}

void RecentContacts::saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const
{
	for (QList<IRecentItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		QDomElement itemElem = AElement.ownerDocument().createElement("item");
		itemElem.setAttribute("type",it->type);
		itemElem.setAttribute("reference",it->reference);
		itemElem.setAttribute("favorite",it->favorite);
		itemElem.setAttribute("dateTime",DateTime(it->dateTime).toX85DateTime());
		AElement.appendChild(itemElem);
	}
}

QList<IRecentItem> RecentContacts::loadItemsFromXML(const Jid &AStreamJid, const QDomElement &AElement) const
{
	QList<IRecentItem> items;
	QDomElement itemElem = AElement.firstChildElement("item");
	while (!itemElem.isNull())
	{
		IRecentItem item;
		item.streamJid = AStreamJid;
		item.type = itemElem.attribute("type");
		item.reference = itemElem.attribute("reference");
		item.favorite = QVariant(itemElem.attribute("favorite")).toBool();
		item.dateTime = DateTime(itemElem.attribute("dateTime")).toLocal();
		items.append(item);
		itemElem = itemElem.nextSiblingElement("item");
	}
	return items;
}

void RecentContacts::saveItemsToFile(const QString &AFileName, const QList<IRecentItem> &AItems) const
{
	QFile file(AFileName);
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QDomDocument doc;
		QDomElement itemsElem = doc.appendChild(doc.createElementNS(PSN_RECENTCONTACTS,PST_RECENTCONTACTS)).toElement();
		saveItemsToXML(itemsElem,AItems);
		file.write(doc.toByteArray());
		file.close();
	}
}

QList<IRecentItem> RecentContacts::loadItemsFromFile(const Jid &AStreamJid, const QString &AFileName) const
{
	QList<IRecentItem> items;

	QFile file(AFileName);
	if (file.exists() && file.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(file.readAll()))
		{
			QDomElement itemsElem = doc.firstChildElement(PST_RECENTCONTACTS);
			items = loadItemsFromXML(AStreamJid,itemsElem);
		}
		file.close();
	}

	return items;
}

void RecentContacts::onRostersModelStreamAdded(const Jid &AStreamJid)
{
	if (FRostersModel && FStreamItems.isEmpty())
		FRostersModel->insertRosterIndex(FRootIndex,FRostersModel->rootIndex());

	FStreamItems[AStreamJid].clear();
	mergeRecentItems(loadItemsFromFile(AStreamJid,recentFileName(AStreamJid)));
}

void RecentContacts::onRostersModelStreamRemoved(const Jid &AStreamJid)
{
	saveItemsToFile(recentFileName(AStreamJid),FStreamItems.value(AStreamJid));
	FStreamItems.remove(AStreamJid);
	
	if (FRostersModel && FStreamItems.isEmpty())
		FRostersModel->removeRosterIndex(FRootIndex);
}

void RecentContacts::onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	QList<IRecentItem> items = FStreamItems.take(ABefore);
	for (QList<IRecentItem>::iterator it=items.begin(); it!=items.end(); ++it)
		it->streamJid = AAfter;
	FStreamItems.insert(AAfter,items);
}

void RecentContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->type() != RIT_RECENT_ITEM)
	{
		IRecentItem item = rosterIndexItem(AIndex);
		if (FVisibleItems.contains(item))
			emit recentItemUpdated(item);
	}
}

void RecentContacts::onRostersModelIndexRemoved(IRosterIndex *AIndex)
{
	onRostersModelIndexInserted(AIndex);
}

void RecentContacts::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	Q_UNUSED(ALabelId);
	if (AIndexes.count() == 1)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AIndexes.value(0));
		if (proxy != NULL)
			FRostersView->contextMenuForIndex(QList<IRosterIndex *>()<<proxy,NULL,AMenu);
	}
}

void RecentContacts::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMultiMap<int, QString> &AToolTips)
{
	Q_UNUSED(ALabelId);
	IRosterIndex *proxy = FIndexToProxy.value(AIndex);
	if (proxy != NULL)
		FRostersView->toolTipsForIndex(proxy,NULL,AToolTips);
}

void RecentContacts::onRostersViewNotifyInserted(int ANotifyId)
{
	QList<IRosterIndex *> indexes;
	
	foreach(IRosterIndex *proxy, FRostersView->notifyIndexes(ANotifyId))
	{
		if (!FIndexProxies.contains(proxy))
		{
			foreach(IRosterIndex *index, FIndexProxies.keys())
				if (FIndexProxies.value(index).contains(proxy))
					indexes.append(index);
		}
	}
	
	if (!indexes.isEmpty())
	{
		int notifyId = FRostersView->insertNotify(FRostersView->notifyById(ANotifyId),indexes);
		FProxyToIndexNotify.insert(ANotifyId,notifyId);
	}
}

void RecentContacts::onRostersViewNotifyRemoved(int ANotifyId)
{
	if (FProxyToIndexNotify.contains(ANotifyId))
	{
		FRostersView->removeNotify(FProxyToIndexNotify.take(ANotifyId));
	}
}

void RecentContacts::onRostersViewNotifyActivated(int ANotifyId)
{
	int notifyId = FProxyToIndexNotify.key(ANotifyId);
	if (notifyId > 0)
		FRostersView->activateNotify(notifyId);
}

void RecentContacts::onPrivateStorageOpened(const Jid &AStreamJid)
{
	FPrivateStorage->loadData(AStreamJid,PST_RECENTCONTACTS,PSN_RECENTCONTACTS);
}

void RecentContacts::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.tagName()==PST_RECENTCONTACTS && AElement.namespaceURI()==PSN_RECENTCONTACTS)
	{
		mergeRecentItems(loadItemsFromXML(AStreamJid,AElement));
	}
}

void RecentContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (ATagName==PST_RECENTCONTACTS && ANamespace==PSN_RECENTCONTACTS)
	{
		FPrivateStorage->loadData(AStreamJid,PST_RECENTCONTACTS,PSN_RECENTCONTACTS);
	}
}

void RecentContacts::onPrivateStorageAboutToClose(const Jid &AStreamJid)
{
	QDomDocument doc;
	QDomElement itemsElem = doc.appendChild(doc.createElementNS(PSN_RECENTCONTACTS,PST_RECENTCONTACTS)).toElement();
	saveItemsToXML(itemsElem,streamItems(AStreamJid));
	FPrivateStorage->saveData(AStreamJid,itemsElem);
}

void RecentContacts::onProxyIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	static const QList<int> updateItemRoles = QList<int>() << RDR_SHOW << RDR_PRIORITY;
	static const QList<int> updateDataRoles = QList<int>() << 0 << Qt::DecorationRole << Qt::DisplayRole;
	if (updateItemRoles.contains(ARole))
		emit recentItemUpdated(rosterIndexItem(AIndex));
	else if (updateDataRoles.contains(ARole))
		emit rosterDataChanged(FProxyToIndex.value(AIndex),ARole);
}

void RecentContacts::onHandlerRecentItemUpdated(const IRecentItem &AItem)
{
	IRecentItemHandler *handler = FItemHandlers.value(AItem.type);
	if (handler && handler->recentItemCanShow(AItem))
	{
		updateItemProxy(AItem);
		updateItemIndex(AItem);
	}
	else
	{
		updateVisibleItems();
	}
}

uint qHash(const IRecentItem &AKey)
{
	return qHash(AKey.type+"~"+AKey.streamJid.pFull()+"~"+AKey.reference);
}

Q_EXPORT_PLUGIN2(plg_recentcontacts, RecentContacts)
