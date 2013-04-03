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
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterindexkindorders.h>
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

#define MAX_STORAGE_CONTACTS         20
#define MIN_VISIBLE_CONTACTS         5

#define MAX_INACTIVE_TIMEOUT         31
#define MIX_INACTIVE_TIMEOUT         1

#define STORAGE_SAVE_TIMEOUT         100
#define STORAGE_FIRST_SAVE_TIMEOUT   5000

#define ADR_STREAM_JID               Action::DR_StreamJid
#define ADR_CONTACT_JID              Action::DR_UserDefined + 1
#define ADR_INDEX_TYPE               Action::DR_UserDefined + 2
#define ADR_RECENT_TYPE              Action::DR_UserDefined + 3
#define ADR_RECENT_REFERENCE         Action::DR_UserDefined + 4

bool recentItemLessThen(const IRecentItem &AItem1, const IRecentItem &AItem2) 
{
	bool favorite1 = AItem1.properties.value(REIP_FAVORITE).toBool();
	bool favorite2 = AItem2.properties.value(REIP_FAVORITE).toBool();
	return favorite1==favorite2 ? AItem1.activeTime>AItem2.activeTime : favorite1>favorite2;
}

RecentContacts::RecentContacts()
{
	FPrivateStorage = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FStatusIcons = NULL;
	FRostersViewPlugin = NULL;

	FRootIndex = NULL;
	FShowFavoriteLabelId = 0;
	
	FMaxVisibleItems = 20;
	FInactiveDaysTimeout = 7;
	FHideLaterContacts = true;
	FAllwaysShowOffline = true;
	FSimpleContactsView = true;
	FSortByLastActivity = true;
	FShowOnlyFavorite = false;

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
			connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRostersModelIndexDataChanged(IRosterIndex*, int)));
			connect(FRostersModel->instance(),SIGNAL(indexRemoving(IRosterIndex *)),SLOT(onRostersModelIndexRemoving(IRosterIndex *)));
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
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFROMRECENT,tr("Remove from recent contacts"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);

	if (FRostersView)
	{
		AdvancedDelegateItem showFavorite(RLID_RECENT_FAVORITE);
		showFavorite.d->kind = AdvancedDelegateItem::CustomData;
		showFavorite.d->data = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT_FAVORITE);
		FShowFavoriteLabelId = FRostersView->registerLabel(showFavorite);

		FRostersView->insertDragDropHandler(this);
		FRostersView->insertLabelHolder(RLHO_RECENT_FILTER,this);
		FRostersView->insertClickHooker(RCHO_RECENTCONTACTS,this);
		FRostersViewPlugin->registerExpandableRosterIndexKind(RIK_RECENT_ROOT,RDR_KIND);

		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_INSERTFAVORITE,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFAVORITE,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFROMRECENT,FRostersView->instance());
	}
	if (FRostersModel)
	{
		FRootIndex = FRostersModel->newRosterIndex(RIK_RECENT_ROOT);
		FRootIndex->setData(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RECENT),Qt::DecorationRole);
		FRootIndex->setData(RIKO_RECENT_ROOT,RDR_KIND_ORDER);
		FRootIndex->setData(tr("Recent Contacts"),RDR_NAME);

		FRostersModel->insertRosterDataHolder(RDHO_RECENTCONTACTS,this);
	}
	registerItemHandler(REIT_CONTACT,this);
	return true;
}

bool RecentContacts::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_HIDEINACTIVEITEMS,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_SIMPLEITEMSVIEW,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_SORTBYACTIVETIME,true);
	Options::setDefaultValue(OPV_ROSTER_RECENT_SHOWONLYFAVORITE,false);
	Options::setDefaultValue(OPV_ROSTER_RECENT_MAXVISIBLEITEMS,20);
	Options::setDefaultValue(OPV_ROSTER_RECENT_INACTIVEDAYSTIMEOUT,7);
	return true;
}

bool RecentContacts::startPlugin()
{
	return true;
}

