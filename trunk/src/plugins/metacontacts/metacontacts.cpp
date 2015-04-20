#include "metacontacts.h"

#include <QDir>
#include <QMouseEvent>
#include <QInputDialog>
#include <QTextDocument>
#include <QItemEditorFactory>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/shortcuts.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterlabels.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterproxyorders.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindexkindorders.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosterlabelholderorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/rosteredithandlerorders.h>
#include <definitions/recentitemtypes.h>
#include <definitions/recentitemproperties.h>
#include <utils/widgetmanager.h>
#include <utils/shortcuts.h>
#include <utils/logger.h>

#define UPDATE_META_TIMEOUT     0
#define STORAGE_SAVE_TIMEOUT    100

#define DIR_METACONTACTS        "metacontacts"

#define ADR_STREAM_JID          Action::DR_StreamJid
#define ADR_CONTACT_JID         Action::DR_Parametr1
#define ADR_METACONTACT_ID      Action::DR_Parametr2
#define ADR_TO_GROUP            Action::DR_Parametr3
#define ADR_FROM_GROUP          Action::DR_Parametr4

static const IMetaContact NullMetaContact = IMetaContact();

static const QList<int> DragRosterKinds = QList<int>() << RIK_CONTACT << RIK_METACONTACT << RIK_METACONTACT_ITEM;
static const QList<int> DropRosterKinds = QList<int>() << RIK_GROUP << RIK_GROUP_BLANK << RIK_CONTACT << RIK_METACONTACT << RIK_METACONTACT_ITEM;

MetaContacts::MetaContacts()
{
	FPluginManager = NULL;
	FPrivateStorage = NULL;
	FRosterManager = NULL;
	FPresenceManager = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
	FStatusIcons = NULL;
	FMessageWidgets = NULL;
	FRecentContacts = NULL;

	FFilterProxyModel = new MetaSortFilterProxyModel(this,this);
	FFilterProxyModel->setDynamicSortFilter(true);

	FSaveTimer.setSingleShot(true);
	connect(&FSaveTimer,SIGNAL(timeout()),SLOT(onSaveContactsToStorageTimerTimeout()));

	FUpdateTimer.setSingleShot(true);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateContactsTimerTimeout()));
}

MetaContacts::~MetaContacts()
{
	delete FFilterProxyModel;
}

void MetaContacts::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Metacontacts");
	APluginInfo->description = tr("Allows to combine single contacts to metacontacts");
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
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageNotifyAboutToClose(const Jid &)),SLOT(onPrivateStorageNotifyAboutToClose(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorageClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
		if (FRosterManager)
		{
			connect(FRosterManager->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
			connect(FRosterManager->instance(),SIGNAL(rosterActiveChanged(IRoster *, bool)),SLOT(onRosterActiveChanged(IRoster *, bool)));
			connect(FRosterManager->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
			connect(FRosterManager->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0,NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if (FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(streamsLayoutChanged(int)),SLOT(onRostersModelStreamsLayoutChanged(int)));
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRostersModelIndexInserted(IRosterIndex *)));
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRostersModelIndexDestroyed(IRosterIndex *)));
			connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRostersModelIndexDataChanged(IRosterIndex *, int)));
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

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IMessageChatWindow *)),SLOT(onMessageChatWindowCreated(IMessageChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRecentContacts").value(0,NULL);
	if (plugin)
	{
		FRecentContacts = qobject_cast<IRecentContacts *>(plugin->instance());
		if (FRecentContacts)
		{
			connect(FRecentContacts->instance(),SIGNAL(recentContactsOpened(const Jid &)),SLOT(onRecentContactsOpened(const Jid &)));
			connect(FRecentContacts->instance(),SIGNAL(recentItemAdded(const IRecentItem &)),SLOT(onRecentItemChanged(const IRecentItem &)));
			connect(FRecentContacts->instance(),SIGNAL(recentItemChanged(const IRecentItem &)),SLOT(onRecentItemChanged(const IRecentItem &)));
			connect(FRecentContacts->instance(),SIGNAL(recentItemRemoved(const IRecentItem &)),SLOT(onRecentItemRemoved(const IRecentItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FRosterManager!=NULL && FPrivateStorage!=NULL;
}

bool MetaContacts::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_COMBINECONTACTS,tr("Combine contacts"),tr("Ctrl+M","Combine contacts"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_DESTROYMETACONTACT,tr("Destroy metacontact"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_DETACHFROMMETACONTACT,tr("Detach from metacontact"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);

	if (FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_METACONTACTS,this);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
		FRostersView->insertLabelHolder(RLHO_METACONTACTS,this);
		FRostersView->insertClickHooker(RCHO_METACONTACTS,this);
		FRostersView->insertEditHandler(REHO_METACONTACTS_RENAME,this);
		FRostersView->insertProxyModel(FFilterProxyModel,RPO_METACONTACTS_FILTER);

		FRostersViewPlugin->registerExpandableRosterIndexKind(RIK_METACONTACT,RDR_METACONTACT_ID,false);

		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_COMBINECONTACTS,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_DESTROYMETACONTACT,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_DETACHFROMMETACONTACT,FRostersView->instance());
	}
	if (FRecentContacts)
	{
		FRecentContacts->registerItemHandler(REIT_METACONTACT,this);
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
		static const QList<int> roles = QList<int>() << RDR_ALL_ROLES
			<< RDR_FULL_JID << RDR_PREP_FULL_JID << RDR_PREP_BARE_JID;
		return roles;
	}
	return QList<int>();
}

QVariant MetaContacts::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder == RDHO_METACONTACTS)
	{
		static bool labelsBlock = false;

		IRosterIndex *proxy = NULL;
		switch (AIndex->kind())
		{
		case RIK_METACONTACT:
			switch (ARole)
			{
			case Qt::DisplayRole:
			case RDR_STREAMS:
			case RDR_STREAM_JID:
			case RDR_NAME:
			case RDR_GROUP:
			case RDR_FORCE_VISIBLE:
			case RDR_METACONTACT_ID:
				break;
			case RDR_LABEL_ITEMS:
				proxy = !labelsBlock ? FMetaIndexToProxy.value(AIndex) : NULL;
				if (proxy)
				{
					labelsBlock = true;
					AdvancedDelegateItems metaLabels = AIndex->data(RDR_LABEL_ITEMS).value<AdvancedDelegateItems>();
					AdvancedDelegateItems proxyLabels = proxy->data(RDR_LABEL_ITEMS).value<AdvancedDelegateItems>();
					labelsBlock = false;

					for (AdvancedDelegateItems::const_iterator it=proxyLabels.constBegin(); it!=proxyLabels.constEnd(); ++it)
					{
						if (!metaLabels.contains(it.key()))
							metaLabels.insert(it.key(),it.value());
					}
					return QVariant::fromValue<AdvancedDelegateItems>(metaLabels);
				}
				break;
			default:
				proxy = FMetaIndexToProxy.value(AIndex);
			}
			break;
		case RIK_METACONTACT_ITEM:
			switch (ARole)
			{
			case RDR_STREAM_JID:
			case RDR_GROUP:
			case RDR_METACONTACT_ID:
				break;
			case RDR_FORCE_VISIBLE:
				return 1;
			default:
				proxy = FMetaItemIndexToProxy.value(AIndex);
			}
			break;
		}
		return proxy!=NULL ? proxy->data(ARole) : QVariant();
	}
	return QVariant();
}

bool MetaContacts::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
	return false;
}

QList<quint32> MetaContacts::rosterLabels(int AOrder, const IRosterIndex *AIndex) const
{
	QList<quint32> labels;
	if (AOrder==RLHO_METACONTACTS && AIndex->kind()==RIK_METACONTACT)
	{
		labels.append(RLID_METACONTACTS_BRANCH);
		labels.append(AdvancedDelegateItem::BranchId);
		labels.append(RLID_ROSTERSVIEW_RESOURCES);
	}
	return labels;
}

AdvancedDelegateItem MetaContacts::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
	if (AOrder==RLHO_METACONTACTS && AIndex->kind()==RIK_METACONTACT)
	{
		if (ALabelId==RLID_ROSTERSVIEW_RESOURCES && FRostersView!=NULL)
		{
			int resourceCount = 0;
			for(int row=0; row<AIndex->childCount(); row++)
			{
				IRosterIndex *metaItemIndex = AIndex->childIndex(row);
				if (metaItemIndex->kind() == RIK_METACONTACT_ITEM)
					resourceCount += metaItemIndex->data(RDR_RESOURCES).toStringList().count();
			}
			if (resourceCount > 1)
			{
				AdvancedDelegateItem label(ALabelId);
				label.d->kind = AdvancedDelegateItem::CustomData;
				label.d->data = QString("(%1)").arg(resourceCount);
				label.d->hints.insert(AdvancedDelegateItem::FontSizeDelta,-1);
				label.d->hints.insert(AdvancedDelegateItem::Foreground,FRostersView->instance()->palette().color(QPalette::Disabled, QPalette::Text));
				return label;
			}
		}
		else if (ALabelId == RLID_METACONTACTS_BRANCH)
		{
			AdvancedDelegateItem label(ALabelId);
			label.d->kind = AdvancedDelegateItem::Branch;
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
			if (labelId == RLID_METACONTACTS_BRANCH)
			{
				FRostersView->instance()->setExpanded(modelIndex,!FRostersView->instance()->isExpanded(modelIndex));
				return true;
			}

			IRosterIndex *proxy = FMetaIndexToProxy.value(AIndex);
			if (proxy)
				return FRostersView->singleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaItemIndexToProxy.value(AIndex);
			if (proxy)
				return FRostersView->singleClickOnIndex(proxy,AEvent);
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
			IRosterIndex *proxy = FMetaIndexToProxy.value(AIndex);
			if (proxy)
				return FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaItemIndexToProxy.value(AIndex);
			if (proxy)
				return FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
	}
	return false;
}

Qt::DropActions MetaContacts::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(ADrag);
	if (DragRosterKinds.contains(AIndex->kind()))
		return Qt::CopyAction|Qt::MoveAction;
	return Qt::IgnoreAction;
}

bool MetaContacts::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->source()==FRostersView->instance() && AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		return DragRosterKinds.contains(indexData.value(RDR_KIND).toInt());
	}
	return false;
}

