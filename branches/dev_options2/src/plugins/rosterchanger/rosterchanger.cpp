#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QItemEditorFactory>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosteredithandlerorders.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/shortcuts.h>
#include <definitions/xmppurihandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/shortcuts.h>
#include <utils/logger.h>

#define ADR_STREAM_JID              Action::DR_StreamJid
#define ADR_CONTACT_JID             Action::DR_Parametr1
#define ADR_SUBSCRIPTION            Action::DR_Parametr2
#define ADR_NAME                    Action::DR_Parametr2
#define ADR_GROUP                   Action::DR_Parametr3
#define ADR_TO_GROUP                Action::DR_Parametr4

static const QList<int> DragRosterKinds = QList<int>() << RIK_CONTACT << RIK_GROUP << RIK_METACONTACT_ITEM;
static const QList<int> DropRosterKinds = QList<int>() << RIK_STREAM_ROOT << RIK_CONTACTS_ROOT << RIK_GROUP << RIK_GROUP_BLANK;

RosterChanger::RosterChanger()
{
	FPluginManager = NULL;
	FRosterManager = NULL;
	FRostersModel = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FXmppUriQueries = NULL;
	FMultiChatManager = NULL;
}

RosterChanger::~RosterChanger()
{

}

//IPlugin
void RosterChanger::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Editor");
	APluginInfo->description = tr("Allows to edit roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(ROSTER_UUID);
}

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
		if (FRosterManager)
		{
			connect(FRosterManager->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterManager->instance(),SIGNAL(rosterSubscriptionSent(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionSent(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterManager->instance(),SIGNAL(rosterSubscriptionReceived(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionReceived(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterManager->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatManager").value(0,NULL);
	if (plugin)
	{
		FMultiChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());
		if (FMultiChatManager)
		{
			connect(FMultiChatManager->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FRosterManager!=NULL;
}

bool RosterChanger::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_ADDCONTACT,tr("Add contact"),tr("Ins","Add contact"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_RENAME,tr("Rename contact/group"),tr("F2","Rename contact/group"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFROMGROUP,tr("Remove contact/group from group"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFROMROSTER,tr("Remove contact/group from roster"),tr("Del","Remove contact/group from roster"),Shortcuts::WidgetShortcut);

	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_SUBSCRIPTION_REQUEST;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCRIBTION);
		notifyType.title = tr("When receiving authorization request");
		notifyType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_SUBSCRIPTION_REQUEST,notifyType);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
		FRostersView->insertEditHandler(REHO_ROSTERCHANGER_RENAME,this);
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_ADDCONTACT,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_RENAME,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFROMGROUP,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFROMROSTER,FRostersView->instance());
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}
	return true;
}

bool RosterChanger::initSettings()
{
	Options::setDefaultValue(OPV_ROSTER_AUTOSUBSCRIBE, false);
	Options::setDefaultValue(OPV_ROSTER_AUTOUNSUBSCRIBE, true);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> RosterChanger::optionsDialogWidgets(const QString &ANode, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANode == OPN_ROSTERVIEW)
	{
		widgets.insertMulti(OHO_ROSTER_MANAGEMENT,FOptionsManager->newOptionsDialogHeader(tr("Contacts list management"),AParent));
		widgets.insertMulti(OWO_ROSTER_AUTOSUBSCRIBE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_AUTOSUBSCRIBE),tr("Automatically accept all subscription requests"),AParent));
		widgets.insertMulti(OWO_ROSTER_AUTOUNSUBSCRIBE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE),tr("Remove subscription when you was deleted from contacts list"),AParent));
	}
	return widgets;
}

Qt::DropActions RosterChanger::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(ADrag);
	if (DragRosterKinds.contains(AIndex->data(RDR_KIND).toInt()))
		return Qt::CopyAction|Qt::MoveAction;
	return Qt::IgnoreAction;
}

bool RosterChanger::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->source()==FRostersView->instance() && AEvent->mimeData()->hasFormat(DDT_ROSTERSVIEW_INDEX_DATA))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		if (DragRosterKinds.contains(indexData.value(RDR_KIND).toInt()))
			return true;
	}
	return false;
}

