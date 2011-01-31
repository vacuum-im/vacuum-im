#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_FROM_STREAM_JID Action::DR_Parametr2
#define ADR_SUBSCRIPTION    Action::DR_Parametr2
#define ADR_NICK            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_REQUEST         Action::DR_Parametr4
#define ADR_TO_GROUP        Action::DR_Parametr4

static const QList<int> DragGroups = QList<int>() << RIT_GROUP << RIT_GROUP_BLANK;

RosterChanger::RosterChanger()
{
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FRostersModel = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
	FXmppUriQueries = NULL;
	FMultiUserChatPlugin = NULL;
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

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscription(IRoster *, const Jid &, int, const QString &)),
				SLOT(onReceiveSubscription(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemRemoved(IRoster *, const IRosterItem &)),
				SLOT(onRosterItemRemoved(IRoster *, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)), SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
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

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
		if (FMultiUserChatPlugin)
		{
			connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_ADDCONTACT,tr("Add contact"),tr("Ins","Add contact"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_RENAME,tr("Rename contact/group"),tr("F2","Rename contact/group"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFROMGROUP,tr("Remove contact/group from group"),tr("Del","Remove contact/group from group"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_REMOVEFROMROSTER,tr("Remove contact/group from roster"),tr("Shift+Del","Remove contact/group from roster"),Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SUBSCRIBE,tr("Subscribe contact"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_UNSUBSCRIBE,tr("Unsubscribe contact"),QKeySequence::UnknownKey,Shortcuts::WidgetShortcut);

	if (FNotifications)
	{
		uchar kindMask = INotification::RosterIcon|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound|INotification::AutoActivate;
		uchar kindDefs = INotification::RosterIcon|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound;
		FNotifications->registerNotificationType(NNT_SUBSCRIPTION_REQUEST,OWO_NOTIFICATIONS_SUBSCRIPTION_REQUEST,tr("Subscription requests"),kindMask,kindDefs);
	}
	if (FRostersView)
	{
		FRostersView->insertDragDropHandler(this);
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_ADDCONTACT,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_RENAME,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFROMGROUP,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_REMOVEFROMROSTER,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SUBSCRIBE,FRostersView->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_UNSUBSCRIBE,FRostersView->instance());
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
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> RosterChanger::optionsWidgets(const QString &ANode, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANode == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_CHENGER, FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_AUTOSUBSCRIBE),tr("Auto accept subscription requests"),AParent));
		widgets.insertMulti(OWO_ROSTER_CHENGER, FOptionsManager->optionsNodeWidget(Options::node(OPV_ROSTER_AUTOUNSUBSCRIBE),tr("Auto unsubscribe contacts"),AParent));
	}
	return widgets;
}

//IRostersDragDropHandler
Qt::DropActions RosterChanger::rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent);
	Q_UNUSED(ADrag);
	int indexType = AIndex.data(RDR_TYPE).toInt();
	if (indexType==RIT_CONTACT || indexType==RIT_GROUP)
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

		int indexType = indexData.value(RDR_TYPE).toInt();
		if (indexType==RIT_CONTACT || indexType==RIT_GROUP)
			return true;
	}
	return false;
}

bool RosterChanger::rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover)
{
	Q_UNUSED(AEvent);
	int indexType = AHover.data(RDR_TYPE).toInt();
	if (DragGroups.contains(indexType) || indexType==RIT_STREAM_ROOT)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AHover.data(RDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen())
			return true;
	}
	return false;
}