bool MetaContacts::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	int hoverKind = AHover->kind();
	if (DropRosterKinds.contains(hoverKind) && (AEvent->dropAction() & (Qt::CopyAction|Qt::MoveAction))>0)
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);
		
		int indexKind = indexData.value(RDR_KIND).toInt();
		if (indexKind == RIK_METACONTACT)
		{
			QStringList indexStreams = indexData.value(RDR_STREAMS).toStringList();
			if (isReadyStreams(indexStreams))
			{
				if (hoverKind == RIK_METACONTACT)
				{
					QStringList hoverStreams = AHover->data(RDR_STREAMS).toStringList();
					return isReadyStreams(hoverStreams) && AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID);
				}
				else if (hoverKind==RIK_CONTACT || hoverKind==RIK_METACONTACT_ITEM)
				{
					Jid hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
					return isReady(hoverStreamJid) && AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID);
				}
				else if (hoverKind==RIK_GROUP || hoverKind==RIK_GROUP_BLANK)
				{
					return indexData.value(RDR_GROUP) != AHover->data(RDR_GROUP);
				}
			}
		}
		else if (indexKind==RIK_CONTACT || indexKind==RIK_METACONTACT_ITEM)
		{
			Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
			if (isReady(indexStreamJid))
			{
				if (hoverKind == RIK_METACONTACT)
				{
					QStringList hoverStreams = AHover->data(RDR_STREAMS).toStringList();
					return isReadyStreams(hoverStreams) && AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID);
				}
				else if (hoverKind == RIK_METACONTACT_ITEM)
				{
					Jid hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
					return isReady(hoverStreamJid) && AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID);
				}
				else if (hoverKind == RIK_CONTACT)
				{
					Jid hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
					return isReady(hoverStreamJid) && (indexStreamJid!=hoverStreamJid || AHover->data(RDR_PREP_BARE_JID)!=indexData.value(RDR_PREP_BARE_JID));
				}
			}
		}
	}
	return false;
}

void MetaContacts::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool MetaContacts::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AHover, Menu *AMenu)
{
	QMap<int, QVariant> indexData;
	QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
	operator>>(stream,indexData);

	int hoverKind = AHover->kind();
	int indexKind = indexData.value(RDR_KIND).toInt();

	if (indexKind==RIK_METACONTACT && (hoverKind==RIK_GROUP || hoverKind==RIK_GROUP_BLANK))
	{
		Action *groupAction = new Action(AMenu);
		groupAction->setData(ADR_STREAM_JID,indexData.value(RDR_STREAMS));
		groupAction->setData(ADR_METACONTACT_ID,indexData.value(RDR_METACONTACT_ID));
		groupAction->setData(ADR_TO_GROUP,AHover->data(RDR_GROUP).toString());

		if (AEvent->dropAction() == Qt::CopyAction)
		{
			groupAction->setText(tr("Copy Metacontact to Group"));
			groupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
			connect(groupAction,SIGNAL(triggered(bool)),SLOT(onCopyMetaContactToGroupByAction()));
		}
		else if (AEvent->dropAction() == Qt::MoveAction)
		{
			groupAction->setText(tr("Move Metacontact to Group"));
			groupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
			groupAction->setData(ADR_FROM_GROUP,QStringList()<<indexData.value(RDR_GROUP).toString());
			connect(groupAction,SIGNAL(triggered(bool)),SLOT(onMoveMetaContactToGroupByAction()));
		}

		AMenu->addAction(groupAction,AG_DEFAULT,true);
		return true;
	}
	else
	{
		QStringList streams, contacts, metas;
		
		if (indexKind != RIK_METACONTACT)
		{
			streams.append(indexData.value(RDR_STREAM_JID).toString());
			contacts.append(indexData.value(RDR_PREP_BARE_JID).toString());
			metas.append(indexData.value(RDR_METACONTACT_ID).toString());
		}
		else foreach(const QString &streamJid, indexData.value(RDR_STREAMS).toStringList())
		{
			streams.append(streamJid);
			contacts.append(indexData.value(RDR_PREP_BARE_JID).toString());
			metas.append(indexData.value(RDR_METACONTACT_ID).toString());
		}

		if (hoverKind != RIK_METACONTACT)
		{
			streams.append(AHover->data(RDR_STREAM_JID).toString());
			contacts.append(AHover->data(RDR_PREP_BARE_JID).toString());
			metas.append(AHover->data(RDR_METACONTACT_ID).toString());
		}
		else foreach(const QString &streamJid, AHover->data(RDR_STREAMS).toStringList())
		{
			streams.append(streamJid);
			contacts.append(AHover->data(RDR_PREP_BARE_JID).toString());
			metas.append(AHover->data(RDR_METACONTACT_ID).toString());
		}

		Action *combineAction = new Action(AMenu);
		combineAction->setText(tr("Combine Contacts..."));
		combineAction->setIcon(RSR_STORAGE_MENUICONS,MNI_METACONTACTS_COMBINE);
		combineAction->setData(ADR_STREAM_JID,streams);
		combineAction->setData(ADR_CONTACT_JID,contacts);
		combineAction->setData(ADR_METACONTACT_ID,metas);
		connect(combineAction,SIGNAL(triggered()),SLOT(onCombineMetaItemsByAction()));
		AMenu->addAction(combineAction,AG_DEFAULT,true);

		return true;
	}
	return false;
}

quint32 MetaContacts::rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const
{
	if (AOrder==REHO_METACONTACTS_RENAME && ADataRole==RDR_NAME && AIndex.data(RDR_KIND).toInt()==RIK_METACONTACT)
	{
		if (isReadyStreams(AIndex.data(RDR_STREAMS).toStringList()))
			return AdvancedDelegateItem::DisplayId;
	}
	return AdvancedDelegateItem::NullId;
}

AdvancedDelegateEditProxy *MetaContacts::rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex)
{
	if (AOrder==REHO_METACONTACTS_RENAME && ADataRole==RDR_NAME && AIndex.data(RDR_KIND).toInt()==RIK_METACONTACT)
		return this;
	return NULL;
}

bool MetaContacts::recentItemValid(const IRecentItem &AItem) const
{
	return FMetaContacts.contains(AItem.streamJid) ? FMetaContacts.value(AItem.streamJid).contains(AItem.reference) : true;
}

bool MetaContacts::recentItemCanShow(const IRecentItem &AItem) const
{
	return FMetaContacts.value(AItem.streamJid).contains(AItem.reference);
}

QIcon MetaContacts::recentItemIcon(const IRecentItem &AItem) const
{
	Q_UNUSED(AItem);
	return QIcon();
}

QString MetaContacts::recentItemName(const IRecentItem &AItem) const
{
	QString name = AItem.properties.value(REIP_NAME).toString();
	return !name.isEmpty() ? name : AItem.reference;
}

IRecentItem MetaContacts::recentItemForIndex(const IRosterIndex *AIndex) const
{
	IRecentItem item;
	if (AIndex->kind() == RIK_METACONTACT)
		item = FMetaRecentItems.value(getMetaIndexRoot(AIndex->data(RDR_STREAM_JID).toString())).value(AIndex->data(RDR_METACONTACT_ID).toString());
	return item;
}

QList<IRosterIndex *> MetaContacts::recentItemProxyIndexes(const IRecentItem &AItem) const
{
	return findMetaIndexes(AItem.streamJid,QUuid(AItem.reference));
}

bool MetaContacts::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex)
{
	Q_UNUSED(AModel);
	if (ADelegate->editRole() == RDR_NAME)
	{
		QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY);
		QByteArray propertyName = ADelegate->editorFactory()->valuePropertyName(value.type());
		QString newName = AEditor->property(propertyName).toString();
		QString oldName = AIndex.data(RDR_NAME).toString();
		if (!newName.isEmpty() && newName!=oldName)
		{
			QUuid metaId = AIndex.data(RDR_METACONTACT_ID).toString();
			foreach(const Jid &streamJid, AIndex.data(RDR_STREAMS).toStringList())
				setMetaContactName(streamJid,metaId,newName);
		}
		return true;
	}
	return false;
}

