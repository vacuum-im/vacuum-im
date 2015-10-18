#include "multiuserchatmanager.h"

#include <QComboBox>
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
#include <definitions/rostertooltiporders.h>
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
#include <definitions/multiuserdataroles.h>
#include <definitions/multiusertooltiporders.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/action.h>
#include <utils/logger.h>

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_ROOM_JID              Action::DR_Parametr2
#define ADR_NICK                  Action::DR_Parametr3
#define ADR_PASSWORD              Action::DR_Parametr4
#define ADR_CLIPBOARD_DATA        Action::DR_Parametr1

#define DIC_CONFERENCE            "conference"

MultiUserChatManager::MultiUserChatManager()
{
	FMessageWidgets = NULL;
	FMessageProcessor = NULL;
	FRostersViewPlugin = NULL;
	FRostersModel = NULL;
	FXmppStreamManager = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FDataForms = NULL;
	FXmppUriQueries = NULL;
	FOptionsManager = NULL;
	FStatusIcons = NULL;
	FRecentContacts = NULL;
}

MultiUserChatManager::~MultiUserChatManager()
{

}

void MultiUserChatManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Multi-User Conferences");
	APluginInfo->description = tr("Allows to use Jabber multi-user conferences");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MultiUserChatManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
		{
			connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveXmppStreamRemoved(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
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
			connect(FRostersModel->instance(),SIGNAL(streamsLayoutChanged(int)),SLOT(onRostersModelStreamsLayoutChanged(int)));
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
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)),
				SLOT(onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)),
				SLOT(onRostersViewIndexToolTips(IRosterIndex *, quint32, QMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FXmppStreamManager!=NULL;
}

bool MultiUserChatManager::initObjects()
{
	Shortcuts::declareShortcut(SCT_APP_MUCJOIN, tr("Join conference"), tr("Ctrl+J","Join conference"), Shortcuts::ApplicationShortcut);
	Shortcuts::insertWidgetShortcut(SCT_APP_MUCJOIN,qApp->desktop());

	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_INVITE,this);
		FMessageProcessor->insertMessageHandler(MHO_MULTIUSERCHAT_GROUPCHAT,this);
	}

	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DFT_MUC_REGISTER);
		FDataForms->insertLocalizer(this,DFT_MUC_ROOMCONFIG);
		FDataForms->insertLocalizer(this,DFT_MUC_ROOM_INFO);
		FDataForms->insertLocalizer(this,DFT_MUC_REQUEST);
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
	}

	if (FRecentContacts)
	{
		FRecentContacts->registerItemHandler(REIT_CONFERENCE,this);
		FRecentContacts->registerItemHandler(REIT_CONFERENCE_PRIVATE,this);
	}

	return true;
}

bool MultiUserChatManager::initSettings()
{
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWENTERS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_ARCHIVESTATUS,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_REJOINAFTERKICK,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_REFERENUMERATION,false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_NICKNAMESUFFIX,", ");
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_USERVIEWMODE,IMultiUserView::ViewSimple);

	if (FOptionsManager)
	{
		IOptionsDialogNode conferencesNode = { ONO_CONFERENCES, OPN_CONFERENCES, MNI_MUC_CONFERENCE, tr("Conferences") };
		FOptionsManager->insertOptionsDialogNode(conferencesNode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}

	return true;
}

QMultiMap<int, IOptionsDialogWidget *> MultiUserChatManager::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_CONFERENCES)
	{
		widgets.insertMulti(OHO_CONFERENCES_MESSAGES,FOptionsManager->newOptionsDialogHeader(tr("Messages"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_SHOWENTERS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_SHOWENTERS),tr("Show users connections and disconnections"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_SHOWSTATUS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_SHOWSTATUS),tr("Show users status changes"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_ARCHIVESTATUS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_ARCHIVESTATUS),tr("Save users status messages in history"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_QUITONWINDOWCLOSE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_QUITONWINDOWCLOSE),tr("Leave the conference when window closed"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_REJOINAFTERKICK,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_REJOINAFTERKICK),tr("Automatically rejoin to conference after kick"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_REFERENUMERATION,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_REFERENUMERATION),tr("Select a user to refer by enumeration in the input field"),AParent));

		widgets.insertMulti(OHO_CONFERENCES_USERVIEW,FOptionsManager->newOptionsDialogHeader(tr("Participants List"),AParent));

		QComboBox *cmbViewMode = new QComboBox(AParent);
		cmbViewMode->addItem(tr("Full"), IMultiUserView::ViewFull);
		cmbViewMode->addItem(tr("Simplified"), IMultiUserView::ViewSimple);
		cmbViewMode->addItem(tr("Compact"), IMultiUserView::ViewCompact);
		widgets.insertMulti(OWO_CONFERENCES_USERVIEWMODE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_GROUPCHAT_USERVIEWMODE),tr("Participants list view:"),cmbViewMode,AParent));
	}
	return widgets;
}


void MultiUserChatManager::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FDiscoNickRequests.contains(AStanza.id()))
	{
		FDiscoNickRequests.removeAll(AStanza.id());
		QDomElement queryElem = AStanza.firstElement("query",NS_DISCO_INFO);
		QDomElement identityElem = AStanza.firstElement("query",NS_DISCO_INFO).firstChildElement("identity");
		if (AStanza.type()=="result" && queryElem.attribute("node")==MUC_NODE_NICK)
		{
			QString nick = queryElem.firstChildElement("identity").attribute("name");
			LOG_STRM_INFO(AStreamJid,QString("Registered nick as discovery request received from=%1, nick=%2, id=%3").arg(AStanza.from(),nick,AStanza.id()));
			emit registeredNickReceived(AStanza.id(),nick);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive registered nick as discovery request from=%1, id=%2: %3").arg(AStanza.from(),AStanza.id(),XmppStanzaError(AStanza).errorMessage()));

			Stanza stanza("iq");
			stanza.setType("get").setId(FStanzaProcessor->newId()).setTo(Jid(AStanza.from()).domain());
			stanza.addElement("query",NS_JABBER_REGISTER);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,10000))
			{
				LOG_STRM_INFO(AStreamJid,QString("Registered nick request sent as register request to=%1, id=%2").arg(stanza.to(),stanza.id()));
				FRegisterNickRequests.insert(stanza.id(),AStanza.id());
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to send registered nick request as register request to=%1").arg(stanza.to()));
				emit registeredNickReceived(AStanza.id(),QString::null);
			}
		}
	}
	else if (FRegisterNickRequests.contains(AStanza.id()))
	{
		QString reqId = FRegisterNickRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			QDomElement queryElem = AStanza.firstElement("query",NS_JABBER_REGISTER);
			QDomElement formElem = Stanza::findElement(queryElem,"x",NS_JABBER_DATA);
			QString nick = queryElem.firstChildElement("username").text();
			if (FDataForms && !formElem.isNull() && nick.isEmpty())
			{
				IDataForm form = FDataForms!=NULL ? FDataForms->dataForm(formElem) : IDataForm();
				nick = FDataForms!=NULL ? FDataForms->fieldValue("nick",form.fields).toString() : nick;
			}
			LOG_STRM_INFO(AStreamJid,QString("Registered nick as register request received from=%1, nick=%2, id=%3").arg(AStanza.from(),nick,AStanza.id()));
			emit registeredNickReceived(reqId,nick);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive registered nick as register request from=%1, id=%2: %3").arg(AStanza.from(),AStanza.id(),XmppStanzaError(AStanza).errorMessage()));
			emit registeredNickReceived(reqId,QString::null);
		}
	}
}