void RosterChanger::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RosterChanger::rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu)
{
	int hoverType = AIndex.data(RDR_TYPE).toInt();
	if ((AEvent->dropAction() & Qt::CopyAction|Qt::MoveAction)>0 && (DragGroups.contains(hoverType) || hoverType==RIT_STREAM_ROOT))
	{
		Jid hoverStreamJid = AIndex.data(RDR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(hoverStreamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			operator>>(stream,indexData);

			int indexType = indexData.value(RDR_TYPE).toInt();
			Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
			bool isNewContact = indexType==RIT_CONTACT && !roster->rosterItem(indexData.value(RDR_BARE_JID).toString()).isValid;

			if (!isNewContact && (hoverStreamJid && indexStreamJid))
			{
				if (AEvent->dropAction() == Qt::CopyAction)
				{
					Action *copyAction = new Action(AMenu);
					copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
					copyAction->setData(ADR_TO_GROUP,AIndex.data(RDR_GROUP));
					if (indexType == RIT_CONTACT)
					{
						copyAction->setText(tr("Copy contact"));
						copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyContactToGroup(bool)));
						AMenu->addAction(copyAction,AG_DEFAULT,true);
					}
					else
					{
						copyAction->setText(tr("Copy group"));
						copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyGroupToGroup(bool)));
						AMenu->addAction(copyAction,AG_DEFAULT,true);
					}
					AMenu->setDefaultAction(copyAction);
					return true;
				}
				else if (AEvent->dropAction() == Qt::MoveAction)
				{
					Action *moveAction = new Action(AMenu);
					moveAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
					moveAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
					moveAction->setData(ADR_TO_GROUP,AIndex.data(RDR_GROUP));
					if (indexType == RIT_CONTACT)
					{
						moveAction->setText(tr("Move contact"));
						moveAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveContactToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					else
					{
						moveAction->setText(tr("Move group"));
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveGroupToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					AMenu->setDefaultAction(moveAction);
					return true;
				}
			}
			else
			{
				Action *copyAction = new Action(AMenu);
				copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				copyAction->setData(ADR_TO_GROUP,AIndex.data(RDR_GROUP));

				if (indexType == RIT_CONTACT)
				{
					copyAction->setText(isNewContact ? tr("Add contact") : tr("Copy contact"));
					copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_BARE_JID));
					copyAction->setData(ADR_NICK,indexData.value(RDR_NAME));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onAddContactToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);
				}
				else
				{
					copyAction->setText(tr("Copy group"));
					copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					copyAction->setData(ADR_FROM_STREAM_JID,indexStreamJid.full());
					copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(copyAction,SIGNAL(triggered(bool)),SLOT(onAddGroupToGroup(bool)));
					AMenu->addAction(copyAction,AG_DEFAULT,true);
				}
				AMenu->setDefaultAction(copyAction);
				return true;
			}
		}
	}
	return false;
}

bool RosterChanger::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "roster")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && !roster->rosterItem(AContactJid).isValid)
		{
			IAddContactDialog *dialog = showAddContactDialog(AStreamJid);
			if (dialog)
			{
				dialog->setContactJid(AContactJid);
				dialog->setNickName(AParams.contains("name") ? AParams.value("name") : AContactJid.node());
				dialog->setGroup(AParams.contains("group") ? AParams.value("group") : QString::null);
				dialog->instance()->show();
			}
		}
		return true;
	}
	else if (AAction == "remove")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && roster->rosterItem(AContactJid).isValid)
		{
			if (QMessageBox::question(NULL, tr("Remove contact"),
			                          tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(AContactJid.hBare()),
			                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItem(AContactJid);
			}
		}
		return true;
	}
	else if (AAction == "subscribe")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
		{
			if (QMessageBox::question(NULL, tr("Subscribe for contact presence"),
			                          tr("You are assured that wish to subscribe for a contact <b>%1</b> presence?").arg(AContactJid.hBare()),
			                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->sendSubscription(AContactJid, IRoster::Subscribe);
			}
		}
		return true;
	}
	else if (AAction == "unsubscribe")
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
		if (roster && roster->isOpen() && ritem.subscription!=SUBSCRIPTION_NONE && ritem.subscription!=SUBSCRIPTION_FROM)
		{
			if (QMessageBox::question(NULL, tr("Unsubscribe from contact presence"),
			                          tr("You are assured that wish to unsubscribe from a contact <b>%1</b> presence?").arg(AContactJid.hBare()),
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
}

void RosterChanger::removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid)
{
	FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		const IRosterItem &ritem = roster->rosterItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,true,false);
	}
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		const IRosterItem &ritem = roster->rosterItem(AContactJid);
		roster->sendSubscription(AContactJid,IRoster::Unsubscribed,AMessage);
		if (ritem.ask==SUBSCRIPTION_SUBSCRIBE || ritem.subscription==SUBSCRIPTION_TO || ritem.subscription==SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Unsubscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,false,true);
	}
}

