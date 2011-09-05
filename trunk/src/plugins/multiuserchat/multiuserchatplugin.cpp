#include "multiuserchatplugin.h"

#include <QInputDialog>

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_HOST                  Action::DR_Parametr1
#define ADR_ROOM                  Action::DR_Parametr2
#define ADR_NICK                  Action::DR_Parametr3
#define ADR_PASSWORD              Action::DR_Parametr4

#define DIC_CONFERENCE            "conference"
#define DIT_TEXT                  "text"

MultiUserChatPlugin::MultiUserChatPlugin()
{
	FPluginManager = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FRostersViewPlugin = NULL;
	FMainWindowPlugin = NULL;
	FTrayManager = NULL;
	FXmppStreams = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FDataForms = NULL;
	FVCardPlugin = NULL;
	FRegistration = NULL;
	FXmppUriQueries = NULL;
	FOptionsManager = NULL;

	FChatMenu = NULL;
}

MultiUserChatPlugin::~MultiUserChatPlugin()
{
	delete FChatMenu;
}

void MultiUserChatPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Multi-User Conferences");
	APluginInfo->description = tr("Allows to use Jabber multi-user conferences");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MultiUserChatPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),
				SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	if (FMessageWidgets)
	{
		plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
		if (plugin)
		{
			FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
			if (FRostersViewPlugin)
			{
				connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)), 
					SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
			}
		}

		plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
		if (plugin)
		{
			FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
		if (plugin)
		{
			FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
		if (plugin)
		{
			FNotifications = qobject_cast<INotifications *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
		if (plugin)
		{
			FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		}

		plugin = APluginManager->pluginInterface("IRegistration").value(0,NULL);
		if (plugin)
		{
			FRegistration = qobject_cast<IRegistration *>(plugin->instance());
			if (FRegistration)
			{
				connect(FRegistration->instance(),SIGNAL(registerFields(const QString &, const IRegisterFields &)),
					SLOT(onRegisterFieldsReceived(const QString &, const IRegisterFields &)));
				connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const QString &)),
					SLOT(onRegisterErrorReceived(const QString &, const QString &)));
			}
		}
	}

	return FXmppStreams!=NULL;
}