QList<int> RecentContacts::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_RECENTCONTACTS)
	{
		return QList<int>() 
			<< Qt::DisplayRole << Qt::DecorationRole << Qt::ForegroundRole << Qt::BackgroundColorRole 
			<< RDR_NAME << RDR_RESOURCES << RDR_SHOW << RDR_STATUS 
			<< RDR_AVATAR_HASH << RDR_AVATAR_IMAGE << RDR_FORCE_VISIBLE;
	}
	return QList<int>();
}

QVariant RecentContacts::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder == RDHO_RECENTCONTACTS)
	{
		switch (AIndex->kind())
		{
		case RIK_RECENT_ROOT:
			{
				QPalette palette = FRostersView!=NULL ? FRostersView->instance()->palette() : QPalette();
				switch (ARole)
				{
				case Qt::ForegroundRole:
					return palette.color(QPalette::Active, QPalette::BrightText);
				case Qt::BackgroundColorRole:
					return palette.color(QPalette::Active, QPalette::Dark);
				case RDR_FORCE_VISIBLE:
					return 1;
				}
				break;
			}
		case RIK_RECENT_ITEM:
			{
				IRosterIndex *proxy = FIndexToProxy.value(AIndex);
				switch (ARole)
				{
				case RDR_SHOW:
					return proxy!=NULL ? proxy->data(ARole) : IPresence::Offline;
				case RDR_FORCE_VISIBLE:
					return (proxy!=NULL ? proxy->data(ARole).toInt() : 0) + (FAllwaysShowOffline ? 1 : 0);
				default:
					return proxy!=NULL ? proxy->data(ARole) : QVariant();
				}
				break;
			}
		}
	}
	return QVariant();
}

bool RecentContacts::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
	return false;
}

Qt::DropActions RecentContacts::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Qt::DropActions actions = Qt::IgnoreAction;
	if (AIndex->data(RDR_KIND).toInt()==RIK_RECENT_ITEM)
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
				operator<<(proxyStream,proxy->indexData());
				ADrag->mimeData()->setData(DDT_ROSTERSVIEW_INDEX_DATA,proxyData);

				QByteArray indexData;
				QDataStream indexStream(&indexData,QIODevice::WriteOnly);
				operator<<(indexStream,AIndex->indexData());
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
	if (AHover->data(RDR_KIND).toInt() == RIK_RECENT_ITEM)
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
	if (AIndex->data(RDR_KIND).toInt() == RIK_RECENT_ITEM)
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
	if (AOrder==RLHO_RECENT_FILTER && FSimpleContactsView && AIndex->kind()==RIK_RECENT_ITEM)
	{
		labels.append(RLID_AVATAR_IMAGE);
		labels.append(RLID_ROSTERSVIEW_STATUS);
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
	if (AOrder == RCHO_RECENTCONTACTS && AIndex->kind()==RIK_RECENT_ITEM)
	{
		QModelIndex modelIndex = FRostersView->mapFromModel(FRostersModel->modelIndexFromRosterIndex(AIndex));
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
	if (AOrder == RCHO_RECENTCONTACTS && AIndex->kind()==RIK_RECENT_ITEM)
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
	QString name = itemProperty(AItem,REIP_NAME).toString();
	return name.isEmpty() ? AItem.reference : name;
}

IRecentItem RecentContacts::recentItemForIndex(const IRosterIndex *AIndex) const
{
	IRecentItem item;
	if (AIndex->kind() == RIK_CONTACT)
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
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AItem.streamJid) : NULL;
	if (sroot)
	{
		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_KIND,RIK_CONTACT);
		findData.insertMulti(RDR_STREAM_JID,AItem.streamJid.pFull());
		findData.insertMulti(RDR_PREP_BARE_JID,AItem.reference);
		proxies = sroot->findChilds(findData,true);
		qSort(proxies.begin(),proxies.end());
	}
	return proxies;
}