IAddContactDialog *RosterChanger::showAddContactDialog(const Jid &AStreamJid)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
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
		return tr("%1 wants to subscribe to your presence.").arg(AContactJid.hBare());
	case IRoster::Subscribed:
		return tr("You are now subscribed for %1 presence.").arg(AContactJid.hBare());
	case IRoster::Unsubscribe:
		return tr("%1 unsubscribed from your presence.").arg(AContactJid.hBare());
	case IRoster::Unsubscribed:
		return tr("You are now unsubscribed from %1 presence.").arg(AContactJid.hBare());
	}
	return QString::null;
}

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, bool ANewGroup, bool ARootGroup, bool ABlankGroup, const char *ASlot, Menu *AParent)
{
	Menu *menu = new Menu(AParent);
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AData.value(ADR_STREAM_JID).toString()) : NULL;
	if (roster)
	{
		QString group;
		QString groupDelim = roster->groupDelimiter();
		QHash<QString,Menu *> menus;
		QSet<QString> allGroups = roster->groups();
		foreach(group,allGroups)
		{
			Menu *parentMenu = menu;
			QList<QString> groupTree = group.split(groupDelim,QString::SkipEmptyParts);
			QString groupName;
			int index = 0;
			while (index < groupTree.count())
			{
				if (groupName.isEmpty())
					groupName = groupTree.at(index);
				else
					groupName += groupDelim + groupTree.at(index);

				if (!menus.contains(groupName))
				{
					Menu *groupMenu = new Menu(parentMenu);
					groupMenu->setTitle(groupTree.at(index));
					groupMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);

					if (!AExceptGroups.contains(groupName))
					{
						Action *curGroupAction = new Action(groupMenu);
						curGroupAction->setText(tr("This group"));
						curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_THIS_GROUP);
						curGroupAction->setData(AData);
						curGroupAction->setData(ADR_TO_GROUP,groupName);
						connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
						groupMenu->addAction(curGroupAction,AG_RVCM_RCHANGER+1);
					}

					if (ANewGroup)
					{
						Action *newGroupAction = new Action(groupMenu);
						newGroupAction->setText(tr("Create new..."));
						newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
						newGroupAction->setData(AData);
						newGroupAction->setData(ADR_TO_GROUP,groupName+groupDelim);
						connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
						groupMenu->addAction(newGroupAction,AG_RVCM_RCHANGER+1);
					}

					menus.insert(groupName,groupMenu);
					parentMenu->addAction(groupMenu->menuAction(),AG_RVCM_RCHANGER,true);
					parentMenu = groupMenu;
				}
				else
					parentMenu = menus.value(groupName);

				index++;
			}
		}

		if (ARootGroup)
		{
			Action *curGroupAction = new Action(menu);
			curGroupAction->setText(tr("Root"));
			curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ROOT_GROUP);
			curGroupAction->setData(AData);
			curGroupAction->setData(ADR_TO_GROUP,QVariant(QString("")));
			connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(curGroupAction,AG_RVCM_RCHANGER+1);
		}

		if (ANewGroup)
		{
			Action *newGroupAction = new Action(menu);
			newGroupAction->setText(tr("Create new..."));
			newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
			newGroupAction->setData(AData);
			newGroupAction->setData(ADR_TO_GROUP,groupDelim);
			connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(newGroupAction,AG_RVCM_RCHANGER+1);
		}

		if (ABlankGroup)
		{
			Action *blankGroupAction = new Action(menu);
			blankGroupAction->setText(FRostersModel!=NULL ? FRostersModel->blankGroupName() : tr("Blank Group"));
			blankGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);
			blankGroupAction->setData(AData);
			blankGroupAction->setData(ADR_TO_GROUP,QVariant(QString("")));
			connect(blankGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(blankGroupAction,AG_RVCM_RCHANGER-1);
		}
	}
	return menu;
}

SubscriptionDialog *RosterChanger::findSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const
{
	foreach (SubscriptionDialog *dialog, FNotifyDialog)
		if (dialog && dialog->streamJid()==AStreamJid && dialog->contactJid()==AContactJid)
			return dialog;
	return NULL;
}