bool MetaContacts::isReady(const Jid &AStreamJid) const
{
	return FPrivateStorage==NULL || FPrivateStorage->isLoaded(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
}

QList<Jid> MetaContacts::findMetaStreams(const QUuid &AMetaId) const
{
	QList<Jid> streams;
	for (QMap<Jid, QHash<QUuid, IMetaContact> >::const_iterator it=FMetaContacts.constBegin(); it!=FMetaContacts.constEnd(); ++it)
	{
		if (it->contains(AMetaId))
			streams.append(it.key());
	}
	return streams;
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const Jid &AItemJid) const
{
	return findMetaContact(AStreamJid,FItemMetaId.value(AStreamJid).value(AItemJid.bare()));
}

IMetaContact MetaContacts::findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaContacts.value(AStreamJid).value(AMetaId,NullMetaContact);
}

QList<IRosterIndex *> MetaContacts::findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	return FMetaIndexes.value(getMetaIndexRoot(AStreamJid)).value(AMetaId);
}

bool MetaContacts::createMetaContact(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid) && !AMetaId.isNull())
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id!=AMetaId || meta.name!=AName || meta.items!=AItems)
		{
			meta.id = AMetaId;
			meta.name = AName;
			meta.items = AItems;
			if (updateMetaContact(AStreamJid,meta))
			{
				LOG_STRM_INFO(AStreamJid,QString("Metacontact created, metaId=%1, name=%2, items=%3").arg(meta.id.toString(),AName).arg(AItems.count()));
				startSaveContactsToStorage(AStreamJid);
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	else if (!AMetaId.isNull())
	{
		REPORT_ERROR("Failed to create metacontact: Stream is not ready");
	}
	else
	{
		REPORT_ERROR("Failed to create metacontact: Invalid parameters");
	}
	return false;
}

bool MetaContacts::setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName)
{
	if (isReady(AStreamJid) && !AMetaId.isNull())
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			if (meta.name != AName)
			{
				meta.name = AName;
				if (updateMetaContact(AStreamJid,meta))
				{
					LOG_STRM_INFO(AStreamJid,QString("Metacontact name changed, metaId=%1, name=%2").arg(AMetaId.toString(),AName));
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact name, metaId=%1: Metacontact not found").arg(AMetaId.toString()));
		}
	}
	else if (!AMetaId.isNull())
	{
		REPORT_ERROR("Failed to change metacontact name: Stream is not ready");
	}
	else
	{
		REPORT_ERROR("Failed to change metacontact name: Invalid parameters");
	}
	return false;
}

bool MetaContacts::setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups)
{
	if (isReady(AStreamJid) && !AMetaId.isNull())
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			if (meta.groups != AGroups)
			{
				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
				if (roster && roster->isOpen())
				{
					QSet<QString> newGroups = AGroups - meta.groups;
					QSet<QString> oldGroups = meta.groups - AGroups;
					foreach(const Jid &item, meta.items)
					{
						IRosterItem ritem = roster->findItem(item);
						roster->setItem(ritem.itemJid,ritem.name,ritem.groups + newGroups - oldGroups);
					}
					LOG_STRM_INFO(AStreamJid,QString("Metacontact groups changed, metaId=%1, groups=%2").arg(AMetaId.toString()).arg(AGroups.count()));
					return true;
				}
				else
				{
					LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact groups, metaId=%1: Roster is not opened").arg(AMetaId.toString()));
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact groups, metaId=%1: Metacontact not found").arg(AMetaId.toString()));
		}
	}
	else if (!AMetaId.isNull())
	{
		REPORT_ERROR("Failed to change metacontact groups: Stream is not ready");
	}
	else
	{
		REPORT_ERROR("Failed to change metacontact groups: Invalid parameters");
	}
	return false;
}

bool MetaContacts::insertMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid) && !AMetaId.isNull())
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			int insertCount = 0;
			foreach(const Jid &item, AItems)
			{
				if (!meta.items.contains(item))
				{
					meta.items.append(item);
					insertCount++;
				}
			}

			if (insertCount > 0)
			{
				if (updateMetaContact(AStreamJid,meta))
				{
					LOG_STRM_INFO(AStreamJid,QString("Metacontact items inserted, metaId=%1, items=%2").arg(AMetaId.toString()).arg(insertCount));
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to insert metacontact items, metaId=%1: Metacontact not found").arg(AMetaId.toString()));
		}
	}
	else if (!AMetaId.isNull())
	{
		REPORT_ERROR("Failed to insert metacontact items: Stream is not ready");
	}
	else
	{
		REPORT_ERROR("Failed to insert metacontact items: Invalid parameters");
	}
	return false;
}

bool MetaContacts::removeMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid) && !AMetaId.isNull())
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			int removeCount = 0;
			foreach(const Jid &item, AItems)
				removeCount += meta.items.removeAll(item);

			if (removeCount > 0)
			{
				if (updateMetaContact(AStreamJid,meta))
				{
					LOG_STRM_INFO(AStreamJid,QString("Metacontact items removed, metaId=%1, items=%2").arg(AMetaId.toString()).arg(removeCount));
					startSaveContactsToStorage(AStreamJid);
					return true;
				}
			}
			else
			{
				return true;
			}
		}
		else
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to remove metacontact items, metaId=%1: Metacontact not found").arg(AMetaId.toString()));
		}
	}
	else if (!AMetaId.isNull())
	{
		REPORT_ERROR("Failed to remove metacontact items: Stream is not ready");
	}
	else
	{
		REPORT_ERROR("Failed to remove metacontact items: Invalid parameters");
	}
	return false;
}

IRosterIndex *MetaContacts::getMetaIndexRoot(const Jid &AStreamJid) const
{
	bool merged = FRostersModel!=NULL ? FRostersModel->streamsLayout()==IRostersModel::LayoutMerged : false;
	return FRostersModel!=NULL ? (!merged ? FRostersModel->streamIndex(AStreamJid) : FRostersModel->contactsRoot()) : NULL;
}

MetaMergedContact MetaContacts::getMergedContact(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	MetaMergedContact meta;

	bool merged = FRostersModel!=NULL ? FRostersModel->streamsLayout()==IRostersModel::LayoutMerged : false;
	for (QMap<Jid, QHash<QUuid, IMetaContact> >::const_iterator streamIt=FMetaContacts.constBegin(); streamIt!=FMetaContacts.constEnd(); ++streamIt)
	{
		if (merged || streamIt.key()==AStreamJid)
		{
			QHash<QUuid, IMetaContact>::const_iterator metaIt = streamIt->find(AMetaId);
			if (metaIt != streamIt->constEnd())
			{
				meta.name = metaIt->name;
				meta.groups += metaIt->groups;
				foreach(const Jid &itemJid, metaIt->items)
					meta.items.insertMulti(streamIt.key(),itemJid);
				foreach(const IPresenceItem &pItem, metaIt->presences)
					meta.presences.insertMulti(streamIt.key(),pItem);
			}
		}
	}

	meta.id = AMetaId;
	if (!meta.presences.isEmpty())
	{
		meta.presence = FPresenceManager!=NULL ? FPresenceManager->sortPresenceItems(meta.presences.values()).value(0) : meta.presences.values().value(0);
		meta.stream = meta.presences.key(meta.presence);
		meta.itemJid = meta.presence.itemJid.bare();
	}
	else
	{
		QList<Jid> items = meta.items.values();
		qSort(items.begin(),items.end());
		meta.itemJid = items.value(0);
		meta.stream = meta.items.key(meta.itemJid);
	}
	
	return meta;
}

QList<IRecentItem> MetaContacts::findMetaRecentContacts(const Jid &AStreamJid, const QUuid &AMetaId) const
{
	QList<IRecentItem> items;
	MetaMergedContact meta = getMergedContact(AStreamJid,AMetaId);
	foreach(const Jid &streamJid, meta.items.uniqueKeys())
	{
		foreach(const IRecentItem &item, FRecentContacts->streamItems(streamJid))
		{
			if (item.type==REIT_CONTACT && FItemMetaId.value(item.streamJid).value(item.reference)==meta.id)
				items.append(item);
		}
	}
	return items;
}