bool RecentContacts::isReady(const Jid &AStreamJid) const
{
	return FPrivateStorage==NULL || FPrivateStorage->isLoaded(AStreamJid,PST_RECENTCONTACTS,PSN_RECENTCONTACTS);
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

QVariant RecentContacts::itemProperty(const IRecentItem &AItem, const QString &AName) const
{
	const IRecentItem &item = findRealItem(AItem);
	return item.properties.value(AName);
}

void RecentContacts::setItemProperty(const IRecentItem &AItem, const QString &AName, const QVariant &AValue)
{
	if (isReady(AItem.streamJid))
	{
		bool itemChanged = false;
		IRecentItem item = findRealItem(AItem);

		if (item.type.isEmpty())
		{
			itemChanged = true;
			item = AItem;
		}

		if (QVariant(AValue.type()) != AValue)
		{
			itemChanged = itemChanged || item.properties.value(AName).toString()!=AValue.toString();
			item.properties.insert(AName,AValue);
		}
		else if (item.properties.contains(AName))
		{
			itemChanged = true;
			item.properties.remove(AName);
		}
	
		if (itemChanged)
		{
			item.updateTime = QDateTime::currentDateTime();
			mergeRecentItems(item.streamJid, QList<IRecentItem>() << item, false);
			startSaveItemsToStorage(item.streamJid);
		}
	}
}

void RecentContacts::setItemActiveTime(const IRecentItem &AItem, const QDateTime &ATime)
{
	if (isReady(AItem.streamJid))
	{
		IRecentItem item = findRealItem(AItem);
		if (item.type.isEmpty())
		{
			item = AItem;
			item.activeTime = ATime;
			mergeRecentItems(item.streamJid,QList<IRecentItem>() << item, false);
			startSaveItemsToStorage(item.streamJid);
		}
		else if (item.activeTime < ATime)
		{
			item.activeTime = ATime;
			mergeRecentItems(item.streamJid, QList<IRecentItem>() << item, false);
		}
	}
}

void RecentContacts::removeItem(const IRecentItem &AItem)
{
	if (isReady(AItem.streamJid))
	{
		int index = FStreamItems.value(AItem.streamJid).indexOf(AItem);
		if (index >= 0)
		{
			QList<IRecentItem> newItems = FStreamItems.value(AItem.streamJid);
			newItems.removeAt(index);
			mergeRecentItems(AItem.streamJid,newItems,true);
			startSaveItemsToStorage(AItem.streamJid);
		}
	}
}

QList<IRecentItem> RecentContacts::visibleItems() const
{
	return FVisibleItems.keys();
}

IRecentItem RecentContacts::rosterIndexItem(const IRosterIndex *AIndex) const
{
	static IRecentItem nullItem;
	if (AIndex->kind() == RIK_RECENT_ITEM)
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
					if (it->properties.value(REIP_FAVORITE).toBool())
						favoriteCount++;
					common.append(*it);
				}
			}
		}
		qSort(common.begin(),common.end(),recentItemLessThen);

		QDateTime firstTime;
		for (QList<IRecentItem>::iterator it=common.begin(); it!=common.end(); )
		{
			if (it->properties.value(REIP_FAVORITE).toBool())
			{
				++it;
			}
			else if (FShowOnlyFavorite)
			{
				it = common.erase(it);
			}
			else if (FHideLaterContacts)
			{
				if (firstTime.isNull())
				{
					firstTime = it->activeTime;
					++it;
				}
				else if (it->activeTime.daysTo(firstTime) > FInactiveDaysTimeout)
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
			index = FRostersModel->newRosterIndex(RIK_RECENT_ITEM);
			index->setData(AItem.type,RDR_RECENT_TYPE);
			index->setData(AItem.streamJid.pFull(),RDR_STREAM_JID);
			index->setData(AItem.reference,RDR_RECENT_REFERENCE);
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
		bool favorite = item.properties.value(REIP_FAVORITE).toBool();

		IRosterIndex *proxy = FIndexToProxy.value(index);
		if (proxy == NULL)
		{
			IRecentItemHandler *handler = FItemHandlers.value(item.type);
			if (handler)
			{
				index->setData(handler->recentItemName(item),RDR_NAME);
				index->setData(handler->recentItemIcon(item),Qt::DecorationRole);
			}
		}
		index->setData(item.activeTime,RDR_RECENT_DATETIME);
		
		if (FSortByLastActivity)
			index->setData((int)(favorite ? 0x80000000 : item.activeTime.secsTo(zero)),RDR_SORT_ORDER);
		else
			index->setData(favorite ? QString("") : index->data(Qt::DisplayRole).toString(),RDR_SORT_ORDER);

		if (FRostersView)
		{
			if (favorite)
				FRostersView->insertLabel(FShowFavoriteLabelId,index);
			else
				FRostersView->removeLabel(FShowFavoriteLabelId,index);
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
				FProxyToIndex.remove(FIndexToProxy.take(index));
				if (proxy)
				{
					FIndexToProxy.insert(index,proxy);
					FProxyToIndex.insert(proxy,index);
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
		setItemProperty(AItem, REIP_NAME, proxy!=NULL ? proxy->data(RDR_NAME).toString() : index->data(RDR_NAME).toString());
	}
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

void RecentContacts::mergeRecentItems(const Jid &AStreamJid, const QList<IRecentItem> &AItems, bool AReplace)
{
	bool hasChanges = false;
	QSet<IRecentItem> newItems;
	QSet<IRecentItem> addedItems;
	QSet<IRecentItem> changedItems;
	QSet<IRecentItem> removedItems;

	QList<IRecentItem> &curItems = FStreamItems[AStreamJid];
	for (QList<IRecentItem>::const_iterator it=AItems.constBegin(); it!=AItems.constEnd(); ++it)
	{
		IRecentItem newItem = *it;
		newItem.streamJid = AStreamJid;

		if (isValidItem(newItem))
		{
			if (!newItem.activeTime.isValid() || newItem.activeTime > QDateTime::currentDateTime())
				newItem.activeTime = QDateTime::currentDateTime();
			if (!newItem.updateTime.isValid() || newItem.updateTime > QDateTime::currentDateTime())
				newItem.updateTime = QDateTime::currentDateTime();
			newItems += newItem;

			int index = curItems.indexOf(newItem);
			if (index >= 0)
			{
				IRecentItem &curItem = curItems[index];
				if (curItem.updateTime < newItem.updateTime)
				{
					curItem.updateTime = newItem.updateTime;
					curItem.properties = newItem.properties;
					changedItems += curItem;
					hasChanges = true;
				}
				if (curItem.activeTime < newItem.activeTime)
				{
					curItem.activeTime = newItem.activeTime;
					changedItems += curItem;
					hasChanges = true;
				}
			}
			else
			{
				curItems.append(newItem);
				addedItems += newItem;
				hasChanges = true;
			}
		}
	}

	if (AReplace)
	{
		removedItems += curItems.toSet()-newItems;
		foreach(const IRecentItem &item, removedItems)
		{
			curItems.removeAll(item);
			hasChanges = true;
		}
	}

	if (hasChanges)
	{
		qSort(curItems.begin(),curItems.end(),recentItemLessThen);

		int favoriteCount = 0;
		while(favoriteCount<curItems.count() && curItems.at(favoriteCount).properties.value(REIP_FAVORITE).toBool())
			favoriteCount++;

		int removeCount = curItems.count() - favoriteCount - MAX_STORAGE_CONTACTS;
		for(int index = curItems.count()-1; removeCount>0 && index>=0; index--)
		{
			if (!curItems.at(index).properties.value(REIP_FAVORITE).toBool())
			{
				removedItems += curItems.takeAt(index);
				removeCount--;
			}
		}

		updateVisibleItems();
	}

	foreach(const IRecentItem &item, addedItems)
		emit recentItemAdded(item);
 
	foreach(const IRecentItem &item, removedItems)
		emit recentItemRemoved(item);

	foreach(const IRecentItem &item, changedItems)
	{
		updateItemIndex(item);
		emit recentItemChanged(item);
	}
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
	if (FPrivateStorage && isReady(AStreamJid))
	{
		QDomDocument doc;
		QDomElement itemsElem = doc.appendChild(doc.createElementNS(PSN_RECENTCONTACTS,PST_RECENTCONTACTS)).toElement();
		saveItemsToXML(itemsElem,streamItems(AStreamJid));
		FPrivateStorage->saveData(AStreamJid,itemsElem);
		return true;
	}
	return false;
}

QString RecentContacts::recentFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_RECENT))
		dir.mkdir(DIR_RECENT);
	dir.cd(DIR_RECENT);
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

QList<IRecentItem> RecentContacts::loadItemsFromXML(const QDomElement &AElement) const
{
	QList<IRecentItem> items;
	QDomElement itemElem = AElement.firstChildElement("item");
	while (!itemElem.isNull())
	{
		IRecentItem item;
		item.type = itemElem.attribute("type");
		item.reference = itemElem.attribute("reference");
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

void RecentContacts::saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const
{
	for (QList<IRecentItem>::const_iterator itemIt=AItems.constBegin(); itemIt!=AItems.constEnd(); ++itemIt)
	{
		QDomElement itemElem = AElement.ownerDocument().createElement("item");
		itemElem.setAttribute("type",itemIt->type);
		itemElem.setAttribute("reference",itemIt->reference);
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

QList<IRecentItem> RecentContacts::loadItemsFromFile(const QString &AFileName) const
{
	QList<IRecentItem> items;

	QFile file(AFileName);
	if (file.exists() && file.open(QIODevice::ReadOnly))
	{
		QDomDocument doc;
		if (doc.setContent(file.readAll()))
		{
			QDomElement itemsElem = doc.firstChildElement(PST_RECENTCONTACTS);
			items = loadItemsFromXML(itemsElem);
		}
		file.close();
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

bool RecentContacts::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	foreach(IRosterIndex *index, ASelected)
		if (rosterIndexItem(index).type.isEmpty())
			return false;
	return !ASelected.isEmpty();
}

bool RecentContacts::isRecentSelectionAccepted(const QList<IRosterIndex *> &AIndexes) const
{
	foreach(IRosterIndex *index, AIndexes)
		if (index->kind() != RIK_RECENT_ITEM)
			return false;
	return true;
}

void RecentContacts::removeRecentItems(const QStringList &ATypes, const QStringList &AStreamJids, const QStringList &AReferences)
{
	for (int index=0; index<ATypes.count(); index++)
	{
		IRecentItem item;
		item.type = ATypes.value(index);
		item.streamJid = AStreamJids.value(index);
		item.reference = AReferences.value(index);
		removeItem(item);
	}
}

void RecentContacts::setItemsFavorite(bool AFavorite, const QStringList &ATypes, const QStringList &AStreamJids, const QStringList &AReferences)
{
	for (int index=0; index<ATypes.count(); index++)
	{
		IRecentItem item;
		item.type = ATypes.value(index);
		item.streamJid = AStreamJids.value(index);
		item.reference = AReferences.value(index);
		setItemProperty(item,REIP_FAVORITE,AFavorite);
	}
}

void RecentContacts::onRostersModelStreamAdded(const Jid &AStreamJid)
{
	if (FRostersModel && FStreamItems.isEmpty())
		FRostersModel->insertRosterIndex(FRootIndex,FRostersModel->rootIndex());

	FStreamItems[AStreamJid].clear();
	mergeRecentItems(AStreamJid,loadItemsFromFile(recentFileName(AStreamJid)),true);
}

void RecentContacts::onRostersModelStreamRemoved(const Jid &AStreamJid)
{
	saveItemsToFile(recentFileName(AStreamJid),FStreamItems.value(AStreamJid));
	FStreamItems.remove(AStreamJid);
	FSaveStreams -= AStreamJid;
	updateVisibleItems();
	
	if (FRostersModel && FStreamItems.isEmpty())
		FRootIndex->remove(false);
}

void RecentContacts::onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter)
{
	QList<IRecentItem> items = FStreamItems.take(ABefore);
	for (QList<IRecentItem>::iterator it=items.begin(); it!=items.end(); ++it)
	{
		IRosterIndex *index = FVisibleItems.take(*it);
		it->streamJid = AAfter;
		if (index)
		{
			index->setData(AAfter.pFull(),RDR_STREAM_JID);
			FVisibleItems.insert(*it,index);
		}
	}
	FStreamItems.insert(AAfter,items);
	
	if (FSaveStreams.contains(ABefore))
	{
		FSaveStreams -= ABefore;
		FSaveStreams += AAfter;
	}
}

void RecentContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->kind() != RIK_RECENT_ITEM)
	{
		IRecentItem item = recentItemForIndex(AIndex);
		if (FVisibleItems.contains(item))
			emit recentItemUpdated(item);
	}
}

void RecentContacts::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (FProxyToIndex.contains(AIndex))
	{
		static const QList<int> updateItemRoles = QList<int>() << RDR_SHOW << RDR_PRIORITY;
		static const QList<int> updateDataRoles = QList<int>() << Qt::DecorationRole << Qt::DisplayRole;
		static const QList<int> updatePropertiesRoles = QList<int>() << RDR_NAME;
		
		if (updateItemRoles.contains(ARole))
			emit recentItemUpdated(recentItemForIndex(AIndex));
		if (updateDataRoles.contains(ARole))
			emit rosterDataChanged(FProxyToIndex.value(AIndex),ARole);
		if (updatePropertiesRoles.contains(ARole))
			updateItemProperties(rosterIndexItem(AIndex));
	}
}

void RecentContacts::onRostersModelIndexRemoving(IRosterIndex *AIndex)
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
		mergeRecentItems(AStreamJid,loadItemsFromXML(AElement),true);
}

void RecentContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (ATagName==PST_RECENTCONTACTS && ANamespace==PSN_RECENTCONTACTS)
		FPrivateStorage->loadData(AStreamJid,PST_RECENTCONTACTS,PSN_RECENTCONTACTS);
}

