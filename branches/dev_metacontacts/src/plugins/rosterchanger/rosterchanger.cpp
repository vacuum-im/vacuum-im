#include "rosterchanger.h"

#include <QMap>
#include <QDropEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QItemEditorFactory>

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_FROM_STREAM_JID Action::DR_Parametr2
#define ADR_SUBSCRIPTION    Action::DR_Parametr2
#define ADR_NICK            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_REQUEST         Action::DR_Parametr4
#define ADR_TO_GROUP        Action::DR_Parametr4

static const QList<int> DragGroups = QList<int>() << RIK_GROUP << RIK_GROUP_BLANK;

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

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &, const IRosterItem &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscriptionSent(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionSent(IRoster *, const Jid &, int, const QString &)));
			connect(FRosterPlugin->instance(),SIGNAL(rosterSubscriptionReceived(IRoster *, const Jid &, int, const QString &)),
				SLOT(onSubscriptionReceived(IRoster *, const Jid &, int, const QString &)));
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
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRosterIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
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

Qt::DropActions RosterChanger::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(ADrag);
	int indexKind = AIndex->data(RDR_KIND).toInt();
	if (indexKind==RIK_CONTACT || indexKind==RIK_GROUP)
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

		int indexKind = indexData.value(RDR_KIND).toInt();
		if (indexKind==RIK_CONTACT || indexKind==RIK_GROUP)
			return true;
	}
	return false;
}

bool RosterChanger::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	int hoverKind = AHover->data(RDR_KIND).toInt();
	if (DragGroups.contains(hoverKind) || hoverKind==RIK_STREAM_ROOT)
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AHover->data(RDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			operator>>(stream,indexData);

			if (hoverKind == RIK_STREAM_ROOT)
				return indexData.value(RDR_STREAM_JID)!=AHover->data(RDR_STREAM_JID);
			else if (hoverKind == RIK_GROUP_BLANK)
				return indexData.value(RDR_KIND).toInt() != RIK_GROUP;
			else if (hoverKind==RIK_GROUP && indexData.value(RDR_KIND).toInt()==RIK_GROUP)
				return indexData.value(RDR_STREAM_JID)!=AHover->data(RDR_STREAM_JID) || indexData.value(RDR_GROUP)!=AHover->data(RDR_GROUP);
			return true;
		}
	}
	return false;
}

void RosterChanger::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool RosterChanger::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu)
{
	int hoverKind = AIndex->data(RDR_KIND).toInt();
	if ((AEvent->dropAction() & (Qt::CopyAction|Qt::MoveAction))>0 && (DragGroups.contains(hoverKind) || hoverKind==RIK_STREAM_ROOT))
	{
		Jid hoverStreamJid = AIndex->data(RDR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(hoverStreamJid) : NULL;
		if (roster && roster->isOpen())
		{
			QMap<int, QVariant> indexData;
			QDataStream stream(AEvent->mimeData()->data(DDT_ROSTERSVIEW_INDEX_DATA));
			operator>>(stream,indexData);

			int indexKind = indexData.value(RDR_KIND).toInt();
			Jid indexStreamJid = indexData.value(RDR_STREAM_JID).toString();
			bool isNewContact = indexKind==RIK_CONTACT && !roster->rosterItem(indexData.value(RDR_PREP_BARE_JID).toString()).isValid;

			if (isNewContact || hoverStreamJid.pBare()!=indexStreamJid.pBare())
			{
				Action *addAction = new Action(AMenu);
				addAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
				addAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
				addAction->setData(ADR_TO_GROUP,AIndex->data(RDR_GROUP));

				if (indexKind == RIK_CONTACT)
				{
					addAction->setText(tr("Add contact"));
					addAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
					addAction->setData(ADR_NICK,indexData.value(RDR_NAME));
					connect(addAction,SIGNAL(triggered(bool)),SLOT(onAddContactToGroup(bool)));
					AMenu->addAction(addAction,AG_DEFAULT,true);
				}
				else
				{
					addAction->setText(tr("Add group"));
					addAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					addAction->setData(ADR_FROM_STREAM_JID,indexStreamJid.full());
					addAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
					connect(addAction,SIGNAL(triggered(bool)),SLOT(onAddGroupToGroup(bool)));
					AMenu->addAction(addAction,AG_DEFAULT,true);
				}
				AMenu->setDefaultAction(addAction);
				return true;
			}
			else if (hoverKind != RIK_STREAM_ROOT)
			{
				if (AEvent->dropAction() == Qt::CopyAction)
				{
					Action *copyAction = new Action(AMenu);
					copyAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
					copyAction->setData(ADR_STREAM_JID,hoverStreamJid.full());
					copyAction->setData(ADR_TO_GROUP,AIndex->data(RDR_GROUP));
					copyAction->setText(tr("Copy to group"));
					if (indexKind == RIK_CONTACT)
					{
						copyAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyContactsToGroup(bool)));
						AMenu->addAction(copyAction,AG_DEFAULT,true);
					}
					else
					{
						copyAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(copyAction,SIGNAL(triggered(bool)),SLOT(onCopyGroupsToGroup(bool)));
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
					moveAction->setData(ADR_TO_GROUP,AIndex->data(RDR_GROUP));
					moveAction->setText(tr("Move to group"));
					if (indexKind == RIK_CONTACT)
					{
						moveAction->setData(ADR_CONTACT_JID,indexData.value(RDR_PREP_BARE_JID));
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP).toString());
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveContactsToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					else
					{
						moveAction->setData(ADR_GROUP,indexData.value(RDR_GROUP));
						connect(moveAction,SIGNAL(triggered(bool)),SLOT(onMoveGroupsToGroup(bool)));
						AMenu->addAction(moveAction,AG_DEFAULT,true);
					}
					AMenu->setDefaultAction(moveAction);
					return true;
				}
			}
		}
	}
	return false;
}

