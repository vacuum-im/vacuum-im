#include "multiuserchatplugin.h"

#include <QClipboard>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <definitions/namespaces.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/dataformtypes.h>
#include <definitions/recentitemtypes.h>
#include <definitions/recentitemproperties.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/rosterindexkindorders.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagehandlerorders.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/action.h>
#include <utils/logger.h>

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_HOST                  Action::DR_Parametr1
#define ADR_ROOM                  Action::DR_Parametr2
#define ADR_NICK                  Action::DR_Parametr3
#define ADR_PASSWORD              Action::DR_Parametr4
#define ADR_CLIPBOARD_DATA        Action::DR_Parametr1

#define DIC_CONFERENCE            "conference"
#define DIT_TEXT                  "text"

MultiUserChatPlugin::MultiUserChatPlugin()
{
	FPluginManager = NULL;
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FRostersViewPlugin = NULL;
	FRostersModel = NULL;
	FXmppStreams = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FDataForms = NULL;
	FVCardPlugin = NULL;
	FRegistration = NULL;
	FXmppUriQueries = NULL;
	FOptionsManager = NULL;
	FStatusIcons = NULL;
	FRecentContacts = NULL;
}

MultiUserChatPlugin::~MultiUserChatPlugin()
{

}

void MultiUserChatPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Multi-User Conferences");
	APluginInfo->description = tr("Allows to use Jabber multi-user conferences");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MultiUserChatPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
		{
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
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
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
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

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
		if (FStatusIcons)
		{
			connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
		}
	}

	plugin = APluginManager->pluginInterface("IRecentContacts").value(0,NULL);
	if (plugin)
	{
		FRecentContacts = qobject_cast<IRecentContacts *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRosterIndexDestroyed(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
				SLOT(onRostersViewClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
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
			connect(FRegistration->instance(),SIGNAL(registerError(const QString &, const XmppError &)),
				SLOT(onRegisterErrorReceived(const QString &, const XmppError &)));
		}
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FXmppStreams!=NULL;
}

bool MultiUserChatPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_APP_MUCJOIN, tr("Join conference"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_MUCSHOWHIDDEN, tr("Show all hidden conferences"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_MUCLEAVEHIDDEN, tr("Leave all hidden conferences"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);

	Shortcuts::declareGroup(SCTG_MESSAGEWINDOWS_MUC, tr("Multi-user chat window"), SGO_MESSAGEWINDOWS_MUC);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_SENDMESSAGE, tr("Send message"), tr("Return","Send message"), Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CLEARWINDOW, tr("Clear window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CHANGENICK, tr("Change nick"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_CHANGETOPIC, tr("Change topic"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_ROOMSETTINGS, tr("Setup conference"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_ENTER, tr("Enter the conference"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_EXIT, tr("Leave the conference"), tr("Ctrl+Q","Leave the conference"));

	Shortcuts::declareShortcut(SCT_ROSTERVIEW_ENTERCONFERENCE, tr("Enter conference"), QKeySequence::UnknownKey, Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_EXITCONFERENCE, tr("Exit conference"), QKeySequence::UnknownKey, Shortcuts::WidgetShortcut);

	Shortcuts::insertWidgetShortcut(SCT_APP_MUCJOIN,qApp->desktop());
	Shortcuts::insertWidgetShortcut(SCT_APP_MUCSHOWHIDDEN,qApp->desktop());
	Shortcuts::insertWidgetShortcut(SCT_APP_MUCLEAVEHIDDEN,qApp->desktop());

	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_INVITE,this);
		FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
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

	if (FNotifications)
	{
		INotificationType inviteType;
		inviteType.order = NTO_MUC_INVITE_MESSAGE;
		inviteType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_INVITE);
		inviteType.title = tr("When receiving an invitation to the conference");
		inviteType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AutoActivate;
		inviteType.kindDefs = inviteType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_INVITE,inviteType);

		INotificationType privateType;
		privateType.order = NTO_MUC_PRIVATE_MESSAGE;
		privateType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_PRIVATE_MESSAGE);
		privateType.title = tr("When receiving a new private message in conference");
		privateType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::TrayAction|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		privateType.kindDefs = privateType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_PRIVATE,privateType);

		INotificationType groupchatType;
		groupchatType.order = NTO_MUC_GROUPCHAT_MESSAGE;
		groupchatType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_MESSAGE);
		groupchatType.title = tr("When receiving a new message in conference");
		groupchatType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized;
		groupchatType.kindDefs = groupchatType.kindMask & ~(INotification::PopupWindow|INotification::ShowMinimized|INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_GROUPCHAT,groupchatType);

		INotificationType mentionType;
		mentionType.order = NTO_MUC_MENTION_MESSAGE;
		mentionType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_MESSAGE);
		mentionType.title = tr("When referring to you at the conference");
		mentionType.kindMask = INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		mentionType.kindDefs = mentionType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_MENTION,mentionType);
	}

	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
	}

	if (FRostersModel)
	{
		FRostersModel->registerSingleGroup(RIK_GROUP_MUC,tr("Conferences"));
	}

	if (FRostersViewPlugin)
	{
		FRostersViewPlugin->rostersView()->insertClickHooker(RCHO_MULTIUSERCHAT,this);
		FRostersViewPlugin->registerExpandableRosterIndexKind(RIK_GROUP_MUC,RDR_KIND);

		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_ENTERCONFERENCE,FRostersViewPlugin->rostersView()->instance());
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_EXITCONFERENCE,FRostersViewPlugin->rostersView()->instance());
	}

	if (FRecentContacts)
	{
		FRecentContacts->registerItemHandler(REIT_CONFERENCE,this);
	}

	return true;
}

