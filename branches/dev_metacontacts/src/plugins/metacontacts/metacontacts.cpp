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
#include <utils/widgetmanager.h>
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

static const QList<int> DragKinds = QList<int>() << RIK_CONTACT << RIK_METACONTACT_ITEM << RIK_METACONTACT;
static const QList<int> DropKinds = QList<int>() << RIK_GROUP << RIK_GROUP_BLANK << RIK_CONTACT << RIK_METACONTACT_ITEM << RIK_METACONTACT;

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
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateStorageDataLoaded(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateStorageDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageNotifyAboutToClose(const Jid &)),SLOT(onPrivateStorageNotifyAboutToClose(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorageClosed(const Jid &)));
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

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

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
		FRostersView->insertDragDropHandler(this);
		FRostersView->insertLabelHolder(RLHO_METACONTACTS,this);
		FRostersView->insertClickHooker(RCHO_METACONTACTS,this);
		FRostersView->insertEditHandler(REHO_METACONTACTS_RENAME,this);
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
		static const QList<int> roles = QList<int>() << RDR_ALL_ROLES;
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
			case RDR_STREAM_JID:
			case RDR_FULL_JID:
			case RDR_PREP_FULL_JID:
			case RDR_PREP_BARE_JID:
			case RDR_RESOURCES:
			case RDR_NAME:
			case RDR_GROUP:
			case RDR_SHOW:
			case RDR_STATUS:
			case RDR_PRIORITY:
			case RDR_METACONTACT_ID:
				break;
			case RDR_LABEL_ITEMS:
				proxy = !labelsBlock ? FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString()) : 0;
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
				proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
			}
			break;
		case RIK_METACONTACT_ITEM:
			switch (ARole)
			{
			case RDR_STREAM_JID:
			case RDR_FULL_JID:
			case RDR_PREP_FULL_JID:
			case RDR_PREP_BARE_JID:
			case RDR_GROUP:
			case RDR_METACONTACT_ID:
				break;
			default:
				proxy = FMetaIndexItemProxy.value(AIndex);
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
		labels.append(RLID_METACONTACTS_BRANCH_ITEMS);
		labels.append(AdvancedDelegateItem::BranchId);
	}
	return labels;
}

AdvancedDelegateItem MetaContacts::rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const
{
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

			IRosterIndex *proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
			if (proxy)
				return FRostersView->singleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
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
			IRosterIndex *proxy = FMetaIndexItems.value(AIndex).value(AIndex->data(RDR_PREP_BARE_JID).toString());
			if (proxy)
				return FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
		else if (AIndex->kind() == RIK_METACONTACT_ITEM)
		{
			IRosterIndex *proxy = FMetaIndexItemProxy.value(AIndex);
			if (proxy)
				return FRostersView->doubleClickOnIndex(proxy,AEvent);
		}
	}
	return false;
}

Qt::DropActions MetaContacts::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(ADrag);
	if (DragKinds.contains(AIndex->data(RDR_KIND).toInt()))
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

		return DragKinds.contains(indexData.value(RDR_KIND).toInt());
	}
	return false;
}