quint32 RosterChanger::rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const
{
	int indexKind = AIndex.data(RDR_KIND).toInt();
	if (AOrder==REHO_ROSTERCHANGER_RENAME && ADataRole==RDR_NAME && (indexKind==RIK_CONTACT || indexKind==RIK_AGENT || indexKind==RIK_GROUP))
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
		if (roster && roster->isOpen())
			return AdvancedDelegateItem::DisplayId;
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
		QByteArray name = ADelegate->editorFactory()->valuePropertyName(value.type());
		QString newName = AEditor->property(name).toString();
		if (!newName.isEmpty() && AIndex.data(RDR_NAME).toString()!=newName)
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AIndex.data(RDR_STREAM_JID).toString()) : NULL;
			if (roster && roster->isOpen())
			{
				int indexKind = AIndex.data(RDR_KIND).toInt();
				if (indexKind == RIK_GROUP)
				{
					QString fullName = AIndex.data(RDR_GROUP).toString();
					fullName.chop(AIndex.data(RDR_NAME).toString().size());
					fullName += newName;
					roster->renameGroup(AIndex.data(RDR_GROUP).toString(),fullName);
				}
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
				{
					roster->renameItem(AIndex.data(RDR_PREP_BARE_JID).toString(),newName);
				}
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && !roster->rosterItem(AContactJid).isValid)
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		if (roster && roster->isOpen() && roster->rosterItem(AContactJid).isValid)
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
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
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
		const IRosterItem &ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
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
}

void RosterChanger::removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid)
{
	FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		const IRosterItem &ritem = roster->rosterItem(AContactJid);
		if (roster->subscriptionRequests().contains(AContactJid.bare()))
			roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
		if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
			roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
		insertAutoSubscribe(AStreamJid,AContactJid,ASilently,true,false);
	}
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
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
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
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

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, bool ANewGroup, bool ARootGroup, bool ABlankGroup, const char *ASlot, Menu *AParent)
{
	Menu *menu = new Menu(AParent);
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AData.value(ADR_STREAM_JID).toString()) : NULL;
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
			curGroupAction->setData(ADR_TO_GROUP,QString());
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
			blankGroupAction->setText(FRostersModel!=NULL ? FRostersModel->singleGroupName(RIK_GROUP_BLANK) : tr("Blank Group"));
			blankGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);
			blankGroupAction->setData(AData);
			blankGroupAction->setData(ADR_TO_GROUP,QString());
			connect(blankGroupAction,SIGNAL(triggered(bool)),ASlot);
			menu->addAction(blankGroupAction,AG_RVCM_RCHANGER-1);
		}
	}
	return menu;
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

	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		dialog = new SubscriptionDialog(this,FPluginManager,AStreamJid,AContactJid,ANotify,AMessage);
		connect(roster->instance(),SIGNAL(closed()),dialog->instance(),SLOT(reject()));
		connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onSubscriptionDialogDestroyed()));
		FSubsDialogs.append(dialog);
		emit subscriptionDialogCreated(dialog);
	}
	return dialog;
}

