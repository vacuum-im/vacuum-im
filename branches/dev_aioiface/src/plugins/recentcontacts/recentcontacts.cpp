#include "recentcontacts.h"

#include <QDir>
#include <QFile>
#include <QStyle>
#include <QPalette>
#include <QMouseEvent>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/shortcuts.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterindextypeorders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/recentitemtypes.h>
#include <definitions/recentitemproperties.h>
#include <utils/iconstorage.h>
#include <utils/datetime.h>
#include <utils/shortcuts.h>

#define DIR_RECENT                   "recent"

#define PST_RECENTCONTACTS           "recent"
#define PSN_RECENTCONTACTS           "vacuum:recent-contacts"

#define MIN_VISIBLE_ITEMS            5
#define STORAGE_SAVE_TIMEOUT         100
#define STORAGE_FIRST_SAVE_TIMEOUT   5000
#define LATER_CONTACTS_DAYS_DELTA    7

#define ADR_STREAM_JID               Action::DR_StreamJid
#define ADR_CONTACT_JID              Action::DR_UserDefined + 1
#define ADR_INDEX_TYPE               Action::DR_UserDefined + 2
#define ADR_RECENT_TYPE              Action::DR_UserDefined + 3
#define ADR_RECENT_REFERENCE         Action::DR_UserDefined + 4

bool recentItemLessThen(const IRecentItem &AItem1, const IRecentItem &AItem2) 
{
	return AItem1.favorite==AItem2.favorite ? AItem1.activeTime>AItem2.activeTime : AItem1.favorite>AItem2.favorite;
}

RecentContacts::RecentContacts()
{
	FPrivateStorage = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FStatusIcons = NULL;
	FRostersViewPlugin = NULL;

	FRootIndex = NULL;
	FShowFavariteLabelId = 0;
	FInsertFavariteLabelId = 0;
	FRemoveFavoriteLabelId = 0;
	
	FMaxVisibleItems = 20;
	FHideLaterContacts = true;
	FAllwaysShowOffline = true;
	FSimpleContactsView = true;

	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveItemsToStorageTimerTimeout()));
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
			connect(FPrivateStorage->instance(),SIGNAL(storageNotifyAboutToClose(const Jid &)),SLOT(onPrivateStorageNotifyAboutToClose(const Jid &)));
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
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
			connect(FRostersView->instance(), SIGNAL(indexToolTips(IRosterIndex*,quint32,QMap<int,QString>&)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex*,quint32,QMap<int,QString>&)));
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

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FPrivateStorage!=NULL;
}

bool RecentContacts::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_INSERTFAVORITE,tr("Add contact to favorites"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFAVORITE,tr("Remove contact from favorites"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);

	if (FRostersView)
	{
		AdvancedDelegateItem showFavorite(RLID_RECENT_FAVORITE);
		showFavorite.d->kind = AdvancedDelegateItem::CustomData;
		showFavorite.d->hideStates = QStyle::State_MouseOver;
		showFavorite.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT_FAVORITE);
		FShowFavariteLabelId = FRostersView->registerLabel(showFavorite);

		AdvancedDelegateItem insertFavorite(RLID_RECENT_INSERT_FAVORITE);
		insertFavorite.d->kind = AdvancedDelegateItem::CustomData;
		insertFavorite.d->showStates = QStyle::State_MouseOver;
		insertFavorite.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT_INSERT_FAVORITE);
		FInsertFavariteLabelId = FRostersView->registerLabel(insertFavorite);

		AdvancedDelegateItem removeFavorive(RLID_RECENT_REMOVE_FAVORITE);
		removeFavorive.d->kind = AdvancedDelegateItem::CustomData;
		removeFavorive.d->showStates = QStyle::State_MouseOver;
		removeFavorive.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT_REMOVE_FAVORITE);
		FRemoveFavoriteLabelId = FRostersView->registerLabel(removeFavorive);

		FRostersView->insertDragDropHandler(this);
		FRostersView->insertLabelHolder(RLHO_RECENT_FILTER,this);
		FRostersView->insertClickHooker(RCHO_RECENTCONTACTS,this);
		FRostersViewPlugin->registerExpandableRosterIndexType(RIT_RECENT_ROOT,RDR_TYPE);

		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_INSERTFAVORITE,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFAVORITE,FRostersView->instance());
	}
	if (FRostersModel)
	{
		FRootIndex = FRostersModel->createRosterIndex(RIT_RECENT_ROOT,FRostersModel->rootIndex());
		FRootIndex->setData(Qt::DecorationRole,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT));
		FRootIndex->setData(RDR_TYPE_ORDER,RITO_RECENT_ROOT);
		FRootIndex->setData(RDR_NAME,tr("Recent Contacts"));
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
	Options::setDefaultValue(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_HIDELATERCONTACTS,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_SIMPLECONTACTSVIEW,true);
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
	static const QList<int> roles = QList<int>() 
		<< Qt::DisplayRole << Qt::DecorationRole << Qt::ForegroundRole << Qt::BackgroundColorRole 
		<< RDR_NAME << RDR_SHOW << RDR_STATUS 
		<< RDR_AVATAR_HASH << RDR_AVATAR_IMAGE << RDR_ALLWAYS_VISIBLE;
	return roles;
}