void MetaContacts::updateMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId)
{
	IRosterIndex *sRoot = getMetaIndexRoot(AStreamJid);
	if (sRoot)
	{
		MetaMergedContact meta = getMergedContact(AStreamJid,AMetaId);
		if (!meta.items.isEmpty())
		{
			QList<IRosterIndex *> &metaIndexesRef = FMetaIndexes[sRoot][meta.id];

			QMap<QString, IRosterIndex *> groupMetaIndexMap;
			foreach(IRosterIndex *metaIndex, metaIndexesRef)
				groupMetaIndexMap.insert(metaIndex->data(RDR_GROUP).toString(),metaIndex);

			QSet<QString> metaGroups = !meta.groups.isEmpty() ? meta.groups : QSet<QString>()<<QString::null;
			QSet<QString> curGroups = groupMetaIndexMap.keys().toSet();
			QSet<QString> newGroups = metaGroups - curGroups;
			QSet<QString> oldGroups = curGroups - metaGroups;

			QSet<QString>::iterator oldGroupsIt = oldGroups.begin();
			foreach(const QString &group, newGroups)
			{
				IRosterIndex *groupIndex = FRostersModel->getGroupIndex(!group.isEmpty() ? RIK_GROUP : RIK_GROUP_BLANK,group,sRoot);

				IRosterIndex *metaIndex;
				if (oldGroupsIt == oldGroups.end())
				{
					metaIndex = FRostersModel->newRosterIndex(RIK_METACONTACT);
					metaIndexesRef.append(metaIndex);

					metaIndex->setData(meta.id.toString(),RDR_METACONTACT_ID);
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
				updateMetaIndexItems(metaIndex,MetaMergedContact());
				metaIndexesRef.removeAll(metaIndex);

				FRostersModel->removeRosterIndex(metaIndex);
				oldGroupsIt = oldGroups.erase(oldGroupsIt);
			}

			QStringList metaStreams;
			foreach(const Jid &streamJid, meta.items.uniqueKeys())
				metaStreams.append(streamJid.pFull());

			foreach(IRosterIndex *metaIndex, metaIndexesRef)
			{
				metaIndex->setData(meta.name,RDR_NAME);
				metaIndex->setData(metaStreams,RDR_STREAMS);
				metaIndex->setData(meta.stream.pFull(),RDR_STREAM_JID);

				updateMetaIndexItems(metaIndex,meta);
			}
		}
		else foreach(IRosterIndex *metaIndex, FMetaIndexes[sRoot].take(meta.id))
		{
			updateMetaIndexItems(metaIndex,meta);
			FRostersModel->removeRosterIndex(metaIndex);
		}
	}
}

void MetaContacts::updateMetaIndexItems(IRosterIndex *AMetaIndex, const MetaMergedContact &AMergedContact)
{
	QMap<Jid, QMap<Jid, IRosterIndex *> > &metaItemsRef = FMetaIndexItems[AMetaIndex];
	QMap<Jid, QMap<Jid, IRosterIndex *> > oldMetaItems = metaItemsRef;

	for(QMultiMap<Jid, Jid>::const_iterator itemIt=AMergedContact.items.constBegin(); itemIt!=AMergedContact.items.constEnd(); ++itemIt)
	{
		QMap<IRosterIndex *, IRosterIndex *> proxyIndexMap;
		foreach(IRosterIndex *index, FRostersModel->findContactIndexes(itemIt.key(),itemIt.value()))
			proxyIndexMap.insert(index->parentIndex(),index);

		IRosterIndex *proxyIndex = !proxyIndexMap.isEmpty() ? proxyIndexMap.value(AMetaIndex->parentIndex(),proxyIndexMap.constBegin().value()) : NULL;
		if (proxyIndex != NULL)
		{
			IRosterIndex *itemIndex = oldMetaItems[itemIt.key()].take(itemIt.value());
			if (itemIndex == NULL)
			{
				itemIndex = FRostersModel->newRosterIndex(RIK_METACONTACT_ITEM);
				metaItemsRef[itemIt.key()].insert(itemIt.value(),itemIndex);

				itemIndex->setData(itemIt.key().pFull(),RDR_STREAM_JID);
				itemIndex->setData(AMergedContact.id.toString(),RDR_METACONTACT_ID);
			}
			itemIndex->setData(AMetaIndex->data(RDR_GROUP),RDR_GROUP);

			IRosterIndex *prevProxyIndex = FMetaItemIndexToProxy.value(itemIndex);
			if (proxyIndex != prevProxyIndex)
			{
				proxyIndex->setData(AMergedContact.id.toString(),RDR_METACONTACT_ID);
				
				FMetaItemProxyToIndex.remove(prevProxyIndex,itemIndex);
				FMetaItemIndexToProxy.insert(itemIndex,proxyIndex);
				FMetaItemProxyToIndex.insertMulti(proxyIndex,itemIndex);
			}

			if (itemIt.key()==AMergedContact.stream && itemIt.value()==AMergedContact.itemJid && FMetaIndexToProxy.value(AMetaIndex)!=itemIndex)
			{
				FMetaProxyToIndex.remove(FMetaIndexToProxy.take(AMetaIndex));
				FMetaIndexToProxy.insert(AMetaIndex,itemIndex);
				FMetaProxyToIndex.insert(itemIndex,AMetaIndex);
				emit rosterDataChanged(AMetaIndex,RDR_ANY_ROLE);
			}

			FRostersModel->insertRosterIndex(itemIndex,AMetaIndex);
		}
	}

	for(QMap<Jid, QMap<Jid, IRosterIndex *> >::const_iterator streamIt=oldMetaItems.constBegin(); streamIt!=oldMetaItems.constEnd(); ++streamIt)
	{
		QMap<Jid, IRosterIndex *> &metaItemsStreamRef = metaItemsRef[streamIt.key()];

		for(QMap<Jid, IRosterIndex *>::const_iterator itemIt=streamIt->constBegin(); itemIt!=streamIt->constEnd(); ++itemIt)
		{
			IRosterIndex *itemIndex = itemIt.value();

			if (FMetaProxyToIndex.contains(itemIndex))
				FMetaIndexToProxy.remove(FMetaProxyToIndex.take(itemIndex));

			IRosterIndex *itemProxyIndex = FMetaItemIndexToProxy.take(itemIndex);
			if (itemProxyIndex)
			{
				FMetaItemProxyToIndex.remove(itemProxyIndex,itemIndex);
				if (!FMetaItemProxyToIndex.contains(itemProxyIndex))
					itemProxyIndex->setData(QVariant(),RDR_METACONTACT_ID);
			}

			metaItemsStreamRef.remove(itemIt.key());
			FRostersModel->removeRosterIndex(itemIndex);
		}

		if (metaItemsStreamRef.isEmpty())
			metaItemsRef.remove(streamIt.key());
	}

	if (metaItemsRef.isEmpty())
		FMetaIndexItems.remove(AMetaIndex);
}

void MetaContacts::updateMetaWindows(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (FMessageWidgets)
	{
		IRosterIndex *sRoot = getMetaIndexRoot(AStreamJid);
		MetaMergedContact meta = getMergedContact(AStreamJid,AMetaId);
		IMessageChatWindow *chatWindow = FMetaChatWindows.value(sRoot).value(meta.id);

		for (QMultiMap<Jid, Jid>::const_iterator itemIt=meta.items.constBegin(); itemIt!=meta.items.constEnd(); ++itemIt)
		{
			IMessageChatWindow *window = FMessageWidgets->findChatWindow(itemIt.key(),itemIt.value());
			if (window!=NULL && window!=chatWindow)
			{
				if (chatWindow==NULL && window->address()->availAddresses(true).count()==1)
				{
					chatWindow = window;
					FMetaChatWindows[sRoot].insert(AMetaId,chatWindow);
					connect(chatWindow->instance(),SIGNAL(tabPageChanged()),SLOT(onMessageChatWindowChanged()));
					connect(chatWindow->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMessageChatWindowDestroyed()));
				}
				else
				{
					window->address()->removeAddress(itemIt.key(),itemIt.value());
				}
			}
		}

		if (chatWindow)
		{
			if (!meta.items.isEmpty())
			{
				QMultiMap<Jid,Jid> newItems;
				QMultiMap<Jid,Jid> oldItems = chatWindow->address()->availAddresses(true);

				for (QMultiMap<Jid, Jid>::const_iterator itemIt=meta.items.constBegin(); itemIt!=meta.items.constEnd(); ++itemIt)
				{
					if (oldItems.contains(itemIt.key(),itemIt.value()))
						oldItems.remove(itemIt.key(),itemIt.value());
					else
						newItems.insertMulti(itemIt.key(),itemIt.value());
				}

				for (QMultiMap<Jid, Jid>::const_iterator itemIt=newItems.constBegin(); itemIt!=newItems.constEnd(); ++itemIt)
					chatWindow->address()->appendAddress(itemIt.key(),itemIt.value());

				for (QMultiMap<Jid, Jid>::const_iterator itemIt=oldItems.constBegin(); itemIt!=oldItems.constEnd(); ++itemIt)
					chatWindow->address()->removeAddress(itemIt.key(),itemIt.value());

				if (chatWindow->tabPageCaption() != meta.name)
					chatWindow->updateWindow(chatWindow->tabPageIcon(),meta.name,tr("%1 - Chat").arg(meta.name),QString::null);
			}
			else
			{
				QMultiMap<Jid,Jid> oldItems = chatWindow->address()->availAddresses(true);
				oldItems.remove(chatWindow->streamJid(),chatWindow->contactJid().bare());

				for (QMultiMap<Jid, Jid>::const_iterator itemIt=oldItems.constBegin(); itemIt!=oldItems.constEnd(); ++itemIt)
					chatWindow->address()->removeAddress(itemIt.key(),itemIt.value());

				FMetaChatWindows[sRoot].remove(AMetaId);
				disconnect(chatWindow->instance(),SIGNAL(tabPageChanged()),this,SLOT(onMessageChatWindowChanged()));
				disconnect(chatWindow->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onMessageChatWindowDestroyed()));
			}
		}
	}
}