bool RosterChanger::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> acceptKinds = QList<int>() << RIK_STREAM_ROOT << RIK_CONTACT << RIK_AGENT << RIK_GROUP;
	if (!ASelected.isEmpty())
	{
		int singleKind = -1;
		Jid singleStream;
		foreach(IRosterIndex *index, ASelected)
		{
			int indexKind = index->kind();
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (!acceptKinds.contains(indexKind))
				return false;
			else if (singleKind!=-1 && singleKind!=indexKind)
				return false;
			else if(!singleStream.isEmpty() && singleStream!=streamJid)
				return false;
			singleKind = indexKind;
			singleStream = streamJid;
		}
		return true;
	}
	return false;
}

QStringList RosterChanger::indexesRoleList(const QList<IRosterIndex *> &AIndexes, int ARole, bool AUnique) const
{
	return FRostersView!=NULL ? FRostersView->indexesRolesMap(AIndexes,QList<int>()<<ARole,AUnique ? ARole : -1).value(ARole) : QStringList();
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
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (isSelectionAccepted(indexes))
		{
			IRosterIndex *index = indexes.first();
			int indexKind = index->data(RDR_KIND).toInt();
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (AId==SCT_ROSTERVIEW_ADDCONTACT && !FRostersView->hasMultiSelection())
			{
				IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
				IRosterItem ritem = roster!=NULL ? roster->rosterItem(index->data(RDR_PREP_BARE_JID).toString()) : IRosterItem();
				
				bool showDialog = indexKind==RIK_GROUP || indexKind==RIK_STREAM_ROOT;
				showDialog = showDialog || (!ritem.isValid && (indexKind==RIK_CONTACT || indexKind==RIK_AGENT));
				
				IAddContactDialog *dialog = showDialog ? showAddContactDialog(streamJid) : NULL;
				if (dialog)
				{
					if (indexKind == RIK_GROUP)
						dialog->setGroup(index->data(RDR_GROUP).toString());
					else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
						dialog->setContactJid(index->data(RDR_PREP_BARE_JID).toString());
				}
			}
			else if (AId==SCT_ROSTERVIEW_RENAME && !FRostersView->hasMultiSelection())
			{
				if (!FRostersView->editRosterIndex(index, RDR_NAME))
				{
					if (indexKind == RIK_GROUP)
						renameGroup(streamJid,index->data(RDR_GROUP).toString());
					else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
						renameContact(streamJid,index->data(RDR_PREP_BARE_JID).toString(),index->data(RDR_NAME).toString());
				}
			}
			else if (AId == SCT_ROSTERVIEW_REMOVEFROMGROUP)
			{
				if (indexKind == RIK_GROUP)
				{
					QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(indexes,QList<int>()<<RDR_GROUP,RDR_GROUP);
					removeGroups(streamJid,rolesMap.value(RDR_GROUP));
				}
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
				{
					QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(indexes,QList<int>()<<RDR_PREP_BARE_JID<<RDR_GROUP);
					removeContactsFromGroups(streamJid,rolesMap.value(RDR_PREP_BARE_JID),rolesMap.value(RDR_GROUP));
				}
			}
			else if (AId == SCT_ROSTERVIEW_REMOVEFROMROSTER)
			{
				if (indexKind == RIK_GROUP)
					removeGroupsContacts(streamJid,indexesRoleList(indexes,RDR_GROUP,true));
				else if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
					removeContactsFromRoster(streamJid,indexesRoleList(indexes,RDR_PREP_BARE_JID,true));
			}
			else if (AId == SCT_ROSTERVIEW_SUBSCRIBE)
			{
				if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
					changeContactsSubscription(streamJid,indexesRoleList(indexes,RDR_PREP_BARE_JID,true),IRoster::Subscribe);
			}
			else if (AId == SCT_ROSTERVIEW_UNSUBSCRIBE)
			{
				if (indexKind==RIK_CONTACT || indexKind==RIK_AGENT)
					changeContactsSubscription(streamJid,indexesRoleList(indexes,RDR_PREP_BARE_JID,true),IRoster::Unsubscribe);
			}
		}
	}
}

