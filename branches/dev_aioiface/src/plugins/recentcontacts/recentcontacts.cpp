#include "recentcontacts.h"

#include <QDir>
#include <QFile>
#include <QStyle>
#include <QPalette>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdataholderorders.h>
#include <utils/iconstorage.h>
#include <utils/options.h>

#define DIR_RECENT                   "recent"

#define PST_RECENTCONTACTS           "recent"
#define PSN_RECENTCONTACTS           "vacuum:recent-contacts"

#define MAX_VISIBLE_ITEMS            30

bool recentItemLessThen(const IRecentItem &AItem1, const IRecentItem &AItem2) 
{
	return AItem1.favorite==AItem2.favorite ? AItem1.lastActionTime>AItem2.lastActionTime : AItem1.favorite>AItem2.favorite;
}

RecentContacts::RecentContacts()
{
	FPrivateStorage = NULL;
	FRostersModel = NULL;
	FRostersViewPlugin = NULL;

	FRootIndex = NULL;
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
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	return FPrivateStorage!=NULL;
}

bool RecentContacts::initObjects()
{
	if (FRostersModel)
	{
		FRootIndex = FRostersModel->createRosterIndex(RIT_RECENT_ROOT,FRostersModel->rootIndex());
		FRootIndex->setData(Qt::DisplayRole,tr("Recent Contacts"));
		FRootIndex->setData(Qt::DecorationRole,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENTCONTACTS_RECENT));
		FRootIndex->setData(RDR_STATES_FORCE_ON,QStyle::State_Children);
		FRootIndex->setRemoveOnLastChildRemoved(false);
		FRootIndex->setRemoveChildsOnRemoved(true);
		FRootIndex->setDestroyOnParentRemoved(true);
		FRootIndex->insertDataHolder(this);
		FRostersModel->insertRosterIndex(FRootIndex,FRostersModel->rootIndex());
	}
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
	static const QList<int> roles = QList<int>() << Qt::DisplayRole << Qt::ForegroundRole << Qt::BackgroundColorRole;
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
		switch (ARole)
		{
		case Qt::ForegroundRole:
			return FRostersViewPlugin->rostersView()->instance()->palette().color(QPalette::Active, QPalette::BrightText);
		case Qt::BackgroundColorRole:
			return FRostersViewPlugin->rostersView()->instance()->palette().color(QPalette::Active, QPalette::Dark);
		}
		break;
	case RIT_RECENT_ITEM:
		switch (ARole)
		{
		}
		break;
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

bool RecentContacts::isItemValid(const IRecentItem &AItem) const
{
	if (AItem.type.isEmpty())
		return false;
	if (!FStreamItems.contains(AItem.streamJid))
		return false;
	return true;
}

QList<IRecentItem> RecentContacts::visibleItems() const
{
	return FVisibleItems.keys();
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
	QList<IRecentItem> items = FStreamItems.value(AItem.streamJid);
	int index = items.indexOf(AItem);
	if (index>=0 && items.at(index).favorite!=AFavorite)
	{
		FStreamItems[AItem.streamJid][index].favorite = AFavorite;
		updateVisibleItems();
	}
}

void RecentContacts::setRecentItem(const IRecentItem &AItem, const QDateTime &ATime)
{
	IRecentItem item = AItem;
	item.lastActionTime = ATime;
	insertRecentItems(QList<IRecentItem>() << item);
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
			for (QList<IRecentItem>::const_iterator it = stream_it->constBegin(); it!=stream_it->constBegin(); ++it)
				if (FItemHandlers.contains(it->type))
					common.append(*it);
		qSort(common.begin(),common.end(),recentItemLessThen);

		QSet<IRecentItem> curVisible = FVisibleItems.keys().toSet();
		QSet<IRecentItem> newVisible = common.mid(0,MAX_VISIBLE_ITEMS).toSet();

		QSet<IRecentItem> addItems = newVisible - curVisible;
		QSet<IRecentItem> removeItems = curVisible - newVisible;

		foreach(IRecentItem item, removeItems)
			removeItemIndex(item);

		foreach(IRecentItem item, addItems)
			updateItemIndex(item);

		if (!addItems.isEmpty() || !removeItems.isEmpty())
			emit visibleItemsChanged();
	}
}

void RecentContacts::updateItemIndex(const IRecentItem &AItem)
{
	IRecentItemHandler *handler = FItemHandlers.value(AItem.type);
	if (handler)
	{
		IRosterIndex *index = FVisibleItems.value(AItem);
		if (index == NULL)
		{
			index = FRostersModel->createRosterIndex(RIT_RECENT_ITEM,FRootIndex);
			index->insertDataHolder(this);
			FRostersModel->insertRosterIndex(index,FRootIndex);
			FVisibleItems.insert(AItem,index);
		}
	}
}

void RecentContacts::removeItemIndex(const IRecentItem &AItem)
{
	IRosterIndex *index = FVisibleItems.take(AItem);
	if (index)
	{
		FRostersModel->removeRosterIndex(index);
		delete index->instance();
	}
}

void RecentContacts::insertRecentItems(const QList<IRecentItem> &AItems)
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
				if (curItem.lastActionTime < it->lastActionTime)
				{
					changedStreams += it->streamJid;
					curItem.lastActionTime = it->lastActionTime;
					emit recentItemChanged(curItem);
				}
			}
			else
			{
				changedStreams += it->streamJid;
				curItems.append(*it);
				emit recentItemAdded(*it);
			}
		}
	}

	foreach(Jid streamJid, changedStreams)
	{
		QList<IRecentItem> &curItems = FStreamItems[streamJid];
		qSort(curItems.begin(),curItems.end(),recentItemLessThen);

		int removeCount = curItems.count() - MAX_VISIBLE_ITEMS;
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

void RecentContacts::saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const
{
	for (QList<IRecentItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		QDomElement itemElem = AElement.ownerDocument().createElement("item");
		itemElem.setAttribute("type",it->type);
		itemElem.setAttribute("reference",it->reference);
		itemElem.setAttribute("lastActionTime",it->lastActionTime.toString(Qt::ISODate));
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
		item.lastActionTime = QDateTime::fromString(itemElem.attribute("lastActionTime"),Qt::ISODate);
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
	FStreamItems[AStreamJid] = loadItemsFromFile(AStreamJid,recentFileName(AStreamJid));
}

void RecentContacts::onRostersModelStreamRemoved(const Jid &AStreamJid)
{
	saveItemsToFile(recentFileName(AStreamJid),FStreamItems.value(AStreamJid));
	FStreamItems.remove(AStreamJid);
}

void RecentContacts::onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	QList<IRecentItem> items = FStreamItems.take(ABefore);
	for (QList<IRecentItem>::iterator it=items.begin(); it!=items.end(); ++it)
		it->streamJid = AAfter;
	FStreamItems.insert(AAfter,items);
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
		insertRecentItems(loadItemsFromXML(AStreamJid,AElement));
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

uint qHash(const IRecentItem &AKey)
{
	return qHash(AKey.type+"||"+AKey.streamJid.pFull()+"||"+AKey.reference);
}

Q_EXPORT_PLUGIN2(plg_recentcontacts, RecentContacts)