bool MultiUserChatPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_APP_MUCJOIN, tr("Join conference"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_MUC_LEAVEHIDDEN, tr("Leave all hidden conferences"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_MUC_SHOWHIDDEN, tr("Show all hidden conferences"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);

	Shortcuts::declareGroup(SCTG_MESSAGEWINDOWS_MUC, tr("Multi-user chat window"), SGO_MESSAGEWINDOWS_MUC);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_SENDMESSAGE, tr("Send message"), tr("Return","Send message"), Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CLEARWINDOW, tr("Clear window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CHANGENICK, tr("Change nick"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CHANGETOPIC, tr("Change topic"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_ROOMSETTINGS, tr("Setup conference"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_ENTER, tr("Enter the conference"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_EXIT, tr("Leave the conference"), tr("Ctrl+Q","Leave the conference"));

	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(this,MHO_MULTIUSERCHAT_INVITE);
	}

	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_MUC_REGISTER);
		FDataForms->insertLocalizer(this,DATA_FORM_MUC_ROOMCONFIG);
		FDataForms->insertLocalizer(this,DATA_FORM_MUC_ROOM_INFO);
		FDataForms->insertLocalizer(this,DATA_FORM_MUC_REQUEST);
	}

	if (FDiscovery)
	{
		registerDiscoFeatures();
		if (FMessageWidgets)
			FDiscovery->insertFeatureHandler(NS_MUC,this,DFO_DEFAULT);
	}

	if (FMessageWidgets)
	{
		FChatMenu = new Menu(NULL);
		FChatMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFERENCE);
		FChatMenu->setTitle(tr("Conferences"));

		Action *action = new Action(FChatMenu);
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_JOIN);
		action->setText(tr("Join conference"));
		action->setShortcutId(SCT_APP_MUCJOIN);
		connect(action,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
		FChatMenu->addAction(action,AG_DEFAULT+100,false);

		action = new Action(FChatMenu);
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_SHOW_ALL_ROOMS);
		action->setText(tr("Show all hidden conferences"));
		action->setShortcutId(SCT_APP_MUC_SHOWHIDDEN);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowAllRoomsTriggered(bool)));
		FChatMenu->addAction(action,AG_DEFAULT+100,false);

		action = new Action(FChatMenu);
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_LEAVE_HIDDEN_ROOMS);
		action->setText(tr("Leave all hidden conferences"));
		action->setShortcutId(SCT_APP_MUC_LEAVEHIDDEN);
		connect(action,SIGNAL(triggered(bool)),SLOT(onLeaveHiddenRoomsTriggered(bool)));
		FChatMenu->addAction(action,AG_DEFAULT+100,false);
	}

	if (FMainWindowPlugin)
	{
		ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->topToolBarChanger();
		QToolButton *button = changer->insertAction(FChatMenu->menuAction(),TBG_MWTTB_MULTIUSERCHAT);
		button->setPopupMode(QToolButton::InstantPopup);
	}

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FChatMenu->menuAction(),AG_TMTM_MULTIUSERCHAT,true);
	}

	if (FNotifications)
	{
		INotificationType inviteType;
		inviteType.order = NTO_MUC_INVITE_MESSAGE;
		inviteType.title = tr("When receiving an invitation to the conference");
		inviteType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AutoActivate;
		inviteType.kindDefs = inviteType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_INVITE,inviteType);

		INotificationType privateType;
		privateType.order = NTO_MUC_PRIVATE_MESSAGE;
		privateType.title = tr("When receiving a new private message in conference");
		privateType.kindMask = INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		privateType.kindDefs = privateType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_PRIVATE,privateType);

		INotificationType groupchatType;
		groupchatType.order = NTO_MUC_GROUPCHAT_MESSAGE;
		groupchatType.title = tr("When receiving a new message in conference");
		groupchatType.kindMask = INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized;
		groupchatType.kindDefs = groupchatType.kindMask & ~(INotification::PopupWindow|INotification::ShowMinimized|INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_GROUPCHAT,groupchatType);

		INotificationType mentionType;
		mentionType.order = NTO_MUC_MENTION_MESSAGE;
		mentionType.title = tr("When referring to you at the conference");
		mentionType.kindMask = INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		mentionType.kindDefs = mentionType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_MENTION,mentionType);
	}

	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}

	return true;
}

bool MultiUserChatPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWENTERS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_ARCHIVESTATUS,false);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_CONFERENCES, OPN_CONFERENCES, tr("Conferences"), MNI_MUC_CONFERENCE };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> MultiUserChatPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_CONFERENCES)
	{
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS),tr("Show users connections/disconnections"),AParent));
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS),tr("Show users status changes"),AParent));
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_ARCHIVESTATUS),tr("Save status messages to history"),AParent));
	}
	return widgets;
}

bool MultiUserChatPlugin::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "join")
	{
		showJoinMultiChatDialog(AStreamJid, AContactJid, QString::null, AParams.value("password"));
		return true;
	}
	else if (AAction == "invite")
	{
		IMultiUserChat *mchat = multiUserChat(AStreamJid, AContactJid);
		if (mchat != NULL)
		{
			foreach(QString userJid, AParams.values("jid"))
				mchat->inviteContact(userJid, QString::null);
		}
		return true;
	}
	return false;
}

bool MultiUserChatPlugin::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature==NS_MUC && ADiscoInfo.contactJid.resource().isEmpty())
	{
		IMultiUserChatWindow *chatWindow = multiChatWindow(AStreamJid,ADiscoInfo.contactJid);
		if (!chatWindow)
			showJoinMultiChatDialog(AStreamJid,ADiscoInfo.contactJid,QString::null,QString::null);
		else
			chatWindow->showTabPage();
		return true;
	}
	return false;
}