bool RosterChanger::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	int hoverKind = AHover->data(RDR_KIND).toInt();
	if (DropRosterKinds.contains(hoverKind))
	{
		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		int indexKind = indexData.value(RDR_KIND).toInt();
		if (hoverKind == RIK_STREAM_ROOT)
		{
			QString hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
			if (isRosterOpened(hoverStreamJid))
			{
				if (indexKind == RIK_CONTACT)
				{
					if (indexData.value(RDR_STREAM_JID) != hoverStreamJid)
						return true;
				}
				else if (indexKind == RIK_GROUP)
				{
					if (indexData.value(RDR_STREAMS).toStringList().count()>1)
						return true;
					else if (!indexData.value(RDR_STREAMS).toStringList().contains(hoverStreamJid))
						return true;
					else if (indexData.value(RDR_GROUP).toString().contains(ROSTER_GROUP_DELIMITER))
						return true;
				}
			}
		}
		else if (hoverKind == RIK_CONTACTS_ROOT)
		{
			if (indexKind == RIK_GROUP)
			{
				if (isAllRostersOpened(indexData.value(RDR_STREAMS).toStringList()) && indexData.value(RDR_GROUP).toString().contains(ROSTER_GROUP_DELIMITER))
					return true;
			}
		}
		else if (hoverKind == RIK_GROUP)
		{
			if (isAllRostersOpened(AHover->data(RDR_STREAMS).toStringList()))
			{
				if (indexKind==RIK_CONTACT || indexKind==RIK_METACONTACT_ITEM)
				{
					if (!AHover->data(RDR_STREAMS).toStringList().contains(indexData.value(RDR_STREAM_JID).toString()))
						return true;
					else if (AHover->data(RDR_GROUP) != indexData.value(RDR_GROUP))
						return true;
				}
				else if (indexKind == RIK_GROUP)
				{
					QString hoverGroup = AHover->data(RDR_GROUP).toString();
					QString indexGroup = indexData.value(RDR_GROUP).toString();
					if (hoverGroup == indexGroup)
						return false;
					else if (hoverGroup.startsWith(indexGroup+ROSTER_GROUP_DELIMITER))
						return false;
					else if (indexGroup == hoverGroup+ROSTER_GROUP_DELIMITER+indexGroup.split(ROSTER_GROUP_DELIMITER).last())
						return false;
					return true;
				}
			}
		}
		else if (hoverKind == RIK_GROUP_BLANK)
		{
			if (isAllRostersOpened(AHover->data(RDR_STREAMS).toStringList()))
			{
				if (indexKind==RIK_CONTACT || indexKind==RIK_METACONTACT_ITEM)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void RosterChanger::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RosterChanger::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AHover, Menu *AMenu)
{
	int hoverKind = AHover->data(RDR_KIND).toInt();
	if (DropRosterKinds.contains(hoverKind) && (AEvent->dropAction() & (Qt::CopyAction|Qt::MoveAction))>0)
	{
		bool isLayoutMerged = FRostersModel!=NULL && FRostersModel->streamsLayout()==IRostersModel::LayoutMerged;

		QMap<int, QVariant> indexData;
		QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
		operator>>(stream,indexData);

		int indexKind = indexData.value(RDR_KIND).toInt();
		if (hoverKind == RIK_STREAM_ROOT)
		{
			QString hoverStreamJid = AHover->data(RDR_STREAM_JID).toString();
			if (isRosterOpened(hoverStreamJid))
			{
				Action *action = new Action(AMenu);
				if (indexKind == RIK_CONTACT)
				{
					action->setText(tr("Add Contact"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
					action->setData(ADR_STREAM_JID,QStringList()<<hoverStreamJid);
					action->setData(ADR_CONTACT_JID,QStringList()<<indexData.value(RDR_PREP_BARE_JID).toString());
					action->setData(RDR_NAME,QStringList()<<indexData.value(RDR_NAME).toString());
					action->setData(ADR_TO_GROUP,QVariant(QString::null));
					connect(action,SIGNAL(triggered(bool)),SLOT(onAddContactsToGroup(bool)));
				}
				else if (indexKind == RIK_GROUP)
				{
					if (isLayoutMerged || !indexData.value(RDR_STREAMS).toStringList().contains(hoverStreamJid))
					{
						QStringList streams;
						QStringList contacts;
						QStringList names;
						foreach(const QString &streamJid, indexData.value(RDR_STREAMS).toStringList())
						{
							IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
							if (roster)
							{
								QString group = indexData.value(RDR_GROUP).toString();
								foreach(const IRosterItem &ritem, roster->groupItems(group))
								{
									streams.append(hoverStreamJid);
									contacts.append(ritem.itemJid.pBare());
									names.append(ritem.name);
								}
							}
						}

						action->setText(tr("Add Group with Contacts"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
						action->setData(ADR_STREAM_JID,streams);
						action->setData(ADR_CONTACT_JID,contacts);
						action->setData(ADR_NAME,names);
						action->setData(ADR_TO_GROUP,indexData.value(RDR_GROUP).toString());
						connect(action,SIGNAL(triggered(bool)),SLOT(onAddContactsToGroup(bool)));
					}
					else if (AEvent->dropAction() == Qt::CopyAction)
					{
						action->setText(tr("Copy Group to Root"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
						action->setData(ADR_STREAM_JID,QStringList()<<hoverStreamJid);
						action->setData(ADR_GROUP,QStringList()<<indexData.value(RDR_GROUP).toString());
						action->setData(ADR_TO_GROUP,QStringList()<<QString::null);
						connect(action,SIGNAL(triggered(bool)),SLOT(onCopyGroupsToGroup(bool)));
					}
					else if (AEvent->dropAction() == Qt::MoveAction)
					{
						action->setText(tr("Move Group to Root"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
						action->setData(ADR_STREAM_JID,QStringList()<<hoverStreamJid);
						action->setData(ADR_GROUP,QStringList()<<indexData.value(RDR_GROUP).toString());
						action->setData(ADR_TO_GROUP,QStringList()<<QString::null);
						connect(action,SIGNAL(triggered(bool)),SLOT(onMoveGroupsToGroup(bool)));
					}
				}

				AMenu->addAction(action,AG_DEFAULT,true);
				AMenu->setDefaultAction(action);
				return true;
			}
		}
		else if (hoverKind == RIK_CONTACTS_ROOT)
		{
			if (indexKind==RIK_GROUP && isAllRostersOpened(indexData.value(RDR_STREAMS).toStringList()))
			{
				QStringList streams;
				QStringList groups;
				foreach(const QString &streamJid, indexData.value(RDR_STREAMS).toStringList())
				{
					streams.append(streamJid);
					groups.append(indexData.value(RDR_GROUP).toString());
				}

				Action *action = new Action(AMenu);
				if (AEvent->dropAction() == Qt::CopyAction)
				{
					action->setText(tr("Copy Group to Root"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					action->setData(ADR_STREAM_JID,streams);
					action->setData(ADR_GROUP,groups);
					action->setData(ADR_TO_GROUP,QStringList()<<QString::null);
					connect(action,SIGNAL(triggered(bool)),SLOT(onCopyGroupsToGroup(bool)));
				}
				else if (AEvent->dropAction() == Qt::MoveAction)
				{
					action->setText(tr("Move Group to Root"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					action->setData(ADR_STREAM_JID,streams);
					action->setData(ADR_GROUP,groups);
					action->setData(ADR_TO_GROUP,QStringList()<<QString::null);
					connect(action,SIGNAL(triggered(bool)),SLOT(onMoveGroupsToGroup(bool)));
				}

				AMenu->addAction(action,AG_DEFAULT,true);
				AMenu->setDefaultAction(action);
				return true;
			}
		}
		else if (hoverKind==RIK_GROUP || hoverKind==RIK_GROUP_BLANK)
		{
			if (indexKind==RIK_CONTACT || indexKind==RIK_METACONTACT_ITEM)
			{
				QString indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
				QString destStreamJid = isLayoutMerged ? indexStreamJid : AHover->data(RDR_STREAMS).toStringList().value(0);
				if (isRosterOpened(destStreamJid))
				{
					Action *action = new Action(AMenu);
					action->setData(ADR_STREAM_JID,QStringList()<<destStreamJid);
					action->setData(ADR_CONTACT_JID,QStringList()<<indexData.value(RDR_PREP_BARE_JID).toString());
					action->setData(ADR_NAME,QStringList()<<indexData.value(RDR_NAME).toString());
					action->setData(ADR_TO_GROUP,AHover->data(RDR_GROUP).toString());

					if (indexStreamJid != destStreamJid)
					{
						action->setText(tr("Add Contact"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
						connect(action,SIGNAL(triggered(bool)),SLOT(onAddContactsToGroup(bool)));
					}
					else if (AEvent->dropAction() == Qt::CopyAction)
					{
						action->setText(tr("Copy Contact to Group"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
						connect(action,SIGNAL(triggered(bool)),SLOT(onCopyContactsToGroup(bool)));
					}
					else if (AEvent->dropAction() == Qt::MoveAction)
					{
						action->setText(tr("Move Contact to Group"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
						action->setData(ADR_GROUP,QStringList()<<indexData.value(RDR_GROUP).toString());
						connect(action,SIGNAL(triggered(bool)),SLOT(onMoveContactsToGroup(bool)));
					}

					AMenu->addAction(action,AG_DEFAULT,true);
					AMenu->setDefaultAction(action);
					return true;
				}
			}
			else if (indexKind==RIK_GROUP && hoverKind!=RIK_GROUP_BLANK)
			{
				QStringList destStreams = isLayoutMerged ? indexData.value(RDR_STREAMS).toStringList() : AHover->data(RDR_STREAMS).toStringList();
				if (isAllRostersOpened(destStreams))
				{
					Action *action = new Action(AMenu);
					if (isLayoutMerged || AHover->data(RDR_STREAMS)==indexData.value(RDR_STREAMS))
					{
						QStringList groups;
						QStringList streams;
						foreach(const QString &streamJid, indexData.value(RDR_STREAMS).toStringList())
						{
							streams.append(streamJid);
							groups.append(indexData.value(RDR_GROUP).toString());
						}
						action->setData(ADR_STREAM_JID,streams);
						action->setData(ADR_GROUP,groups);

						if (AEvent->dropAction() == Qt::CopyAction)
						{
							action->setText(tr("Copy Group to Group"));
							action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
							connect(action,SIGNAL(triggered(bool)),SLOT(onCopyGroupsToGroup(bool)));
						}
						else if (AEvent->dropAction() == Qt::MoveAction)
						{
							action->setText(tr("Move Group to Group"));
							action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
							connect(action,SIGNAL(triggered(bool)),SLOT(onMoveGroupsToGroup(bool)));
						}

						action->setData(ADR_TO_GROUP,AHover->data(RDR_GROUP).toString());
					}
					else
					{
						QStringList streams;
						QStringList contacts;
						QStringList names;
						foreach(const QString &streamJid, indexData.value(RDR_STREAMS).toStringList())
						{
							IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
							if (roster)
							{
								QString group = indexData.value(RDR_GROUP).toString();
								foreach(const IRosterItem &ritem, roster->groupItems(group))
								{
									streams.append(destStreams.value(0));
									contacts.append(ritem.itemJid.pBare());
									names.append(ritem.name);
								}
							}
						}
						action->setData(ADR_STREAM_JID,streams);
						action->setData(ADR_CONTACT_JID,contacts);
						action->setData(ADR_NAME,names);

						QString groupTo = AHover->data(RDR_GROUP).toString();
						groupTo += ROSTER_GROUP_DELIMITER;
						groupTo += indexData.value(RDR_GROUP).toString().split(ROSTER_GROUP_DELIMITER).last();
						action->setData(ADR_TO_GROUP,groupTo);

						action->setText(tr("Add Group with Contacts"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
						connect(action,SIGNAL(triggered(bool)),SLOT(onAddContactsToGroup(bool)));
					}

					AMenu->addAction(action,AG_DEFAULT,true);
					AMenu->setDefaultAction(action);
					return true;
				}
			}
		}
	}
	return false;
}

quint32 RosterChanger::rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const
{
	static const QList<int> acceptKinds = QList<int>() << RIK_GROUP << RIK_CONTACT << RIK_AGENT << RIK_METACONTACT_ITEM;

	int indexKind = AIndex.data(RDR_KIND).toInt();
	if (AOrder==REHO_ROSTERCHANGER_RENAME && ADataRole==RDR_NAME && acceptKinds.contains(indexKind))
	{
		if (indexKind == RIK_GROUP)
		{
			if (isAllRostersOpened(AIndex.data(RDR_STREAMS).toStringList()))
				return AdvancedDelegateItem::DisplayId;
		}
		else if (isRosterOpened(AIndex.data(RDR_STREAM_JID).toString()))
		{
			return AdvancedDelegateItem::DisplayId;
		}
	}
	return AdvancedDelegateItem::NullId;
}

AdvancedDelegateEditProxy *RosterChanger::rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex)
{
	Q_UNUSED(AIndex);
	if (AOrder==REHO_ROSTERCHANGER_RENAME && ADataRole==RDR_NAME)
		return this;
	return NULL;
}

bool RosterChanger::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex)
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
			int indexKind = AIndex.data(RDR_KIND).toInt();
			if (indexKind == RIK_GROUP)
			{
				foreach(const Jid &streamJid, AIndex.data(RDR_STREAMS).toStringList())
				{
					IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
					if (roster && roster->isOpen())
					{
						QString newGroup = AIndex.data(RDR_GROUP).toString();
						newGroup.chop(oldName.size());
						newGroup += newName;
						roster->renameGroup(AIndex.data(RDR_GROUP).toString(),newGroup);
					}
				}
			}
			else
			{
				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
				if (roster && roster->isOpen())
					roster->renameItem(AIndex.data(RDR_PREP_BARE_JID).toString(),newName);
			}
		}
		return true;
	}
	return false;
}

bool RosterChanger::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "roster")
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && !roster->hasItem(AContactJid))
		{
			IAddContactDialog *dialog = showAddContactDialog(AStreamJid);
			if (dialog)
			{
				dialog->setContactJid(AContactJid);
				dialog->setNickName(AParams.contains("name") ? AParams.value("name") : AContactJid.uNode());
				dialog->setGroup(AParams.contains("group") ? AParams.value("group") : QString::null);
				dialog->instance()->show();
			}
		}
		return true;
	}
	else if (AAction == "remove")
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && roster->hasItem(AContactJid))
		{
			if (QMessageBox::question(NULL, tr("Remove contact"),
				tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(Qt::escape(AContactJid.uBare())),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItem(AContactJid);
			}
		}
		return true;
	}
	else if (AAction == "subscribe")
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->findItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
		{
			if (QMessageBox::question(NULL, tr("Subscribe for contact presence"),
				tr("You are assured that wish to subscribe for a contact <b>%1</b> presence?").arg(Qt::escape(AContactJid.uBare())),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->sendSubscription(AContactJid, IRoster::Subscribe);
			}
		}
		return true;
	}
	else if (AAction == "unsubscribe")
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->findItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_NONE && ritem.subscription!=SUBSCRIPTION_FROM)
		{
			if (QMessageBox::question(NULL, tr("Unsubscribe from contact presence"),
				tr("You are assured that wish to unsubscribe from a contact <b>%1</b> presence?").arg(Qt::escape(AContactJid.uBare())),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->sendSubscription(AContactJid, IRoster::Unsubscribe);
			}
		}
		return true;
	}
	return false;
}

//IRosterChanger
bool RosterChanger::isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (Options::node(OPV_ROSTER_AUTOSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoSubscribe;
	return false;
}

bool RosterChanger::isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE).value().toBool())
		return true;
	else if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoUnsubscribe;
	return false;
}

bool RosterChanger::isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
		return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).silent;
	return false;
}

void RosterChanger::insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, bool ASilently, bool ASubscr, bool AUnsubscr)
{
	AutoSubscription &asubscr = FAutoSubscriptions[AStreamJid][AContactJid.bare()];
	asubscr.silent = ASilently;
	asubscr.autoSubscribe = ASubscr;
	asubscr.autoUnsubscribe = AUnsubscr;
	LOG_STRM_INFO(AStreamJid,QString("Inserted auto subscription, jid=%1, silent=%2, subscribe=%3, unsubscribe=%4").arg(AContactJid.bare()).arg(ASilently).arg(ASubscr).arg(AUnsubscr));
}

void RosterChanger::removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
	{
		FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
		LOG_STRM_INFO(AStreamJid,QString("Removed auto subscription, jid=%1").arg(AContactJid.bare()));
	}
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		LOG_STRM_INFO(AStreamJid,QString("Subscribing contact, jid=%1, silent=%2").arg(AContactJid.bare()).arg(ASilently));
		const IRosterItem &ritem = roster->findItem(AContactJid);
		if (roster->subscriptionRequests().contains(AContactJid.bare()))
			roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,true,false);
	}
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		LOG_STRM_INFO(AStreamJid,QString("Unsubscribing contact, jid=%1, silent=%2").arg(AContactJid.bare()).arg(ASilently));
		const IRosterItem &ritem = roster->findItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Unsubscribed,AMessage);
		if (ritem.subscriptionAsk==SUBSCRIPTION_SUBSCRIBE || ritem.subscription==SUBSCRIPTION_TO || ritem.subscription==SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Unsubscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,false,true);
	}
}

IAddContactDialog *RosterChanger::showAddContactDialog(const Jid &AStreamJid)
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		AddContactDialog *dialog = new AddContactDialog(this,FPluginManager,AStreamJid);
		connect(roster->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
		emit addContactDialogCreated(dialog);
		dialog->show();
		return dialog;
	}
	return NULL;
}