bool MetaContacts::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	int hoverKind = AHover->data(RDR_KIND).toInt();
	if (DropKinds.contains(hoverKind) && (AEvent->dropAction() & (Qt::CopyAction|Qt::MoveAction))>0)
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		int indexKind = indexData.value(RDR_KIND).toInt();
		Jid hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
		Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();

		if (isReady(indexStreamJid))
		{
			bool isLayoutMerged = FRostersModel!=NULL ? FRostersModel->streamsLayout()==IRostersModel::LayoutMerged : false;
			if (indexKind==RIK_CONTACT || indexKind==RIK_METACONTACT_ITEM)
			{
				if (hoverKind == RIK_CONTACT)
					return isReady(hoverStreamJid) && (hoverStreamJid!=indexStreamJid || AHover->data(RDR_PREP_BARE_JID)!=indexData.value(RDR_PREP_BARE_JID));
				if (hoverKind==RIK_METACONTACT_ITEM || hoverKind==RIK_METACONTACT)
					return isReady(hoverStreamJid) && (hoverStreamJid!=indexStreamJid || AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID));
			}
			else if (indexKind == RIK_METACONTACT)
			{
				if (hoverKind==RIK_CONTACT || hoverKind==RIK_METACONTACT_ITEM || hoverKind==RIK_METACONTACT)
					return isReady(hoverStreamJid) && (hoverStreamJid!=indexStreamJid || AHover->data(RDR_METACONTACT_ID)!=indexData.value(RDR_METACONTACT_ID));
				else if (hoverKind==RIK_GROUP || hoverKind==RIK_GROUP_BLANK)
					return indexData.value(RDR_GROUP)!=AHover->data(RDR_GROUP) && (isLayoutMerged || AHover->data(RDR_STREAMS).toStringList().contains(indexStreamJid.pFull()));
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

	int hoverKind = AHover->data(RDR_KIND).toInt();
	int indexKind = indexData.value(RDR_KIND).toInt();

	if (indexKind==RIK_METACONTACT && (hoverKind==RIK_GROUP || hoverKind==RIK_GROUP_BLANK))
	{
		Action *groupAction = new Action(AMenu);
		groupAction->setData(ADR_STREAM_JID,indexData.value(RDR_STREAM_JID));
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
	else if (AEvent->dropAction() == Qt::MoveAction)
	{
		QStringList streams = QStringList() << indexData.value(RDR_STREAM_JID).toString() << AHover->data(RDR_STREAM_JID).toString();
		QStringList contacts = QStringList() << indexData.value(RDR_PREP_BARE_JID).toString() << AHover->data(RDR_PREP_BARE_JID).toString();
		QStringList metas = QStringList() << indexData.value(RDR_METACONTACT_ID).toString() << AHover->data(RDR_METACONTACT_ID).toString();

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
		if (isReady(AIndex.data(RDR_STREAM_JID).toString()))
			return AdvancedDelegateItem::DisplayId;
	}
	return AdvancedDelegateItem::NullId;
}

AdvancedDelegateEditProxy *MetaContacts::rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex)
{
	Q_UNUSED(AIndex);
	if (AOrder==REHO_METACONTACTS_RENAME && ADataRole==RDR_NAME)
		return this;
	return NULL;
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
			setMetaContactName(AIndex.data(RDR_STREAM_JID).toString(),AIndex.data(RDR_METACONTACT_ID).toString(),newName);
		return true;
	}
	return false;
}

bool MetaContacts::isReady(const Jid &AStreamJid) const
{
	return FPrivateStorage==NULL || FPrivateStorage->isLoaded(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
}

QMultiMap<Jid, QUuid> MetaContacts::findLinkedContacts(const QUuid &ALinkId) const
{
	return FLinkMetaId.value(ALinkId);
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
	return FMetaIndexes.value(AStreamJid).value(AMetaId);
}

QUuid MetaContacts::createMetaContact(const Jid &AStreamJid, const QString &AName, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta;
		meta.id = QUuid::createUuid();
		meta.name = AName;
		meta.items = AItems;
		if (updateMetaContact(AStreamJid,meta))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Metacontact created, metaId=%1, name=%2, items=%3").arg(meta.id.toString(),AName).arg(AItems.count()));
			startSaveContactsToStorage(AStreamJid);
			return meta.id;
		}
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to create metacontact: Stream is not ready"));
	}
	return QUuid();
}