bool MultiUserChatManager::rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);
	if (AEvent->modifiers()==Qt::NoModifier && Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
	{
		IMultiUserChatWindow *window = findMultiChatWindowForIndex(AIndex);
		if (window)
		{
			if (AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE)==REIT_CONFERENCE_PRIVATE)
				window->openPrivateChatWindow(AIndex->data(RDR_RECENT_REFERENCE).toString());
			else
				window->showTabPage();
			return true;
		}
	}
	return false;
}

bool MultiUserChatManager::rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent)
{
	Q_UNUSED(AOrder);
	if (AEvent->modifiers() == Qt::NoModifier)
	{
		IMultiUserChatWindow *window = getMultiChatWindowForIndex(AIndex);
		if (window)
		{
			if (AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE)==REIT_CONFERENCE_PRIVATE)
			{
				window->openPrivateChatWindow(AIndex->data(RDR_RECENT_REFERENCE).toString());
			}
			else
			{
				if (window->multiUserChat()->state() == IMultiUserChat::Closed)
					window->multiUserChat()->sendStreamPresence();
				window->showTabPage();
			}
			return true;
		}
	}
	return false;
}

bool MultiUserChatManager::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == "join")
	{
		showJoinMultiChatWizard(AStreamJid,AContactJid,QString::null,AParams.value("password"));
		return true;
	}
	else if (AAction == "invite")
	{
		IMultiUserChat *chat = findMultiUserChat(AStreamJid, AContactJid);
		if (chat != NULL)
		{
			foreach(const QString &userJid, AParams.values("jid"))
				chat->sendInvitation(userJid, QString::null);
		}
		return true;
	}
	return false;
}

bool MultiUserChatManager::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature==NS_MUC && ADiscoInfo.contactJid.resource().isEmpty())
	{
		IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,ADiscoInfo.contactJid);
		if (!window)
			showJoinMultiChatWizard(AStreamJid,ADiscoInfo.contactJid,QString::null,QString::null);
		else
			window->showTabPage();
		return true;
	}
	return false;
}

Action *MultiUserChatManager::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature == NS_MUC)
	{
		if (FDiscovery && FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,QString::null)>=0)
		{
			if (findMultiChatWindow(AStreamJid,ADiscoInfo.contactJid) == NULL)
				return createJoinAction(AStreamJid,ADiscoInfo.contactJid,AParent);
		}
		else if (FDiscovery)
		{
			Menu *inviteMenu = createInviteMenu(ADiscoInfo.contactJid,AParent);
			if (!inviteMenu->isEmpty())
				return inviteMenu->menuAction();
			else
				delete inviteMenu;
		}
	}
	return NULL;
}

IDataFormLocale MultiUserChatManager::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DFT_MUC_REGISTER)
	{
		locale.title = tr("Register in conference");
		locale.fields["muc#register_allow"].label = tr("Allow this person to register with the room?");
		locale.fields["muc#register_email"].label = tr("EMail Address");
		locale.fields["muc#register_faqentry"].label = tr("Rules and Notes");
		locale.fields["muc#register_first"].label = tr("Given Name");
		locale.fields["muc#register_last"].label = tr("Family Name");
		locale.fields["muc#register_roomnick"].label = tr("Desired Nickname");
		locale.fields["muc#register_url"].label = tr("Your URL");
	}
	else if (AFormType == DFT_MUC_REQUEST)
	{
		locale.title = tr("Request for voice");
		locale.fields["muc#role"].label = tr("Requested Role");
		locale.fields["muc#jid"].label = tr("User ID");
		locale.fields["muc#roomnick"].label = tr("Room Nickname");
		locale.fields["muc#request_allow"].label = tr("Grant Voice?");
	}
	else if (AFormType == DFT_MUC_ROOMCONFIG)
	{
		locale.title = tr("Configure conference");
		locale.fields["muc#maxhistoryfetch"].label = tr("Maximum Number of History Messages Returned by Room");
		locale.fields["muc#roomconfig_allowpm"].label = tr("Allow Occupants to Send Private Messages?");
		locale.fields["muc#roomconfig_allowinvites"].label = tr("Allow Occupants to Invite Others?");
		locale.fields["muc#roomconfig_changesubject"].label = tr("Allow Occupants to Change Subject?");
		locale.fields["muc#roomconfig_enablelogging"].label = tr("Enable Logging of Room Conversations?");
		locale.fields["muc#roomconfig_getmemberlist"].label = tr("Affiliations that May Retrieve Member List");
		locale.fields["muc#roomconfig_pubsub"].label = tr("XMPP URI of Associated Publish-Subscribe Node");
		locale.fields["muc#roomconfig_lang"].label = tr("Natural Language for Room Discussions");
		locale.fields["muc#roomconfig_maxusers"].label = tr("Maximum Number of Room Occupants");
		locale.fields["muc#roomconfig_membersonly"].label = tr("Make Room Members-Only?");
		locale.fields["muc#roomconfig_moderatedroom"].label = tr("Make Room Moderated?");
		locale.fields["muc#roomconfig_passwordprotectedroom"].label = tr("Password is Required to Enter?");
		locale.fields["muc#roomconfig_persistentroom"].label = tr("Make Room Persistent?");
		locale.fields["muc#roomconfig_presencebroadcast"].label = tr("Roles for which Presence is Broadcasted");
		locale.fields["muc#roomconfig_publicroom"].label = tr("Allow Public Searching for Room?");
		locale.fields["muc#roomconfig_roomadmins"].label = tr("Full List of Room Administrators");
		locale.fields["muc#roomconfig_roomdesc"].label = tr("Description of Room");
		locale.fields["muc#roomconfig_roomname"].label = tr("Natural-Language Room Name");
		locale.fields["muc#roomconfig_roomowners"].label = tr("Full List of Room Owners");
		locale.fields["muc#roomconfig_roomsecret"].label = tr("The Room Password");
		locale.fields["muc#roomconfig_whois"].label = tr("Affiliations that May Discover Real JIDs of Occupants");
		locale.fields["muc#roomconfig_whois"].options["anyone"].label = tr("Anyone");
		locale.fields["muc#roomconfig_whois"].options["moderators"].label = tr("Moderators only");
		//EJabberd extension
		locale.fields["public_list"].label = tr("Make Participants List Public?");
		locale.fields["members_by_default"].label = tr("Make all Occupants as Participants?");
		locale.fields["allow_private_messages"].label = tr("Allow Occupants to Send Private Messages?");
		locale.fields["allow_private_messages_from_visitors"].label = tr("Allow visitors to send private messages to");
		locale.fields["allow_private_messages_from_visitors"].options["anyone"].label = tr("Anyone");
		locale.fields["allow_private_messages_from_visitors"].options["nobody"].label = tr("Nobody");
		locale.fields["allow_private_messages_from_visitors"].options["moderators"].label = tr("Moderators only");
		locale.fields["allow_query_users"].label = tr("Allow Occupants to Query Other Occupants?");
		locale.fields["muc#roomconfig_allowvisitorstatus"].label = tr("Allow Visitors to Send Status Text in Presence Updates?");
		locale.fields["muc#roomconfig_allowvisitornickchange"].label = tr("Allow visitors to change nickname?");
		locale.fields["muc#roomconfig_allowvoicerequests"].label = tr("Allow visitors to send voice requests?");
		locale.fields["muc#roomconfig_voicerequestmininterval"].label = tr("Minimum interval between voice requests (in seconds)");
		locale.fields["captcha_protected"].label = tr("Make this Room CAPTCHA Protected?");
		locale.fields["muc#roomconfig_captcha_whitelist"].label = tr("Do not Request CAPTCHA for Followed Jabber ID");
		//OpenFire extension
		locale.fields["x-muc#roomconfig_reservednick"].label = tr("Allow Login Only With Registered Nickname?");
		locale.fields["x-muc#roomconfig_canchangenick"].label = tr("Allow Occupants to Change Nicknames?");
		locale.fields["x-muc#roomconfig_registration"].label = tr("Allow Users to Register with the Room?");
	}
	else if (AFormType == DFT_MUC_ROOM_INFO)
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
	return locale;
}