Action *MultiUserChatPlugin::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature == NS_MUC)
	{
		if (FDiscovery && FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,"")>=0)
		{
			Action *action = createJoinAction(AStreamJid,ADiscoInfo.contactJid,AParent);
			return action;
		}
		else
		{
			Menu *inviteMenu = createInviteMenu(ADiscoInfo.contactJid,AParent);
			if (inviteMenu->isEmpty())
				delete inviteMenu;
			else
				return inviteMenu->menuAction();
		}
	}
	return NULL;
}

IDataFormLocale MultiUserChatPlugin::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_MUC_REGISTER)
	{
		locale.title = tr("Register in conference");
		locale.fields["muc#register_allow"].label = tr("Allow this person to register with the room?");
		locale.fields["muc#register_first"].label = tr("First Name");
		locale.fields["muc#register_last"].label = tr("Last Name");
		locale.fields["muc#register_roomnick"].label = tr("Desired Nickname");
		locale.fields["muc#register_url"].label = tr("Your URL");
		locale.fields["muc#register_email"].label = tr("EMail Address");
		locale.fields["muc#register_faqentry"].label = tr("Rules and Notes");
	}
	else if (AFormType == DATA_FORM_MUC_ROOMCONFIG)
	{
		locale.title = tr("Configure conference");
		locale.fields["muc#roomconfig_allowinvites"].label = tr("Allow Occupants to Invite Others?");
		locale.fields["muc#roomconfig_changesubject"].label = tr("Allow Occupants to Change Subject?");
		locale.fields["muc#roomconfig_enablelogging"].label = tr("Enable Logging of Room Conversations?");
		locale.fields["muc#roomconfig_lang"].label = tr("Natural Language for Room Discussions");
		locale.fields["muc#roomconfig_maxusers"].label = tr("Maximum Number of Room Occupants");
		locale.fields["muc#roomconfig_membersonly"].label = tr("Make Room Members-Only?");
		locale.fields["muc#roomconfig_moderatedroom"].label = tr("Make Room Moderated?");
		locale.fields["muc#roomconfig_passwordprotectedroom"].label = tr("Password is Required to Enter?");
		locale.fields["muc#roomconfig_persistentroom"].label = tr("Make Room Persistent?");
		locale.fields["muc#roomconfig_presencebroadcast"].label = tr("Roles for which Presence is Broadcast:");
		locale.fields["muc#roomconfig_publicroom"].label = tr("Allow Public Searching for Room?");
		locale.fields["muc#roomconfig_roomadmins"].label = tr("Full List of Room Admins");
		locale.fields["muc#roomconfig_roomdesc"].label = tr("Description of Room");
		locale.fields["muc#roomconfig_roomname"].label = tr("Natural-Language Room Name");
		locale.fields["muc#roomconfig_roomowners"].label = tr("Full List of Room Owners");
		locale.fields["muc#roomconfig_roomsecret"].label = tr("The Room Password");
		locale.fields["muc#roomconfig_whois"].label = tr("Affiliations that May Discover Real JIDs of Occupants");
		//ejabberd muc extension
		locale.fields["public_list"].label = tr("Make participants list public?");
		locale.fields["members_by_default"].label = tr("Default occupants as participants?");
		locale.fields["allow_private_messages"].label = tr("Allow occupants to send private messages?");
		locale.fields["allow_query_users"].label = tr("Allow occupants to query other occupants?");
		locale.fields["muc#roomconfig_allowvisitorstatus"].label = tr("Allow visitors to send status text in presence updates?");
		locale.fields["muc#roomconfig_allowvisitornickchange"].label = tr("Allow visitors to change nickname?");
	}
	else if (AFormType == DATA_FORM_MUC_ROOM_INFO)
	{
		locale.title = tr("Conference information");
		locale.fields["muc#roominfo_contactjid"].label = tr("Contact JID");
		locale.fields["muc#roominfo_description"].label = tr("Description of Room");
		locale.fields["muc#roominfo_lang"].label = tr("Natural Language for Room");
		locale.fields["muc#roominfo_ldapgroup"].label = tr("LDAP Group");
		locale.fields["muc#roominfo_logs"].label = tr("URL for Archived Discussion Logs");
		locale.fields["muc#roominfo_occupants"].label = tr("Current Number of Occupants in Room");
		locale.fields["muc#roominfo_subject"].label = tr("Current Subject or Discussion Topic in Room");
		locale.fields["muc#roominfo_subjectmod"].label = tr("The Room Subject Can be Modified by Participants?");
	}
	else if (AFormType == DATA_FORM_MUC_REQUEST)
	{
		locale.title = tr("Request for voice");
		locale.fields["muc#role"].label = tr("Requested Role");
		locale.fields["muc#jid"].label = tr("User ID");
		locale.fields["muc#roomnick"].label = tr("Room Nickname");
		locale.fields["muc#request_allow"].label = tr("Grant Voice?");
	}
	return locale;
}