SubscriptionDialog *RosterChanger::createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage)
{
	SubscriptionDialog *dialog = findSubscriptionDialog(AStreamJid,AContactJid);
	if (dialog == NULL)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen())
		{
			dialog = new SubscriptionDialog(this,FPluginManager,AStreamJid,AContactJid,ANotify,AMessage);
			connect(roster->instance(),SIGNAL(closed()),dialog->instance(),SLOT(reject()));
			connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onSubscriptionDialogDestroyed()));
			emit subscriptionDialogCreated(dialog);
		}
	}
	return dialog;
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
			dialog->setNickName(action->data(ADR_NICK).toString());
			dialog->setGroup(action->data(ADR_GROUP).toString());
			dialog->setSubscriptionMessage(action->data(Action::DR_Parametr4).toString());
		}
	}
}

void RosterChanger::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QModelIndex index = FRostersView->instance()->currentIndex();
		int itemType = index.data(RDR_TYPE).toInt();
		Jid streamJid = index.data(RDR_STREAM_JID).toString();
		if (AId == SCT_ROSTERVIEW_ADDCONTACT)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->rosterItem(index.data(RDR_BARE_JID).toString()) : IRosterItem();
			bool showDialog = itemType==RIT_GROUP || itemType==RIT_STREAM_ROOT;
			showDialog = showDialog || (!ritem.isValid && (itemType==RIT_CONTACT || itemType==RIT_AGENT));
			IAddContactDialog *dialog = showDialog ? showAddContactDialog(streamJid) : NULL;
			if (dialog)
			{
				if (itemType == RIT_GROUP)
					dialog->setGroup(index.data(RDR_GROUP).toString());
				else if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
					dialog->setContactJid(index.data(RDR_BARE_JID).toString());
			}
		}
		else if (AId == SCT_ROSTERVIEW_RENAME)
		{
			if (itemType == RIT_GROUP)
				renameGroup(streamJid,index.data(RDR_GROUP).toString());
			else if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
				renameContact(streamJid,index.data(RDR_BARE_JID).toString(),index.data(RDR_NAME).toString());
		}
		else if (AId == SCT_ROSTERVIEW_REMOVEFROMGROUP)
		{
			if (itemType == RIT_GROUP)
				removeGroup(streamJid,index.data(RDR_GROUP).toString());
			else if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
				removeContactFromGroup(streamJid,index.data(RDR_BARE_JID).toString(),index.data(RDR_GROUP).toString());
		}
		else if (AId == SCT_ROSTERVIEW_REMOVEFROMROSTER)
		{
			if (itemType == RIT_GROUP)
				removeGroupContacts(streamJid,index.data(RDR_GROUP).toString());
			else if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
				removeContactFromRoster(streamJid,index.data(RDR_BARE_JID).toString());
		}
		else if (AId == SCT_ROSTERVIEW_SUBSCRIBE)
		{
			if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
				contactSubscription(streamJid,index.data(RDR_BARE_JID).toString(),IRoster::Subscribe);
		}
		else if (AId == SCT_ROSTERVIEW_UNSUBSCRIBE)
		{
			if (itemType==RIT_CONTACT || itemType==RIT_AGENT)
				contactSubscription(streamJid,index.data(RDR_BARE_JID).toString(),IRoster::Unsubscribe);
		}
	}
}