QList<int> RecentContacts::rosterDataTypes() const
{
	static const QList<int> types = QList<int>() 
		<< RIT_RECENT_ROOT << RIT_RECENT_ITEM;
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
			case RDR_ALLWAYS_VISIBLE:
				return 1;
			}
			break;
		}
	case RIT_RECENT_ITEM:
		{
			IRosterIndex *proxy = FIndexToProxy.value(AIndex);
			switch (ARole)
			{
			case RDR_SHOW:
				return proxy!=NULL ? proxy->data(ARole) : IPresence::Offline;
			case RDR_ALLWAYS_VISIBLE:
				return (proxy!=NULL ? proxy->data(ARole).toInt() : 0) + (FAllwaysShowOffline ? 1 : 0);
			default:
				return proxy!=NULL ? proxy->data(ARole) : QVariant();
			}
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

Qt::DropActions RecentContacts::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Qt::DropActions actions = Qt::IgnoreAction;
	if (AIndex->data(RDR_TYPE).toInt()==RIT_RECENT_ITEM)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AIndex);
		if (proxy)
		{
			foreach(IRostersDragDropHandler *handler, FRostersView->dragDropHandlers())
				actions |= handler!=this ? handler->rosterDragStart(AEvent,proxy,ADrag) : Qt::IgnoreAction;

			if (actions != Qt::IgnoreAction)
			{
				QByteArray proxyData;
				QDataStream proxyStream(&proxyData,QIODevice::WriteOnly);
				operator<<(proxyStream,proxy->data());
				ADrag->mimeData()->setData(DDT_ROSTERSVIEW_INDEX_DATA,proxyData);

				QByteArray indexData;
				QDataStream indexStream(&indexData,QIODevice::WriteOnly);
				operator<<(indexStream,AIndex->data());
				ADrag->mimeData()->setData(DDT_RECENT_INDEX_DATA,indexData);
			}
		}
	}
	return actions;
}

bool RecentContacts::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	FExteredProxyDragHandlers.clear();
	foreach(IRostersDragDropHandler *handler, FRostersView->dragDropHandlers())
		if (handler!=this && handler->rosterDragEnter(AEvent))
			FExteredProxyDragHandlers.append(handler);
	return !FExteredProxyDragHandlers.isEmpty();
}

bool RecentContacts::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	FMovedProxyDragHandlers.clear();
	if (AHover->data(RDR_TYPE).toInt() == RIT_RECENT_ITEM)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AHover);
		if (proxy)
		{
			foreach(IRostersDragDropHandler *handler, FExteredProxyDragHandlers)
				if (handler!=this && handler->rosterDragMove(AEvent,proxy))
					FMovedProxyDragHandlers.append(handler);
		}
	}
	return !FMovedProxyDragHandlers.isEmpty();
}