bool MultiUserChatPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWENTERS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_ARCHIVESTATUS,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_REJOINAFTERKICK,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_BASHAPPEND,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_NICKNAMESUFIX,", ");

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
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE),tr("Leave the conference when window closed"),AParent));
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_REJOINAFTERKICK),tr("Rejoin in conference after kick"),AParent));
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_BASHAPPEND),tr("Select the user to refer without menu"),AParent));
		widgets.insertMulti(OWO_CONFERENCES, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_NICKNAMESUFIX),tr("Add this suffix when referring to the user:"),AParent));
	}
	return widgets;
}

bool MultiUserChatPlugin::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);
	if (AEvent->modifiers()==Qt::NoModifier && Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool() )
	{
		IMultiUserChatWindow *window = findMultiChatWindowForIndex(AIndex);
		if (window)
		{
			window->showTabPage();
			return true;
		}
	}
	return false;
}

bool MultiUserChatPlugin::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);
	if (AEvent->modifiers() == Qt::NoModifier)
	{
		IMultiUserChatWindow *window = getMultiChatWindowForIndex(AIndex);
		if (window)
		{
			if (!window->multiUserChat()->isConnected() && window->multiUserChat()->roomError().isNull())
				window->multiUserChat()->sendStreamPresence();
			window->showTabPage();
		}
	}
	return false;
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
		IMultiUserChat *chat = findMultiUserChat(AStreamJid, AContactJid);
		if (chat != NULL)
		{
			foreach(const QString &userJid, AParams.values("jid"))
				chat->inviteContact(userJid, QString::null);
		}
		return true;
	}
	return false;
}

bool MultiUserChatPlugin::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature==NS_MUC && ADiscoInfo.contactJid.resource().isEmpty())
	{
		IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,ADiscoInfo.contactJid);
		if (!window)
			showJoinMultiChatDialog(AStreamJid,ADiscoInfo.contactJid,QString::null,QString::null);
		else
			window->showTabPage();
		return true;
	}
	return false;
}

Action *MultiUserChatPlugin::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature == NS_MUC)
	{
		if (FDiscovery && FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,QString::null)>=0)
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

bool MultiUserChatPlugin::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(ADirection);
	if (AOrder == MHO_MULTIUSERCHAT_INVITE)
		return !AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite").isNull();
	return false;
}

bool MultiUserChatPlugin::messageDisplay(const Message &AMessage, int ADirection)
{
	Q_UNUSED(AMessage);
	return ADirection == IMessageProcessor::MessageIn;
}

INotification MultiUserChatPlugin::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::MessageIn)
	{
		QDomElement inviteElem = AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");

		Jid roomJid = AMessage.from();
		Jid streamJid = AMessage.to();
		Jid contactJid = inviteElem.attribute("from");
		LOG_STRM_DEBUG(streamJid,QString("Received invite to room=%1 from=%2").arg(roomJid.full(),contactJid.full()));

		if (!findMultiChatWindow(streamJid,roomJid))
		{
			notify.kinds = ANotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_INVITE);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_MUC_MESSAGE_INVITE;
				notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_INVITE));
				notify.data.insert(NDR_TOOLTIP,tr("You are invited to the conference %1").arg(roomJid.uBare()));
				notify.data.insert(NDR_STREAM_JID,streamJid.full());
				notify.data.insert(NDR_CONTACT_JID,contactJid.full());
				notify.data.insert(NDR_ROSTER_ORDER,RNO_MUC_INVITE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
				notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
				notify.data.insert(NDR_POPUP_CAPTION,tr("Invitation received"));
				notify.data.insert(NDR_POPUP_TITLE,ANotifications->contactName(streamJid,contactJid));
				notify.data.insert(NDR_POPUP_IMAGE,ANotifications->contactAvatar(contactJid));
				notify.data.insert(NDR_POPUP_TEXT,notify.data.value(NDR_TOOLTIP).toString());
				notify.data.insert(NDR_SOUND_FILE,SDF_MUC_INVITE_MESSAGE);
				FActiveInvites.insert(AMessage.data(MDR_MESSAGE_ID).toInt(),AMessage);
			}
		}
	}
	return notify;
}

bool MultiUserChatPlugin::messageShowWindow(int AMessageId)
{
	if (FActiveInvites.contains(AMessageId))
	{
		Message message = FActiveInvites.take(AMessageId);
		QDomElement inviteElem = message.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");

		InviteFields fields;
		fields.streamJid = message.to();
		fields.roomJid = message.from();
		fields.fromJid  = inviteElem.attribute("from");
		fields.password = inviteElem.firstChildElement("password").text();

		QString reason = inviteElem.firstChildElement("reason").text();
		QString msg = tr("You are invited to the conference %1 by %2.<br>Reason: %3").arg(Qt::escape(fields.roomJid.uBare())).arg(Qt::escape(fields.fromJid.uBare())).arg(Qt::escape(reason));
		msg += "<br><br>";
		msg += tr("Do you want to join this conference?");

		QMessageBox *inviteDialog = new QMessageBox(QMessageBox::Question,tr("Invite"),msg,QMessageBox::Yes|QMessageBox::No|QMessageBox::Ignore);
		inviteDialog->setAttribute(Qt::WA_DeleteOnClose,true);
		inviteDialog->setEscapeButton(QMessageBox::Ignore);
		inviteDialog->setModal(false);
		connect(inviteDialog,SIGNAL(finished(int)),SLOT(onInviteDialogFinished(int)));
		FInviteDialogs.insert(inviteDialog,fields);
		inviteDialog->show();

		FMessageProcessor->removeMessageNotify(AMessageId);
		return true;
	}
	return false;
}