bool MultiUserChatManager::messageCheck(int AOrder, const Message &AMessage, int ADirection)
{
	Q_UNUSED(ADirection);
	if (AOrder == MHO_MULTIUSERCHAT_INVITE)
		return !AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite").isNull();
	return false;
}

bool MultiUserChatManager::messageDisplay(const Message &AMessage, int ADirection)
{
	Q_UNUSED(AMessage);
	return ADirection == IMessageProcessor::DirectionIn;
}

INotification MultiUserChatManager::messageNotify(INotifications *ANotifications, const Message &AMessage, int ADirection)
{
	INotification notify;
	if (ADirection == IMessageProcessor::DirectionIn)
	{
		QDomElement inviteElem = AMessage.stanza().firstElement("x",NS_MUC_USER).firstChildElement("invite");

		Jid roomJid = AMessage.from();
		Jid streamJid = AMessage.to();
		Jid contactJid = inviteElem.attribute("from");
		LOG_STRM_DEBUG(streamJid,QString("Received invite to room=%1, from=%2").arg(roomJid.full(),contactJid.full()));

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

bool MultiUserChatManager::messageShowWindow(int AMessageId)
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

bool MultiUserChatManager::messageShowWindow(int AOrder, const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType, int AShowMode)
{
	if (AOrder==MHO_MULTIUSERCHAT_GROUPCHAT && AType==Message::GroupChat)
	{
		if (isReady(AStreamJid))
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

bool MultiUserChatManager::recentItemValid(const IRecentItem &AItem) const
{
	return !AItem.reference.isEmpty();
}

bool MultiUserChatManager::recentItemCanShow(const IRecentItem &AItem) const
{
	if (AItem.type == REIT_CONFERENCE)
	{
		return true;
	}
	else if (AItem.type == REIT_CONFERENCE_PRIVATE)
	{
		Jid userJid = AItem.reference;
		IMultiUserChatWindow *chatWindow = findMultiChatWindow(AItem.streamJid,userJid);
		IMultiUser *user = chatWindow!=NULL ? chatWindow->multiUserChat()->findUser(userJid.resource()) : NULL;
		IMessageChatWindow *privateWindow = chatWindow!=NULL ? chatWindow->findPrivateChatWindow(userJid) : NULL;
		return privateWindow!=NULL || (user!=NULL && user->presence().show!=IPresence::Offline);
	}
	return false;
}

QIcon MultiUserChatManager::recentItemIcon(const IRecentItem &AItem) const
{
	if (FStatusIcons)
	{
		if (AItem.type == REIT_CONFERENCE)
		{
			return FStatusIcons->iconByJidStatus(AItem.reference,IPresence::Offline,SUBSCRIPTION_BOTH,false);
		}
		else if (AItem.type == REIT_CONFERENCE_PRIVATE)
		{
			IMultiUser *user = findMultiChatWindowUser(AItem.streamJid,AItem.reference);
			int show = user!=NULL ? user->presence().show : IPresence::Offline;
			return FStatusIcons->iconByJidStatus(AItem.reference,show,SUBSCRIPTION_BOTH,false);
		}
	}
	return QIcon();
}

QString MultiUserChatManager::recentItemName(const IRecentItem &AItem) const
{
	if (AItem.type == REIT_CONFERENCE)
	{
		QString name = FRecentContacts->itemProperty(AItem,REIP_NAME).toString();
		return !name.isEmpty() ? name : Jid(AItem.reference).uNode();
	}
	else if (AItem.type == REIT_CONFERENCE_PRIVATE)
	{
		Jid userJid = AItem.reference;
		return QString("[%1]").arg(userJid.resource());
	}
	return QString::null;
}

IRecentItem MultiUserChatManager::recentItemForIndex(const IRosterIndex *AIndex) const
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

QList<IRosterIndex *> MultiUserChatManager::recentItemProxyIndexes(const IRecentItem &AItem) const
{
	QList<IRosterIndex *> proxies;
	if (AItem.type == REIT_CONFERENCE)
	{
		IRosterIndex *chatIndex = findMultiChatRosterIndex(AItem.streamJid,AItem.reference);
		if (chatIndex)
			proxies.append(chatIndex);
	}
	return proxies;
}

bool MultiUserChatManager::isReady(const Jid &AStreamJid) const
{
	IXmppStream *stream = FXmppStreamManager!=NULL ? FXmppStreamManager->findXmppStream(AStreamJid) : NULL;
	return stream!=NULL && stream->isOpen();
}

QList<IMultiUserChat *> MultiUserChatManager::multiUserChats() const
{
	return FChats;
}

IMultiUserChat *MultiUserChatManager::findMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChat *chat, FChats)
		if (chat->streamJid()==AStreamJid && chat->roomJid()==ARoomJid.pBare())
			return chat;
	return NULL;
}

IMultiUserChat *MultiUserChatManager::getMultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, bool AIsolated)
{
	IMultiUserChat *chat = findMultiUserChat(AStreamJid,ARoomJid);
	if (!chat)
	{
		if (AStreamJid.isValid() && ARoomJid.isValid())
		{
			LOG_STRM_INFO(AStreamJid,QString("Creating multi user chat, room=%1, nick=%2").arg(ARoomJid.bare(),ANick));
			chat = new MultiUserChat(AStreamJid, ARoomJid.bare(), ANick.isEmpty() ? AStreamJid.uNode() : ANick, APassword, AIsolated, this);
			connect(chat->instance(),SIGNAL(chatDestroyed()),SLOT(onMultiChatDestroyed()));
			FChats.append(chat);
			emit multiUserChatCreated(chat);
		}
		else
		{
			REPORT_ERROR("Failed to get multi user chat: Invalid parameters");
		}
	}
	return chat;
}

QList<IMultiUserChatWindow *> MultiUserChatManager::multiChatWindows() const
{
	return FChatWindows;
}

IMultiUserChatWindow *MultiUserChatManager::findMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (window->streamJid()==AStreamJid && window->contactJid()==ARoomJid.bare())
			return window;
	return NULL;
}