bool MetaContacts::setMetaContactLink(const Jid &AStreamJid, const QUuid &AMetaId, const QUuid &ALinkId)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid,AMetaId);
		if (meta.id == AMetaId)
		{
			if (meta.link != ALinkId)
			{
				meta.link = ALinkId;
				if (updateMetaContact(AStreamJid,meta))
				{
					LOG_STRM_DEBUG(AStreamJid,QString("Metacontact lick changed, metaId=%1, link=%2").arg(AMetaId.toString(),ALinkId.toString()));
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
			LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact lick, metaId=%1: Metacontact not found").arg(AMetaId.toString()));
		}
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact lick: Stream is not ready"));
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
					LOG_STRM_DEBUG(AStreamJid,QString("Metacontact name changed, metaId=%1, name=%2").arg(AMetaId.toString(),AName));
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
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact name: Stream is not ready"));
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
					LOG_STRM_DEBUG(AStreamJid,QString("Metacontact groups changed, metaId=%1, groups=%2").arg(AMetaId.toString()).arg(AGroups.count()));
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
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to change metacontact groups: Stream is not ready"));
	}
	return false;
}

bool MetaContacts::insertMetaContactItems( const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems )
{
	if (isReady(AStreamJid))
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
					LOG_STRM_DEBUG(AStreamJid,QString("Metacontact items inserted, metaId=%1, items=%2").arg(AMetaId.toString()).arg(insertCount));
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
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to insert metacontact items: Stream is not ready"));
	}
	return false;
}

bool MetaContacts::removeMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems)
{
	if (isReady(AStreamJid))
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
					LOG_STRM_DEBUG(AStreamJid,QString("Metacontact items removed, metaId=%1, items=%2").arg(AMetaId.toString()).arg(removeCount));
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
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to remove metacontact items: Stream is not ready"));
	}
	return false;
}

void MetaContacts::updateMetaIndexes(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot)
	{
		if (!AMetaContact.isEmpty())
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
					metaIndexList.append(metaIndex);

					metaIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
					metaIndex->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);
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
				updateMetaIndexItems(AStreamJid,NullMetaContact,metaIndex);

				FMetaIndexItems.remove(metaIndex);
				metaIndexList.removeAll(metaIndex);

				FRostersModel->removeRosterIndex(metaIndex);
				oldGroupsIt = oldGroups.erase(oldGroupsIt);
			}

			QStringList metaItems;
			foreach(const Jid &itemJid, AMetaContact.items)
				metaItems.append(itemJid.pBare());

			QStringList metaResources;
			foreach(const IPresenceItem &pitem, AMetaContact.presences)
			{
				if (pitem.show != IPresence::Offline)
					metaResources.append(pitem.itemJid.pFull());
			}

			IPresenceItem metaPresence = AMetaContact.presences.value(0);
			Jid metaJid = metaPresence.itemJid.isValid() ? metaPresence.itemJid : AMetaContact.items.value(0);

			foreach(IRosterIndex *metaIndex, metaIndexList)
			{
				metaIndex->setData(metaJid.full(),RDR_FULL_JID);
				metaIndex->setData(metaJid.pFull(),RDR_PREP_FULL_JID);
				metaIndex->setData(metaJid.pBare(),RDR_PREP_BARE_JID);
				metaIndex->setData(metaResources,RDR_RESOURCES);

				metaIndex->setData(AMetaContact.name,RDR_NAME);
				metaIndex->setData(metaPresence.show,RDR_SHOW);
				metaIndex->setData(metaPresence.status,RDR_STATUS);
				metaIndex->setData(metaPresence.priority,RDR_PRIORITY);

				metaIndex->setData(metaItems, RDR_METACONTACT_ITEMS);

				updateMetaIndexItems(AStreamJid,AMetaContact,metaIndex);

				emit rosterLabelChanged(RLID_METACONTACTS_BRANCH_ITEMS,metaIndex);
			}
		}
		else foreach(IRosterIndex *metaIndex, FMetaIndexes[AStreamJid].take(AMetaContact.id))
		{
			updateMetaIndexItems(AStreamJid,AMetaContact,metaIndex);
			FMetaIndexItems.remove(metaIndex);

			FRostersModel->removeRosterIndex(metaIndex);
		}
	}
}