bool MultiUserChatPlugin::checkMessage(int AOrder, const Message &AMessage)
{
	Q_UNUSED(AOrder);
	return !AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite").isNull();
}

bool MultiUserChatPlugin::receiveMessage(int AMessageId)
{
	FActiveInvites.append(AMessageId);
	return true;
}

bool MultiUserChatPlugin::showMessage(int AMessageId)
{
	Message message = FMessageProcessor->messageById(AMessageId);
	QDomElement inviteElem = message.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");
	Jid roomJid = message.from();
	Jid fromJid = inviteElem.attribute("from");
	if (roomJid.isValid() && fromJid.isValid())
	{
		InviteFields fields;
		fields.streamJid = message.to();
		fields.roomJid = roomJid;
		fields.fromJid  = fromJid;
		fields.password = inviteElem.firstChildElement("password").text();

		QString reason = inviteElem.firstChildElement("reason").text();
		QString msg = tr("You are invited to the conference %1 by %2.<br>Reason: %3").arg(Qt::escape(roomJid.bare())).arg(Qt::escape(fromJid.full())).arg(Qt::escape(reason));
		msg+="<br><br>";
		msg+=tr("Do you want to join this conference?");

		QMessageBox *inviteDialog = new QMessageBox(QMessageBox::Question,tr("Invite"),msg,QMessageBox::Yes|QMessageBox::No|QMessageBox::Ignore);
		inviteDialog->setAttribute(Qt::WA_DeleteOnClose,true);
		inviteDialog->setEscapeButton(QMessageBox::Ignore);
		inviteDialog->setModal(false);
		connect(inviteDialog,SIGNAL(finished(int)),SLOT(onInviteDialogFinished(int)));
		FInviteDialogs.insert(inviteDialog,fields);
		inviteDialog->show();
	}
	FActiveInvites.removeAt(FActiveInvites.indexOf(AMessageId ));
	FMessageProcessor->removeMessage(AMessageId);
	return true;
}

INotification MultiUserChatPlugin::notifyMessage(INotifications *ANotifications, const Message &AMessage)
{
	INotification notify;
	QDomElement inviteElem = AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");
	Jid roomJid = AMessage.from();
	if (!multiChatWindow(AMessage.to(),roomJid))
	{
		notify.kinds = ANotifications->notificationKinds(NNT_MUC_MESSAGE_INVITE);
		if (notify.kinds > 0)
		{
			Jid fromJid = inviteElem.attribute("from");
			notify.typeId = NNT_MUC_MESSAGE_INVITE;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_INVITE));
			notify.data.insert(NDR_TOOLTIP,tr("You are invited to the conference %1").arg(roomJid.bare()));
			notify.data.insert(NDR_STREAM_JID,AMessage.to());
			notify.data.insert(NDR_CONTACT_JID,fromJid.full());
			notify.data.insert(NDR_ROSTER_ORDER,RNO_MUC_INVITE);
			notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
			notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
			notify.data.insert(NDR_POPUP_CAPTION,tr("Invitation received"));
			notify.data.insert(NDR_POPUP_TITLE,ANotifications->contactName(AMessage.to(),fromJid));
			notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(fromJid));
			notify.data.insert(NDR_POPUP_HTML,Qt::escape(notify.data.value(NDR_TOOLTIP).toString()));
			notify.data.insert(NDR_SOUND_FILE,SDF_MUC_INVITE_MESSAGE);
		}
	}
	return notify;
}