void RosterChanger::onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void RosterChanger::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		int indexKind = AIndexes.first()->kind();
		Jid streamJid = AIndexes.first()->data(RDR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			if (indexKind==RIK_STREAM_ROOT)
			{
				if (AIndexes.count() == 1)
				{
					Action *action = new Action(AMenu);
					action->setText(tr("Add contact..."));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
					action->setData(ADR_STREAM_JID, AIndexes.first()->data(RDR_STREAM_JID));
					action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
					connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
				}
			}
			else if (indexKind == RIK_CONTACT || indexKind == RIK_AGENT)
			{
				QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_PREP_BARE_JID<<RDR_GROUP<<RDR_NAME,RDR_PREP_BARE_JID);
				
				QHash<int,QVariant> data;
				data.insert(ADR_STREAM_JID,streamJid.full());
				data.insert(ADR_CONTACT_JID,rolesMap.value(RDR_PREP_BARE_JID));
				data.insert(ADR_NICK,rolesMap.value(RDR_NAME));
				data.insert(ADR_GROUP,rolesMap.value(RDR_GROUP));

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
				connect(action,SIGNAL(triggered(bool)),SLOT(onChangeContactsSubscription(bool)));
				subsMenu->addAction(action,AG_DEFAULT-1);

				action = new Action(subsMenu);
				action->setText(tr("Unsubscribe contact"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
				action->setData(data);
				action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
				action->setShortcutId(SCT_ROSTERVIEW_UNSUBSCRIBE);
				connect(action,SIGNAL(triggered(bool)),SLOT(onChangeContactsSubscription(bool)));
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

				if (AIndexes.count() == 1)
				{
					IRosterIndex *index = AIndexes.first();
					IRosterItem ritem = roster->rosterItem(index->data(RDR_PREP_BARE_JID).toString());
					if (ritem.isValid)
					{
						action = new Action(AMenu);
						action->setText(tr("Rename contact"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
						action->setData(data);
						action->setShortcutId(SCT_ROSTERVIEW_RENAME);
						connect(action,SIGNAL(triggered(bool)),SLOT(onRenameContact(bool)));
						AMenu->addAction(action,AG_RVCM_RCHANGER);
					}
					else
					{
						action = new Action(AMenu);
						action->setText(tr("Add contact..."));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
						action->setData(ADR_STREAM_JID,streamJid.full());
						action->setData(ADR_CONTACT_JID,index->data(RDR_FULL_JID));
						action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
						connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
						AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);
					}
				}

				bool isAllItemsValid = true;
				bool isAllInEmptyGroup = true;
				QSet<QString> exceptGroups;
				foreach(QString contactJid, data.value(ADR_CONTACT_JID).toStringList())
				{
					IRosterItem ritem = roster->rosterItem(contactJid);
					if (!ritem.isValid)
					{
						isAllItemsValid = false;
						break;
					}
					else if (!ritem.groups.isEmpty())
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
							Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,false,SLOT(onCopyContactsToGroup(bool)),AMenu);
							copyItem->setTitle(tr("Copy to group"));
							copyItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
							AMenu->addAction(copyItem->menuAction(),AG_RVCM_RCHANGER);
						}

						Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,!isAllInEmptyGroup,SLOT(onMoveContactsToGroup(bool)),AMenu);
						moveItem->setTitle(tr("Move to group"));
						moveItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
						AMenu->addAction(moveItem->menuAction(),AG_RVCM_RCHANGER);
					}

					bool isInGroup = false;
					foreach(IRosterIndex *index, AIndexes)
						if (!index->data(RDR_GROUP).toString().isEmpty())
							isInGroup = true;

					if (isInGroup)
					{
						QMap<int, QStringList> fullRolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_PREP_BARE_JID<<RDR_GROUP);
						
						action = new Action(AMenu);
						action->setText(tr("Remove from group"));
						action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_FROM_GROUP);
						action->setData(ADR_STREAM_JID,streamJid.full());
						action->setData(ADR_CONTACT_JID,fullRolesMap.value(RDR_PREP_BARE_JID));
						action->setData(ADR_GROUP,fullRolesMap.value(RDR_GROUP));
						action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
						connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactsFromGroups(bool)));
						AMenu->addAction(action,AG_RVCM_RCHANGER);
					}
				}

				action = new Action(AMenu);
				action->setText(tr("Remove from roster"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
				action->setData(data);
				action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveContactsFromRoster(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER);
			}
			else if (indexKind == RIK_GROUP)
			{
				QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_GROUP);

				QHash<int,QVariant> data;
				data.insert(ADR_STREAM_JID,streamJid.full());
				data.insert(ADR_GROUP,rolesMap.value(RDR_GROUP));

				if (AIndexes.count() == 1)
				{
					Action *action = new Action(AMenu);
					action->setText(tr("Add contact..."));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
					action->setData(ADR_STREAM_JID,streamJid.full());
					action->setData(ADR_GROUP,AIndexes.first()->data(RDR_GROUP));
					action->setShortcutId(SCT_ROSTERVIEW_ADDCONTACT);
					connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER_ADD_CONTACT,true);

					action = new Action(AMenu);
					action->setText(tr("Rename group"));
					action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
					action->setData(ADR_STREAM_JID,streamJid.full());
					action->setData(ADR_GROUP,AIndexes.first()->data(RDR_GROUP));
					action->setShortcutId(SCT_ROSTERVIEW_RENAME);
					connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
					AMenu->addAction(action,AG_RVCM_RCHANGER);
				}

				QSet<QString> exceptGroups = rolesMap.value(RDR_GROUP).toSet();

				Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,false,SLOT(onCopyGroupsToGroup(bool)),AMenu);
				copyGroup->setTitle(tr("Copy to group"));
				copyGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
				AMenu->addAction(copyGroup->menuAction(),AG_RVCM_RCHANGER);

				Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,false,SLOT(onMoveGroupsToGroup(bool)),AMenu);
				moveGroup->setTitle(tr("Move to group"));
				moveGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
				AMenu->addAction(moveGroup->menuAction(),AG_RVCM_RCHANGER);

				Action *action = new Action(AMenu);
				action->setText(tr("Remove group"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_GROUP);
				action->setData(data);
				action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMGROUP);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroups(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER);

				action = new Action(AMenu);
				action->setText(tr("Remove contacts"));
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACTS);
				action->setData(data);
				action->setShortcutId(SCT_ROSTERVIEW_REMOVEFROMROSTER);
				connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupsContacts(bool)));
				AMenu->addAction(action,AG_RVCM_RCHANGER);
			}
		}
	}
}