void MetaContacts::updateMetaIndexItems(const Jid &AStreamJid, const IMetaContact &AMetaContact, IRosterIndex *AMetaIndex)
{
	QMap<Jid, IRosterIndex *> &metaItemsMap = FMetaIndexItems[AMetaIndex];

	QMap<Jid, IRosterIndex *> itemIndexMap = metaItemsMap;
	foreach(const Jid &itemJid, AMetaContact.items)
	{
		QMap<IRosterIndex *, IRosterIndex *> proxyIndexMap;
		foreach(IRosterIndex *index, FRostersModel->findContactIndexes(AStreamJid,itemJid))
			proxyIndexMap.insert(index->parentIndex(),index);

		IRosterIndex *proxyIndex = !proxyIndexMap.isEmpty() ? proxyIndexMap.value(AMetaIndex->parentIndex(),proxyIndexMap.constBegin().value()) : NULL;
		if (proxyIndex != NULL)
		{
			IRosterIndex *itemIndex = itemIndexMap.take(itemJid);
			if (itemIndex == NULL)
			{
				itemIndex = FRostersModel->newRosterIndex(RIK_METACONTACT_ITEM);
				itemIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
				itemIndex->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);
				metaItemsMap.insert(itemJid,itemIndex);
			}

			itemIndex->setData(AMetaIndex->data(RDR_GROUP),RDR_GROUP);
			itemIndex->setData(proxyIndex->data(RDR_FULL_JID),RDR_FULL_JID);
			itemIndex->setData(proxyIndex->data(RDR_PREP_FULL_JID),RDR_PREP_FULL_JID);
			itemIndex->setData(proxyIndex->data(RDR_PREP_BARE_JID),RDR_PREP_BARE_JID);

			IRosterIndex *prevProxyIndex = FMetaIndexItemProxy.value(itemIndex);
			if (proxyIndex != prevProxyIndex)
			{
				if (prevProxyIndex)
				{
					FMetaIndexProxyItem.remove(prevProxyIndex,itemIndex);
					if (!FMetaIndexProxyItem.contains(prevProxyIndex))
						prevProxyIndex->setData(QVariant(),RDR_METACONTACT_ID);
				}
				proxyIndex->setData(AMetaContact.id.toString(),RDR_METACONTACT_ID);

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
	if (FMessageWidgets)
	{
		IMessageChatWindow *chatWindow = FMetaChatWindows.value(AStreamJid).value(AMetaContact.id);
		
		foreach(const Jid &itemJid, ANewItems)
		{
			IMessageChatWindow *window = FMessageWidgets->findChatWindow(AStreamJid,itemJid);
			if (window!=NULL && window!=chatWindow)
				window->address()->removeAddress(AStreamJid,itemJid);
		}

		if (chatWindow)
		{
			foreach(const Jid &itemJid, AOldItems)
				chatWindow->address()->removeAddress(AStreamJid,itemJid);
			foreach(const Jid &itemJid, ANewItems)
				chatWindow->address()->appendAddress(AStreamJid,itemJid);
			if (chatWindow->tabPageCaption() != AMetaContact.name)
				chatWindow->updateWindow(chatWindow->tabPageIcon(),AMetaContact.name,tr("%1 - Chat").arg(AMetaContact.name),QString::null);
		}
	}
}

bool MetaContacts::updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster!=NULL && !AMetaContact.isNull())
	{
		IMetaContact after = AMetaContact;
		IMetaContact before = findMetaContact(AStreamJid,after.id);

		QHash<Jid, QUuid> &itemMetaHash = FItemMetaId[AStreamJid];
		IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;

		after.groups.clear();
		after.presences.clear();
		foreach(const Jid &itemJid, after.items)
		{
			IRosterItem ritem = roster->rosterItem(itemJid);
			if (ritem.isValid && !itemJid.node().isEmpty())
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
					itemMetaHash.insert(itemJid,after.id);
				}

				if (after.name.isEmpty() && !ritem.name.isEmpty())
					after.name = ritem.name;

				if (!ritem.groups.isEmpty())
					after.groups += ritem.groups;

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
			after.presences = FPresencePlugin->sortPresenceItems(after.presences);

		if (after != before)
		{
			QSet<Jid> newMetaItems = after.items.toSet() - before.items.toSet();
			QSet<Jid> oldMetaItems = before.items.toSet() - after.items.toSet();

			foreach(const Jid &itemJid, oldMetaItems)
				itemMetaHash.remove(itemJid);

			if (!after.isEmpty())
			{
				if (after.link != before.link)
				{
					if (!before.link.isNull())
						FLinkMetaId[before.link].remove(AStreamJid,before.id);
					if (!after.link.isNull())
						FLinkMetaId[before.link].remove(AStreamJid,after.id);
				}
				FMetaContacts[AStreamJid][after.id] = after;
			}
			else
			{
				if (!before.link.isNull())
					FLinkMetaId[before.link].remove(AStreamJid,before.id);
				FMetaContacts[AStreamJid].remove(after.id);
			}

			updateMetaIndexes(AStreamJid,after);
			updateMetaWindows(AStreamJid,after,newMetaItems,oldMetaItems);

			emit metaContactChanged(AStreamJid,after,before);
			return true;
		}
		else
		{
			updateMetaIndexes(AStreamJid,after);
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
	FUpdateMeta[AStreamJid][AMetaId] += NULL;
	FUpdateTimer.start(UPDATE_META_TIMEOUT);
}

void MetaContacts::startUpdateMetaContactIndex(const Jid &AStreamJid, const QUuid &AMetaId, IRosterIndex *AIndex)
{
	FUpdateMeta[AStreamJid][AMetaId] += AIndex;
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster != NULL)
			return roster->rosterItem(AItemJid).isValid;
	}
	return false;
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

bool MetaContacts::hasProxiedIndexes(const QList<IRosterIndex *> &AIndexes) const
{
	foreach(IRosterIndex *index, AIndexes)
	{
		if (FMetaIndexItems.contains(index))
			return true;
		else if (FMetaIndexItemProxy.contains(index))
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
			foreach(IRosterIndex *metaItem, FMetaIndexItems.value(index))
				proxies.append(FMetaIndexItemProxy.value(metaItem));
		}
		else if (FMetaIndexItemProxy.contains(index))
		{
			proxies.append(FMetaIndexItemProxy.value(index));
		}
		else if (ASelfProxy)
		{
			proxies.append(index);
		}
	}
	proxies.removeAll(NULL);
	return proxies.toSet().toList();
}