IMultiUserChatWindow *MultiUserChatManager::getMultiChatWindow(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IMultiUserChatWindow *window = NULL;
	if (FMessageProcessor && FMessageProcessor->isActiveStream(AStreamJid))
	{
		window = findMultiChatWindow(AStreamJid,ARoomJid);
		if (!window)
		{
			IMultiUserChat *chat = getMultiUserChat(AStreamJid,ARoomJid,ANick,APassword,false);
			if (chat)
			{
				LOG_STRM_INFO(AStreamJid,QString("Creating multi user chat window, room=%1, nick=%2").arg(ARoomJid.bare(),ANick));
				
				window = new MultiUserChatWindow(this,chat);
				WidgetManager::setWindowSticky(window->instance(),true);
				
				connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onMultiChatWindowDestroyed()));
				connect(window->instance(),SIGNAL(multiChatContextMenu(Menu *)),SLOT(onMultiChatWindowContextMenu(Menu *)));
				connect(window->instance(),SIGNAL(multiUserContextMenu(IMultiUser *, Menu *)),SLOT(onMultiChatWindowUserContextMenu(IMultiUser *, Menu *)));
				connect(window->instance(),SIGNAL(multiUserToolTips(IMultiUser *, QMap<int,QString> &)),SLOT(onMultiChatWindowUserToolTips(IMultiUser *, QMap<int,QString> &)));
				connect(window->instance(),SIGNAL(privateChatWindowCreated(IMessageChatWindow *)),SLOT(onMultiChatWindowPrivateWindowChanged(IMessageChatWindow *)));
				connect(window->instance(),SIGNAL(privateChatWindowDestroyed(IMessageChatWindow *)),SLOT(onMultiChatWindowPrivateWindowChanged(IMessageChatWindow *)));

				connect(window->multiUserChat()->instance(),SIGNAL(roomTitleChanged(const QString &)),SLOT(onMultiChatPropertiesChanged()));
				connect(window->multiUserChat()->instance(),SIGNAL(nicknameChanged(const QString &, const XmppError &)),SLOT(onMultiChatPropertiesChanged()));
				connect(window->multiUserChat()->instance(),SIGNAL(passwordChanged(const QString &)),SLOT(onMultiChatPropertiesChanged()));
				connect(window->multiUserChat()->instance(),SIGNAL(presenceChanged(const IPresenceItem &)),SLOT(onMultiChatPropertiesChanged()));
				connect(window->multiUserChat()->instance(),SIGNAL(userChanged(IMultiUser *, int, const QVariant &)),SLOT(onMultiChatUserChanged(IMultiUser *, int, const QVariant &)));

				connect(window->infoWidget()->instance(),SIGNAL(contextMenuRequested(Menu *)),SLOT(onMultiChatWindowInfoContextMenu(Menu *)));
				connect(window->infoWidget()->instance(),SIGNAL(toolTipsRequested(QMap<int,QString> &)),SLOT(onMultiChatWindowInfoToolTips(QMap<int,QString> &)));
				
				FChatWindows.append(window);
				getMultiChatRosterIndex(window->streamJid(),window->contactJid(),window->multiUserChat()->nickname(),window->multiUserChat()->password());

				emit multiChatWindowCreated(window);
			}
		}
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

QList<IRosterIndex *> MultiUserChatManager::multiChatRosterIndexes() const
{
	return FChatIndexes;
}

IRosterIndex *MultiUserChatManager::findMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	foreach(IRosterIndex *index, FChatIndexes)
	{
		if (AStreamJid==index->data(RDR_STREAM_JID).toString() && ARoomJid.pBare()==index->data(RDR_PREP_BARE_JID).toString())
			return index;
	}
	return NULL;
}

IRosterIndex *MultiUserChatManager::getMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword)
{
	IRosterIndex *chatIndex = findMultiChatRosterIndex(AStreamJid,ARoomJid);
	if (chatIndex == NULL)
	{
		IRosterIndex *chatGroup = getConferencesGroupIndex(AStreamJid);
		if (chatGroup)
		{
			chatIndex = FRostersModel->newRosterIndex(RIK_MUC_ITEM);
			chatIndex->setData(AStreamJid.pFull(),RDR_STREAM_JID);
			chatIndex->setData(ARoomJid.bare(),RDR_FULL_JID);
			chatIndex->setData(ARoomJid.pBare(),RDR_PREP_FULL_JID);
			chatIndex->setData(ARoomJid.pBare(),RDR_PREP_BARE_JID);
			chatIndex->setData(ANick,RDR_MUC_NICK);
			chatIndex->setData(APassword,RDR_MUC_PASSWORD);

			FChatIndexes.append(chatIndex);
			updateMultiChatRosterIndex(AStreamJid,ARoomJid);

			FRostersModel->insertRosterIndex(chatIndex,chatGroup);
			updateMultiChatRecentItem(chatIndex);

			emit multiChatRosterIndexCreated(chatIndex);
		}
		else
		{
			REPORT_ERROR("Failed to get multi user chat roster index: Conferences group index not created");
		}
	}
	return chatIndex;
}

QDialog *MultiUserChatManager::showMultiChatWizard(QWidget *AParent)
{
	CreateMultiChatWizard *wizard = new CreateMultiChatWizard(AParent);
	wizard->show();
	return wizard;
}

QDialog *MultiUserChatManager::showJoinMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent)
{
	CreateMultiChatWizard *wizard = new CreateMultiChatWizard(CreateMultiChatWizard::ModeJoin,AStreamJid,ARoomJid,ANick,APassword,AParent);
	wizard->show();
	return wizard;
}

QDialog *MultiUserChatManager::showCreateMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent)
{
	CreateMultiChatWizard *wizard = new CreateMultiChatWizard(CreateMultiChatWizard::ModeCreate,AStreamJid,ARoomJid,ANick,APassword,AParent);
	wizard->show();
	return wizard;
}


QDialog *MultiUserChatManager::showManualMultiChatWizard(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick, const QString &APassword, QWidget *AParent)
{
	CreateMultiChatWizard *wizard = new CreateMultiChatWizard(CreateMultiChatWizard::ModeManual,AStreamJid,ARoomJid,ANick,APassword,AParent);
	wizard->show();
	return wizard;
}

QString MultiUserChatManager::requestRegisteredNick(const Jid &AStreamJid, const Jid &ARoomJid)
{
	if (FStanzaProcessor && AStreamJid.isValid() && ARoomJid.isValid())
	{
		Stanza stanza("iq");
		stanza.setType("get").setId(FStanzaProcessor->newId()).setTo(ARoomJid.bare());
		stanza.addElement("query",NS_DISCO_INFO).setAttribute("node",MUC_NODE_NICK);
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,stanza,10000))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Registered nick request sent as discovery request to=%1, id=%2").arg(ARoomJid.bare(),stanza.id()));
			FDiscoNickRequests.append(stanza.id());
			return stanza.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send registered nick request as discovery request to=%1").arg(ARoomJid.bare()));
		}
	}
	return QString::null;
}