void RosterChanger::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
	QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
	if (roster && roster->isOpen())
	{
		int itemType = AIndex->data(RDR_TYPE).toInt();
		IRosterItem ritem = roster->rosterItem(AIndex->data(RDR_BARE_JID).toString());
		if (itemType == RIT_STREAM_ROOT)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Add contact..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			action->setData(ADR_STREAM_JID,AIndex->data(RDR_JID));
			action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
		}
		else if (itemType == RIT_CONTACT || itemType == RIT_AGENT)
		{
			Menu *menu = new Menu (AMenu);
			menu->setTitle(tr("Roster management"));
			menu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MANAGEMENT);
			AMenu->addAction(menu->menuAction(),AG_RVCM_RCHANGER,true);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_CONTACT_JID,AIndex->data(RDR_BARE_JID).toString());

			Menu *subsMenu = new Menu(AMenu);
			subsMenu->setTitle(tr("Subscription"));
			subsMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBTION);
			AMenu->addAction(subsMenu->menuAction(),AG_RVCM_RCHANGER,true);

			Action *action = new Action(subsMenu);
			action->setText(tr("Subscribe contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			action->setShortcutId(SCT_ROSTERVIEW_SUBSCRIBE);
			connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setText(tr("Unsubscribe contact"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			action->setShortcutId(SCT_ROSTERVIEW_UNSUBSCRIBE);
			connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
			subsMenu->addAction(action,AG_DEFAULT-1);

			action = new Action(subsMenu);
			action->setText(tr("Send"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_SEND);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Request"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REQUEST);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Remove"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REMOVE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribed);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			action = new Action(subsMenu);
			action->setText(tr("Refuse"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REFUSE);
			action->setData(data);
			action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
			connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
			subsMenu->addAction(action);

			if (ritem.isValid)
			{
				QSet<QString> exceptGroups = ritem.groups;

				data.insert(ADR_NICK,AIndex->data(RDR_NAME));
				data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

				action = new Action(menu);
				action->setText(tr("Rename contact"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
				action->setData(data);
				action->setShortcutId(SCT_ROSTERVIEW_RENAME);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRenameContact(bool)));
				menu->addAction(action);

				if (itemType == RIT_CONTACT)
				{
					Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,false,SLOT(onCopyContactToGroup(bool)),menu);
					copyItem->setTitle(tr("Copy to group"));
					copyItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					menu->addAction(copyItem->menuAction());

					Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,true,SLOT(onMoveContactToGroup(bool)),menu);
					moveItem->setTitle(tr("Move to group"));
					moveItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
					menu->addAction(moveItem->menuAction());
				}

				if (!AIndex->data(RDR_GROUP).toString().isEmpty())
				{
					action = new Action(menu);
					action->setText(tr("Remove from group"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_FROM_GROUP);
					action->setData(data);
					action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
					connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactFromGroup(bool)));
					menu->addAction(action);
				}
			}
			else
			{
				action = new Action(AMenu);
				action->setText(tr("Add contact..."));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				action->setData(ADR_STREAM_JID,streamJid);
				action->setData(ADR_CONTACT_JID,AIndex->data(RDR_JID));
				action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
				connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
			}

			action = new Action(menu);
			action->setText(tr("Remove from roster"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
			action->setData(data);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactFromRoster(bool)));
			menu->addAction(action);
		}
		else if (itemType == RIT_GROUP)
		{
			Menu *menu = new Menu (AMenu);
			menu->setTitle(tr("Roster management"));
			menu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MANAGEMENT);
			AMenu->addAction(menu->menuAction(),AG_RVCM_RCHANGER,true);

			QHash<int,QVariant> data;
			data.insert(ADR_STREAM_JID,streamJid);
			data.insert(ADR_GROUP,AIndex->data(RDR_GROUP));

			Action *action = new Action(AMenu);
			action->setText(tr("Add contact..."));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			action->setData(data);
			action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);

			action = new Action(menu);
			action->setText(tr("Rename group"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
			action->setData(data);
			action->setShortcutId(SCT_ROSTERVIEW_RENAME);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
			menu->addAction(action);

			QSet<QString> exceptGroups;
			exceptGroups << AIndex->data(RDR_GROUP).toString();

			Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,false,SLOT(onCopyGroupToGroup(bool)),menu);
			copyGroup->setTitle(tr("Copy to group"));
			copyGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
			menu->addAction(copyGroup->menuAction());

			Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,false,SLOT(onMoveGroupToGroup(bool)),menu);
			moveGroup->setTitle(tr("Move to group"));
			moveGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
			menu->addAction(moveGroup->menuAction());

			action = new Action(menu);
			action->setText(tr("Remove group"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_GROUP);
			action->setData(data);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
			menu->addAction(action);

			action = new Action(menu);
			action->setText(tr("Remove contacts"));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACTS);
			action->setData(data);
			action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
			connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupContacts(bool)));
			menu->addAction(action);
		}
	}
}