void MetaContacts::renameMetaContact(const Jid &AStreamJid, const QUuid &AMetaId)
{
	if (isReady(AStreamJid))
	{
		IMetaContact meta = findMetaContact(AStreamJid, AMetaId);
		QString newName = QInputDialog::getText(NULL,tr("Rename Metacontact"),tr("Enter name:"),QLineEdit::Normal,meta.name);
		if (!newName.isEmpty() && newName!=meta.name)
			setMetaContactName(AStreamJid,AMetaId,newName);
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
		CombineContactsDialog *dialog = new CombineContactsDialog(FPluginManager,this,AStreams,AContacts,AMetas);
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
		meta.link = metaElem.attribute("link");
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
		metaElem.setAttribute("link",metaIt->link.toString());
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

void MetaContacts::onRosterAdded(IRoster *ARoster)
{
	FLoadStreams += ARoster->streamJid();
	FMetaContacts[ARoster->streamJid()].clear();
	QTimer::singleShot(0,this,SLOT(onLoadContactsFromFileTimerTimeout()));
}

void MetaContacts::onRosterRemoved(IRoster *ARoster)
{
	QHash<QUuid, QList<IRosterIndex *> > metaIndexes = FMetaIndexes.take(ARoster->streamJid());
	for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt=metaIndexes.constBegin(); metaIt!=metaIndexes.constEnd(); ++metaIt)
	{
		foreach(IRosterIndex *metaIndex, metaIt.value())
		{
			foreach(IRosterIndex *itemIndex, FMetaIndexItems.take(metaIndex))
			{
				FMetaIndexProxyItem.remove(FMetaIndexItemProxy.take(itemIndex),itemIndex);
				FRostersModel->removeRosterIndex(itemIndex);
			}
			FRostersModel->removeRosterIndex(metaIndex);
		}
	}

	FSaveStreams -= ARoster->streamJid();
	FLoadStreams -= ARoster->streamJid();
	FUpdateMeta.remove(ARoster->streamJid());

	FItemMetaId.remove(ARoster->streamJid());
	FMetaChatWindows.remove(ARoster->streamJid());

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
	FItemMetaId[ARoster->streamJid()] = FItemMetaId.take(ABefore);
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

void MetaContacts::onPrivateStorageOpened(const Jid &AStreamJid)
{
	QString id = FPrivateStorage->loadData(AStreamJid,"storage",NS_STORAGE_METACONTACTS);
	if (!id.isEmpty())
	{
		FLoadRequestId[AStreamJid] = id;
		LOG_STRM_INFO(AStreamJid,"Load metacontacts from storage request sent");
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send load metacontacts from storage request");
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
	for (QMap<Jid, QHash<QUuid, QList<IRosterIndex *> > >::const_iterator streamIt=FMetaIndexes.constBegin(); streamIt!=FMetaIndexes.constEnd(); ++streamIt)
	{
		IRosterIndex *sroot = FRostersModel->streamsLayout()==IRostersModel::LayoutMerged ? FRostersModel->contactsRoot() : FRostersModel->streamRoot(streamIt.key());
		for (QHash<QUuid, QList<IRosterIndex *> >::const_iterator metaIt = streamIt->constBegin(); metaIt!=streamIt->constEnd(); ++metaIt)
		{
			foreach(IRosterIndex *index, metaIt.value())
			{
				IRosterIndex *pIndex = index->parentIndex();
				if (FRostersModel->isGroupKind(pIndex->kind()))
				{
					IRosterIndex *groupIndex = FRostersModel->getGroupIndex(pIndex->kind(),pIndex->data(RDR_GROUP).toString(),sroot);
					FRostersModel->insertRosterIndex(index,groupIndex);
				}
				else if (pIndex==FRostersModel->contactsRoot() || pIndex==FRostersModel->streamRoot(streamIt.key()))
				{
					FRostersModel->insertRosterIndex(index,sroot);
				}
			}
		}
	}
}

void MetaContacts::onRostersModelIndexInserted(IRosterIndex *AIndex)
{
	if (AIndex->kind()==RIK_CONTACT && !FMetaIndexItemProxy.contains(AIndex))
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		Jid itemJid = AIndex->data(RDR_PREP_BARE_JID).toString();

		QUuid metaId = FItemMetaId.value(streamJid).value(itemJid);
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
	if (AIndex->kind() == RIK_METACONTACT_ITEM)
		emit rosterDataChanged(AIndex->parentIndex(),ARole);
	else foreach(IRosterIndex *metaItemIndex, FMetaIndexProxyItem.values(AIndex))
		emit rosterDataChanged(metaItemIndex,ARole);
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
		QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_KIND<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_METACONTACT_ID);
		
		bool isMultiSelection = AIndexes.count()>1;

		QStringList uniqueKinds = rolesMap.value(RDR_KIND).toSet().toList();
		QStringList uniqueMetas = rolesMap.value(RDR_METACONTACT_ID).toSet().toList();

		if (isReadyStreams(rolesMap.value(RDR_STREAM_JID)))
		{
			if (isMultiSelection && (uniqueMetas.count()>1 || uniqueMetas.value(0).isEmpty()))
			{
				Action *combineAction = new Action(AMenu);
				combineAction->setText(tr("Combine Contacts..."));
				combineAction->setIcon(RSR_STORAGE_MENUICONS,MNI_METACONTACTS_COMBINE);
				combineAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				combineAction->setData(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				combineAction->setData(ADR_METACONTACT_ID,rolesMap.value(RDR_METACONTACT_ID));
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
	int removeFlags = 0;
	QList<IRosterIndex *> indexes;
	foreach(IRosterIndex *notifyIndex, FRostersView->notifyIndexes(ANotifyId))
	{
		if (notifyIndex->kind() == RIK_METACONTACT_ITEM)
		{
			indexes.append(notifyIndex->parentIndex());
		}
		else foreach(IRosterIndex *itemIndex, FMetaIndexProxyItem.values(notifyIndex))
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
		FProxyToIndexNotify.insert(ANotifyId,notifyId);
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
		FMetaChatWindows[AWindow->streamJid()].insert(meta.id,AWindow);
		connect(AWindow->instance(),SIGNAL(tabPageChanged()),SLOT(onMessageChatWindowChanged()));
		connect(AWindow->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMessageChatWindowDestroyed()));

		updateMetaWindows(AWindow->streamJid(),meta,meta.items.toSet(),QSet<Jid>());
	}
}

void MetaContacts::onMessageChatWindowChanged()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		IMetaContact meta = findMetaContact(window->streamJid(),window->contactJid());
		if (!meta.isNull())
			updateMetaWindows(window->streamJid(),meta,QSet<Jid>(),QSet<Jid>());
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
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		if (isReady(streamJid))
		{
			bool editInRoster = false;

			QUuid metaId = action->data(ADR_METACONTACT_ID).toString();
			if (FRostersView && FRostersView->instance()->isActiveWindow() && FRostersView->rostersModel())
			{
				QString group = action->data(ADR_FROM_GROUP).toStringList().value(0);
				foreach(IRosterIndex *index, findMetaIndexes(streamJid,metaId))
				{
					if (index->data(RDR_GROUP).toString() == group)
					{
						editInRoster = FRostersView->editRosterIndex(index,RDR_NAME);
						break;
					}
				}
			}

			if (!editInRoster)
				renameMetaContact(streamJid, metaId);
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
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QUuid metaId = action->data(ADR_METACONTACT_ID).toString();

		IMetaContact meta = findMetaContact(streamJid,metaId);
		if (!meta.isEmpty())
		{
			meta.groups += action->data(ADR_TO_GROUP).toString();
			setMetaContactGroups(streamJid,metaId,meta.groups);
		}
	}
}

void MetaContacts::onMoveMetaContactToGroupByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QUuid metaId = action->data(ADR_METACONTACT_ID).toString();

		IMetaContact meta = findMetaContact(streamJid,metaId);
		if (!meta.isEmpty())
		{
			meta.groups += action->data(ADR_TO_GROUP).toString();
			meta.groups -= action->data(ADR_FROM_GROUP).toString();
			setMetaContactGroups(streamJid,metaId,meta.groups);
		}
	}
}

void MetaContacts::onUpdateContactsTimerTimeout()
{
	for (QMap<Jid, QMap<QUuid, QSet<IRosterIndex *> > >::iterator streamIt=FUpdateMeta.begin(); streamIt!=FUpdateMeta.end(); streamIt=FUpdateMeta.erase(streamIt))
	{
		for(QMap<QUuid, QSet<IRosterIndex *> >::const_iterator metaIt=streamIt->constBegin(); metaIt!=streamIt->constEnd(); ++metaIt)
		{
			IMetaContact meta = findMetaContact(streamIt.key(),metaIt.key());
			if (!meta.isNull())
			{
				QList<IRosterIndex *> metaIndexList = findMetaIndexes(streamIt.key(),metaIt.key());
				if (metaIt->contains(NULL))
				{
					updateMetaContact(streamIt.key(),meta);
				}
				else foreach(IRosterIndex *metaIndex, metaIt.value())
				{
					if (metaIndexList.contains(metaIndex))
						updateMetaIndexItems(streamIt.key(),meta,metaIndex);
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

void MetaContacts::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_RENAME && indexes.count()==1)
		{
			IRosterIndex *index = indexes.first();
			if (index->kind()==RIK_METACONTACT && !FRostersView->editRosterIndex(index,RDR_NAME))
				renameMetaContact(index->data(RDR_STREAM_JID).toString(),index->data(RDR_METACONTACT_ID).toString());
		}
	}
}

Q_EXPORT_PLUGIN2(plg_metacontacts, MetaContacts)