void RosterChanger::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Q_UNUSED(AWindow);
	if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
	{
		IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AUser->data(MUDR_STREAM_JID).toString()) : NULL;
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

void RosterChanger::changeContactsSubscription(const Jid &AStreamJid, const QStringList &AContacts, int ASubsc)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		foreach(QString contactJid, AContacts)
		{
			if (ASubsc == IRoster::Subscribe)
				subscribeContact(AStreamJid,contactJid);
			else if (ASubsc == IRoster::Unsubscribe)
				unsubscribeContact(AStreamJid,contactJid);
		}
	}
}

void RosterChanger::sendSubscription(const Jid &AStreamJid, const QStringList &AContacts, int ASubsc) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen())
	{
		foreach(QString contactJid, AContacts)
			roster->sendSubscription(contactJid,ASubsc);
	}
}

void RosterChanger::addContactToGroup(const Jid &AStreamJid, const Jid &AContactJid, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
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
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && roster->rosterItem(AContactJid).isValid)
	{
		QString newName = QInputDialog::getText(NULL,tr("Rename contact"),tr("Enter name for: <b>%1</b>").arg(Qt::escape(AContactJid.uBare())),QLineEdit::Normal,AOldName);
		if (!newName.isEmpty() && newName != AOldName)
			roster->renameItem(AContactJid,newName);
	}
}

void RosterChanger::copyContactsToGroup(const Jid &AStreamJid, const QStringList &AContacts, const QString &AGroup) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AContacts.isEmpty())
	{
		QString newGroupName;
		QString groupDelim = roster->groupDelimiter();
		if (AGroup.endsWith(groupDelim))
			newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));

		for(int i=0; i<AContacts.count(); i++)
		{
			if (!newGroupName.isEmpty())
				roster->copyItemToGroup(AContacts.at(i),AGroup==groupDelim ? newGroupName : AGroup+newGroupName);
			else if (!AGroup.endsWith(groupDelim))
				roster->copyItemToGroup(AContacts.at(i),AGroup);
		}
	}
}