QString RosterChanger::subscriptionNotify(int ASubsType, const Jid &AContactJid) const
{
	switch (ASubsType)
	{
	case IRoster::Subscribe:
		return tr("%1 wants to subscribe to your presence.").arg(AContactJid.uBare());
	case IRoster::Subscribed:
		return tr("You are now subscribed for %1 presence.").arg(AContactJid.uBare());
	case IRoster::Unsubscribe:
		return tr("%1 unsubscribed from your presence.").arg(AContactJid.uBare());
	case IRoster::Unsubscribed:
		return tr("You are now unsubscribed from %1 presence.").arg(AContactJid.uBare());
	}
	return QString::null;
}

QList<int> RosterChanger::findNotifies(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QList<int> notifies;
	if (FNotifications)
	{
		foreach(int notifyId, FNotifySubsDialog.keys())
		{
			INotification notify = FNotifications->notificationById(notifyId);
			if (AStreamJid==notify.data.value(NDR_STREAM_JID).toString() && (AContactJid && notify.data.value(NDR_CONTACT_JID).toString()))
				notifies.append(notifyId);
		}
	}
	return notifies;
}

void RosterChanger::removeObsoleteNotifies(const Jid &AStreamJid, const Jid &AContactJid, int ASubsType, bool ASent)
{
	foreach(int notifyId, findNotifies(AStreamJid, AContactJid))
	{
		int subsType = FNotifySubsType.value(notifyId,-1);

		bool remove = false;
		if (subsType == IRoster::Subscribe)
		{
			if (ASent)
				remove = ASubsType==IRoster::Subscribed || ASubsType==IRoster::Unsubscribed;
			else
				remove = ASubsType==IRoster::Unsubscribe;
		}
		else if (subsType == IRoster::Subscribed)
		{
			if (!ASent)
				remove = ASubsType==IRoster::Unsubscribed;
		}
		else if (subsType == IRoster::Unsubscribe)
		{
			if (!ASent)
				remove = ASubsType==IRoster::Subscribe;
		}
		else if (subsType == IRoster::Unsubscribed)
		{
			if (ASent)
				remove = ASubsType==IRoster::Subscribe;
			else
				remove = ASubsType==IRoster::Subscribed;
		}

		if (remove)
			FNotifications->removeNotification(notifyId);
	}
}