void RosterChanger::contactSubscription(const Jid &AStreamJid, const Jid &AContactJid, int ASubsc)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		if (ASubsc == IRoster::Subscribe)
			subscribeContact(AStreamJid,AContactJid);
		else if (ASubsc == IRoster::Unsubscribe)
			unsubscribeContact(AStreamJid,AContactJid);
	}
}

void RosterChanger::sendSubscription(const Jid &AStreamJid, const Jid &AContactJid, int ASubsc) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		roster->sendSubscription(AContactJid,ASubsc);
	}
}

void RosterChanger::addContactToGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		IRosterItem ritem = roster->rosterItem(AContactJid);
		if (!ritem.isValid)
			roster->setItem(AContactJid,QString::null,QSet<QString>()<<AGroup);
		else
			roster->copyItemToGroup(AContactJid,AGroup);
	}
}

void RosterChanger::renameContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AOldName) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && roster->rosterItem(AContactJid).isValid)
	{
		bool ok = false;
		QString newName = QInputDialog::getText(NULL,tr("Rename contact"),tr("Enter name for: <b>%1</b>").arg(AContactJid.hBare()),QLineEdit::Normal,AOldName,&ok);
		if (ok && !newName.isEmpty() && newName != AOldName)
			roster->renameItem(AContactJid,newName);
	}
}

void RosterChanger::copyContactToGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QString groupDelim = roster->groupDelimiter();
		if (AGroup.endsWith(groupDelim))
		{
			bool ok = false;
			QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),QLineEdit::Normal,QString(),&ok);
			if (ok && !newGroupName.isEmpty())
				roster->copyItemToGroup(AContactJid,AGroup==groupDelim ? newGroupName : AGroup+newGroupName);
		}
		else
			roster->copyItemToGroup(AContactJid,AGroup);
	}
}

void RosterChanger::moveContactToGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroupFrom, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(groupDelim))
		{
			bool ok = false;
			QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),QLineEdit::Normal,QString(),&ok);
			if (ok && !newGroupName.isEmpty())
				roster->moveItemToGroup(AContactJid,AGroupFrom,AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
		}
		else
			roster->moveItemToGroup(AContactJid,AGroupFrom,AGroupTo);
	}
}

void RosterChanger::removeContactFromGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		roster->removeItemFromGroup(AContactJid,AGroup);
	}
}

void RosterChanger::removeContactFromRoster(const Jid &AStreamJid, const Jid &AContactJid) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		if (roster->rosterItem(AContactJid).isValid)
		{
			if (QMessageBox::question(NULL,tr("Remove contact"),
				tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(AContactJid.hBare()),
				QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				roster->removeItem(AContactJid);
			}
		}
		else if (FRostersModel)
		{
			QMultiHash<int, QVariant> data;
			data.insert(RDR_TYPE,RIT_CONTACT);
			data.insert(RDR_TYPE,RIT_AGENT);
			data.insert(RDR_BARE_JID,AContactJid.pBare());
			IRosterIndex *streamIndex = FRostersModel->streamRoot(AStreamJid);
			foreach(IRosterIndex *index, streamIndex->findChild(data,true))
				FRostersModel->removeRosterIndex(index);
		}
	}
}

void RosterChanger::addGroupToGroup(const Jid &AToStreamJid, const Jid &AFromStreamJid, const QString &AGroup, const QString &AGroupTo) const
{
	IRoster *toRoster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AToStreamJid) : NULL;
	IRoster *fromRoster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AFromStreamJid) : NULL;
	if (fromRoster && toRoster && toRoster->isOpen())
	{
		QList<IRosterItem> toItems;
		QList<IRosterItem> fromItems = fromRoster->groupItems(AGroup);
		QString fromGroupLast = AGroup.split(fromRoster->groupDelimiter(),QString::SkipEmptyParts).last();
		foreach(IRosterItem fromItem, fromItems)
		{
			QSet<QString> newGroups;
			foreach(QString group, fromItem.groups)
			{
				if (group.startsWith(AGroup))
				{
					QString newGroup = group;
					newGroup.remove(0,AGroup.size());
					if (!AGroupTo.isEmpty())
						newGroup.prepend(AGroupTo + toRoster->groupDelimiter() + fromGroupLast);
					else
						newGroup.prepend(fromGroupLast);
					newGroups += newGroup;
				}
			}
			IRosterItem toItem = toRoster->rosterItem(fromItem.itemJid);
			if (!toItem.isValid)
			{
				toItem.isValid = true;
				toItem.itemJid = fromItem.itemJid;
				toItem.name = fromItem.name;
				toItem.groups = newGroups;
			}
			else
			{
				toItem.groups += newGroups;
			}
			toItems.append(toItem);
		}
		toRoster->setItems(toItems);
	}
}