bool MultiUserChatPlugin::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	if (AOrder==MHO_MULTIUSERCHAT_GROUPCHAT && AType==Message::GroupChat)
	{
		IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
		if (stream && stream->isOpen())
		{
			QString nick = AContactJid.resource().isEmpty() ? AContactJid.node() : AContactJid.resource();
			IMultiUserChatWindow *window = getMultiChatWindow(AStreamJid,AContactJid.bare(),nick,QString::null);
			if (window)
			{
				if (AShowMode == IMessageHandler::SM_ASSIGN)
					window->assignTabPage();
				else if (AShowMode == IMessageHandler::SM_SHOW)
					window->showTabPage();
				else if (AShowMode == IMessageHandler::SM_MINIMIZED)
					window->showMinimizedTabPage();
				return true;
			}
		}
	}
	return false;
}

bool MultiUserChatPlugin::recentItemValid(const IRecentItem &AItem) const
{
	return !AItem.reference.isEmpty();
}

bool MultiUserChatPlugin::recentItemCanShow(const IRecentItem &AItem) const
{
	Q_UNUSED(AItem);
	return true;
}

QIcon MultiUserChatPlugin::recentItemIcon(const IRecentItem &AItem) const
{
	return FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AItem.reference,IPresence::Offline,SUBSCRIPTION_BOTH,false) : QIcon();
}

QString MultiUserChatPlugin::recentItemName(const IRecentItem &AItem) const
{
	QString name = FRecentContacts->itemProperty(AItem,REIP_NAME).toString();
	return name.isEmpty() ? AItem.reference : name;
}

IRecentItem MultiUserChatPlugin::recentItemForIndex(const IRosterIndex *AIndex) const
{
	IRecentItem item;
	if (AIndex->kind() == RIK_MUC_ITEM)
	{
		item.type = REIT_CONFERENCE;
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_PREP_BARE_JID).toString();
	}
	return item;
}

QList<IRosterIndex *> MultiUserChatPlugin::recentItemProxyIndexes(const IRecentItem &AItem) const
{
	QList<IRosterIndex *> proxies;
	IRosterIndex *index = findMultiChatRosterIndex(AItem.streamJid,AItem.reference);
	if (index)
		proxies.append(index);
	return proxies;
}

IPluginManager *MultiUserChatPlugin::pluginManager() const
{
	return FPluginManager;
}

bool MultiUserChatPlugin::requestRoomNick(const Jid &AStreamJid, const Jid &ARoomJid)
{
	if (FDiscovery)
	{
		if (FDiscovery->requestDiscoInfo(AStreamJid,ARoomJid.bare(),MUC_NODE_ROOM_NICK))
		{
			LOG_STRM_INFO(AStreamJid,QString("Room nick request sent as discovery request, room=%1").arg(ARoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send room nick request as discovery request, room=%1").arg(ARoomJid.bare()));
		}
	}
	else if (FDataForms && FRegistration)
	{
		QString requestId = FRegistration->sendRegiterRequest(AStreamJid,ARoomJid.domain());
		if (!requestId.isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,QString("Room nick request sent as register request, room=%1, id=%2").arg(ARoomJid.domain(),requestId));
			FNickRequests.insert(requestId, qMakePair<Jid,Jid>(AStreamJid,ARoomJid));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send room nick request as register request, room=%1").arg(ARoomJid.domain()));
		}
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed to send room nick request, room=%1: Required interfaces not found").arg(ARoomJid.domain()));
	}
	return false;
}

QList<IMultiUserChat *> MultiUserChatPlugin::multiUserChats() const
{
	return FChats;
}

IMultiUserChat *MultiUserChatPlugin::findMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChat *chat, FChats)
		if (chat->streamJid()==AStreamJid && chat->roomJid()==ARoomJid)
			return chat;
	return NULL;
}

IMultiUserChat *MultiUserChatPlugin::getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IMultiUserChat *chat = findMultiUserChat(AStreamJid,ARoomJid);
	if (!chat)
	{
		LOG_STRM_INFO(AStreamJid,QString("Creating multi user chat, room=%1, nick=%2").arg(ARoomJid.full(),ANick));
		chat = new MultiUserChat(this,AStreamJid, ARoomJid, ANick.isEmpty() ? AStreamJid.uNode() : ANick, APassword, this);
		connect(chat->instance(),SIGNAL(roomNameChanged(const QString &)),SLOT(onMultiUserChatChanged()));
		connect(chat->instance(),SIGNAL(presenceChanged(int, const QString &)),SLOT(onMultiUserChatChanged()));
		connect(chat->instance(),SIGNAL(userNickChanged(IMultiUser *, const QString &, const QString &)),SLOT(onMultiUserChatChanged()));
		connect(chat->instance(),SIGNAL(chatDestroyed()),SLOT(onMultiUserChatDestroyed()));
		FChats.append(chat);
		emit multiUserChatCreated(chat);
	}
	return chat;
}

QList<IMultiUserChatWindow *> MultiUserChatPlugin::multiChatWindows() const
{
	return FChatWindows;
}

IMultiUserChatWindow *MultiUserChatPlugin::findMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (window->streamJid()==AStreamJid && window->contactJid()==ARoomJid)
			return window;
	return NULL;
}