void MultiUserChatManager::registerDiscoFeatures()
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

	dfeature.var = MUC_FEATURE_PUBLIC;
	dfeature.name = tr("Public room");
	dfeature.description = tr("A room that can be found by any user through normal means such as searching and service discovery");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_HIDDEN;
	dfeature.name = tr("Hidden room");
	dfeature.description = tr("A room that cannot be found by any user through normal means such as searching and service discovery");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_OPEN;
	dfeature.name = tr("Open room");
	dfeature.description = tr("A room that anyone may enter without being on the member list");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_MEMBERSONLY;
	dfeature.name = tr("Members-only room");
	dfeature.description = tr("A room that a user cannot enter without being on the member list");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_UNMODERATED;
	dfeature.name = tr("Unmoderated room");
	dfeature.description = tr("A room in which any occupant is allowed to send messages to all occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_MODERATED;
	dfeature.name = tr("Moderated room");
	dfeature.description = tr("A room in which only those with 'voice' may send messages to all occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_NONANONYMOUS;
	dfeature.name = tr("Non-anonymous room");
	dfeature.description = tr("A room in which an occupant's full JID is exposed to all other occupants");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_SEMIANONYMOUS;
	dfeature.name = tr("Semi-anonymous room");
	dfeature.description = tr("A room in which an occupant's full JID can be discovered by room admins only");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_UNSECURED;
	dfeature.name = tr("Unsecured room");
	dfeature.description = tr("A room that anyone is allowed to enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_PASSWORD;
	dfeature.name = tr("Password-protected room");
	dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_PASSWORDPROTECTED;
	dfeature.name = tr("Password-protected room");
	dfeature.description = tr("A room that a user cannot enter without first providing the correct password");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_TEMPORARY;
	dfeature.name = tr("Temporary room");
	dfeature.description = tr("A room that is destroyed if the last occupant exits");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_PERSISTENT;
	dfeature.name = tr("Persistent room");
	dfeature.description = tr("A room that is not destroyed if the last occupant exits");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = MUC_FEATURE_ROOMS;
	dfeature.name = tr("List of rooms");
	dfeature.description = tr("Contains the list of multi-user chat rooms");
	FDiscovery->insertDiscoFeature(dfeature);
}

bool MultiUserChatManager::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	if (ASelected.count() > 1)
	{
		foreach(IRosterIndex *index, ASelected)
			if (index->kind() != RIK_MUC_ITEM)
				return false;
	}
	return !ASelected.isEmpty();
}

IMultiUser *MultiUserChatManager::findMultiChatWindowUser(const Jid &AStreamJid, const Jid &AContactJid) const
{
	IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,AContactJid);
	return window!=NULL ? window->multiUserChat()->findUser(AContactJid.resource()) : NULL;
}

void MultiUserChatManager::updateMultiChatRosterIndex(const Jid &AStreamJid, const Jid &ARoomJid)
{
	IRosterIndex *chatIndex = findMultiChatRosterIndex(AStreamJid,ARoomJid);
	if (chatIndex)
	{
		IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,ARoomJid);
		if (window)
		{
			int show = window->multiUserChat()->roomPresence().show;
			chatIndex->setData(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(ARoomJid,show,SUBSCRIPTION_BOTH,false) : QIcon(), Qt::DecorationRole);
			chatIndex->setData(window->multiUserChat()->roomTitle(),RDR_NAME);
			chatIndex->setData(window->multiUserChat()->roomPresence().status,RDR_STATUS);
			chatIndex->setData(window->multiUserChat()->roomPresence().show,RDR_SHOW);
			chatIndex->setData(window->multiUserChat()->nickname(),RDR_MUC_NICK);
			chatIndex->setData(window->multiUserChat()->password(),RDR_MUC_PASSWORD);
		}
		else
		{
			QString name = multiChatRecentName(AStreamJid,ARoomJid);
			chatIndex->setData(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(ARoomJid,IPresence::Offline,SUBSCRIPTION_BOTH,false) : QIcon(), Qt::DecorationRole);
			chatIndex->setData(name.isEmpty() ? ARoomJid.uBare() : name,RDR_NAME);
			chatIndex->setData(QString(),RDR_STATUS);
			chatIndex->setData(IPresence::Offline,RDR_SHOW);
		}
	}
}

IRosterIndex *MultiUserChatManager::getConferencesGroupIndex(const Jid &AStreamJid)
{
	IRosterIndex *sroot = FRostersModel!=NULL ? FRostersModel->streamRoot(AStreamJid) : NULL;
	if (sroot)
	{
		IRosterIndex *chatGroup = FRostersModel->getGroupIndex(RIK_GROUP_MUC,tr("Conferences"),sroot);
		chatGroup->setData(RIKO_GROUP_MUC,RDR_KIND_ORDER);
		return chatGroup;
	}
	return NULL;
}

IMultiUserChatWindow *MultiUserChatManager::findMultiChatWindowForIndex(const IRosterIndex *AIndex) const
{
	IMultiUserChatWindow *window = NULL;
	if (AIndex->kind() == RIK_MUC_ITEM)
		window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_PREP_BARE_JID).toString());
	else if (AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE)
		window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_RECENT_REFERENCE).toString());
	else if (AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE_PRIVATE)
		window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_RECENT_REFERENCE).toString());
	return window;
}

IMultiUserChatWindow *MultiUserChatManager::getMultiChatWindowForIndex(const IRosterIndex *AIndex)
{
	IMultiUserChatWindow *window = NULL;
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	if (isReady(streamJid))
	{
		if (AIndex->kind() == RIK_MUC_ITEM)
		{
			Jid roomJid = AIndex->data(RDR_PREP_BARE_JID).toString();
			QString nick = AIndex->data(RDR_MUC_NICK).toString();
			QString password = AIndex->data(RDR_MUC_PASSWORD).toString();
			window = getMultiChatWindow(streamJid,roomJid,nick,password);
		}
		else if (FRecentContacts && AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE)
		{
			IRecentItem item = FRecentContacts->rosterIndexItem(AIndex);
			QString nick = FRecentContacts->itemProperty(item,REIP_CONFERENCE_NICK).toString();
			QString password = FRecentContacts->itemProperty(item,REIP_CONFERENCE_PASSWORD).toString();
			window = getMultiChatWindow(streamJid,item.reference,nick,password);
		}
	}
	return window;
}

void MultiUserChatManager::updateMultiChatRecentItem(IRosterIndex *AChatIndex)
{
	if (AChatIndex)
		emit recentItemUpdated(recentItemForIndex(AChatIndex));
}

void MultiUserChatManager::updateMultiChatRecentItemProperties(IMultiUserChat *AChat)
{
	if (FRecentContacts && FRecentContacts->isReady(AChat->streamJid()))
	{
		IRecentItem item = multiChatRecentItem(AChat);
		FRecentContacts->setItemProperty(item,REIP_NAME,AChat->roomTitle());
		FRecentContacts->setItemProperty(item,REIP_CONFERENCE_NICK,AChat->nickname());
		FRecentContacts->setItemProperty(item,REIP_CONFERENCE_PASSWORD,AChat->password());
	}
}