void RecentContacts::onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid)
{
	saveItemsToStorage(AStreamJid);
}

void RecentContacts::onRostersViewIndexContextMenuAboutToShow()
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

void RecentContacts::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void RecentContacts::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	static bool blocked = false;
	if (!blocked && ALabelId==AdvancedDelegateItem::DisplayId)
	{
		QSet<Action *> recentActions;
		bool isMultiSelection = AIndexes.count()>1;
		if (!isMultiSelection && AIndexes.value(0)->kind()==RIK_RECENT_ROOT)
		{
			Action *hideInactive = new Action(AMenu);
			hideInactive->setText(tr("Hide Inactive Contacts"));
			hideInactive->setCheckable(true);
			hideInactive->setChecked(FHideLaterContacts);
			connect(hideInactive,SIGNAL(triggered()),SLOT(onChangeHideInactiveItems()));
			AMenu->addAction(hideInactive,AG_RVCM_RECENT_OPTIONS);
			recentActions += hideInactive;

			Action *showOffline = new Action(AMenu);
			showOffline->setText(tr("Always Show Offline Contacts"));
			showOffline->setCheckable(true);
			showOffline->setChecked(FAllwaysShowOffline);
			connect(showOffline,SIGNAL(triggered()),SLOT(onChangeAlwaysShowOfflineItems()));
			AMenu->addAction(showOffline,AG_RVCM_RECENT_OPTIONS);
			recentActions += showOffline;
			
			Action *simpleView = new Action(AMenu);
			simpleView->setText(tr("Simplify Contacts View"));
			simpleView->setCheckable(true);
			simpleView->setChecked(FSimpleContactsView);
			connect(simpleView,SIGNAL(triggered()),SLOT(onChangeSimpleContactsView()));
			AMenu->addAction(simpleView,AG_RVCM_RECENT_OPTIONS);
			recentActions += simpleView;
			
			Action *sortByActivity = new Action(AMenu);
			sortByActivity->setText(tr("Sort by Last Activity"));
			sortByActivity->setCheckable(true);
			sortByActivity->setChecked(FSortByLastActivity);
			connect(sortByActivity,SIGNAL(triggered()),SLOT(onChangeSortByLastActivity()));
			AMenu->addAction(sortByActivity,AG_RVCM_RECENT_OPTIONS);
			recentActions += sortByActivity;

			Action *showFavorite = new Action(AMenu);
			showFavorite->setText(tr("Show Only Favorite Contacts"));
			showFavorite->setCheckable(true);
			showFavorite->setChecked(FShowOnlyFavorite);
			connect(showFavorite,SIGNAL(triggered()),SLOT(onChangeShowOnlyFavorite()));
			AMenu->addAction(showFavorite,AG_RVCM_RECENT_OPTIONS);
			recentActions += showFavorite;
		}
		else if (isSelectionAccepted(AIndexes))
		{
			bool ready = true;

			QMap<int, QStringList> rolesMap;
			foreach(IRosterIndex *index, AIndexes)
			{
				IRecentItem item = rosterIndexItem(index);
				rolesMap[RDR_RECENT_TYPE].append(item.type);
				rolesMap[RDR_STREAM_JID].append(item.streamJid.full());
				rolesMap[RDR_RECENT_REFERENCE].append(item.reference);
				ready = ready && isReady(item.streamJid);
			}

			if (ready)
			{
				QHash<int,QVariant> data;
				data.insert(ADR_RECENT_TYPE,rolesMap.value(RDR_RECENT_TYPE));
				data.insert(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				data.insert(ADR_RECENT_REFERENCE,rolesMap.value(RDR_RECENT_REFERENCE));

				bool favorite = findRealItem(rosterIndexItem(AIndexes.value(0))).properties.value(REIP_FAVORITE).toBool();
				if (isMultiSelection || !favorite)
				{
					Action *insertFavorite = new Action(AMenu);
					insertFavorite->setText(tr("Add to Favorites"));
					insertFavorite->setIcon(RSR_STORAGE_MENUICONS,MNI_RECENT_INSERT_FAVORITE);
					insertFavorite->setData(data);
					insertFavorite->setShortcutId(SCT_ROSTERVIEW_INSERTFAVORITE);
					connect(insertFavorite,SIGNAL(triggered(bool)),SLOT(onInsertToFavoritesByAction()));
					AMenu->addAction(insertFavorite,AG_RVCM_RECENT_FAVORITES);
					recentActions += insertFavorite;

				}
				if (isMultiSelection || favorite)
				{
					Action *removeFavorite = new Action(AMenu);
					removeFavorite->setText(tr("Remove from Favorites"));
					removeFavorite->setIcon(RSR_STORAGE_MENUICONS,MNI_RECENT_REMOVE_FAVORITE);
					removeFavorite->setData(data);
					removeFavorite->setShortcutId(SCT_ROSTERVIEW_REMOVEFAVORITE);
					connect(removeFavorite,SIGNAL(triggered(bool)),SLOT(onRemoveFromFavoritesByAction()));
					AMenu->addAction(removeFavorite,AG_RVCM_RECENT_FAVORITES);
					recentActions += removeFavorite;
				}
				if (isRecentSelectionAccepted(AIndexes))
				{
					Action *removeRecent = new Action(AMenu);
					removeRecent->setText(tr("Remove from Recent Contacts"));
					removeRecent->setIcon(RSR_STORAGE_MENUICONS,MNI_RECENT_REMOVE_RECENT);
					removeRecent->setData(data);
					removeRecent->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMRECENT);
					connect(removeRecent,SIGNAL(triggered(bool)),SLOT(onRemoveFromRecentByAction()));
					AMenu->addAction(removeRecent,AG_RVCM_RECENT_FAVORITES);
					recentActions += removeRecent;
				}
			}

			if (isRecentSelectionAccepted(AIndexes))
			{
				QList<IRosterIndex *> proxies = indexesProxies(AIndexes,false);
				if (!proxies.isEmpty())
				{
					blocked = true;

					QSet<Action *> oldActions = AMenu->groupActions().toSet();
					FRostersView->contextMenuForIndex(proxies,NULL,AMenu);
					connect(AMenu,SIGNAL(aboutToShow()),SLOT(onRostersViewIndexContextMenuAboutToShow()));
					FProxyContextMenuActions[AMenu] = AMenu->groupActions().toSet() - oldActions + recentActions;

					blocked = false;
				}
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

void RecentContacts::onRemoveFromRecentByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeRecentItems(action->data(ADR_RECENT_TYPE).toStringList(),action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_RECENT_REFERENCE).toStringList());
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
			if (AId == SCT_ROSTERVIEW_REMOVEFROMRECENT)
			{
				bool storageOpened = true;
				QMap<int, QStringList> rolesMap;
				foreach(IRosterIndex *index, selectedIndexes)
				{
					IRecentItem item = rosterIndexItem(index);
					rolesMap[RDR_RECENT_TYPE].append(item.type);
					rolesMap[RDR_STREAM_JID].append(item.streamJid.full());
					rolesMap[RDR_RECENT_REFERENCE].append(item.reference);
					storageOpened = storageOpened && FPrivateStorage!=NULL && FPrivateStorage->isOpen(item.streamJid);
				}
				if (storageOpened)
					removeRecentItems(rolesMap.value(RDR_RECENT_TYPE),rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_RECENT_REFERENCE));
			}
			else
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
}