bool MultiUserChatPlugin::createMessageWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AStreamJid);
	Q_UNUSED(AContactJid);
	Q_UNUSED(AType);
	Q_UNUSED(AShowMode);
	return false;
}

bool MultiUserChatPlugin::requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid)
{
	if (FDiscovery)
	{
		return FDiscovery->requestDiscoInfo(AStreamJid,ARoomJid.bare(),MUC_NODE_ROOM_NICK);
	}
	else if (FDataForms && FRegistration)
	{
		QString requestId = FRegistration->sendRegiterRequest(AStreamJid,ARoomJid.domain());
		if (!requestId.isEmpty())
		{
			FNickRequests.insert(requestId, qMakePair<Jid,Jid>(AStreamJid,ARoomJid));
			return true;
		}
	}
	return false;
}

IMultiUserChat *MultiUserChatPlugin::getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IMultiUserChat *chat = multiUserChat(AStreamJid,ARoomJid);
	if (!chat)
	{
		chat = new MultiUserChat(this,AStreamJid,ARoomJid,ANick.isEmpty() ? AStreamJid.node() : ANick,APassword,this);
		connect(chat->instance(),SIGNAL(chatDestroyed()),SLOT(onMultiUserChatDestroyed()));
		FChats.append(chat);
		emit multiUserChatCreated(chat);
	}
	return chat;
}

IMultiUserChat *MultiUserChatPlugin::multiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChat *chat, FChats)
		if (chat->streamJid() == AStreamJid && chat->roomJid() == ARoomJid)
			return chat;
	return NULL;
}

IMultiUserChatWindow *MultiUserChatPlugin::getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IMultiUserChatWindow *chatWindow = multiChatWindow(AStreamJid,ARoomJid);
	if (!chatWindow && FMessageWidgets)
	{
		IMultiUserChat *chat = getMultiUserChat(AStreamJid,ARoomJid,ANick,APassword);
		chatWindow = new MultiUserChatWindow(this,chat);
		chatWindow->setTabPageNotifier(FMessageWidgets!=NULL ? FMessageWidgets->newTabPageNotifier(chatWindow) : NULL);
		WidgetManager::setWindowSticky(chatWindow->instance(),true);
		connect(chatWindow->instance(),SIGNAL(multiUserContextMenu(IMultiUser *, Menu *)),SLOT(onMultiUserContextMenu(IMultiUser *, Menu *)));
		connect(chatWindow->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMultiChatWindowDestroyed()));
		insertChatAction(chatWindow);
		FChatWindows.append(chatWindow);
		emit multiChatWindowCreated(chatWindow);
	}
	return chatWindow;
}

IMultiUserChatWindow *MultiUserChatPlugin::multiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChatWindow *chatWindow,FChatWindows)
		if (chatWindow->streamJid()==AStreamJid && chatWindow->roomJid()==ARoomJid)
			return chatWindow;
	return NULL;
}

void MultiUserChatPlugin::showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	JoinMultiChatDialog *dialog = new JoinMultiChatDialog(this,AStreamJid,ARoomJid,ANick,APassword);
	dialog->show();
}

void MultiUserChatPlugin::insertChatAction(IMultiUserChatWindow *AWindow)
{
	if (FChatMenu)
	{
		Action *action = new Action(FChatMenu);
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFERENCE);
		action->setText(tr("%1 as %2").arg(AWindow->multiUserChat()->roomJid().bare()).arg(AWindow->multiUserChat()->nickName()));
		connect(action,SIGNAL(triggered(bool)),SLOT(onChatActionTriggered(bool)));
		FChatMenu->addAction(action,AG_DEFAULT,false);
		FChatActions.insert(AWindow,action);
	}
}