void MultiUserChatManager::updateMultiUserRecentItems(IMultiUserChat *AChat, const QString &ANick)
{
	if (FRecentContacts!=NULL && AChat!=NULL)
	{
		if (!ANick.isEmpty())
		{
			emit recentItemUpdated(multiChatRecentItem(AChat,ANick));
		}
		else foreach(const IRecentItem &item, FRecentContacts->streamItems(AChat->streamJid()))
		{
			if (item.type == REIT_CONFERENCE_PRIVATE)
			{
				Jid userJid = item.reference;
				if (AChat->roomJid() == userJid.pBare())
					emit recentItemUpdated(item);
			}
		}
	}
}

QString MultiUserChatManager::multiChatRecentName(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	IRecentItem item;
	item.type = REIT_CONFERENCE;
	item.streamJid = AStreamJid;
	item.reference = ARoomJid.pBare();
	return FRecentContacts!=NULL ? FRecentContacts->itemProperty(item,REIP_NAME).toString() : QString::null;
}

IRecentItem MultiUserChatManager::multiChatRecentItem(IMultiUserChat *AChat, const QString &ANick) const
{
	IRecentItem item;
	item.streamJid = AChat->streamJid();

	if (ANick.isEmpty())
	{
		item.type = REIT_CONFERENCE;
		item.reference = AChat->roomJid().pBare();
	}
	else
	{
		Jid userJid = AChat->roomJid();
		userJid.setResource(ANick);
		item.type = REIT_CONFERENCE_PRIVATE;
		item.reference = userJid.pFull();
	}

	return item;
}

Action *MultiUserChatManager::createWizardAction(QWidget *AParent) const
{
	Action *action = new Action(AParent);
	action->setText(tr("Join Conference..."));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_JOIN);
	connect(action,SIGNAL(triggered(bool)),SLOT(onWizardRoomActionTriggered(bool)));
	return action;
}

Menu *MultiUserChatManager::createInviteMenu(const Jid &AContactJid, QWidget *AParent) const
{
	Menu *inviteMenu = new Menu(AParent);
	inviteMenu->setTitle(tr("Invite to"));
	inviteMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);

	QSet<Jid> invited;
	foreach(IMultiUserChatWindow *window, FChatWindows)
	{
		IMultiUserChat *mchat = window->multiUserChat();
		if (!invited.contains(mchat->roomJid()) && mchat->isOpen() && mchat->mainUser()->role()!=MUC_ROLE_VISITOR && !mchat->isUserPresent(AContactJid))
		{
			Action *action = new Action(inviteMenu);
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFERENCE);
			action->setText(TextManager::getElidedString(mchat->roomTitle(),Qt::ElideRight,50));
			action->setData(ADR_STREAM_JID,window->streamJid().full());
			action->setData(ADR_CONTACT_JID,AContactJid.full());
			action->setData(ADR_ROOM_JID,window->multiUserChat()->roomJid().bare());
			connect(action,SIGNAL(triggered(bool)),SLOT(onInviteActionTriggered(bool)));
			inviteMenu->addAction(action,AG_DEFAULT,true);
			invited += mchat->roomJid();
		}
	}

	return inviteMenu;
}

Action *MultiUserChatManager::createJoinAction(const Jid &AStreamJid, const Jid &ARoomJid, QWidget *AParent) const
{
	Action *action = new Action(AParent);
	action->setText(tr("Join Conference"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_JOIN);
	action->setData(ADR_STREAM_JID,AStreamJid.full());
	action->setData(ADR_ROOM_JID,ARoomJid.bare());
	connect(action,SIGNAL(triggered(bool)),SLOT(onJoinRoomActionTriggered(bool)));
	return action;
}

void MultiUserChatManager::onWizardRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		showMultiChatWizard();
}

void MultiUserChatManager::onJoinRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(Action::DR_StreamJid).toString();
		Jid roomJid = action->data(ADR_ROOM_JID).toString();
		QString nick = action->data(ADR_NICK).toString();
		QString password = action->data(ADR_PASSWORD).toString();
		showJoinMultiChatWizard(streamJid,roomJid,nick,password);
	}
}

void MultiUserChatManager::onOpenRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUserChatWindow *window = findMultiChatWindow(action->data(ADR_STREAM_JID).toString(),action->data(ADR_ROOM_JID).toString());
		if (window)
			window->showTabPage();
	}
}

void MultiUserChatManager::onEnterRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streamJid = action->data(ADR_STREAM_JID).toStringList();
		QStringList roomJid = action->data(ADR_ROOM_JID).toStringList();
		QStringList nick = action->data(ADR_NICK).toStringList();
		QStringList password = action->data(ADR_PASSWORD).toStringList();

		for (int i=0; i<streamJid.count(); i++)
		{
			IMultiUserChatWindow *window = getMultiChatWindow(streamJid.at(i),roomJid.at(i),nick.at(i),password.at(i));
			if (window!=NULL && window->multiUserChat()->state()==IMultiUserChat::Closed)
				window->multiUserChat()->sendStreamPresence();
		}
	}
}

void MultiUserChatManager::onExitRoomActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streamJid = action->data(ADR_STREAM_JID).toStringList();
		QStringList roomJid = action->data(ADR_ROOM_JID).toStringList();
		for (int i=0; i<streamJid.count(); i++)
		{
			IMultiUserChatWindow *window = findMultiChatWindow(streamJid.at(i),roomJid.at(i));
			if (window)
				window->exitAndDestroy(QString::null);
		}
	}
}

void MultiUserChatManager::onCopyToClipboardActionTriggered( bool )
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		QApplication::clipboard()->setText(action->data(ADR_CLIPBOARD_DATA).toString());
}

void MultiUserChatManager::onStatusIconsChanged()
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
	{
		updateMultiChatRosterIndex(window->streamJid(),window->contactJid());
		updateMultiUserRecentItems(window->multiUserChat());
	}
}

void MultiUserChatManager::onActiveXmppStreamRemoved(const Jid &AStreamJid)
{
	foreach(IMultiUserChatWindow *window, FChatWindows)
		if (window->streamJid() == AStreamJid)
			delete window->instance();

	foreach(QMessageBox *inviteDialog, FInviteDialogs.keys())
		if (FInviteDialogs.value(inviteDialog).streamJid == AStreamJid)
			inviteDialog->reject();

	foreach(int messageId, FActiveInvites.keys())
	{
		if (AStreamJid == FActiveInvites.value(messageId).to())
		{
			FActiveInvites.remove(messageId);
			FMessageProcessor->removeMessageNotify(messageId);
		}
	}
}

void MultiUserChatManager::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FXmppStreamManager!=NULL && AId==SCT_APP_MUCJOIN)
	{
		foreach(IXmppStream *xmppStream, FXmppStreamManager->xmppStreams())
		{
			if (isReady(xmppStream->streamJid()))
			{
				showJoinMultiChatWizard(xmppStream->streamJid(),Jid::null,QString::null,QString::null);
				break;
			}
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
				if (window->multiUserChat()->state()==IMultiUserChat::Closed && window->multiUserChat()->roomError().isNull())
					window->multiUserChat()->sendStreamPresence();
				window->showTabPage();
			}
		}
	}
}

void MultiUserChatManager::onMultiChatDestroyed()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (chat)
	{
		LOG_STRM_INFO(chat->streamJid(),QString("Multi user chat destroyed, room=%1").arg(chat->roomJid().bare()));
		FChats.removeAll(chat);
		emit multiUserChatDestroyed(chat);
	}
}