void RosterChanger::moveContactsToGroup(const Jid &AStreamJid, const QStringList &AContacts, const QStringList &AGroupsFrom, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AContacts.isEmpty() && AContacts.count()==AGroupsFrom.count())
	{
		QString newGroupName;
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(groupDelim))
			newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));

		for(int i=0; i<AContacts.count(); i++)
		{
			QString group = AGroupsFrom.at(i);
			if (!newGroupName.isEmpty())
				roster->moveItemToGroup(AContacts.at(i),group,AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
			else if (!AGroupTo.endsWith(groupDelim))
				roster->moveItemToGroup(AContacts.at(i),group,AGroupTo);
		}
	}
}

void RosterChanger::removeContactsFromGroups(const Jid &AStreamJid, const QStringList &AContacts, const QStringList &AGroups) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AContacts.isEmpty() && AContacts.count()==AGroups.count())
	{
		for (int i=0; i<AContacts.count(); i++)
			roster->removeItemFromGroup(AContacts.at(i),AGroups.at(i));
	}
}

void RosterChanger::removeContactsFromRoster(const Jid &AStreamJid, const QStringList &AContacts) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AContacts.isEmpty())
	{
		int button = QMessageBox::No;
		if (AContacts.count() == 1)
		{
			IRosterItem ritem = roster->rosterItem(AContacts.first());
			QString name = ritem.isValid && !ritem.name.isEmpty() ? ritem.name : Jid(AContacts.first()).uBare();
			if (ritem.isValid)
			{
				button = QMessageBox::question(NULL,tr("Remove contact"),
					tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(Qt::escape(name)),
					QMessageBox::Yes | QMessageBox::No);
			}
		}
		else
		{
			button = QMessageBox::question(NULL,tr("Remove contacts"),
				tr("You are assured that wish to remove <b>%n contact(s)</b> from roster?","",AContacts.count()),
				QMessageBox::Yes | QMessageBox::No);
		}

		QMultiMap<int, QVariant> findData;
		findData.insertMulti(RDR_KIND,RIK_CONTACT);
		findData.insertMulti(RDR_KIND,RIK_AGENT);
		findData.insertMulti(RDR_STREAM_JID,AStreamJid.pFull());
		foreach(QString contactJid, AContacts)
		{
			IRosterItem ritem = roster->rosterItem(contactJid);
			if (!ritem.isValid)
			{
				findData.insertMulti(RDR_PREP_BARE_JID,contactJid);
			}
			else if(button == QMessageBox::Yes)
			{
				roster->removeItem(contactJid);
			}
		}

		if (findData.contains(RDR_PREP_BARE_JID))
		{
			IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
			if (sroot)
			{
				foreach(IRosterIndex *index, sroot->findChilds(findData,true))
					FRostersModel->removeRosterIndex(index);
			}
		}
	}
}

void RosterChanger::addGroupToGroup(const Jid &AToStreamJid, const Jid &AFromStreamJid, const QString &AGroup, const QString &AGroupTo) const
{
	IRoster *toRoster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AToStreamJid) : NULL;
	IRoster *fromRoster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AFromStreamJid) : NULL;
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
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && roster->groups().contains(AGroup))
	{
		QString groupDelim = roster->groupDelimiter();
		QList<QString> groupTree = AGroup.split(groupDelim,QString::SkipEmptyParts);
		QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),QLineEdit::Normal,groupTree.last());
		if (!newGroupPart.isEmpty())
		{
			QString newGroupName = AGroup;
			newGroupName.chop(groupTree.last().size());
			newGroupName += newGroupPart;
			roster->renameGroup(AGroup,newGroupName);
		}
	}
}

void RosterChanger::copyGroupsToGroup(const Jid &AStreamJid, const QStringList &AGroups, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AGroups.isEmpty())
	{
		QString newGroupName;
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(groupDelim))
			newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));

		for(int i=0; i<AGroups.count(); i++)
		{
			if (!newGroupName.isEmpty())
				roster->copyGroupToGroup(AGroups.at(i),AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
			else if (!AGroupTo.endsWith(groupDelim))
				roster->copyGroupToGroup(AGroups.at(i),AGroupTo);
		}

	}
}