void MultiUserChatPlugin::removeChatAction(IMultiUserChatWindow *AWindow)
{
	if (FChatMenu && FChatActions.contains(AWindow))
		FChatMenu->removeAction(FChatActions.take(AWindow));
}

void MultiUserChatPlugin::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	QIcon icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_CONFERENCE);

	dfeature.active = true;
	dfeature.icon = icon;
	dfeature.var = NS_MUC;
	dfeature.name = tr("Multi-User Conferences");
	dfeature.description = tr("Supports the multi-user conferences");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = false;
	dfeature.icon = QIcon();

	dfeature.var = MUC_HIDDEN;
	dfeature.name = tr("Hidden room");
	dfeature.description = tr("A room that cannot be found by any user through normal means such as searching and service discovery");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_MEMBERSONLY;
	dfeature.name = tr("Members-only room");
	dfeature.description = tr("A room that a user cannot enter without being on the member list");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_MODERATED;
	dfeature.name = tr("Moderated room");
	dfeature.description = tr("A room in which only those with 'voice' may send messages to all occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_NONANONYMOUS;
	dfeature.name = tr("Non-anonymous room");
	dfeature.description = tr("A room in which an occupant's full JID is exposed to all other occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_OPEN;
	dfeature.name = tr("Open room");
	dfeature.description = tr("A room that anyone may enter without being on the member list");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_PASSWORD;
	dfeature.name = tr("Password-protected room");
	dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_PASSWORDPROTECTED;
	dfeature.name = tr("Password-protected room");
	dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_PERSISTENT;
	dfeature.name = tr("Persistent room");
	dfeature.description = tr("A room that is not destroyed if the last occupant exits");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_PUBLIC;
	dfeature.name = tr("Public room");
	dfeature.description = tr("A room that can be found by any user through normal means such as searching and service discovery");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_SEMIANONYMOUS;
	dfeature.name = tr("Semi-anonymous room");
	dfeature.description = tr("A room in which an occupant's full JID can be discovered by room admins only");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_TEMPORARY;
	dfeature.name = tr("Temporary room");
	dfeature.description = tr("A room that is destroyed if the last occupant exits");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_UNMODERATED;
	dfeature.name = tr("Unmoderated room");
	dfeature.description = tr("A room in which any occupant is allowed to send messages to all occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_UNSECURED;
	dfeature.name = tr("Unsecured room");
	dfeature.description = tr("A room that anyone is allowed to enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);
}

QString MultiUserChatPlugin::streamVCardNick(const Jid &AStreamJid) const
{
	QString nick;
	if (FVCardPlugin!=NULL && FVCardPlugin->hasVCard(AStreamJid.bare()))
	{
		IVCard *vCard = FVCardPlugin->vcard(AStreamJid.bare());
		nick = vCard->value(VVN_NICKNAME);
		vCard->unlock();
	}
	return nick;
}

Menu *MultiUserChatPlugin::createInviteMenu(const Jid &AContactJid, QWidget *AParent) const
{
	Menu *inviteMenu = new Menu(AParent);
	inviteMenu->setTitle(tr("Invite to"));
	inviteMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
	foreach(IMultiUserChatWindow *window, FChatWindows)
	{
		IMultiUserChat *mchat = window->multiUserChat();
		if (mchat->isOpen() && mchat->mainUser()->role()!=MUC_ROLE_VISITOR && !mchat->isUserPresent(AContactJid))
		{
			Action *action = new Action(inviteMenu);
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFERENCE);
			action->setText(tr("%1 from %2").arg(window->roomJid().full()).arg(window->multiUserChat()->nickName()));
			action->setData(ADR_STREAM_JID,window->streamJid().full());
			action->setData(ADR_HOST,AContactJid.full());
			action->setData(ADR_ROOM,window->roomJid().full());
			connect(action,SIGNAL(triggered(bool)),SLOT(onInviteActionTriggered(bool)));
			inviteMenu->addAction(action,AG_DEFAULT,true);
		}
	}
	return inviteMenu;
}

Action *MultiUserChatPlugin::createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QObject *AParent) const
{
	Action *action = new Action(AParent);
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_JOIN);
	action->setText(tr("Join conference"));
	action->setData(ADR_STREAM_JID,AStreamJid.full());
	action->setData(ADR_HOST,ARoomJid.domain());
	action->setData(ADR_ROOM,ARoomJid.node());
	connect(action,SIGNAL(triggered(bool)),SLOT(onJoinActionTriggered(bool)));
	return action;
}