void RecentContacts::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RecentContacts::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu)
{
	bool accepted = false;
	if (AIndex->data(RDR_TYPE).toInt() == RIT_RECENT_ITEM)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AIndex);
		if (proxy)
		{
			foreach(IRostersDragDropHandler *handler, FMovedProxyDragHandlers)
				if (handler!=this && handler->rosterDropAction(AEvent,proxy,AMenu))
					accepted = true;
		}
	}
	return accepted;
}

QList<quint32> RecentContacts::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder==RLHO_RECENT_FILTER && FSimpleContactsView && AIndex->type()==RIT_RECENT_ITEM)
	{
		labels.append(RLID_AVATAR_IMAGE);
		labels.append(RLID_SCHANGER_STATUS);
	}
	return labels;
}

AdvancedDelegateItem RecentContacts::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	Q_UNUSED(AOrder); Q_UNUSED(ALabelId); Q_UNUSED(AIndex);
	static AdvancedDelegateItem null = AdvancedDelegateItem();
	return null;
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

bool RecentContacts::recentItemValid(const IRecentItem &AItem) const
{
	return !AItem.reference.isEmpty() && AItem.streamJid.pBare()!=AItem.reference && !Jid(AItem.reference).node().isEmpty();
}

bool RecentContacts::recentItemCanShow(const IRecentItem &AItem) const
{
	Q_UNUSED(AItem);
	return true;
}

QIcon RecentContacts::recentItemIcon(const IRecentItem &AItem) const
{
	if (FStatusIcons)
		return FStatusIcons->iconByJid(AItem.streamJid,AItem.reference);
	return QIcon();
}

QString RecentContacts::recentItemName(const IRecentItem &AItem) const
{
	QString name = itemProperty(AItem,RIP_ITEM_NAME).toString();
	return name.isEmpty() ? AItem.reference : name;
}

IRecentItem RecentContacts::recentItemForIndex(const IRosterIndex *AIndex) const
{
	IRecentItem item;
	if (AIndex->type()==RIT_CONTACT)
	{
		item.type = REIT_CONTACT;
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_PREP_BARE_JID).toString();
	}
	return item;
}

QList<IRosterIndex *> RecentContacts::recentItemProxyIndexes(const IRecentItem &AItem) const
{
	QList<IRosterIndex *> proxies;
	IRosterIndex *root = FRostersModel!=NULL ? FRostersModel->streamRoot(AItem.streamJid) : NULL;
	if (root)
	{
		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_TYPE,RIT_CONTACT);
		findData.insertMulti(RDR_PREP_BARE_JID,AItem.reference);
		proxies =  sortItemProxies(root->findChilds(findData,true));
	}
	return proxies;
}