SubscriptionDialog *RosterChanger::findSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach (SubscriptionDialog *dialog, FSubsDialogs)
		if (dialog && dialog->streamJid()==AStreamJid && dialog->contactJid()==AContactJid)
			return dialog;
	return NULL;
}

SubscriptionDialog *RosterChanger::createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage)
{
	SubscriptionDialog *dialog = findSubscriptionDialog(AStreamJid,AContactJid);
	if (dialog != NULL)
	{
		dialog->reject();
		dialog = NULL;
	}

	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		dialog = new SubscriptionDialog(this,FPluginManager,AStreamJid,AContactJid,ANotify,AMessage);
		connect(roster->instance(),SIGNAL(closed()),dialog->instance(),SLOT(reject()));
		connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onSubscriptionDialogDestroyed()));
		FSubsDialogs.append(dialog);
		emit subscriptionDialogCreated(dialog);
	}
	else if (roster)
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to create subscription dialog: Roster is not opened");
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,"Failed to create subscription dialog: Roster not found");
	}

	return dialog;
}

bool RosterChanger::isRosterOpened(const Jid &AStreamJid) const
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	return roster!=NULL && roster->isOpen();
}

bool RosterChanger::isAllRostersOpened(const QStringList &AStreams) const
{
	foreach(const QString &streamJid, AStreams)
		if (!isRosterOpened(streamJid))
			return false;
	return !AStreams.isEmpty();
}

bool RosterChanger::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> acceptKinds = QList<int>() << RIK_STREAM_ROOT << RIK_CONTACT << RIK_AGENT << RIK_GROUP << RIK_METACONTACT << RIK_METACONTACT_ITEM;

	int singleKind = -1;
	foreach(IRosterIndex *index, ASelected)
	{
		int indexKind = index->kind();
		if (!acceptKinds.contains(indexKind))
			return false;
		else if (singleKind!=-1 && singleKind!=indexKind)
			return false;
		else if (indexKind==RIK_STREAM_ROOT && ASelected.count()>1)
			return false;
		else if (indexKind==RIK_GROUP && !isAllRostersOpened(index->data(RDR_STREAMS).toStringList()))
			return false;
		else if (indexKind!=RIK_GROUP && !isRosterOpened(index->data(RDR_STREAM_JID).toString()))
			return false;
		singleKind = indexKind;
	}
	return !ASelected.isEmpty();
}

QMap<int, QStringList> RosterChanger::metaIndexesRolesMap(const QList<IRosterIndex *> &AIndexes) const
{
	QMap<int, QStringList> rolesMap;
	foreach(IRosterIndex *index, AIndexes)
	{
		for(int row=0; row<index->childCount(); row++)
		{
			IRosterIndex *metaItemIndex = index->childIndex(row);
			rolesMap[RDR_STREAM_JID].append(metaItemIndex->data(RDR_STREAM_JID).toString());
			rolesMap[RDR_PREP_FULL_JID].append(metaItemIndex->data(RDR_PREP_FULL_JID).toString());
			rolesMap[RDR_PREP_BARE_JID].append(metaItemIndex->data(RDR_PREP_BARE_JID).toString());
			rolesMap[RDR_GROUP].append(metaItemIndex->data(RDR_GROUP).toString());
		}
	}
	return rolesMap;
}

QMap<int, QStringList> RosterChanger::groupIndexesRolesMap(const QList<IRosterIndex *> &AIndexes) const
{
	QMap<int, QStringList> rolesMap;
	foreach(IRosterIndex *index, AIndexes)
	{
		QString group = index->data(RDR_GROUP).toString();
		foreach(const QString &streamJid, index->data(RDR_STREAMS).toStringList())
		{
			rolesMap[RDR_STREAM_JID].append(streamJid);
			rolesMap[RDR_GROUP].append(group);
		}
	}
	return rolesMap;
}

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, bool ANewGroup, bool ARootGroup, bool ABlankGroup, const char *ASlot, Menu *AParent)
{
	QStringList groupStreams;
	if (FRostersModel && FRostersModel->streamsLayout()==IRostersModel::LayoutMerged)
		groupStreams = FRostersModel->contactsRoot()->data(RDR_STREAMS).toStringList();
	else
		groupStreams = AData.value(ADR_STREAM_JID).toStringList();

	Menu *menu = new Menu(AParent);
	QHash<QString, Menu *> groupMenuMap;
	foreach (const Jid &streamJid, groupStreams)
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
		foreach(const QString &group, roster!=NULL ? roster->groups() : QSet<QString>())
		{
			QString groupPath;
			Menu *parentMenu = menu;
			QList<QString> groupTree = group.split(ROSTER_GROUP_DELIMITER);
			for (int index=0; index<groupTree.count(); index++)
			{
				if (groupPath.isEmpty())
					groupPath = groupTree.at(index);
				else
					groupPath += ROSTER_GROUP_DELIMITER + groupTree.at(index);

				if (!AExceptGroups.contains(groupPath+ROSTER_GROUP_DELIMITER))
				{
					if (!groupMenuMap.contains(groupPath))
					{
						Menu *groupMenu = new Menu(parentMenu);
						groupMenu->setTitle(groupTree.at(index));
						groupMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);

						if (!AExceptGroups.contains(groupPath))
						{
							Action *curGroupAction = new Action(groupMenu);
							curGroupAction->setData(AData);
							curGroupAction->setText(tr("In this Group"));
							curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_THIS_GROUP);
							curGroupAction->setData(ADR_TO_GROUP,groupPath);
							connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
							groupMenu->addAction(curGroupAction,AG_RVCM_RCHANGER+1);
						}

						if (ANewGroup)
						{
							Action *newGroupAction = new Action(groupMenu);
							newGroupAction->setData(AData);
							newGroupAction->setText(tr("Create New..."));
							newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
							newGroupAction->setData(ADR_TO_GROUP,groupPath+ROSTER_GROUP_DELIMITER);
							connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
							groupMenu->addAction(newGroupAction,AG_RVCM_RCHANGER+1);
						}

						groupMenuMap.insert(groupPath,groupMenu);
						parentMenu->addAction(groupMenu->menuAction(),AG_RVCM_RCHANGER,true);
						parentMenu = groupMenu;
					}
					else
					{
						parentMenu = groupMenuMap.value(groupPath);
					}
				}
				else
				{
					break;
				}
			}
		}
	}

	if (ABlankGroup)
	{
		Action *blankGroupAction = new Action(menu);
		blankGroupAction->setData(AData);
		blankGroupAction->setText(FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_BLANK) : tr("Without Groups"));
		blankGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);
		blankGroupAction->setData(ADR_TO_GROUP,QString());
		connect(blankGroupAction,SIGNAL(triggered(bool)),ASlot);
		menu->addAction(blankGroupAction,AG_RVCM_RCHANGER+1);
	}

	if (ARootGroup)
	{
		Action *rootGroupAction = new Action(menu);
		rootGroupAction->setData(AData);
		rootGroupAction->setText(tr("To Root"));
		rootGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ROOT_GROUP);
		rootGroupAction->setData(ADR_TO_GROUP,QString());
		connect(rootGroupAction,SIGNAL(triggered(bool)),ASlot);
		menu->addAction(rootGroupAction,AG_RVCM_RCHANGER+1);
	}

	if (ANewGroup)
	{
		Action *newGroupAction = new Action(menu);
		newGroupAction->setData(AData);
		newGroupAction->setText(tr("Create New..."));
		newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
		newGroupAction->setData(ADR_TO_GROUP,ROSTER_GROUP_DELIMITER);
		connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
		menu->addAction(newGroupAction,AG_RVCM_RCHANGER+1);
	}

	return menu;
}