void RosterChanger::renameGroup(const Jid &AStreamJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && roster->groups().contains(AGroup))
	{
		bool ok = false;
		QString groupDelim = roster->groupDelimiter();
		QList<QString> groupTree = AGroup.split(groupDelim,QString::SkipEmptyParts);
		QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),QLineEdit::Normal,groupTree.last(),&ok);
		if (ok && !newGroupPart.isEmpty())
		{
			QString newGroupName = AGroup;
			newGroupName.chop(groupTree.last().size());
			newGroupName += newGroupPart;
			roster->renameGroup(AGroup,newGroupName);
		}
	}
}

void RosterChanger::copyGroupToGroup(const Jid &AStreamJid, const QString &AGroup, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(groupDelim))
		{
			bool ok = false;
			QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),QLineEdit::Normal,QString(),&ok);
			if (ok && !newGroupName.isEmpty())
				roster->copyGroupToGroup(AGroup,AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
		}
		else
			roster->copyGroupToGroup(AGroup,AGroupTo);
	}
}

void RosterChanger::moveGroupToGroup(const Jid &AStreamJid, const QString &AGroup, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(roster->groupDelimiter()))
		{
			bool ok = false;
			QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),QLineEdit::Normal,QString(),&ok);
			if (ok && !newGroupName.isEmpty())
				roster->moveGroupToGroup(AGroup,AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
		}
		else
			roster->moveGroupToGroup(AGroup,AGroupTo);
	}
}

void RosterChanger::removeGroup(const Jid &AStreamJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		roster->removeGroup(AGroup);
	}
}

void RosterChanger::removeGroupContacts(const Jid &AStreamJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		QList<IRosterItem> ritems = roster->groupItems(AGroup);
		if (ritems.count()>0 &&	QMessageBox::question(NULL,tr("Remove contacts"),
			tr("You are assured that wish to remove %1 contact(s) from roster?").arg(ritems.count()),
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			roster->removeItems(ritems);
		}
	}
}

void RosterChanger::onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AMessage)
{
	INotification notify;
	if (FNotifications)
	{
		notify.kinds =  FNotifications->notificationKinds(NNT_SUBSCRIPTION_REQUEST);
		if (ASubsType != IRoster::Subscribe)
			notify.kinds = (notify.kinds & INotification::PopupWindow);
		notify.type = NNT_SUBSCRIPTION_REQUEST;
		notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_SUBSCRIBTION));
		notify.data.insert(NDR_TOOLTIP,tr("Subscription message from %1").arg(FNotifications->contactName(ARoster->streamJid(),AContactJid)));
		notify.data.insert(NDR_STREAM_JID,ARoster->streamJid().full());
		notify.data.insert(NDR_CONTACT_JID,AContactJid.full());
		notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_SUBSCRIBTION);
		notify.data.insert(NDR_POPUP_CAPTION, tr("Subscription message"));
		notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(ARoster->streamJid(),AContactJid));
		notify.data.insert(NDR_POPUP_IMAGE, FNotifications->contactAvatar(AContactJid));
		notify.data.insert(NDR_POPUP_TEXT,subscriptionNotify(ASubsType,AContactJid));
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
	}

	int notifyId = -1;
	SubscriptionDialog *dialog = NULL;
	const IRosterItem &ritem = ARoster->rosterItem(AContactJid);
	if (ASubsType == IRoster::Subscribe)
	{
		if (!isAutoSubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
		{
			dialog = createSubscriptionDialog(ARoster->streamJid(),AContactJid,subscriptionNotify(ASubsType,AContactJid),AMessage);
			if (dialog!=NULL && !FNotifyDialog.values().contains(dialog))
			{
				if (FNotifications)
				{
					if (notify.kinds > 0)
						notifyId = FNotifications->appendNotification(notify);
				}
				else
				{
					dialog->instance()->show();
				}
			}
		}
		else
		{
			ARoster->sendSubscription(AContactJid,IRoster::Subscribed);
			if (isAutoSubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
				ARoster->sendSubscription(AContactJid,IRoster::Subscribe);
		}
	}
	else if (ASubsType == IRoster::Unsubscribed)
	{
		if (FNotifications && notify.kinds > 0 && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
			notifyId = FNotifications->appendNotification(notify);

		if (isAutoUnsubscribe(ARoster->streamJid(),AContactJid) && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_NONE)
			ARoster->sendSubscription(AContactJid,IRoster::Unsubscribed);
	}
	else  if (ASubsType == IRoster::Subscribed)
	{
		if (FNotifications && notify.kinds > 0 && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
			notifyId = FNotifications->appendNotification(notify);
	}
	else if (ASubsType == IRoster::Unsubscribe)
	{
		if (FNotifications && notify.kinds > 0 && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
			notifyId = FNotifications->appendNotification(notify);
	}

	if (notifyId>0 || dialog!=NULL)
		FNotifyDialog.insert(notifyId,dialog);
}

void RosterChanger::onContactSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		contactSubscription(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_SUBSCRIPTION).toInt());
}

void RosterChanger::onSendSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		sendSubscription(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_SUBSCRIPTION).toInt());
}