IMultiUserChatWindow *MultiUserChatPlugin::getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IMultiUserChatWindow *window =NULL;
	if (FMessageProcessor && FMessageProcessor->isActiveStream(AStreamJid) && ARoomJid.isValid())
	{
		window = findMultiChatWindow(AStreamJid,ARoomJid);
		if (!window)
		{
			IMultiUserChat *chat = getMultiUserChat(AStreamJid,ARoomJid,ANick,APassword);
			if (chat)
			{
				LOG_STRM_INFO(AStreamJid,QString("Creating multi user chat window, room=%1, nick=%2").arg(ARoomJid.full(),ANick));
				
				window = new MultiUserChatWindow(this,chat);
				WidgetManager::setWindowSticky(window->instance(),true);
				connect(window->instance(),SIGNAL(multiChatContextMenu(Menu *)),SLOT(onMultiChatContextMenu(Menu *)));
				connect(window->instance(),SIGNAL(multiUserContextMenu(IMultiUser *, Menu *)),SLOT(onMultiUserContextMenu(IMultiUser *, Menu *)));
				connect(window->instance(),SIGNAL(multiUserToolTips(IMultiUser *, QMap<int,QString> &)),SLOT(onMultiUserToolTips(IMultiUser *, QMap<int,QString> &)));
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMultiChatWindowDestroyed()));
				connect(window->infoWidget()->instance(),SIGNAL(contextMenuRequested(Menu *)),SLOT(onMultiChatWindowInfoContextMenu(Menu *)));
				connect(window->infoWidget()->instance(),SIGNAL(toolTipsRequested(QMap<int,QString> &)),SLOT(onMultiChatWindowInfoToolTips(QMap<int,QString> &)));
				FChatWindows.append(window);

				getMultiChatRosterIndex(window->streamJid(),window->contactJid(),window->multiUserChat()->nickName(),window->multiUserChat()->password());
				emit multiChatWindowCreated(window);
			}
			else
			{
				REPORT_ERROR("Failed to get multi user chat window: Multi user chat not created");
			}
		}
	}
	else if (!ARoomJid.isValid())
	{
		REPORT_ERROR("Failed to get multi user chat window: Invalid room jid");
	}
	else if (FMessageProcessor)
	{
		REPORT_ERROR("Failed to get multi user chat window: Stream is not active");
	}
	else
	{
		REPORT_ERROR("Failed to get multi user chat window: Required interfaces not found");
	}
	return window;
}

QList<IRosterIndex *> MultiUserChatPlugin::multiChatRosterIndexes() const
{
	return FChatIndexes;
}

IRosterIndex *MultiUserChatPlugin::findMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	for (QList<IRosterIndex *>::const_iterator it= FChatIndexes.constBegin(); it!=FChatIndexes.constEnd(); ++it)
	{
		IRosterIndex *index = *it;
		if (AStreamJid==index->data(RDR_STREAM_JID).toString() && (ARoomJid.isEmpty() || ARoomJid==index->data(RDR_PREP_BARE_JID).toString()))
			return index;
	}
	return NULL;
}

IRosterIndex *MultiUserChatPlugin::getMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IRosterIndex *chatIndex = findMultiChatRosterIndex(AStreamJid,ARoomJid);
	if (chatIndex == NULL)
	{
		IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
		if (sroot)
		{
			IRosterIndex *chatGroup = FRostersModel->getGroupIndex(RIK_GROUP_MUC,tr("Conferences"),sroot);
			chatGroup->setData(RIKO_GROUP_MUC,RDR_KIND_ORDER);

			chatIndex = FRostersModel->newRosterIndex(RIK_MUC_ITEM);
			FChatIndexes.append(chatIndex);

			chatIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
			chatIndex->setData(ARoomJid.full(),RDR_FULL_JID);
			chatIndex->setData(ARoomJid.pFull(),RDR_PREP_FULL_JID);
			chatIndex->setData(ARoomJid.pBare(),RDR_PREP_BARE_JID);

			IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,ARoomJid);
			if (window == NULL)
			{
				if (FStatusIcons)
					chatIndex->setData(FStatusIcons->iconByJidStatus(ARoomJid,IPresence::Offline,SUBSCRIPTION_BOTH,false),Qt::DecorationRole);
				chatIndex->setData(QString(),RDR_STATUS);
				chatIndex->setData(IPresence::Offline,RDR_SHOW);
				chatIndex->setData(getRoomName(AStreamJid,ARoomJid),RDR_NAME);
				chatIndex->setData(ANick,RDR_MUC_NICK);
				chatIndex->setData(APassword,RDR_MUC_PASSWORD);
			}
			else
			{
				updateChatRosterIndex(window);
			}

			FRostersModel->insertRosterIndex(chatIndex,chatGroup);
			emit multiChatRosterIndexCreated(chatIndex);

			updateRecentItemProxy(chatIndex);
		}
		else
		{
			REPORT_ERROR("Failed to get multi user chat roster index: Stream root index not found");
		}
	}
	return chatIndex;
}

void MultiUserChatPlugin::showJoinMultiChatDialog(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
	if (stream && stream->isOpen())
	{
		JoinMultiChatDialog *dialog = new JoinMultiChatDialog(this,AStreamJid,ARoomJid,ANick,APassword);
		dialog->show();
	}
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
		IVCard *vCard = FVCardPlugin->getVCard(AStreamJid.bare());
		nick = vCard->value(VVN_NICKNAME);
		vCard->unlock();
	}
	return nick;
}

void MultiUserChatPlugin::updateRecentItemProxy(IRosterIndex *AIndex)
{
	if (AIndex)
	{
		IRecentItem item;
		item.type = REIT_CONFERENCE;
		item.streamJid = AIndex->data(RDR_STREAM_JID).toString();
		item.reference = AIndex->data(RDR_PREP_BARE_JID).toString();
		emit recentItemUpdated(item);
	}
}