void MetaContacts::updateMetaRecentItems(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (FRecentContacts)
	{
		IRosterIndex *sRoot = getMetaIndexRoot(AStreamJid);
		MetaMergedContact meta = getMergedContact(AStreamJid,AMetaId);
		if (!meta.items.isEmpty())
		{
			int contactFavorite = 0;
			IRecentItem recentContact;
			QList<IRecentItem> recentMetas;
			foreach(const Jid &streamJid, meta.items.uniqueKeys())
			{
				foreach(const IRecentItem &item, FRecentContacts->streamItems(streamJid))
				{
					if (item.type == REIT_CONTACT)
					{
						if (meta.items.contains(streamJid,item.reference))
						{
							if (recentContact.reference.isEmpty())
								recentContact = item;
							else if (recentContact.activeTime < item.activeTime)
								recentContact = item;
							if (item.properties.value(REIP_FAVORITE).toBool())
								contactFavorite++;
						}
					}
					else if (item.type == REIT_METACONTACT)
					{
						if (meta.id == item.reference)
							recentMetas.append(item);
					}
				}
			}

			foreach(const IRecentItem &item, recentMetas)
			{
				if (recentContact.isNull() || recentContact.streamJid!=item.streamJid)
				{
					FUpdatingRecentItem = item;
					FMetaRecentItems[sRoot].remove(AMetaId);

					if (FRecentContacts->isReady(item.streamJid))
						FRecentContacts->removeItem(item);
					else
						emit recentItemUpdated(item);
				}
			}

			if (!recentContact.isNull())
			{
				IRecentItem item;
				item.type = REIT_METACONTACT;
				item.streamJid = recentContact.streamJid;
				item.reference = meta.id.toString();

				FUpdatingRecentItem = item;
				FMetaRecentItems[sRoot].insert(AMetaId,item);

				if (FRecentContacts->isReady(recentContact.streamJid))
				{
					FRecentContacts->setItemActiveTime(item,recentContact.activeTime);
					FRecentContacts->setItemProperty(item,REIP_FAVORITE,contactFavorite>0);
				}

				emit recentItemUpdated(item);
			}
		}
		else foreach(const IRecentItem &item, FRecentContacts->streamItems(AStreamJid))
		{
			if (item.type==REIT_METACONTACT && item.reference==meta.id)
			{
				FUpdatingRecentItem = item;
				FMetaRecentItems[sRoot].remove(AMetaId);

				if (FRecentContacts->isReady(item.streamJid))
					FRecentContacts->removeItem(item);
				else
					emit recentItemUpdated(item);

				break;
			}
		}
		FUpdatingRecentItem = IRecentItem();
	}
}

bool MetaContacts::updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster!=NULL && !AMetaContact.isNull())
	{
		IMetaContact after = AMetaContact;
		IMetaContact before = findMetaContact(AStreamJid,after.id);

		QHash<Jid, QUuid> &itemMetaIdRef = FItemMetaId[AStreamJid];
		IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(AStreamJid) : NULL;

		after.groups.clear();
		after.presences.clear();
		foreach(const Jid &itemJid, after.items)
		{
			IRosterItem rItem = roster->findItem(itemJid);
			if (!rItem.isNull() && !itemJid.node().isEmpty())
			{
				if (!before.items.contains(itemJid))
				{
					IMetaContact prevMeta = findMetaContact(AStreamJid,itemJid);
					if (!prevMeta.id.isNull())
					{
						foreach(const Jid &oldItemJid, after.items)
							prevMeta.items.removeAll(oldItemJid);
						updateMetaContact(AStreamJid,prevMeta);
					}
					itemMetaIdRef.insert(itemJid,after.id);
				}

				if (after.name.isEmpty() && !rItem.name.isEmpty())
					after.name = rItem.name;

				if (!rItem.groups.isEmpty())
					after.groups += rItem.groups;

				if (presence)
					after.presences += presence->findItems(itemJid);
			}
			else
			{
				after.items.removeAll(itemJid);
			}
		}

		if (after.name.isEmpty())
			after.name = after.items.value(0).uBare();
		if (!after.presences.isEmpty())
			after.presences = FPresenceManager->sortPresenceItems(after.presences);

		if (after != before)
		{
			QSet<Jid> oldMetaItems = before.items.toSet() - after.items.toSet();

			foreach(const Jid &itemJid, oldMetaItems)
				itemMetaIdRef.remove(itemJid);

			if (!after.isEmpty())
				FMetaContacts[AStreamJid][after.id] = after;
			else
				FMetaContacts[AStreamJid].remove(after.id);

			updateMetaIndexes(AStreamJid,after.id);
			updateMetaWindows(AStreamJid,after.id);
			updateMetaRecentItems(AStreamJid,after.id);

			emit metaContactChanged(AStreamJid,after,before);
			return true;
		}
		else
		{
			updateMetaIndexes(AStreamJid,after.id);
		}
	}
	else if (AMetaContact.isNull())
	{
		REPORT_ERROR("Failed to update metacontact: Invalid parameters");
	}
	return false;
}

void MetaContacts::updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts)
{
	QSet<QUuid> oldMetaId = FMetaContacts[AStreamJid].keys().toSet();

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
	FUpdateMeta[AStreamJid] += AMetaId;
	FUpdateTimer.start(UPDATE_META_TIMEOUT);
}

bool MetaContacts::isReadyStreams(const QStringList &AStreams) const
{
	foreach(const Jid &streamJid, AStreams)
		if (!isReady(streamJid))
			return false;
	return !AStreams.isEmpty();
}

bool MetaContacts::isValidItem(const Jid &AStreamJid, const Jid &AItemJid) const
{
	if (AItemJid.isValid() && !AItemJid.node().isEmpty())
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		if (roster != NULL)
			return roster->hasItem(AItemJid);
	}
	return false;
}

bool MetaContacts::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	foreach(IRosterIndex *index, ASelected)
	{
		int indexKind = index->kind();
		if (indexKind!=RIK_CONTACT && indexKind!=RIK_METACONTACT && indexKind!=RIK_METACONTACT_ITEM)
			return false;
		else if (indexKind==RIK_CONTACT && !isValidItem(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString()))
			return false;
	}
	return !ASelected.isEmpty();
}

bool MetaContacts::hasProxiedIndexes(const QList<IRosterIndex *> &AIndexes) const
{
	foreach(IRosterIndex *index, AIndexes)
	{
		if (FMetaIndexItems.contains(index))
			return true;
		else if (FMetaItemIndexToProxy.contains(index))
			return true;
	}
	return false;
}

QList<IRosterIndex *> MetaContacts::indexesProxies(const QList<IRosterIndex *> &AIndexes, bool ASelfProxy) const
{
	QList<IRosterIndex *> proxies;
	foreach(IRosterIndex *index, AIndexes)
	{
		if (FMetaIndexItems.contains(index))
		{
			for (int row=0; row<index->childCount(); row++)
				proxies.append(FMetaItemIndexToProxy.value(index->childIndex(row)));
		}
		else if (FMetaItemIndexToProxy.contains(index))
		{
			proxies.append(FMetaItemIndexToProxy.value(index));
		}
		else if (ASelfProxy)
		{
			proxies.append(index);
		}
	}
	proxies.removeAll(NULL);
	return proxies.toSet().toList();
}

QMap<int, QStringList> MetaContacts::indexesRolesMap(const QList<IRosterIndex *> &AIndexes, const QList<int> &ARoles) const
{
	QMap<int, QStringList> rolesMap;
	if (FRostersView)
	{
		rolesMap = FRostersView->indexesRolesMap(AIndexes,ARoles);
		for (int i=0; i<AIndexes.count(); i++)
		{
			IRosterIndex *metaIndex = AIndexes.at(i);
			if (metaIndex->kind() == RIK_METACONTACT)
			{
				foreach(const QString &streamJid, metaIndex->data(RDR_STREAMS).toStringList())
				{
					if (rolesMap.value(RDR_STREAM_JID).at(i) != streamJid)
					{
						foreach(int role, ARoles)
						{
							if (role != RDR_STREAM_JID)
								rolesMap[role].append(metaIndex->data(role).toString());
							else
								rolesMap[role].append(streamJid);
						}
					}
				}
			}
		}
	}
	return rolesMap;
}

void MetaContacts::renameMetaContact(const QStringList &AStreams, const QStringList &AMetas)
{
	if (isReadyStreams(AStreams) && !AStreams.isEmpty() && AStreams.count()==AMetas.count())
	{
		MetaMergedContact meta = getMergedContact(AStreams.value(0),AMetas.value(0));
		QString newName = QInputDialog::getText(NULL,tr("Rename Metacontact"),tr("Enter name:"),QLineEdit::Normal,meta.name);
		if (!newName.isEmpty() && newName!=meta.name)
		{
			for (int i=0; i<AStreams.count(); i++)
				setMetaContactName(AStreams.at(i),AMetas.at(i),newName);
		}
	}
}

void MetaContacts::removeMetaItems(const QStringList &AStreams, const QStringList &AContacts)
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
				removeMetaContactItems(streamIt.key(),metaIt.key(),metaIt.value());
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
				removeMetaContactItems(AStreams.at(i),meta.id,meta.items);
		}
	}
}

void MetaContacts::startSaveContactsToStorage(const Jid &AStreamJid)
{
	if (FPrivateStorage && isReady(AStreamJid))
	{
		FSaveStreams += AStreamJid;
		FSaveTimer.start(STORAGE_SAVE_TIMEOUT);
	}
	else if (FPrivateStorage)
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to start save metacontacts to storage: Stream not ready");
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
			LOG_STRM_INFO(AStreamJid,"Save metacontacts to storage request sent");
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send save metacontacts to storage request");
		}
	}
	else if (FPrivateStorage)
	{
		REPORT_ERROR("Failed to save metacontacts to storage: Stream not ready");
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
			REPORT_ERROR(QString("Failed to load metacontacts from file content: %1").arg(xmlError));
			file.remove();
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load metacontacts from file: %1").arg(file.errorString()));
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
		REPORT_ERROR(QString("Failed to save metacontacts to file: %1").arg(file.errorString()));
	}
}