void RosterChanger::sendSubscription(const QStringList &AStreams, const QStringList &AContacts, int ASubsc) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count())
	{
		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
				roster->sendSubscription(AContacts.at(i),ASubsc);
		}
	}
}

void RosterChanger::changeSubscription(const QStringList &AStreams, const QStringList &AContacts, int ASubsc)
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count())
	{
		for(int i=0; i<AStreams.count(); i++)
		{
			if (isRosterOpened(AStreams.at(i)))
			{
				if (ASubsc == IRoster::Subscribe)
					subscribeContact(AStreams.at(i),AContacts.at(i));
				else if (ASubsc == IRoster::Unsubscribe)
					unsubscribeContact(AStreams.at(i),AContacts.at(i));
			}
		}
	}
}

void RosterChanger::renameContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AOldName) const
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && roster->hasItem(AContactJid))
	{
		QString newName = QInputDialog::getText(NULL,tr("Rename Contact"),tr("Enter name for: <b>%1</b>").arg(Qt::escape(AContactJid.uBare())),QLineEdit::Normal,AOldName);
		if (!newName.isEmpty() && newName!=AOldName)
			roster->renameItem(AContactJid,newName);
	}
}

void RosterChanger::addContactsToGroup(const QStringList &AStreams, const QStringList &AContacts, const QStringList &ANames, const QString &AGroup) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AContacts.count()==ANames.count())
	{
		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				IRosterItem ritem = roster->findItem(AContacts.at(i));
				if (ritem.isNull())
					roster->setItem(AContacts.at(i),ANames.at(i),QSet<QString>()<<AGroup);
				else
					roster->copyItemToGroup(ritem.itemJid,AGroup);
			}
		}
	}
}

void RosterChanger::copyContactsToGroup(const QStringList &AStreams, const QStringList &AContacts, const QString &AGroup) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && isAllRostersOpened(AStreams))
	{
		QString newGroupName;
		if (AGroup.endsWith(ROSTER_GROUP_DELIMITER))
			newGroupName = QInputDialog::getText(NULL,tr("Create Group"),tr("Enter group name:"));

		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				if (!newGroupName.isEmpty())
					roster->copyItemToGroup(AContacts.at(i),AGroup==ROSTER_GROUP_DELIMITER ? newGroupName : AGroup+newGroupName);
				else if (!AGroup.endsWith(ROSTER_GROUP_DELIMITER))
					roster->copyItemToGroup(AContacts.at(i),AGroup);
			}
		}
	}
}

void RosterChanger::moveContactsToGroup(const QStringList &AStreams, const QStringList &AContacts, const QStringList &AGroupsFrom, const QString &AGroupTo) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AStreams.count()==AGroupsFrom.count() && isAllRostersOpened(AStreams))
	{
		QString newGroupName;
		if (AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
			newGroupName = QInputDialog::getText(NULL,tr("Create Group"),tr("Enter group name:"));

		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				QString groupFrom = AGroupsFrom.at(i);
				if (!newGroupName.isEmpty())
					roster->moveItemToGroup(AContacts.at(i),groupFrom,AGroupTo==ROSTER_GROUP_DELIMITER ? newGroupName : AGroupTo+newGroupName);
				else if (!AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
					roster->moveItemToGroup(AContacts.at(i),groupFrom,AGroupTo);
			}
		}
	}
}

void RosterChanger::removeContactsFromGroups(const QStringList &AStreams, const QStringList &AContacts, const QStringList &AGroups) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && AStreams.count()==AGroups.count())
	{
		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
				roster->removeItemFromGroup(AContacts.at(i),AGroups.at(i));
		}
	}
}

void RosterChanger::removeContactsFromRoster(const QStringList &AStreams, const QStringList &AContacts) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AContacts.count() && isAllRostersOpened(AStreams))
	{
		int button = QMessageBox::No;
		if (AContacts.count() == 1)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.first()) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->findItem(AContacts.first()) : IRosterItem();
			QString name = !ritem.isNull() && !ritem.name.isEmpty() ? ritem.name : Jid(AContacts.first()).uBare();
			if (!ritem.isNull())
			{
				button = QMessageBox::question(NULL,tr("Remove Contact"),
					tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(Qt::escape(name)),
					QMessageBox::Yes | QMessageBox::No);
			}
			else 
			{
				button = QMessageBox::Yes;
			}
		}
		else
		{
			button = QMessageBox::question(NULL,tr("Remove Contacts"),
				tr("You are assured that wish to remove <b>%n contact(s)</b> from roster?","",AContacts.count()),
				QMessageBox::Yes | QMessageBox::No);
		}

		for(int i=0; button==QMessageBox::Yes && i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				IRosterItem ritem = roster->findItem(AContacts.at(i));
				if (ritem.isNull())
				{
					QMultiMap<int, QVariant> findData;
					findData.insertMulti(RDR_KIND,RIK_CONTACT);
					findData.insertMulti(RDR_KIND,RIK_AGENT);
					findData.insertMulti(RDR_STREAM_JID,AStreams.at(i));
					findData.insertMulti(RDR_PREP_BARE_JID,AContacts.at(i));

					IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreams.at(i)) : NULL;
					IRosterIndex *group = sroot!=NULL ? FRostersModel->findGroupIndex(RIK_GROUP_NOT_IN_ROSTER,QString::null,sroot) : NULL;
					if (group)
					{
						foreach(IRosterIndex *index, group->findChilds(findData,true))
							FRostersModel->removeRosterIndex(index);
					}
				}
				else
				{
					roster->removeItem(AContacts.at(i));
				}
			}
		}
	}
}

void RosterChanger::renameGroups(const QStringList &AStreams, const QStringList &AGroups, const QString &AOldName) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count() && isAllRostersOpened(AStreams))
	{
		QString newGroupPart = QInputDialog::getText(NULL,tr("Rename Group"),tr("Enter group name:"),QLineEdit::Normal,AOldName);
		for(int i=0; !newGroupPart.isEmpty() && newGroupPart!=AOldName && i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				QString newGroupName = AGroups.at(i);
				QList<QString> groupTree = newGroupName.split(ROSTER_GROUP_DELIMITER);
				newGroupName.chop(groupTree.last().size());
				newGroupName += newGroupPart;
				roster->renameGroup(AGroups.at(i),newGroupName);
			}
		}
	}
}