void MultiUserChatPlugin::onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu)
{
	IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
	if (chatWindow)
	{
		if (FDiscovery && FDiscovery->hasDiscoInfo(chatWindow->streamJid(), AUser->contactJid()))
		{
			IDiscoInfo info = FDiscovery->discoInfo(chatWindow->streamJid(), AUser->contactJid());
			foreach(QString feature, info.features)
			{
				foreach(Action *action, FDiscovery->createFeatureActions(chatWindow->streamJid(),feature,info,AMenu))
					AMenu->addAction(action, AG_MUCM_DISCOVERY_FEATURES, true);
			}
		}
		emit multiUserContextMenu(chatWindow,AUser,AMenu);
	}
}

void MultiUserChatPlugin::onMultiUserChatDestroyed()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (FChats.contains(chat))
	{
		FChats.removeAll(chat);
		emit multiUserChatDestroyed(chat);
	}
}

void MultiUserChatPlugin::onMultiChatWindowDestroyed()
{
	IMultiUserChatWindow *chatWindow = qobject_cast<IMultiUserChatWindow *>(sender());
	if (chatWindow)
	{
		removeChatAction(chatWindow);
		FChatWindows.removeAll(chatWindow);
		emit multiChatWindowCreated(chatWindow);
	}
}

void MultiUserChatPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
	QList<IMultiUserChatWindow *> chatWindows = FChatWindows;
	foreach(IMultiUserChatWindow *chatWindow, chatWindows)
		if (chatWindow->streamJid() == AXmppStream->streamJid())
			chatWindow->exitAndDestroy("",0);

	QList<QMessageBox *> inviteDialogs = FInviteDialogs.keys();
	foreach(QMessageBox * inviteDialog,inviteDialogs)
		if (FInviteDialogs.value(inviteDialog).streamJid == AXmppStream->streamJid())
			inviteDialog->done(QMessageBox::Ignore);

	for (int i=0; i<FActiveInvites.count();i++)
		if (AXmppStream->streamJid() == FMessageProcessor->messageById(FActiveInvites.at(i)).to())
		{
			FMessageProcessor->removeMessage(FActiveInvites.at(i));
			FActiveInvites.removeAt(i--);
		}
}

void MultiUserChatPlugin::onJoinActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString host = action->data(ADR_HOST).toString();
		QString room = action->data(ADR_ROOM).toString();
		QString nick = action->data(ADR_NICK).toString();
		QString password = action->data(ADR_PASSWORD).toString();
		Jid streamJid = action->data(Action::DR_StreamJid).toString();
		Jid roomJid(room,host,"");
		showJoinMultiChatDialog(streamJid,roomJid,nick,password);
	}
}

void MultiUserChatPlugin::onShowAllRoomsTriggered(bool)
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (!window->isVisibleTabPage())
			window->showTabPage();
}

void MultiUserChatPlugin::onLeaveHiddenRoomsTriggered(bool)
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (!window->isVisibleTabPage())
			window->exitAndDestroy(QString::null);
}