bool RecentContacts::isValidItem(const IRecentItem &AItem) const
{
	if (AItem.type.isEmpty())
		return false;
	if (!FStreamItems.contains(AItem.streamJid))
		return false;
	if (FItemHandlers.contains(AItem.type))
		return FItemHandlers.value(AItem.type)->recentItemValid(AItem);
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

QVariant RecentContacts::itemProperty(const IRecentItem &AItem, const QString &AName) const
{
	const IRecentItem &item = findRealItem(AItem);
	return item.properties.value(AName);
}

void RecentContacts::setItemProperty(const IRecentItem &AItem, const QString &AName, const QVariant &AValue)
{
	IRecentItem &item = findRealItem(AItem);
	QString strValue = AValue.toString();
	if (!item.type.isEmpty() && item.properties.value(AName).toString()!=strValue)
	{
		if (!strValue.isEmpty())
			item.properties.insert(AName,AValue);
		else
			item.properties.remove(AName);
		item.updateTime = QDateTime::currentDateTime();
		startSaveItemsToStorage(item.streamJid);
		emit recentItemChanged(item);
	}
}

void RecentContacts::setItemFavorite(const IRecentItem &AItem, bool AFavorite)
{
	IRecentItem item = findRealItem(AItem);
	if (item.type.isEmpty() && AFavorite)
	{
		item = AItem;
		item.favorite = true;
		mergeRecentItems(QList<IRecentItem>() << item);
		startSaveItemsToStorage(item.streamJid);
	}
	else if (!item.type.isEmpty() && item.favorite!=AFavorite)
	{
		item.favorite = AFavorite;
		item.updateTime = QDateTime::currentDateTime();
		mergeRecentItems(QList<IRecentItem>() << item);
		startSaveItemsToStorage(item.streamJid);
	}
}

void RecentContacts::setItemActiveTime(const IRecentItem &AItem, const QDateTime &ATime)
{
	IRecentItem item = findRealItem(AItem);
	if (item.type.isEmpty())
	{
		item = AItem;
		item.favorite = false;
		item.activeTime = ATime;
		mergeRecentItems(QList<IRecentItem>() << item);
	}
	else if (item.activeTime < ATime)
	{
		item.activeTime = ATime;
		mergeRecentItems(QList<IRecentItem>() << item);
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

IRecentItem RecentContacts::rosterIndexItem(const IRosterIndex *AIndex) const
{
	static IRecentItem nullItem;
	if (AIndex->type() == RIT_RECENT_ITEM)
	{
		IRecentItem item;
		item.type = AIndex->data(RDR_RECENT_TYPE).toString();
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_RECENT_REFERENCE).toString();
		return item;
	}
	else
	{
		foreach(IRecentItemHandler *handler, FItemHandlers)
		{
			IRecentItem item = handler->recentItemForIndex(AIndex);
			if (isValidItem(item))
				return item;
		}
	}
	return nullItem;
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
		int favoriteCount = 0;
		QList<IRecentItem> common;
		for (QMap<Jid, QList<IRecentItem> >::const_iterator stream_it=FStreamItems.constBegin(); stream_it!=FStreamItems.constEnd(); ++stream_it)
		{
			for (QList<IRecentItem>::const_iterator it = stream_it->constBegin(); it!=stream_it->constEnd(); ++it)
			{
				IRecentItemHandler *handler = FItemHandlers.value(it->type);
				if (handler!=NULL && handler->recentItemCanShow(*it))
				{
					if (it->favorite)
						favoriteCount++;
					common.append(*it);
				}
			}
		}
		qSort(common.begin(),common.end(),recentItemLessThen);

		if (FHideLaterContacts)
		{
			QDateTime firstTime;
			for (QList<IRecentItem>::iterator it=common.begin(); it!=common.end(); )
			{
				if (!it->favorite)
				{
					if (firstTime.isNull())
					{
						firstTime = it->activeTime;
						++it;
					}
					else if (firstTime.daysTo(it->activeTime) > LATER_CONTACTS_DAYS_DELTA)
					{
						it = common.erase(it);
					}
					else
					{
						++it;
					}
				}
				else
				{
					++it;
				}
			}
		}

		QSet<IRecentItem> curVisible = FVisibleItems.keys().toSet();
		QSet<IRecentItem> newVisible = common.mid(0,FMaxVisibleItems+favoriteCount).toSet();

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
				QString name = handler->recentItemName(item);
				index->setData(RDR_NAME, name);
				index->setData(Qt::DecorationRole, handler->recentItemIcon(item));
			}
		}
		index->setData(RDR_RECENT_DATETIME,item.activeTime);
		index->setData(RDR_SORT_ORDER, (int)(item.favorite ? 0x80000000 : item.activeTime.secsTo(zero)));

		if (FRostersView)
		{
			if (item.favorite)
			{
				FRostersView->insertLabel(FShowFavariteLabelId,index);
				FRostersView->removeLabel(FInsertFavariteLabelId,index);
				FRostersView->insertLabel(FRemoveFavoriteLabelId,index);
			}
			else
			{
				FRostersView->removeLabel(FShowFavariteLabelId,index);
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

void RecentContacts::updateItemProperties(const IRecentItem &AItem)
{
	IRosterIndex *index = FVisibleItems.value(AItem);
	if (index)
	{
		IRosterIndex *proxy = FIndexToProxy.value(index);
		setItemProperty(AItem, RIP_ITEM_NAME, proxy!=NULL ? proxy->data(RDR_NAME).toString() : index->data(RDR_NAME).toString());
	}
}

void RecentContacts::mergeRecentItems(const QList<IRecentItem> &AItems)
{
	QSet<Jid> changedStreams;
	QSet<IRecentItem> addedItems;
	QSet<IRecentItem> changedItems;
	QSet<IRecentItem> removedItems;

	for (QList<IRecentItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		IRecentItem newItem = *it;
		if (isValidItem(newItem))
		{
			QList<IRecentItem> &curItems = FStreamItems[it->streamJid];

			if (!newItem.activeTime.isValid() || newItem.activeTime > QDateTime::currentDateTime())
				newItem.activeTime = QDateTime::currentDateTime();
			if (!newItem.updateTime.isValid() || newItem.updateTime > QDateTime::currentDateTime())
				newItem.updateTime = QDateTime::currentDateTime();

			int index = curItems.indexOf(newItem);
			if (index >= 0)
			{
				IRecentItem &curItem = curItems[index];
				if (curItem.updateTime < newItem.updateTime)
				{
					curItem.favorite = newItem.favorite;
					curItem.updateTime = newItem.updateTime;
					curItem.properties = newItem.properties;
					changedItems += curItem;
					changedStreams += newItem.streamJid;
				}
				if (curItem.activeTime < newItem.activeTime)
				{
					curItem.activeTime = newItem.activeTime;
					changedItems += curItem;
					changedStreams += newItem.streamJid;
				}
			}
			else
			{
				curItems.append(newItem);
				addedItems += newItem;
				changedStreams += it->streamJid;
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
				removedItems += curItems.takeAt(index);
				removeCount--;
			}
		}
	}

	if (!changedStreams.isEmpty())
		updateVisibleItems();

	foreach(IRecentItem item, addedItems)
		emit recentItemAdded(item);
 
	foreach(IRecentItem item, removedItems)
		emit recentItemRemoved(item);

	foreach(IRecentItem item, changedItems)
	{
		updateItemIndex(item);
		emit recentItemChanged(item);
	}
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

IRecentItem RecentContacts::findRealItem(const IRecentItem &AItem) const
{
	static IRecentItem nullItem;
	int index = FStreamItems.value(AItem.streamJid).indexOf(AItem);
	return index>=0 ? FStreamItems[AItem.streamJid].value(index) : nullItem;
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

QList<IRosterIndex *> RecentContacts::indexesProxies(const QList<IRosterIndex *> &AIndexes, bool AExclusive) const
{
	QList<IRosterIndex *> proxies;
	foreach(IRosterIndex *index, AIndexes)
	{
		if (FIndexToProxy.contains(index))
			proxies.append(FIndexToProxy.value(index));
		else if (!AExclusive)
			proxies.append(index);
	}
	return proxies;
}

void RecentContacts::startSaveItemsToStorage(const Jid &AStreamJid)
{
	if (FPrivateStorage)
	{
		FSaveTimer.start(STORAGE_SAVE_TIMEOUT);
		FSaveStreams += AStreamJid;
	}
}

bool RecentContacts::saveItemsToStorage(const Jid &AStreamJid)
{
	if (FPrivateStorage && FPrivateStorage->isOpen(AStreamJid))
	{
		QDomDocument doc;
		QDomElement itemsElem = doc.appendChild(doc.createElementNS(PSN_RECENTCONTACTS,PST_RECENTCONTACTS)).toElement();
		saveItemsToXML(itemsElem,streamItems(AStreamJid));
		FPrivateStorage->saveData(AStreamJid,itemsElem);
		return true;
	}
	return false;
}

void RecentContacts::saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const
{
	for (QList<IRecentItem>::const_iterator itemIt=AItems.constBegin(); itemIt!=AItems.constEnd(); ++itemIt)
	{
		QDomElement itemElem = AElement.ownerDocument().createElement("item");
		itemElem.setAttribute("type",itemIt->type);
		itemElem.setAttribute("reference",itemIt->reference);
		itemElem.setAttribute("favorite",itemIt->favorite);
		itemElem.setAttribute("activeTime",DateTime(itemIt->activeTime).toX85DateTime());
		itemElem.setAttribute("updateTime",DateTime(itemIt->updateTime).toX85DateTime());
		
		for (QMap<QString, QVariant>::const_iterator propIt=itemIt->properties.constBegin(); propIt!=itemIt->properties.constEnd(); ++propIt)
		{
			QDomElement propElem = AElement.ownerDocument().createElement("property");
			propElem.setAttribute("name",propIt.key());
			propElem.appendChild(AElement.ownerDocument().createTextNode(propIt->toString()));
			itemElem.appendChild(propElem);
		}
		
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
		item.activeTime = DateTime(itemElem.attribute("activeTime")).toLocal();
		item.updateTime = DateTime(itemElem.attribute("updateTime")).toLocal();

		QDomElement propElem = itemElem.firstChildElement("property");
		while(!propElem.isNull())
		{
			item.properties.insert(propElem.attribute("name"),propElem.text());
			propElem = propElem.nextSiblingElement("property");
		}
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

bool RecentContacts::isSelectionAccepted(const QList<IRosterIndex *> AIndexes) const
{
	foreach(IRosterIndex *index, AIndexes)
		if (rosterIndexItem(index).type.isEmpty())
			return false;
	return true;
}

bool RecentContacts::isRecentSelectionAccepted(const QList<IRosterIndex *> AIndexes) const
{
	foreach(IRosterIndex *index, AIndexes)
		if (index->type() != RIT_RECENT_ITEM)
			return false;
	return true;
}

void RecentContacts::setItemsFavorite(bool AFavorite, const QStringList &ATypes, const QStringList &AStreamJids, const QStringList &AReferences)
{
	for (int index=0; index<ATypes.count(); index++)
	{
		IRecentItem item;
		item.type = ATypes.value(index);
		item.streamJid = AStreamJids.value(index);
		item.reference = AReferences.value(index);
		setItemFavorite(item,AFavorite);
	}
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
	FSaveStreams -= AStreamJid;
	updateVisibleItems();
	
	if (FRostersModel && FStreamItems.isEmpty())
		FRostersModel->removeRosterIndex(FRootIndex);
}

void RecentContacts::onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	QList<IRecentItem> items = FStreamItems.take(ABefore);
	for (QList<IRecentItem>::iterator it=items.begin(); it!=items.end(); ++it)
		it->streamJid = AAfter;
	FStreamItems.insert(AAfter,items);
	
	if (FSaveStreams.contains(ABefore))
	{
		FSaveStreams -= ABefore;
		FSaveStreams += AAfter;
	}
}

void RecentContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->type() != RIT_RECENT_ITEM)
	{
		IRecentItem item = recentItemForIndex(AIndex);
		if (FVisibleItems.contains(item))
			emit recentItemUpdated(item);
	}
}

void RecentContacts::onRostersModelIndexRemoved(IRosterIndex *AIndex)
{
	onRostersModelIndexInserted(AIndex);
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
		FSaveTimer.start(STORAGE_FIRST_SAVE_TIMEOUT);
	}
}

void RecentContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (ATagName==PST_RECENTCONTACTS && ANamespace==PSN_RECENTCONTACTS)
	{
		FPrivateStorage->loadData(AStreamJid,PST_RECENTCONTACTS,PSN_RECENTCONTACTS);
	}
}

void RecentContacts::onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid)
{
	saveItemsToStorage(AStreamJid);
}

void RecentContacts::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void RecentContacts::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	static bool blocked = false;
	if (!blocked && ALabelId==AdvancedDelegateItem::DisplayId)
	{
		if (!FRostersView->hasMultiSelection() && AIndexes.value(0)->type()==RIT_RECENT_ROOT)
		{
			Action *hideLater = new Action(AMenu);
			hideLater->setText(tr("Hide Later Contacts"));
			hideLater->setCheckable(true);
			hideLater->setChecked(FHideLaterContacts);
			connect(hideLater,SIGNAL(triggered()),SLOT(onChangeHideLaterContacts()));
			AMenu->addAction(hideLater,AG_DEFAULT,true);

			Action *showOffline = new Action(AMenu);
			showOffline->setText(tr("Always Show Offline Contacts"));
			showOffline->setCheckable(true);
			showOffline->setChecked(FAllwaysShowOffline);
			connect(showOffline,SIGNAL(triggered()),SLOT(onChangeAlwaysShowOfflineContacts()));
			AMenu->addAction(showOffline,AG_DEFAULT,true);
			
			Action *simpleView = new Action(AMenu);
			simpleView->setText(tr("Simplify Contacts View"));
			simpleView->setCheckable(true);
			simpleView->setChecked(FSimpleContactsView);
			connect(simpleView,SIGNAL(triggered()),SLOT(onChangeSimpleContactsView()));
			AMenu->addAction(simpleView,AG_DEFAULT,true);
		}
		else if (isSelectionAccepted(AIndexes))
		{
			QMap<int, QStringList> rolesMap;
			foreach(IRosterIndex *index, AIndexes)
			{
				IRecentItem item = rosterIndexItem(index);
				rolesMap[RDR_RECENT_TYPE].append(item.type);
				rolesMap[RDR_STREAM_JID].append(item.streamJid.full());
				rolesMap[RDR_RECENT_REFERENCE].append(item.reference);
			}

			QHash<int,QVariant> data;
			data.insert(ADR_RECENT_TYPE,rolesMap.value(RDR_RECENT_TYPE));
			data.insert(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
			data.insert(ADR_RECENT_REFERENCE,rolesMap.value(RDR_RECENT_REFERENCE));

			bool favorite = findRealItem(rosterIndexItem(AIndexes.value(0))).favorite;
			if (FRostersView->hasMultiSelection() || !favorite)
			{
				Action *insertFavorite = new Action(AMenu);
				insertFavorite->setText(tr("Add to Favorites"));
				insertFavorite->setIcon(RSR_STORAGE_MENUICONS,MNI_RECENT_INSERT_FAVORITE);
				insertFavorite->setData(data);
				insertFavorite->setShortcutId(SCT_ROSTERVIEW_INSERTFAVORITE);
				connect(insertFavorite,SIGNAL(triggered(bool)),SLOT(onInsertToFavoritesByAction()));
				AMenu->addAction(insertFavorite,AG_RVCM_RECENT_FAVORITES);
			}
			if (FRostersView->hasMultiSelection() || favorite)
			{
				Action *removeFavorite = new Action(AMenu);
				removeFavorite->setText(tr("Remove from Favorites"));
				removeFavorite->setIcon(RSR_STORAGE_MENUICONS,MNI_RECENT_REMOVE_FAVORITE);
				removeFavorite->setData(data);
				removeFavorite->setShortcutId(SCT_ROSTERVIEW_REMOVEFAVORITE);
				connect(removeFavorite,SIGNAL(triggered(bool)),SLOT(onRemoveFromFavoritesByAction()));
				AMenu->addAction(removeFavorite,AG_RVCM_RECENT_FAVORITES);
			}

			if (isRecentSelectionAccepted(AIndexes))
			{
				blocked = true;
				QList<IRosterIndex *> proxies = indexesProxies(AIndexes,false);
				if (!proxies.isEmpty())
					FRostersView->contextMenuForIndex(proxies,NULL,AMenu);
				blocked = false;
			}
		}
	}
}

void RecentContacts::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		IRosterIndex *proxy = FIndexToProxy.value(AIndex);
		if (proxy != NULL)
			FRostersView->toolTipsForIndex(proxy,NULL,AToolTips);
	}
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