void RosterChanger::copyGroupsToGroup(const QStringList &AStreams, const QStringList &AGroups, const QString &AGroupTo) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count() && isAllRostersOpened(AStreams))
	{
		QString newGroupName;
		if (AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
			newGroupName = QInputDialog::getText(NULL,tr("Create Group"),tr("Enter group name:"));

		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				if (!newGroupName.isEmpty())
					roster->copyGroupToGroup(AGroups.at(i),AGroupTo==ROSTER_GROUP_DELIMITER ? newGroupName : AGroupTo+newGroupName);
				else if (!AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
					roster->copyGroupToGroup(AGroups.at(i),AGroupTo);
			}
		}
	}
}

void RosterChanger::moveGroupsToGroup(const QStringList &AStreams, const QStringList &AGroups, const QString &AGroupTo) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count() && isAllRostersOpened(AStreams))
	{
		QString newGroupName;
		if (AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
			newGroupName = QInputDialog::getText(NULL,tr("Create Group"),tr("Enter group name:"));

		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
			{
				if (!newGroupName.isEmpty())
					roster->moveGroupToGroup(AGroups.at(i),AGroupTo==ROSTER_GROUP_DELIMITER ? newGroupName : AGroupTo+newGroupName);
				else if (!AGroupTo.endsWith(ROSTER_GROUP_DELIMITER))
					roster->moveGroupToGroup(AGroups.at(i),AGroupTo);
			}
		}
	}
}

void RosterChanger::removeGroups(const QStringList &AStreams, const QStringList &AGroups) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count())
	{
		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
				roster->removeGroup(AGroups.at(i));
		}
	}
}

void RosterChanger::removeGroupsContacts(const QStringList &AStreams, const QStringList &AGroups) const
{
	if (!AStreams.isEmpty() && AStreams.count()==AGroups.count())
	{
		int itemsCount = 0;
		for(int i=0; i<AStreams.count(); i++)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
			if (roster && roster->isOpen())
				itemsCount += roster->groupItems(AGroups.at(i)).count();
		}

		if (itemsCount > 0)
		{
			int button = QMessageBox::question(NULL,tr("Remove Contacts"),
				tr("You are assured that wish to remove <b>%n contact(s)</b> from roster?","",itemsCount),
				QMessageBox::Yes | QMessageBox::No);

			for(int i=0; button==QMessageBox::Yes && i<AStreams.count(); i++)
			{
				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreams.at(i)) : NULL;
				if (roster && roster->isOpen())
					foreach(const IRosterItem &ritem, roster->groupItems(AGroups.at(i)))
						roster->removeItem(ritem.itemJid);
			}
		}
	}
}

bool RosterChanger::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		if (FNotifications)
		{
			int notifyId = FNotifySubsDialog.key(qobject_cast<SubscriptionDialog *>(AObject));
			if (notifyId > 0)
				FNotifications->activateNotification(notifyId);
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void RosterChanger::onSendSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		sendSubscription(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_SUBSCRIPTION).toInt());
}

void RosterChanger::onChangeSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		changeSubscription(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_SUBSCRIPTION).toInt());
}

void RosterChanger::onSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Q_UNUSED(AText);
	removeObsoleteNotifies(ARoster->streamJid(),AItemJid,ASubsType,true);
}

void RosterChanger::onSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AMessage)
{
	INotification notify;
	if (FNotifications)
	{
		removeObsoleteNotifies(ARoster->streamJid(),AItemJid,ASubsType,false);
		notify.kinds =  FNotifications->enabledTypeNotificationKinds(NNT_SUBSCRIPTION_REQUEST);
		if (ASubsType != IRoster::Subscribe)
			notify.kinds = (notify.kinds & INotification::PopupWindow);
		notify.typeId = NNT_SUBSCRIPTION_REQUEST;
		notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCRIBTION));
		notify.data.insert(NDR_TOOLTIP,tr("Subscription message from %1").arg(FNotifications->contactName(ARoster->streamJid(),AItemJid)));
		notify.data.insert(NDR_STREAM_JID,ARoster->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID,AItemJid.full());
		notify.data.insert(NDR_ROSTER_ORDER,RNO_SUBSCRIPTION);
		notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
		notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
		notify.data.insert(NDR_POPUP_CAPTION, tr("Subscription message"));
		notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(ARoster->streamJid(),AItemJid));
		notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AItemJid));
		notify.data.insert(NDR_POPUP_TEXT,subscriptionNotify(ASubsType,AItemJid));
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
	}

	int notifyId = -1;
	SubscriptionDialog *dialog = NULL;
	const IRosterItem &ritem = ARoster->findItem(AItemJid);
	if (ASubsType == IRoster::Subscribe)
	{
		if (!isAutoSubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
		{
			dialog = createSubscriptionDialog(ARoster->streamJid(),AItemJid,subscriptionNotify(ASubsType,AItemJid),AMessage);
			if (dialog)
			{
				if (FNotifications)
				{
					if (notify.kinds > 0)
					{
						dialog->installEventFilter(this);
						notify.data.insert(NDR_ALERT_WIDGET,(qint64)dialog);
						notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)dialog);
						notifyId = FNotifications->appendNotification(notify);
					}
					else
					{
						dialog->deleteLater();
					}
				}
				else
				{
					dialog->instance()->show();
				}
			}
		}
		else
		{
			ARoster->sendSubscription(AItemJid,IRoster::Subscribed);
			if (isAutoSubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
				ARoster->sendSubscription(AItemJid,IRoster::Subscribe);
		}
	}
	else if (ASubsType == IRoster::Unsubscribed)
	{
		if (FNotifications && notify.kinds>0 && !isSilentSubsctiption(ARoster->streamJid(),AItemJid))
			notifyId = FNotifications->appendNotification(notify);

		if (isAutoUnsubscribe(ARoster->streamJid(),AItemJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_NONE)
			ARoster->sendSubscription(AItemJid,IRoster::Unsubscribed);
	}
	else  if (ASubsType == IRoster::Subscribed)
	{
		if (FNotifications && notify.kinds>0 && !isSilentSubsctiption(ARoster->streamJid(),AItemJid))
			notifyId = FNotifications->appendNotification(notify);
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		if (FNotifications && notify.kinds>0 && !isSilentSubsctiption(ARoster->streamJid(),AItemJid))
			notifyId = FNotifications->appendNotification(notify);
	}

	if (notifyId > 0)
	{
		FNotifySubsType.insert(notifyId,ASubsType);
		FNotifySubsDialog.insert(notifyId,dialog);
	}
}

void RosterChanger::onRenameContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterManager ? FRosterManager->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			bool editInRoster = false;
			Jid contactJid = action->data(ADR_CONTACT_JID).toStringList().value(0);
			if (FRostersView && FRostersView->instance()->isActiveWindow() && FRostersView->rostersModel())
			{
				QString group = action->data(ADR_GROUP).toStringList().value(0);
				QList<IRosterIndex *> indexes = FRostersView->rostersModel()->findContactIndexes(streamJid,contactJid);
				foreach(IRosterIndex *index, indexes)
				{
					if (index->data(RDR_GROUP).toString() == group)
					{
						editInRoster = FRostersView->editRosterIndex(index,RDR_NAME);
						break;
					}
				}
			}
			if (!editInRoster)
			{
				renameContact(streamJid,contactJid,action->data(ADR_NAME).toString());
			}
		}
	}
}