void RosterChanger::moveGroupsToGroup(const Jid &AStreamJid, const QStringList &AGroups, const QString &AGroupTo) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AGroups.isEmpty())
	{
		QString newGroupName;
		QString groupDelim = roster->groupDelimiter();
		if (AGroupTo.endsWith(groupDelim))
			newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"));
		
		for(int i=0; i<AGroups.count(); i++)
		{
			if (!newGroupName.isEmpty())
				roster->moveGroupToGroup(AGroups.at(i),AGroupTo==groupDelim ? newGroupName : AGroupTo+newGroupName);
			else if (!AGroupTo.endsWith(groupDelim))
				roster->moveGroupToGroup(AGroups.at(i),AGroupTo);
		}
	}
}

void RosterChanger::removeGroups(const Jid &AStreamJid, const QStringList &AGroups) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AGroups.isEmpty())
	{
		foreach(QString group, AGroups)
			roster->removeGroup(group);
	}
}

void RosterChanger::removeGroupsContacts(const Jid &AStreamJid, const QStringList &AGroups) const
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster && roster->isOpen() && !AGroups.isEmpty())
	{
		QSet<Jid> items;
		foreach(QString group, AGroups)
			foreach(IRosterItem ritem, roster->groupItems(group))
				items += ritem.itemJid;

		if (items.count()>0 &&	QMessageBox::question(NULL,tr("Remove contacts"),
			tr("You are assured that wish to remove <b>%n contact(s)</b> from roster?","",items.count()),
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
		{
			foreach(Jid itemJid, items)
				roster->removeItem(itemJid);
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
			FNotifications->activateNotification(notifyId);
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void RosterChanger::onSendSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		sendSubscription(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_SUBSCRIPTION).toInt());
}

void RosterChanger::onChangeContactsSubscription(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		changeContactsSubscription(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_SUBSCRIPTION).toInt());
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
		notify.data.insert(NDR_POPUP_HTML,Qt::escape(subscriptionNotify(ASubsType,AItemJid)));
		notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
	}

	int notifyId = -1;
	SubscriptionDialog *dialog = NULL;
	const IRosterItem &ritem = ARoster->rosterItem(AItemJid);
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
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(streamJid) : NULL;
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
				renameContact(streamJid,contactJid,action->data(ADR_NICK).toString());
			}
		}
	}
}

void RosterChanger::onCopyContactsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyContactsToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveContactsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveContactsToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveContactsFromGroups(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactsFromGroups(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList(),action->data(ADR_GROUP).toStringList());
}

void RosterChanger::onRemoveContactsFromRoster(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeContactsFromRoster(action->data(ADR_STREAM_JID).toString(),action->data(ADR_CONTACT_JID).toStringList());
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
	{
		QString streamJid = action->data(ADR_STREAM_JID).toString();
		IRoster *roster = FRosterPlugin ? FRosterPlugin->findRoster(streamJid) : NULL;
		if (roster && roster->isOpen())
		{
			bool editInRoster = false;
			QString group = action->data(ADR_GROUP).toString();
			if (FRostersView && FRostersView->instance()->isActiveWindow() && FRostersView->rostersModel())
			{
				IRosterIndex *sroot = FRostersView->rostersModel()->streamRoot(roster->streamJid());
				IRosterIndex *index = FRostersView->rostersModel()->findGroupIndex(RIK_GROUP,group,roster->groupDelimiter(),sroot);
				if (index)
					editInRoster = FRostersView->editRosterIndex(index,RDR_NAME);
			}
			if (!editInRoster)
			{
				renameGroup(streamJid,group);
			}
		}
	}
}

void RosterChanger::onCopyGroupsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		copyGroupsToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onMoveGroupsToGroup(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		moveGroupsToGroup(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toStringList(),action->data(ADR_TO_GROUP).toString());
}

void RosterChanger::onRemoveGroups(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroups(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toStringList());
}

void RosterChanger::onRemoveGroupsContacts(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		removeGroupsContacts(action->data(ADR_STREAM_JID).toString(),action->data(ADR_GROUP).toStringList());
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
	else if (AItem.ask != ABefore.ask)
	{
		if (AItem.ask == SUBSCRIPTION_SUBSCRIBE)
		{
			removeObsoleteNotifies(ARoster->streamJid(),AItem.itemJid,IRoster::Subscribe,true);
		}
	}
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
	FAutoSubscriptions.remove(ARoster->streamJid());
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
		FNotifications->removeNotification(notifyId);
	}
}

Q_EXPORT_PLUGIN2(plg_rosterchanger, RosterChanger)