void MultiUserChatPlugin::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if (ALabelId==RLID_DISPLAY && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		if (index->type() == RIT_STREAM_ROOT)
		{
			int show = index->data(RDR_SHOW).toInt();
			if (show!=IPresence::Offline && show!=IPresence::Error)
			{
				Action *action = createJoinAction(index->data(RDR_FULL_JID).toString(),Jid::null,AMenu);
				AMenu->addAction(action,AG_RVCM_MULTIUSERCHAT,true);
			}
		}
	}
}

void MultiUserChatPlugin::onChatActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IMultiUserChatWindow *window = FChatActions.key(action,NULL);
	if (window)
		window->showTabPage();
}

void MultiUserChatPlugin::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.node == MUC_NODE_ROOM_NICK)
	{
		if (ADiscoInfo.error.code == -1)
		{
			QString nick = ADiscoInfo.identity.value(FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,DIT_TEXT)).name;
			if (nick.isEmpty())
				nick = streamVCardNick(ADiscoInfo.streamJid);
			emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,nick);
		}
		else if (FDataForms && FRegistration)
		{
			QString requestId = FRegistration->sendRegiterRequest(ADiscoInfo.streamJid,ADiscoInfo.contactJid.domain());
			if (!requestId.isEmpty())
				FNickRequests.insert(requestId, qMakePair<Jid,Jid>(ADiscoInfo.streamJid,ADiscoInfo.contactJid));
			else
				emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,streamVCardNick(ADiscoInfo.streamJid));
		}
		else
		{
			emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,streamVCardNick(ADiscoInfo.streamJid));
		}
	}
}

void MultiUserChatPlugin::onRegisterFieldsReceived(const QString &AId, const IRegisterFields &AFields)
{
	if (FNickRequests.contains(AId))
	{
		QPair<Jid,Jid> params = FNickRequests.take(AId);
		QString nick = FDataForms!=NULL ? FDataForms->fieldValue("nick",AFields.form.fields).toString() : AFields.username;
		if (nick.isEmpty())
			nick = streamVCardNick(params.first);
		emit roomNickReceived(params.first,params.second,nick);
	}
}

void MultiUserChatPlugin::onRegisterErrorReceived(const QString &AId, const QString &AError)
{
	Q_UNUSED(AError);
	if (FNickRequests.contains(AId))
	{
		QPair<Jid,Jid> params = FNickRequests.take(AId);
		emit roomNickReceived(params.first,params.second,streamVCardNick(params.first));
	}
}

void MultiUserChatPlugin::onInviteDialogFinished(int AResult)
{
	QMessageBox *inviteDialog = qobject_cast<QMessageBox *>(sender());
	if (inviteDialog)
	{
		InviteFields fields = FInviteDialogs.take(inviteDialog);
		if (AResult == QMessageBox::Yes)
		{
			showJoinMultiChatDialog(fields.streamJid,fields.roomJid,"",fields.password);
		}
		else if (AResult == QMessageBox::No)
		{
			Message decline;
			decline.setTo(fields.roomJid.eBare());
			Stanza &mstanza = decline.stanza();
			QDomElement declElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("decline")).toElement();
			declElem.setAttribute("to",fields.fromJid.eFull());
			QString reason = tr("I'm too busy right now");
			reason = QInputDialog::getText(inviteDialog,tr("Decline invite"),tr("Enter a reason"),QLineEdit::Normal,reason);
			if (!reason.isEmpty())
				declElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(reason));
			FMessageProcessor->sendMessage(fields.streamJid,decline);
		}
	}
}

void MultiUserChatPlugin::onInviteActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_HOST).toString();
		Jid roomJid = action->data(ADR_ROOM).toString();
		IMultiUserChatWindow *window = multiChatWindow(streamJid,roomJid);
		if (window && contactJid.isValid())
		{
			bool ok;
			QString reason = tr("You are welcome here");
			reason = QInputDialog::getText(window->instance(),tr("Invite user"),tr("Enter a reason"),QLineEdit::Normal,reason,&ok);
			if (ok)
				window->multiUserChat()->inviteContact(contactJid,reason);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_multiuserchat, MultiUserChatPlugin)