void RosterChanger::onAddContactsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		addContactsToGroup(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_NAME).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onCopyContactsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyContactsToGroup(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveContactsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveContactsToGroup(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveContactsFromGroups(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactsFromGroups(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_GROUP).toStringList());
}

void RosterChanger::onRemoveContactsFromRoster(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactsFromRoster(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_CONTACT_JID).toStringList());
}

void RosterChanger::onRenameGroups(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList groups = action->data(ADR_GROUP).toStringList();
		if (!streams.isEmpty() && streams.count()==groups.count())
		{
			bool editInRoster = false;
			if (FRostersView && FRostersView->instance()->isActiveWindow() && FRostersModel)
			{
				for(int i=0; i<streams.count(); i++)
				{
					IRoster *roster = FRosterManager ? FRosterManager->findRoster(streams.at(i)) : NULL;
					if (roster && roster->isOpen())
					{
						IRosterIndex *sroot = FRostersModel->streamRoot(roster->streamJid());
						IRosterIndex *index = sroot!=NULL ? FRostersView->rostersModel()->findGroupIndex(RIK_GROUP,groups.at(i),sroot) : NULL;
						if (index!=NULL && FRostersView->editRosterIndex(index,RDR_NAME))
						{
							editInRoster = true;
							break;
						}
					}
				}
			}
			if (!editInRoster)
			{
				QString name = action->data(ADR_NAME).toString();
				renameGroups(streams,groups,name);
			}
		}
	}
}

void RosterChanger::onCopyGroupsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyGroupsToGroup(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveGroupsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveGroupsToGroup(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveGroups(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroups(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_GROUP).toStringList());
}

void RosterChanger::onRemoveGroupsContacts(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroupsContacts(action->data(ADR_STREAM_JID).toStringList(),action->data(ADR_GROUP).toStringList());
}

void RosterChanger::onShowAddContactDialog(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IAddContactDialog *dialog = showAddContactDialog(action->data(ADR_STREAM_JID).toString());
		if (dialog)
		{
			dialog->setContactJid(action->data(ADR_CONTACT_JID).toString());
			dialog->setNickName(action->data(ADR_NAME).toString());
			dialog->setGroup(action->data(ADR_GROUP).toString());
			dialog->setSubscriptionMessage(action->data(Action::DR_Parametr4).toString());
		}
	}
}

void RosterChanger::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Q_UNUSED(ABefore);
	if (AItem.subscription != ABefore.subscription)
	{
		if (AItem.subscription == SUBSCRIPTION_REMOVE)
		{
			if (isSilentSubsctiption(ARoster->streamJid(), AItem.itemJid))
				insertAutoSubscribe(ARoster->streamJid(), AItem.itemJid, true, false, false);
			else
				removeAutoSubscribe(ARoster->streamJid(), AItem.itemJid);
		}
		else if (AItem.subscription == SUBSCRIPTION_BOTH)
		{
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,true);
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,false);
		}
		else if (AItem.subscription == SUBSCRIPTION_FROM)
		{
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,true);
		}
		else if (AItem.subscription == SUBSCRIPTION_TO)
		{
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribed,false);
		}
	}
	else if (AItem.subscriptionAsk != ABefore.subscriptionAsk)
	{
		if (AItem.subscriptionAsk == SUBSCRIPTION_SUBSCRIBE)
		{
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribe,true);
		}
	}
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
	FAutoSubscriptions.remove(ARoster->streamJid());
}

void RosterChanger::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (isSelectionAccepted(indexes))
		{
			IRosterIndex *index = indexes.first();
			int indexKind = index->data(RDR_KIND).toInt();
			if (AId==SCT_ROSTERVIEW_ADDCONTACT && indexes.count()==1)
			{
				Jid streamJid;
				if (indexKind == RIK_GROUP)
					streamJid = index->data(RDR_STREAMS).toStringList().value(0);
				else
					streamJid = index->data(RDR_STREAM_JID).toString();

				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(streamJid) : NULL;
				if (roster && roster->isOpen())
				{
					IRosterItem ritem = roster->findItem(index->data(RDR_PREP_BARE_JID).toString());

					bool showDialog = indexKind==RIK_GROUP || indexKind==RIK_STREAM_ROOT;
					showDialog = showDialog || (ritem.isNull() && (indexKind==RIK_CONTACT || indexKind==RIK_AGENT));

					IAddContactDialog *dialog = showDialog ? showAddContactDialog(streamJid) : NULL;
					if (dialog)
					{
						if (indexKind == RIK_GROUP)
							dialog->setGroup(index->data(RDR_GROUP).toString());
						else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
							dialog->setContactJid(index->data(RDR_PREP_BARE_JID).toString());
					}
				}
			}
			else if (AId==SCT_ROSTERVIEW_RENAME && indexes.count()==1)
			{
				if (indexKind == RIK_GROUP)
				{
					if (!FRostersView->editRosterIndex(index, RDR_NAME))
					{
						QMap<int, QStringList> rolesMap = groupIndexesRolesMap(indexes);
						renameGroups(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_GROUP),index->data(RDR_NAME).toString());
					}
				}
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_METACONTACT_ITEM)
				{
					if (!FRostersView->editRosterIndex(index, RDR_NAME))
						renameContact(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString(),index->data(RDR_NAME).toString());
				}
			}
			else if (AId == SCT_ROSTERVIEW_REMOVEFROMGROUP)
			{
				if (indexKind == RIK_GROUP)
				{
					QMap<int, QStringList> rolesMap = groupIndexesRolesMap(indexes);
					removeGroups(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_GROUP));
				}
				else if (indexKind == RIK_METACONTACT)
				{
					QMap<int, QStringList> rolesMap = metaIndexesRolesMap(indexes);
					removeContactsFromGroups(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID),rolesMap.value(RDR_GROUP));
				}
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_METACONTACT_ITEM)
				{
					QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(indexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_GROUP);
					removeContactsFromGroups(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID),rolesMap.value(RDR_GROUP));
				}
			}
			else if (AId == SCT_ROSTERVIEW_REMOVEFROMROSTER)
			{
				if (indexKind == RIK_GROUP)
				{
					QMap<int, QStringList> rolesMap = groupIndexesRolesMap(indexes);
					removeGroupsContacts(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_GROUP));
				}
				else if (indexKind == RIK_METACONTACT)
				{
					QMap<int, QStringList> rolesMap = metaIndexesRolesMap(indexes);
					removeContactsFromRoster(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID));
				}
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_METACONTACT_ITEM)
				{
					QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(indexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID,RDR_PREP_BARE_JID,RDR_STREAM_JID);
					removeContactsFromRoster(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID));
				}
			}
		}
	}
}