void RecentContacts::onProxyIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	static const QList<int> updateItemRoles = QList<int>() << RDR_SHOW << RDR_PRIORITY;
	static const QList<int> updateDataRoles = QList<int>() << 0 << Qt::DecorationRole << Qt::DisplayRole;
	static const QList<int> updatePropertiesRoles = QList<int>() << RDR_NAME;

	if (updateItemRoles.contains(ARole))
		emit recentItemUpdated(recentItemForIndex(AIndex));
	else if (updateDataRoles.contains(ARole))
		emit rosterDataChanged(FProxyToIndex.value(AIndex),ARole);
	else if (updatePropertiesRoles.contains(ARole))
		updateItemProperties(rosterIndexItem(AIndex));
}

void RecentContacts::onHandlerRecentItemUpdated(const IRecentItem &AItem)
{
	IRecentItemHandler *handler = FItemHandlers.value(AItem.type);
	if (handler && handler->recentItemCanShow(AItem))
	{
		updateItemProxy(AItem);
		updateItemIndex(AItem);
		updateItemProperties(AItem);
	}
	else
	{
		updateVisibleItems();
	}
}

void RecentContacts::onInsertToFavoritesByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		setItemsFavorite(true,action->data(ADR_RECENT_TYPE).toStringList(),action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_RECENT_REFERENCE).toStringList());
}

void RecentContacts::onRemoveFromFavoritesByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		setItemsFavorite(false,action->data(ADR_RECENT_TYPE).toStringList(),action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_RECENT_REFERENCE).toStringList());
}

void RecentContacts::onSaveItemsToStorageTimerTimeout()
{
	for (QSet<Jid>::iterator it=FSaveStreams.begin(); it!=FSaveStreams.end(); )
	{
		if (saveItemsToStorage(*it))
			it = FSaveStreams.erase(it);
		else
			it++;
	}
}

void RecentContacts::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersModel && FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> selectedIndexes = FRostersView->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_INSERTFAVORITE || AId==SCT_ROSTERVIEW_REMOVEFAVORITE)
		{
			if (isSelectionAccepted(selectedIndexes))
			{
				QMap<int, QStringList> rolesMap;
				foreach(IRosterIndex *index, selectedIndexes)
				{
					IRecentItem item = rosterIndexItem(index);
					rolesMap[RDR_RECENT_TYPE].append(item.type);
					rolesMap[RDR_STREAM_JID].append(item.streamJid.full());
					rolesMap[RDR_RECENT_REFERENCE].append(item.reference);
				}
				setItemsFavorite(AId==SCT_ROSTERVIEW_INSERTFAVORITE,rolesMap.value(RDR_RECENT_TYPE),rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_RECENT_REFERENCE));
			}
		}
		else if (isRecentSelectionAccepted(selectedIndexes))
		{
			QList<IRosterIndex *> selectedProxies = indexesProxies(selectedIndexes);
			if (!selectedProxies.isEmpty() && FRostersView->isSelectionAcceptable(selectedProxies))
			{
				FRostersView->setSelectedRosterIndexes(selectedProxies);
				Shortcuts::activateShortcut(AId,AWidget);
				FRostersView->setSelectedRosterIndexes(selectedIndexes);
			}
		}
	}
}