void MetaContacts::onRosterOpened(IRoster *ARoster)
{
	QString id = FPrivateStorage!=NULL ? FPrivateStorage->loadData(ARoster->streamJid(),"storage",NS_STORAGE_METACONTACTS) : QString::null;
	if (!id.isEmpty())
	{
		FLoadRequestId[ARoster->streamJid()] = id;
		LOG_STRM_INFO(ARoster->streamJid(),"Load metacontacts from storage request sent");
	}
	else
	{
		LOG_STRM_WARNING(ARoster->streamJid(),"Failed to send load metacontacts from storage request");
	}
}

void MetaContacts::onRosterActiveChanged(IRoster *ARoster, bool AActive)
{
	if (AActive)
	{
		FLoadStreams += ARoster->streamJid();
		QTimer::singleShot(0,this,SLOT(onLoadContactsFromFileTimerTimeout()));
	}
	else
	{
		FSaveStreams -= ARoster->streamJid();
		FLoadStreams -= ARoster->streamJid();
		FUpdateMeta.remove(ARoster->streamJid());

		FItemMetaId.remove(ARoster->streamJid());

		QHash<QUuid, IMetaContact> metas = FMetaContacts.take(ARoster->streamJid());
		foreach(const QUuid &metaId, metas.keys())
		{
			updateMetaIndexes(ARoster->streamJid(),metaId);
			updateMetaRecentItems(ARoster->streamJid(),metaId);
		}

		saveMetaContactsToFile(metaContactsFileName(ARoster->streamJid()),metas.values());
	}
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
	FUpdateMeta.insert(ARoster->streamJid(),FUpdateMeta.take(ABefore));
	
	for (QHash<const IRosterIndex *, QMap<Jid, QMap<Jid, IRosterIndex *> > >::iterator it=FMetaIndexItems.begin(); it!=FMetaIndexItems.constEnd(); ++it)
		if (it->contains(ABefore))
			it->insert(ARoster->streamJid(),it->take(ABefore));

	FItemMetaId.insert(ARoster->streamJid(), FItemMetaId.take(ABefore));
	FMetaContacts.insert(ARoster->streamJid(), FMetaContacts.take(ABefore));
}

void MetaContacts::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	if (AItem.subscription!=ABefore.subscription || AItem.groups!=ABefore.groups)
	{
		QUuid metaId = FItemMetaId.value(ARoster->streamJid()).value(AItem.itemJid);
		if (!metaId.isNull())
			startUpdateMetaContact(ARoster->streamJid(),metaId);
	}
}

void MetaContacts::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (AItem.show!=ABefore.show || AItem.priority!=ABefore.priority || AItem.status!=ABefore.status)
	{
		QUuid metaId = FItemMetaId.value(APresence->streamJid()).value(AItem.itemJid.bare());
		if (!metaId.isNull())
			startUpdateMetaContact(APresence->streamJid(),metaId);
	}
}

void MetaContacts::onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	if (AElement.namespaceURI() == NS_STORAGE_METACONTACTS)
	{
		if (FLoadRequestId.value(AStreamJid) == AId)
		{
			FLoadRequestId.remove(AStreamJid);
			LOG_STRM_INFO(AStreamJid,"Metacontacts loaded from storage");
			updateMetaContacts(AStreamJid,loadMetaContactsFromXML(AElement));
			emit metaContactsOpened(AStreamJid);
		}
		else
		{
			LOG_STRM_INFO(AStreamJid,"Metacontacts reloaded from storage");
			updateMetaContacts(AStreamJid,loadMetaContactsFromXML(AElement));
		}
	}
}

void MetaContacts::onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	Q_UNUSED(ATagName);
	if (ANamespace == NS_STORAGE_METACONTACTS)
	{
		if (!FPrivateStorage->loadData(AStreamJid,ATagName,NS_STORAGE_METACONTACTS).isEmpty())
			LOG_STRM_INFO(AStreamJid,"Reload metacontacts from storage request sent");
		else
			LOG_STRM_WARNING(AStreamJid,"Failed to send reload metacontacts from storage request");
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

void MetaContacts::onPrivateStorageClosed(const Jid &AStreamJid)
{
	emit metaContactsClosed(AStreamJid);
}

void MetaContacts::onRostersModelStreamsLayoutChanged(int ABefore)
{
	Q_UNUSED(ABefore);
	for (QMap<const IRosterIndex *, QHash<QUuid, QList<IRosterIndex *> > >::iterator streamIt=FMetaIndexes.begin(); streamIt!=FMetaIndexes.end(); streamIt=FMetaIndexes.erase(streamIt))
	{
		for (QHash<QUuid, QList<IRosterIndex *> >::iterator metaIt = streamIt->begin(); metaIt!=streamIt->end(); metaIt=streamIt->erase(metaIt))
		{
			foreach(IRosterIndex *metaIndex, metaIt.value())
				FRostersModel->removeRosterIndex(metaIndex);
		}
	}
	
	FMetaRecentItems.clear();
	for (QMap<Jid, QHash<QUuid, IMetaContact> >::const_iterator streamIt=FMetaContacts.constBegin(); streamIt!=FMetaContacts.constEnd(); ++streamIt)
	{
		for(QHash<QUuid, IMetaContact>::const_iterator metaIt=streamIt->constBegin(); metaIt!=streamIt->constEnd(); ++metaIt)
		{
			updateMetaIndexes(streamIt.key(),metaIt.key());
			updateMetaRecentItems(streamIt.key(),metaIt.key());
		}
	}
	
	QList<IMessageChatWindow *> metaChatWindows;
	for(QMap<const IRosterIndex *, QHash<QUuid, IMessageChatWindow *> >::iterator streamIt=FMetaChatWindows.begin(); streamIt!=FMetaChatWindows.end(); streamIt=FMetaChatWindows.erase(streamIt))
		metaChatWindows += streamIt->values();

	foreach(IMessageChatWindow *window, metaChatWindows)
	{
		IMetaContact meta = findMetaContact(window->streamJid(),window->contactJid());
		if (!meta.isNull())
			updateMetaWindows(window->streamJid(),meta.id);
	}

	FUpdateMeta.clear();
}

void MetaContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->kind()==RIK_CONTACT && !FMetaItemProxyToIndex.contains(AIndex))
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		QUuid metaId = FItemMetaId.value(streamJid).value(contactJid);
		if (!metaId.isNull())
			startUpdateMetaContact(streamJid,metaId);
	}
}

void MetaContacts::onRostersModelIndexDestroyed(IRosterIndex *AIndex)
{
	if (AIndex->kind() == RIK_CONTACT)
	{
		for (QMultiHash<const IRosterIndex *, IRosterIndex *>::iterator it=FMetaItemProxyToIndex.find(AIndex); it!=FMetaItemProxyToIndex.end() && it.key()==AIndex; it=FMetaItemProxyToIndex.erase(it))
			FMetaItemIndexToProxy.remove(it.value());
	}
	else if (AIndex->kind() == RIK_METACONTACT_ITEM)
	{
		FMetaItemProxyToIndex.remove(FMetaItemIndexToProxy.take(AIndex),AIndex);

		QHash<const IRosterIndex *, QMap<Jid, QMap<Jid, IRosterIndex *> > >::iterator metaIt = FMetaIndexItems.find(AIndex->parentIndex());
		if (metaIt != FMetaIndexItems.end())
		{
			QMap<Jid, QMap<Jid, IRosterIndex *> >::iterator streamIt = metaIt->find(AIndex->data(RDR_STREAM_JID).toString());
			if (streamIt != metaIt->end())
				streamIt->remove(AIndex->data(RDR_PREP_BARE_JID).toString());
		}
	}
	else if (AIndex->kind() == RIK_METACONTACT)
	{
		FMetaIndexItems.remove(AIndex);
		FMetaProxyToIndex.remove(FMetaIndexToProxy.take(AIndex));

		QMap<const IRosterIndex *, QHash<QUuid, QList<IRosterIndex *> > >::iterator rootIt = FMetaIndexes.find(getMetaIndexRoot(AIndex->data(RDR_STREAM_JID).toString()));
		if (rootIt != FMetaIndexes.end())
		{
			QHash<QUuid, QList<IRosterIndex *> >::iterator metaIt = rootIt->find(AIndex->data(RDR_METACONTACT_ID).toString());
			if (metaIt != rootIt->end())
				metaIt->removeAll(AIndex);
		}
	}
	else if (AIndex->kind()==RIK_STREAM_ROOT || AIndex->kind()==RIK_CONTACTS_ROOT)
	{
		FMetaIndexes.remove(AIndex);
	}
}

void MetaContacts::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	IRosterIndex *metaIndex = FMetaProxyToIndex.value(AIndex);
	if (metaIndex != NULL)
		emit rosterDataChanged(metaIndex,ARole);
	else foreach(IRosterIndex *metaItemIndex, FMetaItemProxyToIndex.values(AIndex))
		emit rosterDataChanged(metaItemIndex,ARole);
}