void RosterChanger::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void RosterChanger::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		bool isMultiSelection = AIndexes.count()>1;
		IRosterIndex *firstIndex = AIndexes.first();
		int indexKind = firstIndex->kind();
		if (indexKind == RIK_STREAM_ROOT)
		{
			if (!isMultiSelection)
			{
				Action *action = new Action(AMenu);
				action->setText(tr("Add Contact..."));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				action->setData(ADR_STREAM_JID, firstIndex->data(RDR_STREAM_JID));
				action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
			}
		}
		else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
		{
			QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_GROUP<<RDR_NAME,RDR_PREP_BARE_JID,RDR_STREAM_JID);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
			data.insert(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
			data.insert(ADR_NAME,rolesMap.value(RDR_NAME));
			data.insert(ADR_GROUP,rolesMap.value(RDR_GROUP));

			Menu *subsMenu = new Menu(AMenu);
			subsMenu->setTitle(tr("Subscription"));
			subsMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBTION);
			AMenu->addAction(subsMenu->menuAction(),AG_RVCM_RCHANGER,true);

			Action *action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Subscribe Contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onChangeSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Unsubscribe Contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onChangeSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Send"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_SEND);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Request"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REQUEST);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Remove"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REMOVE);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setData(data);
			action->setText(tr("Refuse"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REFUSE);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			if (!isMultiSelection)
			{
				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(firstIndex->data(RDR_STREAM_JID).toString()) : NULL;
				IRosterItem ritem = roster!=NULL ? roster->findItem(firstIndex->data(RDR_PREP_BARE_JID).toString()) : IRosterItem();
				if (!ritem.isNull())
				{
					action = new Action(AMenu);
					action->setData(data);
					action->setText(tr("Rename"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
					action->setShortcutId(SCT_ROSTERVIEW_RENAME);
					connect(action,SIGNAL(triggered(bool)),SLOT(onRenameContact(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER,true);
				}
				else if (roster)
				{
					action = new Action(AMenu);
					action->setText(tr("Add to Roster..."));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
					action->setData(ADR_STREAM_JID,firstIndex->data(RDR_STREAM_JID));
					action->setData(ADR_CONTACT_JID,firstIndex->data(RDR_FULL_JID));
					action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
					connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
				}
			}

			QSet<QString> exceptGroups;
			bool isAllItemsValid = true;
			bool isAllInEmptyGroup = true;
			foreach(IRosterIndex *index, AIndexes)
			{
				IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(index->data(RDR_STREAM_JID).toString()) : NULL;
				IRosterItem ritem = roster!=NULL ? roster->findItem(index->data(RDR_PREP_BARE_JID).toString()) : IRosterItem();
				if (ritem.isNull())
				{
					isAllItemsValid = false;
					break;
				}
				else if (!index->data(RDR_GROUP).toString().isEmpty())
				{
					isAllInEmptyGroup = false;
				}
				exceptGroups = !exceptGroups.isEmpty() ? (exceptGroups & ritem.groups) : ritem.groups;
				exceptGroups += QString::null;
			}
			exceptGroups -= QString::null;

			if (isAllItemsValid)
			{
				if (indexKind == RIK_CONTACT)
				{
					if (!isAllInEmptyGroup)
					{
						Menu *copyMenu = createGroupMenu(data,exceptGroups,true,false,false,SLOT(onCopyContactsToGroup(bool)),AMenu);
						copyMenu->setTitle(tr("Copy to Group"));
						copyMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
						AMenu->addAction(copyMenu->menuAction(),AG_RVCM_RCHANGER);
					}

					Menu *moveMenu = createGroupMenu(data,exceptGroups,true,false,!isAllInEmptyGroup,SLOT(onMoveContactsToGroup(bool)),AMenu);
					moveMenu->setTitle(tr("Move to Group"));
					moveMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
					AMenu->addAction(moveMenu->menuAction(),AG_RVCM_RCHANGER);
				}

				if (!isAllInEmptyGroup)
				{
					QMap<int, QStringList> fullRolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_GROUP);

					action = new Action(AMenu);
					action->setText(tr("Remove from Group"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_FROM_GROUP);
					action->setData(ADR_STREAM_JID,fullRolesMap.value(RDR_STREAM_JID));
					action->setData(ADR_CONTACT_JID,fullRolesMap.value(RDR_PREP_BARE_JID));
					action->setData(ADR_GROUP,fullRolesMap.value(RDR_GROUP));
					action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
					connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactsFromGroups(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER);
				}
			}

			action = new Action(AMenu);
			action->setData(data);
			action->setText(tr("Remove from Roster"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactsFromRoster(bool)));
			AMenu->addAction(action,AG_RVCM_RCHANGER);
		}
		else if (indexKind == RIK_GROUP)
		{
			QMap<int, QStringList> rolesMap = groupIndexesRolesMap(AIndexes);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
			data.insert(ADR_GROUP,rolesMap.value(RDR_GROUP));

			if (!isMultiSelection)
			{
				if (rolesMap.value(RDR_STREAM_JID).count() == 1)
				{
					Action *action = new Action(AMenu);
					action->setText(tr("Add Contact..."));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
					action->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID).first());
					action->setData(ADR_GROUP,rolesMap.value(RDR_GROUP).first());
					action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
					connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
				}

				Action *action = new Action(AMenu);
				action->setData(data);
				action->setText(tr("Rename"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
				action->setShortcutId(SCT_ROSTERVIEW_RENAME);
				action->setData(ADR_NAME,firstIndex->data(RDR_NAME));
				connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroups(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER);
			}

			bool isAllInRoot = true;
			QSet<QString> exceptGroups;
			foreach(const QString &group, rolesMap.value(RDR_GROUP))
			{
				exceptGroups += group+ROSTER_GROUP_DELIMITER;
				exceptGroups += group.left(group.lastIndexOf(ROSTER_GROUP_DELIMITER));
				isAllInRoot = isAllInRoot && !group.contains(ROSTER_GROUP_DELIMITER);
			}

			Menu *copyMenu = createGroupMenu(data,exceptGroups,true,!isAllInRoot,false,SLOT(onCopyGroupsToGroup(bool)),AMenu);
			copyMenu->setTitle(tr("Copy to Group"));
			copyMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
			AMenu->addAction(copyMenu->menuAction(),AG_RVCM_RCHANGER);

			Menu *moveMenu = createGroupMenu(data,exceptGroups,true,!isAllInRoot,false,SLOT(onMoveGroupsToGroup(bool)),AMenu);
			moveMenu->setTitle(tr("Move to Group"));
			moveMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
			AMenu->addAction(moveMenu->menuAction(),AG_RVCM_RCHANGER);

			Action *action = new Action(AMenu);
			action->setData(data);
			action->setText(tr("Remove Group"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_GROUP);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroups(bool)));
			AMenu->addAction(action,AG_RVCM_RCHANGER);

			action = new Action(AMenu);
			action->setData(data);
			action->setText(tr("Remove Contacts"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACTS);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupsContacts(bool)));
			AMenu->addAction(action,AG_RVCM_RCHANGER);
		}
	}
}

void RosterChanger::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Q_UNUSED(AWindow);
	if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
	{
		IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AUser->data(MUDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen() && !roster->hasItem(AUser->data(MUDR_REAL_JID).toString()))
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Add Contact..."));
			action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REAL_JID));
			action->setData(ADR_NAME,AUser->data(MUDR_NICK_NAME));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_MUCM_ROSTERCHANGER,true);
		}
	}
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
	if (FNotifySubsDialog.contains(ANotifyId))
	{
		SubscriptionDialog *dialog = FNotifySubsDialog.take(ANotifyId);
		if (dialog)
			WidgetManager::showActivateRaiseWindow(dialog);
		FNotifications->removeNotification(ANotifyId);
	}
}

void RosterChanger::onNotificationRemoved(int ANotifyId)
{
	if (FNotifySubsDialog.contains(ANotifyId))
	{
		SubscriptionDialog *dialog = FNotifySubsDialog.take(ANotifyId);
		if (dialog)
			dialog->reject();
		FNotifySubsType.remove(ANotifyId);
	}
}

void RosterChanger::onSubscriptionDialogDestroyed()
{
	SubscriptionDialog *dialog = static_cast<SubscriptionDialog *>(sender());
	if (dialog)
	{
		FSubsDialogs.removeAll(dialog);
		int notifyId = FNotifySubsDialog.key(dialog);
		if (notifyId > 0)
			FNotifications->removeNotification(notifyId);
	}
}

Q_EXPORT_PLUGIN2(plg_rosterchanger, RosterChanger)