void RosterChanger::onAddContactToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		addContactToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRenameContact(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		renameContact(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_NICK).toString());
}

void RosterChanger::onCopyContactToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyContactToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveContactToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveContactToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_GROUP).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveContactFromGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactFromGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString(),action->data(ADR_GROUP).toString());
}

void RosterChanger::onRemoveContactFromRoster(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactFromRoster(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toString());
}

void RosterChanger::onAddGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		addGroupToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_FROM_STREAM_JID).toString(),action->data(ADR_GROUP).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRenameGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		renameGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toString());
}

void RosterChanger::onCopyGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyGroupToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveGroupToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveGroupToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toString(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toString());
}

void RosterChanger::onRemoveGroupContacts(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroupContacts(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toString());
}

void RosterChanger::onRosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem)
{
	if (isSilentSubsctiption(ARoster->streamJid(), ARosterItem.itemJid))
		insertAutoSubscribe(ARoster->streamJid(), ARosterItem.itemJid, true, false, false);
	else
		removeAutoSubscribe(ARoster->streamJid(), ARosterItem.itemJid);
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
	FAutoSubscriptions.remove(ARoster->streamJid());
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
	if (FNotifyDialog.contains(ANotifyId))
	{
		SubscriptionDialog *dialog = FNotifyDialog.value(ANotifyId);
		if (dialog)
			dialog->show();
		FNotifications->removeNotification(ANotifyId);
	}
}

void RosterChanger::onNotificationRemoved(int ANotifyId)
{
	if (FNotifyDialog.contains(ANotifyId))
	{
		SubscriptionDialog *dialog = FNotifyDialog.take(ANotifyId);
		if (dialog && !dialog->isVisible())
			dialog->reject();
	}
}

void RosterChanger::onSubscriptionDialogDestroyed()
{
	SubscriptionDialog *dialog = static_cast<SubscriptionDialog *>(sender());
	if (dialog)
	{
		int notifyId = FNotifyDialog.key(dialog);
		FNotifications->removeNotification(notifyId);
	}
}

void RosterChanger::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Q_UNUSED(AWindow);
	if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AUser->data(MUDR_STREAM_JID).toString()) : NULL;
		if (roster && !roster->rosterItem(AUser->data(MUDR_REAL_JID).toString()).isValid)
		{
			Action *action = new Action(AMenu);
			action->setText(tr("Add contact..."));
			action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAM_JID));
			action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REAL_JID));
			action->setData(ADR_NICK,AUser->data(MUDR_NICK_NAME));
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
			AMenu->addAction(action,AG_MUCM_ROSTERCHANGER,true);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_rosterchanger, RosterChanger)