void MultiUserChatPlugin::updateRecentItemProperties(IRosterIndex *AIndex)
{
	if (FRecentContacts && FRecentContacts->isReady(AIndex->data(RDR_STREAM_JID).toString()))
	{
		IRecentItem item = recentItemForIndex(AIndex);
		FRecentContacts->setItemProperty(item,REIP_CONFERENCE_NICK,AIndex->data(RDR_MUC_NICK).toString());
		FRecentContacts->setItemProperty(item,REIP_CONFERENCE_PASSWORD,AIndex->data(RDR_MUC_PASSWORD).toString());
	}
}

void MultiUserChatPlugin::updateChatRosterIndex(IMultiUserChatWindow *AWindow)
{
	IRosterIndex *chatIndex = AWindow!=NULL ? findMultiChatRosterIndex(AWindow->streamJid(),AWindow->contactJid()) : NULL;
	if (chatIndex)
	{
		if (FStatusIcons)
			chatIndex->setData(FStatusIcons->iconByJidStatus(AWindow->contactJid(),AWindow->multiUserChat()->show(),SUBSCRIPTION_BOTH,false),Qt::DecorationRole);
		if (!AWindow->multiUserChat()->roomError().isNull())
			chatIndex->setData(AWindow->multiUserChat()->roomError().errorMessage(),RDR_STATUS);
		else
			chatIndex->setData(QString(),RDR_STATUS);
		chatIndex->setData(getRoomName(AWindow->streamJid(),AWindow->contactJid()),RDR_NAME);
		chatIndex->setData(AWindow->multiUserChat()->show(),RDR_SHOW);
		chatIndex->setData(AWindow->multiUserChat()->nickName(),RDR_MUC_NICK);
		chatIndex->setData(AWindow->multiUserChat()->password(),RDR_MUC_PASSWORD);
		updateRecentItemProperties(chatIndex);
	}
}

bool MultiUserChatPlugin::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	if (ASelected.count() > 1)
	{
		foreach(IRosterIndex *index, ASelected)
			if (index->kind() != RIK_MUC_ITEM)
				return false;
	}
	return !ASelected.isEmpty();
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
			action->setText(tr("%1 from %2").arg(window->contactJid().uBare()).arg(window->multiUserChat()->nickName()));
			action->setData(ADR_STREAM_JID,window->streamJid().full());
			action->setData(ADR_HOST,AContactJid.full());
			action->setData(ADR_ROOM,window->contactJid().full());
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
	connect(action,SIGNAL(triggered(bool)),SLOT(onJoinRoomActionTriggered(bool)));
	return action;
}

IMultiUserChatWindow *MultiUserChatPlugin::findMultiChatWindowForIndex(const IRosterIndex *AIndex) const
{
	IMultiUserChatWindow *window = NULL;
	if (AIndex->kind() == RIK_MUC_ITEM)
		window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_PREP_BARE_JID).toString());
	else if (AIndex->kind() == RIK_RECENT_ITEM)
		window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_RECENT_REFERENCE).toString());
	return window;
}

IMultiUserChatWindow *MultiUserChatPlugin::getMultiChatWindowForIndex(const IRosterIndex *AIndex)
{
	IMultiUserChatWindow *window = NULL;
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();

	IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(streamJid) : NULL;
	if (stream && stream->isOpen())
	{
		if (AIndex->kind() == RIK_MUC_ITEM)
		{
			Jid roomJid = AIndex->data(RDR_PREP_BARE_JID).toString();
			window = getMultiChatWindow(streamJid,roomJid,AIndex->data(RDR_MUC_NICK).toString(),AIndex->data(RDR_MUC_PASSWORD).toString());
		}
		else if (FRecentContacts && AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE)
		{
			IRecentItem item = FRecentContacts->rosterIndexItem(AIndex);
			Jid roomJid = AIndex->data(RDR_RECENT_REFERENCE).toString();
			QString nick = FRecentContacts->itemProperty(item,REIP_CONFERENCE_NICK).toString();
			QString password = FRecentContacts->itemProperty(item,REIP_CONFERENCE_PASSWORD).toString();
			window = getMultiChatWindow(streamJid,roomJid,nick.isEmpty() ? streamJid.uNode() : nick,password);
		}
	}
	return window;
}

QString MultiUserChatPlugin::getRoomName(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	QString name = ARoomJid.uBare();

	IMultiUserChat *chat = findMultiUserChat(AStreamJid, ARoomJid);
	if (chat)
		name = chat->roomName();

	if (FRecentContacts && name==ARoomJid.uBare())
	{
		IRecentItem item;
		item.type = REIT_CONFERENCE;
		item.streamJid = AStreamJid;
		item.reference = ARoomJid.pBare();

		QString recent_name = FRecentContacts->itemProperty(item,REIP_NAME).toString();
		if (!recent_name.isEmpty())
			name = recent_name;
	}

	return name;
}

void MultiUserChatPlugin::onMultiChatContextMenu(Menu *AMenu)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiChatContextMenu(window,AMenu);
}

void MultiUserChatPlugin::onMultiUserContextMenu(IMultiUser *AUser, Menu *AMenu)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiUserContextMenu(window,AUser,AMenu);
}

void MultiUserChatPlugin::onMultiUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiUserToolTips(window,AUser,AToolTips);
}

void MultiUserChatPlugin::onMultiUserChatChanged()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (chat)
		updateChatRosterIndex(findMultiChatWindow(chat->streamJid(),chat->roomJid()));
}

void MultiUserChatPlugin::onMultiUserChatDestroyed()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (FChats.contains(chat))
	{
		LOG_STRM_INFO(chat->streamJid(),QString("Multi user chat destroyed, room=%1").arg(chat->roomJid().full()));
		FChats.removeAll(chat);
		emit multiUserChatDestroyed(chat);
	}
}