void MultiUserChatManager::onMultiChatPropertiesChanged()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (chat)
	{
		updateMultiChatRosterIndex(chat->streamJid(),chat->roomJid());
		updateMultiChatRecentItemProperties(chat);
	}
}

void MultiUserChatManager::onMultiChatUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore)
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (chat)
	{
		if (AData == MUDR_NICK)
		{
			if (FRecentContacts && AUser!=chat->mainUser())
			{
				IRecentItem oldItem = multiChatRecentItem(chat,ABefore.toString());
				QList<IRecentItem> realItems = FRecentContacts->streamItems(chat->streamJid());
				
				int oldIndex = realItems.indexOf(oldItem);
				if (oldIndex >= 0)
				{
					IRecentItem newItem = realItems.value(oldIndex);
					newItem.reference = AUser->userJid().pFull();

					FRecentContacts->removeItem(oldItem);
					FRecentContacts->setItemActiveTime(newItem,oldItem.activeTime);
				}
			}
		}
		else if (AData == MUDR_PRESENCE)
		{
			updateMultiUserRecentItems(chat,AUser->nick());
		}
	}
}

void MultiUserChatManager::onMultiChatWindowDestroyed()
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
	{
		LOG_STRM_INFO(window->streamJid(),QString("Multi user chat window destroyed, room=%1").arg(window->multiUserChat()->roomJid().bare()));
		FChatWindows.removeAll(window);
		updateMultiChatRosterIndex(window->streamJid(),window->contactJid());
		emit multiChatWindowDestroyed(window);
	}
}

void MultiUserChatManager::onMultiChatWindowContextMenu(Menu *AMenu)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiChatContextMenu(window,AMenu);
}

void MultiUserChatManager::onMultiChatWindowUserContextMenu(IMultiUser *AUser, Menu *AMenu)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiUserContextMenu(window,AUser,AMenu);
}

void MultiUserChatManager::onMultiChatWindowUserToolTips(IMultiUser *AUser, QMap<int,QString> &AToolTips)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		emit multiUserToolTips(window,AUser,AToolTips);
}

void MultiUserChatManager::onMultiChatWindowPrivateWindowChanged(IMessageChatWindow *AWindow)
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
		updateMultiUserRecentItems(window->multiUserChat(),AWindow->contactJid().resource());
}

void MultiUserChatManager::onMultiChatWindowInfoContextMenu(Menu *AMenu)
{
	if (FRostersViewPlugin)
	{
		IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
		IRosterIndex *chatIndex = widget!=NULL ? findMultiChatRosterIndex(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()) : NULL;
		if (chatIndex)
			FRostersViewPlugin->rostersView()->contextMenuForIndex(QList<IRosterIndex *>()<<chatIndex,NULL,AMenu);
	}
}

void MultiUserChatManager::onMultiChatWindowInfoToolTips(QMap<int,QString> &AToolTips)
{
	if (FRostersViewPlugin)
	{
		IMessageInfoWidget *widget = qobject_cast<IMessageInfoWidget *>(sender());
		IRosterIndex *chatIndex = widget!=NULL ? findMultiChatRosterIndex(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid()) : NULL;
		if (chatIndex)
			FRostersViewPlugin->rostersView()->toolTipsForIndex(chatIndex,NULL,AToolTips);
	}
}

void MultiUserChatManager::onRostersModelStreamsLayoutChanged(int ABefore)
{
	Q_UNUSED(ABefore);
	for (QList<IRosterIndex *>::const_iterator it=FChatIndexes.constBegin(); it!=FChatIndexes.constEnd(); ++it)
	{
		IRosterIndex *chatGroup = getConferencesGroupIndex((*it)->data(RDR_STREAM_JID).toString());
		if (chatGroup)
			FRostersModel->insertRosterIndex(*it,chatGroup);
	}
}

void MultiUserChatManager::onRostersModelIndexDestroyed(IRosterIndex *AIndex)
{
	if (FChatIndexes.removeOne(AIndex))
	{
		updateMultiChatRecentItem(AIndex);
		emit multiChatRosterIndexDestroyed(AIndex);
	}
}

void MultiUserChatManager::onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex->kind() == RIK_MUC_ITEM)
	{
		if (ARole == RDR_NAME)
		{
			IMultiUserChatWindow *window = findMultiChatWindow(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_PREP_BARE_JID).toString());
			if (window)
				updateMultiUserRecentItems(window->multiUserChat(),QString::null);
		}
	}
}

void MultiUserChatManager::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MultiUserChatManager::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		bool isMultiSelection = AIndexes.count()>1;
		IRosterIndex *index = AIndexes.first();
		if (index->kind() == RIK_STREAM_ROOT)
		{
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (isReady(streamJid))
			{
				Action *action = createWizardAction(AMenu);
				AMenu->addAction(action,AG_RVCM_MULTIUSERCHAT_JOIN,true);
			}
		}
		else if (index->kind()==RIK_CONTACTS_ROOT || index->kind()==RIK_GROUP_MUC)
		{
			foreach(const Jid &streamJid, index->data(RDR_STREAMS).toStringList())
			{
				if (isReady(streamJid))
				{
					Action *action = createWizardAction(AMenu);
					AMenu->addAction(action,AG_RVCM_MULTIUSERCHAT_JOIN,true);
					break;
				}
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
				data.insert(ADR_ROOM_JID,rolesMap.value(RDR_PREP_BARE_JID));
				data.insert(ADR_NICK,rolesMap.value(RDR_MUC_NICK));
				data.insert(ADR_PASSWORD,rolesMap.value(RDR_MUC_PASSWORD));

				Action *join = new Action(AMenu);
				join->setData(data);
				join->setText(tr("Join to Conference"));
				join->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
				connect(join,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
				AMenu->addAction(join,AG_RVCM_MULTIUSERCHAT_EXIT);

				if (isMultiSelection)
				{
					Action *exit = new Action(AMenu);
					exit->setData(data);
					exit->setText(tr("Exit from Conference"));
					exit->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
					connect(exit,SIGNAL(triggered(bool)),SLOT(onExitRoomActionTriggered(bool)));
					AMenu->addAction(exit,AG_RVCM_MULTIUSERCHAT_EXIT);
				}
			}
			else
			{
				if (!window->isActiveTabPage())
				{
					Action *open = new Action(AMenu);
					open->setText(tr("Open Conference Dialog"));
					open->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
					open->setData(ADR_ROOM_JID,index->data(RDR_PREP_BARE_JID));
					open->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_MESSAGE);
					open->setShortcutId(SCT_ROSTERVIEW_SHOWCHATDIALOG);
					connect(open,SIGNAL(triggered(bool)),SLOT(onOpenRoomActionTriggered(bool)));
					AMenu->addAction(open,AG_RVCM_MULTIUSERCHAT_OPEN);
				}

				if (window->multiUserChat()->state() == IMultiUserChat::Closed)
				{
					Action *enter = new Action(AMenu);
					enter->setText(tr("Enter to Conference"));
					enter->setData(ADR_STREAM_JID,QStringList()<<index->data(RDR_STREAM_JID).toString());
					enter->setData(ADR_ROOM_JID,QStringList()<<index->data(RDR_PREP_BARE_JID).toString());
					enter->setData(ADR_NICK,QStringList()<<index->data(RDR_MUC_NICK).toString());
					enter->setData(ADR_PASSWORD,QStringList()<<index->data(RDR_MUC_PASSWORD).toString());
					enter->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
					connect(enter,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
					AMenu->addAction(enter,AG_RVCM_MULTIUSERCHAT_EXIT);
				}

				Action *exit = new Action(AMenu);
				exit->setText(tr("Exit from Conference"));
				exit->setData(ADR_STREAM_JID,QStringList()<<index->data(RDR_STREAM_JID).toString());
				exit->setData(ADR_ROOM_JID,QStringList()<<index->data(RDR_PREP_BARE_JID).toString());
				exit->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
				connect(exit,SIGNAL(triggered(bool)),SLOT(onExitRoomActionTriggered(bool)));
				AMenu->addAction(exit,AG_RVCM_MULTIUSERCHAT_EXIT);

				window->contextMenuForRoom(AMenu);
			}
		}
		else if (!isMultiSelection && index->kind()==RIK_RECENT_ITEM && index->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE)
		{
			if (FRecentContacts && isReady(index->data(RDR_STREAM_JID).toString()))
			{
				IRecentItem item = FRecentContacts->rosterIndexItem(index);
				if (!item.isNull() && recentItemProxyIndexes(item).isEmpty())
				{
					Action *join = new Action(AMenu);
					join->setText(tr("Join to Conference"));
					join->setData(ADR_STREAM_JID,QStringList()<<index->data(RDR_STREAM_JID).toString());
					join->setData(ADR_ROOM_JID,QStringList()<<index->data(RDR_RECENT_REFERENCE).toString());
					join->setData(ADR_NICK,QStringList()<<FRecentContacts->itemProperty(item,REIP_CONFERENCE_NICK).toString());
					join->setData(ADR_PASSWORD,QStringList()<<FRecentContacts->itemProperty(item,REIP_CONFERENCE_PASSWORD).toString());
					join->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
					connect(join,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
					AMenu->addAction(join,AG_RVCM_MULTIUSERCHAT_EXIT);
				}
			}
		}
		else if (!isMultiSelection && index->kind()==RIK_RECENT_ITEM && index->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE_PRIVATE)
		{
			IMultiUserChatWindow *window = findMultiChatWindowForIndex(index);
			IMultiUser *user = window!=NULL ? window->multiUserChat()->findUser(Jid(index->data(RDR_RECENT_REFERENCE).toString()).resource()) : NULL;
			if (user)
				window->contextMenuForUser(user,AMenu);
		}
	}
}