void RecentContacts::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_HIDEINACTIVEITEMS));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_SIMPLEITEMSVIEW));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_SORTBYACTIVETIME));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_SHOWONLYFAVORITE));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_MAXVISIBLEITEMS));
	onOptionsChanged(Options::node(OPV_ROSTER_RECENT_INACTIVEDAYSTIMEOUT));
}

void RecentContacts::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE)
	{
		FAllwaysShowOffline = ANode.value().toBool();
		foreach(IRosterIndex *index, FVisibleItems.values())
			emit rosterDataChanged(index,RDR_FORCE_VISIBLE);
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_HIDEINACTIVEITEMS)
	{
		FHideLaterContacts = ANode.value().toBool();
		updateVisibleItems();
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_SIMPLEITEMSVIEW)
	{
		FSimpleContactsView = ANode.value().toBool();
		rosterLabelChanged(RLID_AVATAR_IMAGE);
		rosterLabelChanged(RLID_ROSTERSVIEW_STATUS);
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_SORTBYACTIVETIME)
	{
		FSortByLastActivity = ANode.value().toBool();
		foreach(IRecentItem item, FVisibleItems.keys())
			updateItemIndex(item);
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_SHOWONLYFAVORITE)
	{
		FShowOnlyFavorite = ANode.value().toBool();
		updateVisibleItems();
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_MAXVISIBLEITEMS)
	{
		FMaxVisibleItems = qMin(qMax(ANode.value().toInt(),MIN_VISIBLE_CONTACTS),MAX_STORAGE_CONTACTS);
		updateVisibleItems();
	}
	else if (ANode.path() == OPV_ROSTER_RECENT_INACTIVEDAYSTIMEOUT)
	{
		FInactiveDaysTimeout = qMin(qMax(ANode.value().toInt(),MIX_INACTIVE_TIMEOUT),MAX_INACTIVE_TIMEOUT);
		updateVisibleItems();
	}
}

void RecentContacts::onChangeAlwaysShowOfflineItems()
{
	Options::node(OPV_ROSTER_RECENT_ALWAYSSHOWOFFLINE).setValue(!FAllwaysShowOffline);
}

void RecentContacts::onChangeHideInactiveItems()
{
	Options::node(OPV_ROSTER_RECENT_HIDEINACTIVEITEMS).setValue(!FHideLaterContacts);
}

void RecentContacts::onChangeSimpleContactsView()
{
	Options::node(OPV_ROSTER_RECENT_SIMPLEITEMSVIEW).setValue(!FSimpleContactsView);
}

void RecentContacts::onChangeSortByLastActivity()
{
	Options::node(OPV_ROSTER_RECENT_SORTBYACTIVETIME).setValue(!FSortByLastActivity);
}

void RecentContacts::onChangeShowOnlyFavorite()
{
	Options::node(OPV_ROSTER_RECENT_SHOWONLYFAVORITE).setValue(!FShowOnlyFavorite);
}

uint qHash(const IRecentItem &AKey)
{
	return qHash(AKey.type+"~"+AKey.streamJid.pFull()+"~"+AKey.reference);
}

Q_EXPORT_PLUGIN2(plg_recentcontacts, RecentContacts)