void MetaContacts::onRostersViewIndexContextMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	Menu *proxyMenu = FProxyContextMenu.value(menu);
	if (proxyMenu != NULL)
	{
		// Emit aboutToShow in proxyMenu
		proxyMenu->popup(QPoint(0,0));

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

		proxyMenu->hide();
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
		QMap<int, QStringList> rolesMap = indexesRolesMap(AIndexes,QList<int>()<<RDR_KIND<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_METACONTACT_ID);
		
		if (isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			bool isMultiSelection = AIndexes.count()>1;

			QList<QUuid> uniqueMetas;
			QMultiMap<Jid, Jid> uniqueContacts;
			QStringList uniqueKinds = rolesMap.value(RDR_KIND).toSet().toList();
			for(int i=0; i<rolesMap.value(RDR_STREAM_JID).count(); i++)
			{
				Jid streamJid = rolesMap.value(RDR_STREAM_JID).at(i);
				Jid contactJid = rolesMap.value(RDR_PREP_BARE_JID).at(i);
				QUuid metaId = rolesMap.value(RDR_METACONTACT_ID).at(i);

				if (!metaId.isNull() && !uniqueMetas.contains(metaId))
					uniqueMetas.append(metaId);
				else if (metaId.isNull() && !uniqueContacts.contains(streamJid,contactJid))
					uniqueContacts.insertMulti(streamJid,contactJid);
			}

			if (isMultiSelection && uniqueMetas.count()+uniqueContacts.count()>1)
			{
				Action *combineAction = new Action(AMenu);
				combineAction->setText(tr("Combine Contacts..."));
				combineAction->setIcon(RSR_STORAGE_MENUICONS,MNI_METACONTACTS_COMBINE);
				combineAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				combineAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				combineAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				combineAction->setShortcutId(SCT_ROSTERVIEW_COMBINECONTACTS);
				connect(combineAction,SIGNAL(triggered()),SLOT(onCombineMetaItemsByAction()));
				AMenu->addAction(combineAction,AG_RVCM_METACONTACTS,true);
			}

			if (uniqueKinds.count()==1 && uniqueKinds.value(0).toInt()==RIK_METACONTACT_ITEM)
			{
				Action *detachAction = new Action(AMenu);
				detachAction->setText(tr("Detach from Metacontact"));
				detachAction->setIcon(RSR_STORAGE_MENUICONS,MNI_METACONTACTS_DETACH);
				detachAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				detachAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				detachAction->setShortcutId(SCT_ROSTERVIEW_DETACHFROMMETACONTACT);
				connect(detachAction,SIGNAL(triggered()),SLOT(onRemoveMetaItemsByAction()));
				AMenu->addAction(detachAction,AG_RVCM_METACONTACTS,true);
			}

			if (uniqueKinds.count()==1 && uniqueKinds.value(0).toInt()==RIK_METACONTACT)
			{
				Action *destroyAction = new Action(AMenu);
				destroyAction->setText(tr("Destroy Metacontact"));
				destroyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_METACONTACTS_DESTROY);
				destroyAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				destroyAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				destroyAction->setShortcutId(SCT_ROSTERVIEW_DESTROYMETACONTACT);
				connect(destroyAction,SIGNAL(triggered()),SLOT(onDestroyMetaContactsByAction()));
				AMenu->addAction(destroyAction,AG_RVCM_METACONTACTS,true);
			}
			
			if (!isMultiSelection && uniqueKinds.value(0).toInt()==RIK_METACONTACT)
			{
				Action *renameAction = new Action(AMenu);
				renameAction->setText(tr("Rename"));
				renameAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
				renameAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				renameAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
				renameAction->setData(ADR_FROM_GROUP, AIndexes.value(0)->data(RDR_GROUP));
				renameAction->setShortcutId(SCT_ROSTERVIEW_RENAME);
				connect(renameAction,SIGNAL(triggered()),SLOT(onRenameMetaContactByAction()));
				AMenu->addAction(renameAction,AG_RVCM_RCHANGER,true);
			}
		}

		if (hasProxiedIndexes(AIndexes))
		{
			blocked = true;

			Menu *proxyMenu = new Menu(AMenu);
			FProxyContextMenu.insert(AMenu,proxyMenu);
			FRostersView->contextMenuForIndex(indexesProxies(AIndexes),NULL,proxyMenu);
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
			QStringList metaAvatars;
			QMap<Jid, QString> metaAccounts;
			QMap<Jid, QStringList> metaItems;
			QList<IPresenceItem> metaPresences;

			for (int row=0; row<AIndex->childCount(); row++)
			{
				IRosterIndex *metaItemIndex = AIndex->childIndex(row);
				if (metaItemIndex->kind() == RIK_METACONTACT_ITEM)
				{
					Jid streamJid = metaItemIndex->data(RDR_STREAM_JID).toString();
					Jid itemJid = metaItemIndex->data(RDR_PREP_BARE_JID).toString();

					QMap<int, QString> toolTips;
					FRostersView->toolTipsForIndex(metaItemIndex,NULL,toolTips);
					if (!toolTips.isEmpty())
					{
						QString avatarToolTip = toolTips.value(RTTO_AVATAR_IMAGE);
						if (!avatarToolTip.isEmpty() && metaAvatars.count()<10 && !metaAvatars.contains(avatarToolTip))
							metaAvatars += avatarToolTip;

						QString accountToolTip = toolTips.value(RTTO_ROSTERSVIEW_INFO_ACCOUNT);
						if (!accountToolTip.isEmpty())
							metaAccounts.insert(streamJid,accountToolTip);

						QString jidToolTip = toolTips.value(RTTO_ROSTERSVIEW_INFO_JABBERID);
						if (!jidToolTip.isEmpty())
							metaItems[streamJid].append(jidToolTip);
					}

					IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(streamJid) : NULL;
					if (presence)
					{
						foreach(const IPresenceItem &pItem, presence->findItems(itemJid))
							if (pItem.show!=IPresence::Offline && !metaPresences.contains(pItem))
								metaPresences.append(pItem);
					}
				}
			}

			if (!metaAvatars.isEmpty())
				AToolTips.insert(RTTO_AVATAR_IMAGE,QString("<span>%1</span>").arg(metaAvatars.join("<nbsp>")));

			if (!metaItems.isEmpty())
			{
				QStringList accountsToolTip;
				for (QMap<Jid, QStringList>::const_iterator accountIt=metaItems.constBegin(); accountIt!=metaItems.constEnd(); ++accountIt)
				{
					QString accountToolTip = metaAccounts.value(accountIt.key());
					if (!accountToolTip.isEmpty())
						accountsToolTip.append(accountToolTip + "<p/><nbsp>" + accountIt->join("<p/><nbsp>"));
					else
						accountsToolTip.append(accountIt->join("<p/><nbsp>"));
				}
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_JABBERID,QString("<span>%1</span>").arg(accountsToolTip.join("<hr><p/><nbsp>")));
			}
			else
			{
				AToolTips.remove(RTTO_ROSTERSVIEW_INFO_JABBERID);
			}

			metaPresences = FPresenceManager!=NULL ? FPresenceManager->sortPresenceItems(metaPresences) : metaPresences;
			for (int resIndex=0; resIndex<10 && resIndex<metaPresences.count(); resIndex++)
			{
				AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_TOPLINE,"<hr>");

				int orderShift = resIndex*100;
				IPresenceItem pItem = metaPresences.at(resIndex);

				QString statusIconSet = FStatusIcons!=NULL ? FStatusIcons->iconsetByJid(pItem.itemJid) : QString::null;
				QString statusIconKey = FStatusIcons!=NULL ? FStatusIcons->iconKeyByStatus(pItem.show,SUBSCRIPTION_BOTH,false) : QString::null;
				QString statusIconFile = FStatusIcons!=NULL ? FStatusIcons->iconFileName(statusIconSet,statusIconKey) : QString::null;
				AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_NAME+orderShift,QString("<img src='%1'> %2 (%3)").arg(statusIconFile).arg(Qt::escape(pItem.itemJid.uFull())).arg(pItem.priority));

				if (!pItem.status.isEmpty())
					AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_STATUS_TEXT+orderShift,Qt::escape(pItem.status).replace('\n',"<br>"));

				if (resIndex < metaPresences.count()-1)
					AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_MIDDLELINE+orderShift,"<hr>");

				AToolTips.insert(RTTO_ROSTERSVIEW_RESOURCE_BOTTOMLINE,"<hr>");
			}

			AToolTips.remove(RTTO_ROSTERSVIEW_INFO_ACCOUNT);
			AToolTips.remove(RTTO_ROSTERSVIEW_INFO_SUBCRIPTION);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaItemIndexToProxy.value(AIndex);
			if (proxy != NULL)
				FRostersView->toolTipsForIndex(proxy,NULL,AToolTips);
		}
	}
}

void MetaContacts::onRostersViewNotifyInserted(int ANotifyId)
{
	int removeFlags = 0;
	QList<IRosterIndex *> indexes;
	foreach(IRosterIndex *notifyIndex, FRostersView->notifyIndexes(ANotifyId))
	{
		if (notifyIndex->kind() == RIK_METACONTACT_ITEM)
		{
			indexes.append(notifyIndex->parentIndex());
		}
		else foreach(IRosterIndex *itemIndex, FMetaItemProxyToIndex.values(notifyIndex))
		{
			removeFlags = IRostersNotify::ExpandParents;
			indexes.append(itemIndex);
		}
	}

	if (!indexes.isEmpty())
	{
		IRostersNotify notify = FRostersView->notifyById(ANotifyId);
		notify.flags &= ~(removeFlags);

		int notifyId = FRostersView->insertNotify(notify,indexes);
		FNotifyProxyToIndex.insert(ANotifyId,notifyId);
	}
}