void RecentContacts::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_HIDELATERCONTACTS));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_SIMPLECONTACTSVIEW));
}

void RecentContacts::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE)
	{
		FAllwaysShowOffline = ANode.value().toBool();
		foreach(IRosterIndex *index, FVisibleItems.values())
			emit rosterDataChanged(index,RDR_ALLWAYS_VISIBLE);
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_HIDELATERCONTACTS)
	{
		FHideLaterContacts = ANode.value().toBool();
		updateVisibleItems();
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_SIMPLECONTACTSVIEW)
	{
		FSimpleContactsView = ANode.value().toBool();
		rosterLabelChanged(RLID_AVATAR_IMAGE);
		rosterLabelChanged(RLID_SCHANGER_STATUS);
	}
}

void RecentContacts::onChangeAlwaysShowOfflineContacts()
{
	Options::node(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE).setValue(!FAllwaysShowOffline);
}

void RecentContacts::onChangeHideLaterContacts()
{
	Options::node(OPV_ROSTER_RECENT_HIDELATERCONTACTS).setValue(!FHideLaterContacts);
}

void RecentContacts::onChangeSimpleContactsView()
{
	Options::node(OPV_ROSTER_RECENT_SIMPLECONTACTSVIEW).setValue(!FSimpleContactsView);
}

uint qHash(const IRecentItem &AKey)
{
	return qHash(AKey.type+"~"+AKey.streamJid.pFull()+"~"+AKey.reference);
}

Q_EXPORT_PLUGIN2(plg_recentcontacts, RecentContacts)