void MultiUserChatPlugin::onMultiChatWindowDestroyed()
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
	{
		LOG_STRM_INFO(window->streamJid(),QString("Multi user chat window destroyed, room=%1").arg(window->multiUserChat()->roomJid().full()));
		FChatWindows.removeAll(window);
		emit multiChatWindowDestroyed(window);
	}
}

void MultiUserChatPlugin::onMultiChatWindowInfoContextMenu(Menu *AMenu)
{
	if (FRostersViewPlugin)
	{
		IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
		IRosterIndex *index = widget!=NULL ? findMultiChatRosterIndex(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()) : NULL;
		if (index)
			FRostersViewPlugin->rostersView()->contextMenuForIndex(QList<IRosterIndex *>()<<index,NULL,AMenu);
	}
}

void MultiUserChatPlugin::onMultiChatWindowInfoToolTips(QMap<int,QString> &AToolTips)
{
	if (FRostersViewPlugin)
	{
		IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
		IRosterIndex *index = widget!=NULL ? findMultiChatRosterIndex(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()) : NULL;
		if (index)
			FRostersViewPlugin->rostersView()->toolTipsForIndex(index,NULL,AToolTips);
	}
}

void MultiUserChatPlugin::onStatusIconsChanged()
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		updateChatRosterIndex(window);
}

void MultiUserChatPlugin::onJoinRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString host = action->data(ADR_HOST).toString();
		QString room = action->data(ADR_ROOM).toString();
		QString nick = action->data(ADR_NICK).toString();
		QString password = action->data(ADR_PASSWORD).toString();
		Jid streamJid = action->data(Action::DR_StreamJid).toString();
		Jid roomJid(room,host,QString::null);
		showJoinMultiChatDialog(streamJid,roomJid,nick,password);
	}
}

void MultiUserChatPlugin::onOpenRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUserChatWindow *window = findMultiChatWindow(action->data(ADR_STREAM_JID).toString(),action->data(ADR_ROOM).toString());
		if (window)
			window->showTabPage();
	}
}

void MultiUserChatPlugin::onEnterRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streamJid = action->data(ADR_STREAM_JID).toStringList();
		QStringList roomJid = action->data(ADR_ROOM).toStringList();
		QStringList nick = action->data(ADR_NICK).toStringList();
		QStringList password = action->data(ADR_PASSWORD).toStringList();
		for (int i=0; i<streamJid.count(); i++)
		{
			IMultiUserChatWindow *window = getMultiChatWindow(streamJid.at(i),roomJid.at(i),nick.at(i),password.at(i));
			if (window && !window->multiUserChat()->isConnected())
				window->multiUserChat()->sendStreamPresence();
		}
	}
}

void MultiUserChatPlugin::onExitRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streamJid = action->data(ADR_STREAM_JID).toStringList();
		QStringList roomJid = action->data(ADR_ROOM).toStringList();
		for (int i=0; i<streamJid.count(); i++)
		{
			IMultiUserChatWindow *window = findMultiChatWindow(streamJid.at(i),roomJid.at(i));
			if (window)
				window->exitAndDestroy(QString::null);
		}
	}
}

void MultiUserChatPlugin::onCopyToClipboardActionTriggered( bool )
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
}

void MultiUserChatPlugin::onRosterIndexDestroyed(IRosterIndex *AIndex)
{
	int index = FChatIndexes.indexOf(AIndex);
	if (index >= 0)
	{
		FChatIndexes.removeAt(index);
		emit multiChatRosterIndexDestroyed(AIndex);
		updateRecentItemProxy(AIndex);
	}
}

void MultiUserChatPlugin::onActiveStreamRemoved(const Jid &AStreamJid)
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (window->streamJid() == AStreamJid)
			window->exitAndDestroy(QString::null,0);

	foreach(QMessageBox * inviteDialog, FInviteDialogs.keys())
		if (FInviteDialogs.value(inviteDialog).streamJid == AStreamJid)
			inviteDialog->done(QMessageBox::Ignore);

	foreach(int messageId, FActiveInvites.keys())
	{
		if (AStreamJid == FActiveInvites.value(messageId).to())
		{
			FActiveInvites.remove(messageId);
			FMessageProcessor->removeMessageNotify(messageId);
		}
	}
}

void MultiUserChatPlugin::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	Q_UNUSED(AWidget);
	if (AId == SCT_APP_MUCJOIN)
	{
		showJoinMultiChatDialog(Jid::null,Jid::null,QString::null,QString::null);
	}
	else if (AId == SCT_APP_MUCSHOWHIDDEN)
	{
		if (!Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		{
			foreach(IMultiUserChatWindow *window, FChatWindows)
				if (!window->isVisibleTabPage())
					window->showTabPage();
		}
	}
	else if (AId == SCT_APP_MUCLEAVEHIDDEN)
	{
		if (!Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		{
			foreach(IMultiUserChatWindow *window, FChatWindows)
				if (!window->isVisibleTabPage())
					window->exitAndDestroy(QString::null);
		}
	}
	else if (FRostersViewPlugin!=NULL && AWidget==FRostersViewPlugin->rostersView()->instance())
	{
		QList<IRosterIndex *> indexes = FRostersViewPlugin->rostersView()->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_SHOWCHATDIALOG && indexes.count()==1)
		{
			IMultiUserChatWindow *window = getMultiChatWindowForIndex(indexes.first());
			if (window)
			{
				if (!window->multiUserChat()->isConnected() && window->multiUserChat()->roomError().isNull())
					window->multiUserChat()->sendStreamPresence();
				window->showTabPage();
			}
		}
		else if (AId==SCT_ROSTERVIEW_ENTERCONFERENCE)
		{
			foreach(IRosterIndex *index, indexes)
			{
				IMultiUserChatWindow *window = getMultiChatWindow(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString(),index->data(RDR_MUC_NICK).toString(),index->data(RDR_MUC_PASSWORD).toString());
				if (window && !window->multiUserChat()->isConnected())
					window->multiUserChat()->sendStreamPresence();
			}
		}
		else if (AId == SCT_ROSTERVIEW_EXITCONFERENCE && isSelectionAccepted(indexes) && indexes.first()->kind()==RIK_MUC_ITEM)
		{
			foreach(IRosterIndex *index, indexes)
			{
				IMultiUserChatWindow *window = findMultiChatWindow(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString());
				if (window)
					window->exitAndDestroy(QString::null);
			}
		}
	}
}