void MultiUserChatManager::onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int,QString> &AToolTips)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		if (AIndex->kind()==RIK_RECENT_ITEM && AIndex->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE_PRIVATE)
		{
			Jid userJid = AIndex->data(RDR_RECENT_REFERENCE).toString();
			IMultiUserChatWindow *window = findMultiChatWindowForIndex(AIndex);
			IMultiUser *user = window!=NULL ? window->multiUserChat()->findUser(userJid.resource()) : NULL;
			if (user)
			{
				window->toolTipsForUser(user,AToolTips);
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_NAME,tr("<big><b>[%1]</b></big> in [%2]").arg(Qt::escape(user->nick()),Qt::escape(window->multiUserChat()->roomName())));
				AToolTips.insert(RTTO_MULTIUSERCHAT_ROOM,tr("<b>Conference:</b> %1").arg(window->multiUserChat()->roomJid().uBare()));
			}
		}
	}
}

void MultiUserChatManager::onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId == AdvancedDelegateItem::DisplayId)
	{
		foreach(IRosterIndex *index, AIndexes)
		{
			IMultiUserChatWindow *window = findMultiChatWindowForIndex(index);
			if (window)
			{
				QString name = window->multiUserChat()->roomTitle().trimmed();
				if (!name.isEmpty())
				{
					Action *nameAction = new Action(AMenu);
					nameAction->setText(TextManager::getElidedString(name,Qt::ElideRight,50));
					nameAction->setData(ADR_CLIPBOARD_DATA,name);
					connect(nameAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
					AMenu->addAction(nameAction, AG_RVCBM_NAME, true);
				}

				QString subject = window->multiUserChat()->subject().trimmed();
				if (!subject.isEmpty())
				{
					Action *subjectAction = new Action(AMenu);
					subjectAction->setText(TextManager::getElidedString(subject,Qt::ElideRight,50));
					subjectAction->setData(ADR_CLIPBOARD_DATA,subject);
					connect(subjectAction,SIGNAL(triggered(bool)),SLOT(onCopyToClipboardActionTriggered(bool)));
					AMenu->addAction(subjectAction, AG_RVCBM_MUC_SUBJECT, true);
				}
			}
		}
	}
}

void MultiUserChatManager::onInviteActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		Jid roomJid = action->data(ADR_ROOM_JID).toString();
		IMultiUserChatWindow *window = findMultiChatWindow(streamJid,roomJid);
		if (window)
		{
			bool ok;
			QString reason = tr("Please, enter this conference.");
			reason = QInputDialog::getText(window->instance(),tr("Invite User - %1").arg(roomJid.bare()),tr("Enter a message:"),QLineEdit::Normal,reason,&ok);

			if (ok)
				window->multiUserChat()->sendInvitation(contactJid,reason);
		}
	}
}

void MultiUserChatManager::onInviteDialogFinished(int AResult)
{
	QMessageBox *inviteDialog = qobject_cast<QMessageBox *>(sender());
	if (inviteDialog)
	{
		InviteFields fields = FInviteDialogs.take(inviteDialog);
		if (AResult == QMessageBox::Yes)
		{
			LOG_STRM_INFO(fields.streamJid,QString("Invite request from=%1 to room=%2 accepted").arg(fields.fromJid.full(),fields.roomJid.bare()));
			showJoinMultiChatWizard(fields.streamJid,fields.roomJid,QString::null,fields.password);
		}
		else if (AResult == QMessageBox::No)
		{
			Message decline;
			decline.setTo(fields.roomJid.bare());

			Stanza &mstanza = decline.stanza();
			QDomElement declElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("decline")).toElement();
			declElem.setAttribute("to",fields.fromJid.full());

			bool ok;
			QString reason = tr("I'm too busy right now");
			reason = QInputDialog::getText(inviteDialog,tr("Decline Invite - %1").arg(fields.roomJid.bare()),tr("Enter a message:"),QLineEdit::Normal,reason,&ok);

			if (ok)
			{
				if (!reason.isEmpty())
					declElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(reason));

				if (FMessageProcessor->sendMessage(fields.streamJid,decline,IMessageProcessor::DirectionOut))
					LOG_STRM_INFO(fields.streamJid,QString("Invite request from=%1 to room=%2 rejected").arg(fields.fromJid.full(),fields.roomJid.bare()));
				else
					LOG_STRM_WARNING(fields.streamJid,QString("Failed to send invite reject message to=%1").arg(fields.fromJid.full()));
			}
		}
	}
}

Q_EXPORT_PLUGIN2(plg_multiuserchat, MultiUserChatManager)