void MetaContacts::onRostersViewNotifyRemoved(int ANotifyId)
{
	if (FNotifyProxyToIndex.contains(ANotifyId))
		FRostersView->removeNotify(FNotifyProxyToIndex.take(ANotifyId));
}

void MetaContacts::onRostersViewNotifyActivated(int ANotifyId)
{
	int notifyId = FNotifyProxyToIndex.key(ANotifyId);
	if (notifyId > 0)
		FRostersView->activateNotify(notifyId);
}

void MetaContacts::onMessageChatWindowCreated(IMessageChatWindow *AWindow)
{
	IMetaContact meta = findMetaContact(AWindow->streamJid(),AWindow->contactJid());
	if (!meta.isNull())
		updateMetaWindows(AWindow->streamJid(),meta.id);
}

void MetaContacts::onMessageChatWindowChanged()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		IMetaContact meta = findMetaContact(window->streamJid(),window->contactJid());
		if (!meta.isNull())
			updateMetaWindows(window->streamJid(),meta.id);
	}
}

void MetaContacts::onMessageChatWindowDestroyed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		for(QMap<const IRosterIndex *, QHash<QUuid, IMessageChatWindow *> >::iterator streamIt=FMetaChatWindows.begin(); streamIt!=FMetaChatWindows.end(); ++streamIt)
		{
			for (QHash<QUuid, IMessageChatWindow *>::iterator metaIt=streamIt->begin(); metaIt!=streamIt->end(); ++metaIt)
			{
				if (metaIt.value() == window)
				{
					metaIt = streamIt->erase(metaIt);
					return;
				}
			}
		}
	}
}

void MetaContacts::onRecentContactsOpened(const Jid &AStreamJid)
{
	QSet<QUuid> updatedMetas;
	foreach(const IRecentItem &item, FRecentContacts->streamItems(AStreamJid))
	{
		if (item.type == REIT_CONTACT)
		{
			QUuid metaId = FItemMetaId.value(AStreamJid).value(item.reference);
			if (!metaId.isNull() && updatedMetas.contains(metaId))
			{
				updateMetaRecentItems(AStreamJid,metaId);
				updatedMetas += metaId;
			}
		}
		else if (item.type==REIT_METACONTACT && updatedMetas.contains(item.reference))
		{
			updateMetaRecentItems(AStreamJid,item.reference);
			updatedMetas += item.reference;
		}
	}
}

void MetaContacts::onRecentItemChanged(const IRecentItem &AItem)
{
	if (FUpdatingRecentItem != AItem)
	{
		if (AItem.type == REIT_METACONTACT)
		{
			IRosterIndex *sRoot = getMetaIndexRoot(AItem.streamJid);
			IRecentItem prevItem = FMetaRecentItems.value(sRoot).value(AItem.reference);
			if (!prevItem.isNull() && prevItem.properties.value(REIP_FAVORITE)!=AItem.properties.value(REIP_FAVORITE))
			{
				bool isFavorite = AItem.properties.value(REIP_FAVORITE).toBool();
				foreach(const IRecentItem &item, findMetaRecentContacts(AItem.streamJid,AItem.reference))
				{
					if (FRecentContacts->isReady(item.streamJid))
					{
						FUpdatingRecentItem = item;
						FRecentContacts->setItemProperty(item,REIP_FAVORITE,isFavorite);
					}
				}
				FUpdatingRecentItem = IRecentItem();
			}
			FMetaRecentItems[sRoot].insert(AItem.reference,AItem);
		}
		else if (AItem.type == REIT_CONTACT)
		{
			QUuid metaId = FItemMetaId.value(AItem.streamJid).value(AItem.reference);
			if (!metaId.isNull())
				updateMetaRecentItems(AItem.streamJid,metaId);
		}
	}
}

void MetaContacts::onRecentItemRemoved(const IRecentItem &AItem)
{
	if (FUpdatingRecentItem != AItem)
	{
		if (AItem.type == REIT_METACONTACT)
		{
			IRosterIndex *sRoot = getMetaIndexRoot(AItem.streamJid);
			FMetaRecentItems[sRoot].remove(AItem.reference);
			foreach(const IRecentItem &item, findMetaRecentContacts(AItem.streamJid,AItem.reference))
			{
				if (FRecentContacts->isReady(item.streamJid))
				{
					FUpdatingRecentItem = item;
					FRecentContacts->removeItem(item);
				}
			}
			FUpdatingRecentItem = IRecentItem();
		}
		else if (AItem.type == REIT_CONTACT)
		{
			QUuid metaId = FItemMetaId.value(AItem.streamJid).value(AItem.reference);
			if (!metaId.isNull())
				updateMetaRecentItems(AItem.streamJid,metaId);
		}
	}
}

void MetaContacts::onRemoveMetaItemsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeMetaItems(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList());
}

void MetaContacts::onCombineMetaItemsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		combineMetaItems(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_METACONTACT_ID).toStringList());
}

void MetaContacts::onRenameMetaContactByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		if (isReadyStreams(streams))
		{
			bool editInRoster = false;

			QUuid metaId = action->data(ADR_METACONTACT_ID).toStringList().value(0);
			if (FRostersView && FRostersView->instance()->isActiveWindow() && FRostersView->rostersModel())
			{
				QString group = action->data(ADR_FROM_GROUP).toStringList().value(0);
				foreach(IRosterIndex *index, findMetaIndexes(streams.value(0),metaId))
				{
					if (index->data(RDR_GROUP).toString() == group)
					{
						editInRoster = FRostersView->editRosterIndex(index,RDR_NAME);
						break;
					}
				}
			}

			if (!editInRoster)
				renameMetaContact(streams, action->data(ADR_METACONTACT_ID).toStringList());
		}
	}
}

void MetaContacts::onDestroyMetaContactsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		destroyMetaContacts(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_METACONTACT_ID).toStringList());
}

void MetaContacts::onCopyMetaContactToGroupByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QUuid metaId = action->data(ADR_METACONTACT_ID).toString();
		foreach(const Jid &streamJid, action->data(ADR_STREAM_JID).toStringList())
		{
			IMetaContact meta = findMetaContact(streamJid,metaId);
			if (!meta.isEmpty())
			{
				meta.groups += action->data(ADR_TO_GROUP).toString();
				setMetaContactGroups(streamJid,metaId,meta.groups);
			}
		}
	}
}

void MetaContacts::onMoveMetaContactToGroupByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QUuid metaId = action->data(ADR_METACONTACT_ID).toString();
		foreach(const Jid &streamJid, action->data(ADR_STREAM_JID).toStringList())
		{
			IMetaContact meta = findMetaContact(streamJid,metaId);
			if (!meta.isEmpty())
			{
				meta.groups += action->data(ADR_TO_GROUP).toString();
				meta.groups -= action->data(ADR_FROM_GROUP).toString();
				setMetaContactGroups(streamJid,metaId,meta.groups);
			}
		}
	}
}

void MetaContacts::onUpdateContactsTimerTimeout()
{
	for (QMap<Jid, QSet<QUuid> >::iterator streamIt=FUpdateMeta.begin(); streamIt!=FUpdateMeta.end(); streamIt=FUpdateMeta.erase(streamIt))
	{
		foreach(const QUuid &metaId, streamIt.value())
		{
			IMetaContact meta = findMetaContact(streamIt.key(),metaId);
			if (!meta.isNull())
				updateMetaContact(streamIt.key(),meta);
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

void MetaContacts::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		QMap<int, QStringList> rolesMap = indexesRolesMap(indexes,QList<int>()<<RDR_KIND<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_METACONTACT_ID);
		if (isSelectionAccepted(indexes) && isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			QStringList uniqueKinds = rolesMap.value(RDR_KIND).toSet().toList();
			if (AId==SCT_ROSTERVIEW_RENAME && indexes.count()==1)
			{
				IRosterIndex *index = indexes.first();
				if (index->kind()==RIK_METACONTACT && !FRostersView->editRosterIndex(index,RDR_NAME))
				{
					QMap<int, QStringList> rolesMap = indexesRolesMap(indexes,QList<int>()<<RDR_STREAM_JID<<RDR_METACONTACT_ID);
					renameMetaContact(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_METACONTACT_ID));
				}
			}
			else if (AId==SCT_ROSTERVIEW_COMBINECONTACTS && indexes.count()>1)
			{
				combineMetaItems(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID),rolesMap.value(RDR_METACONTACT_ID));
			}
			else if (AId==SCT_ROSTERVIEW_DESTROYMETACONTACT && uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT)
			{
				destroyMetaContacts(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_METACONTACT_ID));
			}
			else if (AId==SCT_ROSTERVIEW_DETACHFROMMETACONTACT && uniqueKinds.count()==1 && uniqueKinds.at(0).toInt()==RIK_METACONTACT_ITEM)
			{
				removeMetaItems(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID));
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