void MultiUserChatPlugin::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MultiUserChatPlugin::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		bool isMultiSelection = AIndexes.count()>1;
		IRosterIndex *index = AIndexes.first();
		if (index->kind()==RIK_STREAM_ROOT)
		{
			int show = index->data(RDR_SHOW).toInt();
			if (show!=IPresence::Offline && show!=IPresence::Error)
			{
				Action *action = createJoinAction(index->data(RDR_STREAM_JID).toString(),Jid::null,AMenu);
				AMenu->addAction(action,AG_RVCM_MULTIUSERCHAT_JOIN,true);
			}
		}
		else if (index->kind() == RIK_GROUP_MUC)
		{
			int show = index->parentIndex()->data(RDR_SHOW).toInt();
			if (show!=IPresence::Offline && show!=IPresence::Error)
			{
				Action *action = createJoinAction(index->data(RDR_STREAM_JID).toString(),Jid::null,AMenu);
				AMenu->addAction(action,AG_RVCM_MULTIUSERCHAT_JOIN,true);
			}
		}
		else if (index->kind() == RIK_MUC_ITEM)
		{
			IMultiUserChatWindow *window =findMultiChatWindow(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString());
			if (window==NULL || isMultiSelection)
			{
				QMap<int, QStringList> rolesMap = FRostersViewPlugin->rostersView()->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_MUC_NICK<<RDR_MUC_PASSWORD);

				QHash<int,QVariant> data;
				data.insert(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				data.insert(ADR_ROOM,rolesMap.value(RDR_PREP_BARE_JID));
				data.insert(ADR_NICK,rolesMap.value(RDR_MUC_NICK));
				data.insert(ADR_PASSWORD,rolesMap.value(RDR_MUC_PASSWORD));

				Action *enter = new Action(AMenu);
				enter->setText(tr("Enter"));
				enter->setData(data);
				enter->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
				enter->setShortcutId(SCT_ROSTERVIEW_ENTERCONFERENCE);
				connect(enter,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
				AMenu->addAction(enter,AG_RVCM_MULTIUSERCHAT_EXIT);

				if (isMultiSelection)
				{
					Action *exit = new Action(AMenu);
					exit->setText(tr("Exit"));
					exit->setData(data);
					exit->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
					exit->setShortcutId(SCT_ROSTERVIEW_EXITCONFERENCE);
					connect(exit,SIGNAL(triggered(bool)),SLOT(onExitRoomActionTriggered(bool)));
					AMenu->addAction(exit,AG_RVCM_MULTIUSERCHAT_EXIT);
				}
			}
			else
			{
				Action *open = new Action(AMenu);
				open->setText(tr("Open Conference Dialog"));
				open->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
				open->setData(ADR_ROOM,index->data(RDR_PREP_BARE_JID));
				open->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_MESSAGE);
				open->setShortcutId(SCT_ROSTERVIEW_SHOWCHATDIALOG);
				connect(open,SIGNAL(triggered(bool)),SLOT(onOpenRoomActionTriggered(bool)));
				AMenu->addAction(open,AG_RVCM_MULTIUSERCHAT_OPEN);

				if (!window->multiUserChat()->isConnected())
				{
					Action *enter = new Action(AMenu);
					enter->setText(tr("Enter"));
					enter->setData(ADR_STREAM_JID,QStringList()<<index->data(RDR_STREAM_JID).toString());
					enter->setData(ADR_ROOM,QStringList()<<index->data(RDR_PREP_BARE_JID).toString());
					enter->setData(ADR_NICK,QStringList()<<index->data(RDR_MUC_NICK).toString());
					enter->setData(ADR_PASSWORD,QStringList()<<index->data(RDR_MUC_PASSWORD).toString());
					enter->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
					enter->setShortcutId(SCT_ROSTERVIEW_ENTERCONFERENCE);
					connect(enter,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
					AMenu->addAction(enter,AG_RVCM_MULTIUSERCHAT_EXIT);
				}

				Action *exit = new Action(AMenu);
				exit->setText(tr("Exit"));
				exit->setData(ADR_STREAM_JID,QStringList()<<index->data(RDR_STREAM_JID).toString());
				exit->setData(ADR_ROOM,QStringList()<<index->data(RDR_PREP_BARE_JID).toString());
				exit->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
				exit->setShortcutId(SCT_ROSTERVIEW_EXITCONFERENCE);
				connect(exit,SIGNAL(triggered(bool)),SLOT(onExitRoomActionTriggered(bool)));
				AMenu->addAction(exit,AG_RVCM_MULTIUSERCHAT_EXIT);

				window->contextMenuForRoom(AMenu);
			}
		}
	}
}

void MultiUserChatPlugin::onRostersViewClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		foreach(IRosterIndex *index, AIndexes)
		{
			IMultiUserChatWindow *window = findMultiChatWindowForIndex(index);
			if (window)
			{
				QString name = window->multiUserChat()->roomName().trimmed();
				if (!name.isEmpty())
				{
					Action *nameAction = new Action(AMenu);
					nameAction->setText(TextManager::getElidedString(name,Qt::ElideRight,50));
					nameAction->setData(ADR_CLIPBOARD_DATA,name);
					nameAction->setShortcutId(SCT_ROSTERVIEW_COPYNAME);
					connect(nameAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
					AMenu->addAction(nameAction, AG_RVCBM_NAME, true);
				}

				QString subject = window->multiUserChat()->subject().trimmed();
				if (!subject.isEmpty())
				{
					Action *subjectAction = new Action(AMenu);
					subjectAction->setText(TextManager::getElidedString(subject,Qt::ElideRight,50));
					subjectAction->setData(ADR_CLIPBOARD_DATA,subject);
					subjectAction->setShortcutId(SCT_ROSTERVIEW_COPYNAME);
					connect(subjectAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
					AMenu->addAction(subjectAction, AG_RVCBM_MUC_SUBJECT, true);
				}
			}
		}
	}
}

void MultiUserChatPlugin::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.node == MUC_NODE_ROOM_NICK)
	{
		if (ADiscoInfo.error.isNull())
		{
			QString nick = ADiscoInfo.identity.value(FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,DIT_TEXT)).name;
			LOG_STRM_INFO(ADiscoInfo.streamJid,QString("Room nick received in discovery from=%1, nick=%2").arg(ADiscoInfo.contactJid.full(),nick));
			emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,nick);
		}
		else if (FDataForms && FRegistration)
		{
			LOG_STRM_WARNING(ADiscoInfo.streamJid,QString("Failed to receive room nick in discovery from=%1: %2").arg(ADiscoInfo.contactJid.full(),ADiscoInfo.error.condition()));
			QString requestId = FRegistration->sendRegiterRequest(ADiscoInfo.streamJid,ADiscoInfo.contactJid.domain());
			if (!requestId.isEmpty())
			{
				LOG_STRM_INFO(ADiscoInfo.streamJid,QString("Room nick request sent as register request, room=%1, id=%2").arg(ADiscoInfo.contactJid.domain(),requestId));
				FNickRequests.insert(requestId, qMakePair<Jid,Jid>(ADiscoInfo.streamJid,ADiscoInfo.contactJid));
			}
			else
			{
				LOG_STRM_WARNING(ADiscoInfo.streamJid,QString("Failed to send room nick request as register request, room=%1").arg(ADiscoInfo.contactJid.domain()));
				emit roomNickReceived(ADiscoInfo.streamJid,ADiscoInfo.contactJid,streamVCardNick(ADiscoInfo.streamJid));
			}
		}
		else
		{
			LOG_STRM_WARNING(ADiscoInfo.streamJid,QString("Failed to receive room nick in discovery from=%1: %2").arg(ADiscoInfo.contactJid.full(),ADiscoInfo.error.condition()));
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
		LOG_STRM_INFO(params.first,QString("Room nick received in register fields from=%1, nick=%2").arg(params.second.full(),nick));
		if (nick.isEmpty())
			nick = streamVCardNick(params.first);
		emit roomNickReceived(params.first,params.second,nick);
	}
}

void MultiUserChatPlugin::onRegisterErrorReceived(const QString &AId, const XmppError &AError)
{
	Q_UNUSED(AError);
	if (FNickRequests.contains(AId))
	{
		QPair<Jid,Jid> params = FNickRequests.take(AId);
		LOG_STRM_WARNING(params.first,QString("Failed to receive room nick in register fields from=%1: %2").arg(params.second.full(),AError.condition()));
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
			LOG_STRM_INFO(fields.streamJid,QString("Invite request from=%1 to room=%2 accepted").arg(fields.fromJid.full(),fields.roomJid.full()));
			showJoinMultiChatDialog(fields.streamJid,fields.roomJid,QString::null,fields.password);
		}
		else if (AResult == QMessageBox::No)
		{
			Message decline;
			decline.setTo(fields.roomJid.bare());

			Stanza &mstanza = decline.stanza();
			QDomElement declElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("decline")).toElement();
			declElem.setAttribute("to",fields.fromJid.full());

			QString reason = tr("I'm too busy right now");
			reason = QInputDialog::getText(inviteDialog,tr("Decline invite"),tr("Enter a reason"),QLineEdit::Normal,reason);
			if (!reason.isEmpty())
				declElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(reason));

			if (FMessageProcessor->sendMessage(fields.streamJid,decline,IMessageProcessor::MessageOut))
				LOG_STRM_INFO(fields.streamJid,QString("Invite request from=%1 to room=%2 rejected").arg(fields.fromJid.full(),fields.roomJid.full()));
			else
				LOG_STRM_WARNING(fields.streamJid,QString("Failed to send invite reject message to=%1").arg(fields.fromJid.full()));
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

		IMultiUserChatWindow *window = findMultiChatWindow(streamJid,roomJid);
		if (window)
		{
			bool ok;
			QString reason = tr("Please, enter this conference!");
			reason = QInputDialog::getText(window->instance(),tr("Invite user"),tr("Enter a reason"),QLineEdit::Normal,reason,&ok);
			if (ok)
				window->multiUserChat()->inviteContact(contactJid,reason);
		}
	}
}

Q_EXPORT_PLUGIN2(plg_multiuserchat, MultiUserChatPlugin)
