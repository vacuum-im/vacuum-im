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
#include <definitions/stanzahandlerorders.h>
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
#include <definitions/statisticsparams.h>
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

#define SHC_MUC_INVITE            "/message/x[@xmlns='" NS_MUC_USER "']/invite"
#define SHC_MUC_DIRECT_INVITE     "/message/x[@xmlns='" NS_JABBER_X_CONFERENCE "']"

MultiUserChatManager::MultiUserChatManager()
{

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
	Q_UNUSED(APluginManager); Q_UNUSED(AInitOrder); 

	if (FMessageProcessor)
	{
		connect(FMessageProcessor->instance(),SIGNAL(activeStreamRemoved(const Jid &)),SLOT(onActiveXmppStreamRemoved(const Jid &)));
	}

	if (FXmppStreamManager)
	{
		connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
		connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
	}
	
	if (FStatusIcons)
	{
		connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
	}

	if (FRostersModel)
	{
		connect(FRostersModel->instance(),SIGNAL(streamsLayoutChanged(int)),SLOT(onRostersModelStreamsLayoutChanged(int)));
		connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRostersModelIndexDestroyed(IRosterIndex *)));
		connect(FRostersModel->instance(),SIGNAL(indexDataChanged(IRosterIndex *, int)),SLOT(onRostersModelIndexDataChanged(IRosterIndex *, int)));
	}

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

	if (FNotifications)
	{
		connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
		connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
	}

	if (FMessageWidgets)
	{
		connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IMessageChatWindow *)),SLOT(onMessageChatWindowCreated(IMessageChatWindow *)));
	}

	if (FMessageArchiver)
	{
		connect(FMessageArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
			SLOT(onMessageArchiverRequestFailed(const QString &, const XmppError &)));
		connect(FMessageArchiver->instance(),SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
			SLOT(onMessageArchiverHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
		connect(FMessageArchiver->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
			SLOT(onMessageArchiverCollectionLoaded(const QString &, const IArchiveCollection &)));
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FXmppStreamManager!=NULL;
}

bool MultiUserChatManager::initObjects()
{
	Shortcuts::declareShortcut(SCT_APP_MULTIUSERCHAT_WIZARD, tr("Join conference"), tr("Ctrl+J","Join conference"), Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SHOWMUCUSERS, tr("Show/Hide conference participants list"), tr("Ctrl+U","Show/Hide conference participants list"));

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
		groupchatType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		groupchatType.kindDefs = groupchatType.kindMask & ~(INotification::PopupWindow|INotification::ShowMinimized|INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_GROUPCHAT,groupchatType);

		INotificationType mentionType;
		mentionType.order = NTO_MUC_MENTION_MESSAGE;
		mentionType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_MESSAGE);
		mentionType.title = tr("When referring to you at the conference");
		mentionType.kindMask = INotification::RosterNotify|INotification::TrayNotify|INotification::PopupWindow|INotification::SoundPlay|INotification::AlertWidget|INotification::TabPageNotify|INotification::ShowMinimized|INotification::AutoActivate;
		mentionType.kindDefs = mentionType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_MUC_MESSAGE_MENTION,mentionType);
	}

	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(XUHO_DEFAULT,this);
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

	if (FMainWindowPlugin)
	{
		Menu *menu = FMainWindowPlugin->mainWindow()->mainMenu();
		menu->addAction(createWizardAction(menu),AG_MMENU_MULTIUSERCHAT_JOIN,true);
	}

	return true;
}

bool MultiUserChatManager::initSettings()
{
	Options::setDefaultValue(OPV_MUC_SHOWENTERS,true);
	Options::setDefaultValue(OPV_MUC_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MUC_ARCHIVESTATUS,false);
	Options::setDefaultValue(OPV_MUC_QUITONWINDOWCLOSE,false);
	Options::setDefaultValue(OPV_MUC_REJOINAFTERKICK,false);
	Options::setDefaultValue(OPV_MUC_REFERENUMERATION,false);
	Options::setDefaultValue(OPV_MUC_NICKNAMESUFFIX,",");
	Options::setDefaultValue(OPV_MUC_USERVIEWMODE,IMultiUserView::ViewSimple);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_NOTIFYSILENCE,false);

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
		widgets.insertMulti(OWO_CONFERENCES_SHOWENTERS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_SHOWENTERS),tr("Show users connections and disconnections"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_SHOWSTATUS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_SHOWSTATUS),tr("Show users status changes"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_ARCHIVESTATUS,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_ARCHIVESTATUS),tr("Save users status messages in history"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_QUITONWINDOWCLOSE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_QUITONWINDOWCLOSE),tr("Leave the conference when window closed"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_REJOINAFTERKICK,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_REJOINAFTERKICK),tr("Automatically rejoin to conference after kick"),AParent));
		widgets.insertMulti(OWO_CONFERENCES_REFERENUMERATION,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_REFERENUMERATION),tr("Select a user to refer by enumeration in the input field"),AParent));

		widgets.insertMulti(OHO_CONFERENCES_USERVIEW,FOptionsManager->newOptionsDialogHeader(tr("Participants List"),AParent));

		QComboBox *cmbViewMode = new QComboBox(AParent);
		cmbViewMode->addItem(tr("Full"), IMultiUserView::ViewFull);
		cmbViewMode->addItem(tr("Simplified"), IMultiUserView::ViewSimple);
		cmbViewMode->addItem(tr("Compact"), IMultiUserView::ViewCompact);
		widgets.insertMulti(OWO_CONFERENCES_USERVIEWMODE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_USERVIEWMODE),tr("Participants list view:"),cmbViewMode,AParent));
	}
	return widgets;
}

bool MultiUserChatManager::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIInvite.value(AStreamJid)==AHandleId && AStanza.type()!=STANZA_TYPE_ERROR)
	{
		AAccept = true;

		QDomElement mucInviteElem = AStanza.firstElement("x",NS_MUC_USER).firstChildElement("invite");
		QDomElement directInviteElem = AStanza.firstElement("x",NS_JABBER_X_CONFERENCE);

		ChatInvite invite;
		invite.id = AStanza.id();
		invite.streamJid = AStreamJid;
		if (!mucInviteElem.isNull())
		{
			invite.roomJid = AStanza.from();
			invite.fromJid = mucInviteElem.attribute("from");
			invite.reason = mucInviteElem.firstChildElement("reason").text();
			invite.thread = mucInviteElem.firstChildElement("continue").attribute("thread");
			invite.isContinue = !mucInviteElem.firstChildElement("continue").isNull();
			invite.password = mucInviteElem.parentNode().toElement().firstChildElement("password").text();
			LOG_STRM_INFO(AStreamJid,QString("Received mediated invite to room=%1, from=%2").arg(invite.roomJid.full(),invite.fromJid.full()));
		}
		else if (!directInviteElem.isNull())
		{
			invite.roomJid = directInviteElem.attribute("jid");
			invite.fromJid = AStanza.from();
			invite.reason = directInviteElem.attribute("reason");
			invite.thread = directInviteElem.attribute("thread");
			invite.isContinue = directInviteElem.hasAttribute("continue") ? QVariant(directInviteElem.attribute("continue")).toBool() : false;
			invite.password = directInviteElem.attribute("password");
			LOG_STRM_INFO(AStreamJid,QString("Received direct invite to room=%1, from=%2").arg(invite.roomJid.full(),invite.fromJid.full()));
		}

		if (invite.roomJid.isValid() && invite.fromJid.isValid() && !findMultiChatWindow(AStreamJid,invite.roomJid))
		{
			if (FDiscovery && !FDiscovery->hasDiscoInfo(AStreamJid,invite.roomJid))
				FDiscovery->requestDiscoInfo(AStreamJid,invite.roomJid);

			INotification notify;
			notify.kinds = FNotifications!=NULL ? FNotifications->enabledTypeNotificationKinds(NNT_MUC_MESSAGE_INVITE) : 0;
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_MUC_MESSAGE_INVITE;
				notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_INVITE));
				notify.data.insert(NDR_TOOLTIP,tr("You are invited to the conference %1").arg(invite.roomJid.uBare()));
				notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
				notify.data.insert(NDR_CONTACT_JID,invite.fromJid.full());
				notify.data.insert(NDR_ROSTER_ORDER,RNO_MUC_INVITE);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::Blink|IRostersNotify::AllwaysVisible|IRostersNotify::HookClicks);
				notify.data.insert(NDR_ROSTER_CREATE_INDEX,true);
				notify.data.insert(NDR_POPUP_CAPTION,tr("Invitation received"));
				notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AStreamJid,invite.fromJid));
				notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(invite.fromJid));
				notify.data.insert(NDR_POPUP_TEXT,notify.data.value(NDR_TOOLTIP).toString());
				notify.data.insert(NDR_SOUND_FILE,SDF_MUC_INVITE_MESSAGE);
				FInviteNotify.insert(FNotifications->appendNotification(notify), invite);
			}
		}

		return true;
	}
	return false;
}

void MultiUserChatManager::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FDiscoNickRequests.contains(AStanza.id()))
	{
		FDiscoNickRequests.removeAll(AStanza.id());
		QDomElement queryElem = AStanza.firstElement("query",NS_DISCO_INFO);
		QDomElement identityElem = AStanza.firstElement("query",NS_DISCO_INFO).firstChildElement("identity");
		if (AStanza.isResult() && queryElem.attribute("node")==MUC_NODE_NICK)
		{
			QString nick = queryElem.firstChildElement("identity").attribute("name");
			LOG_STRM_INFO(AStreamJid,QString("Registered nick as discovery request received from=%1, nick=%2, id=%3").arg(AStanza.from(),nick,AStanza.id()));
			emit registeredNickReceived(AStanza.id(),nick);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive registered nick as discovery request from=%1, id=%2: %3").arg(AStanza.from(),AStanza.id(),XmppStanzaError(AStanza).errorMessage()));

			Stanza stanza(STANZA_KIND_IQ);
			stanza.setType(STANZA_TYPE_GET).setTo(Jid(AStanza.from()).domain()).setUniqueId();
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
		if (AStanza.isResult())
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
	return false;
}

bool MultiUserChatManager::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
	if (AFeature==NS_MUC && !ADiscoInfo.contactJid.hasResource())
	{
		IMultiUserChatWindow *window = findMultiChatWindow(AStreamJid,ADiscoInfo.contactJid);
		if (window != NULL)
			window->showTabPage();
		else
			showJoinMultiChatWizard(AStreamJid,ADiscoInfo.contactJid,QString::null,QString::null);
		return true;
	}
	return false;
}

Action *MultiUserChatManager::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
	if (AFeature==NS_MUC && FDiscovery)
	{
		if (FDiscovery->findIdentity(ADiscoInfo.identity,DIC_CONFERENCE,QString::null) < 0)
		{
			Menu *inviteMenu = createInviteMenu(QStringList()<<ADiscoInfo.streamJid.full(), QStringList()<<ADiscoInfo.contactJid.full(), AParent);
			if (!inviteMenu->isEmpty())
				return inviteMenu->menuAction();
			else
				delete inviteMenu;
		}
		else if (findMultiChatWindow(AStreamJid,ADiscoInfo.contactJid) == NULL)
		{
			return createJoinAction(AStreamJid,ADiscoInfo.contactJid,AParent);
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
		IMultiUserChatWindow *mucWindow = findMultiChatWindow(AItem.streamJid,userJid);
		IMultiUser *user = mucWindow!=NULL ? mucWindow->multiUserChat()->findUser(userJid.resource()) : NULL;
		IMessageChatWindow *privateWindow = mucWindow!=NULL ? mucWindow->findPrivateChatWindow(userJid) : NULL;
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
		Stanza stanza(STANZA_KIND_IQ);
		stanza.setType(STANZA_TYPE_GET).setTo(ARoomJid.bare()).setUniqueId();
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

	dfeature.active = true;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_CONFERENCE);;
	dfeature.var = NS_MUC;
	dfeature.name = tr("Multi-User Conferences");
	dfeature.description = tr("Supports the multi-user conferences");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_INVITE);
	dfeature.var = NS_JABBER_X_CONFERENCE;
	dfeature.name = tr("Direct Invitations to Conferences");
	dfeature.description = tr("Supports the direct invitations to conferences");
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
			if (index->kind()!=RIK_MUC_ITEM && index->kind()!=RIK_CONTACT)
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
	action->setShortcutId(SCT_APP_MULTIUSERCHAT_WIZARD);
	connect(action,SIGNAL(triggered(bool)),SLOT(onWizardRoomActionTriggered(bool)));
	return action;
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

Menu *MultiUserChatManager::createInviteMenu(const QStringList &AStreams, const QStringList &AContacts, QWidget *AParent) const
{
	Menu *inviteMenu = new Menu(AParent);
	inviteMenu->setTitle(tr("Invite to"));
	inviteMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);

	QSet<Jid> invitedTo;
	foreach(IMultiUserChatWindow *window, FChatWindows)
	{
		IMultiUserChat *mchat = window->multiUserChat();
		if (mchat->isOpen() && !invitedTo.contains(mchat->roomJid()))
		{
			QStringList contacts;
			for (int i=0; i<AStreams.count() && i<AContacts.count(); i++)
			{
				Jid stream = AStreams.at(i);
				Jid contact = AContacts.at(i);

				if (contacts.contains(contact.pFull()))
					continue;

				if (mchat->isUserPresent(contact))
					continue;

				if (FDiscovery!=NULL && !FDiscovery->checkDiscoFeature(stream,contact,NS_MUC))
					continue;

				contacts.append(contact.pFull());
			}

			if (!contacts.isEmpty())
			{
				Action *action = new Action(inviteMenu);
				action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFERENCE);
				action->setText(TextManager::getElidedString(mchat->roomTitle(),Qt::ElideRight,50));
				action->setData(ADR_STREAM_JID,window->streamJid().full());
				action->setData(ADR_ROOM_JID,window->multiUserChat()->roomJid().bare());
				action->setData(ADR_CONTACT_JID,contacts);
				connect(action,SIGNAL(triggered(bool)),SLOT(onInviteActionTriggered(bool)));
				inviteMenu->addAction(action,AG_DEFAULT,true);
			}

			invitedTo += mchat->roomJid();
		}
	}

	return inviteMenu;
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
}

void MultiUserChatManager::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FXmppStreamManager!=NULL && AId==SCT_APP_MULTIUSERCHAT_WIZARD)
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

void MultiUserChatManager::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle mucInviteHandle;
		mucInviteHandle.handler = this;
		mucInviteHandle.order = SHO_MI_MULTIUSERCHAT_INVITE;
		mucInviteHandle.direction = IStanzaHandle::DirectionIn;
		mucInviteHandle.streamJid = AXmppStream->streamJid();
		mucInviteHandle.conditions.append(SHC_MUC_INVITE);
		mucInviteHandle.conditions.append(SHC_MUC_DIRECT_INVITE);
		FSHIInvite.insert(mucInviteHandle.streamJid, FStanzaProcessor->insertStanzaHandle(mucInviteHandle));
	}
}

void MultiUserChatManager::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
		FStanzaProcessor->removeStanzaHandle(FSHIInvite.take(AXmppStream->streamJid()));

	foreach(int notifyId, FInviteNotify.keys())
		if (AXmppStream->streamJid() == FInviteNotify.value(notifyId).streamJid)
			FNotifications->removeNotification(notifyId);

	foreach(QMessageBox *inviteDialog, FInviteDialogs.keys())
		if (AXmppStream->streamJid() == FInviteDialogs.value(inviteDialog).streamJid)
			inviteDialog->reject();
}

void MultiUserChatManager::onRostersModelStreamsLayoutChanged(int ABefore)
{
	Q_UNUSED(ABefore);
	for (QList<IRosterIndex *>::const_iterator it=FChatIndexes.constBegin(); it!=FChatIndexes.constEnd(); ++it)
	{
		IRosterIndex *chatGroup = getConferencesGroupIndex((*it)->data(RDR_STREAM_JID).toString());
		if (chatGroup)
			FRostersModel->insertRosterIndex(*it,chatGroup);
		updateMultiChatRecentItem(*it);
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
		else if (index->kind() == RIK_CONTACT)
		{
			// For single selection menu created in createDiscoFeatureAction
			if (isMultiSelection)
			{
				QMap<int, QStringList> rolesMap = FRostersViewPlugin->rostersView()->indexesRolesMap(AIndexes, QList<int>()<<RDR_STREAM_JID<<RDR_PREP_FULL_JID, RDR_PREP_FULL_JID);
				Menu *inviteMenu = createInviteMenu(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_FULL_JID),AMenu);
				if (!inviteMenu->isEmpty())
					AMenu->addAction(inviteMenu->menuAction(),AG_RVCM_MULTIUSERCHAT_INVITE,true);
				else
					delete inviteMenu;
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

				foreach(const Jid &streamJid, rolesMap.value(RDR_STREAM_JID))
				{
					if (isReady(streamJid))
					{
						Action *join = new Action(AMenu);
						join->setData(data);
						join->setText(tr("Join to Conference"));
						join->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
						connect(join,SIGNAL(triggered(bool)),SLOT(onEnterRoomActionTriggered(bool)));
						AMenu->addAction(join,AG_RVCM_MULTIUSERCHAT_EXIT);
						break;
					}
				}

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

				if (isReady(window->streamJid()) && window->multiUserChat()->state()==IMultiUserChat::Closed)
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
		else if (index->kind()==RIK_RECENT_ITEM && index->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE)
		{
			if (FRecentContacts && !isMultiSelection && isReady(index->data(RDR_STREAM_JID).toString()))
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
		else if (index->kind()==RIK_RECENT_ITEM && index->data(RDR_RECENT_TYPE).toString()==REIT_CONFERENCE_PRIVATE)
		{
			if (!isMultiSelection)
			{
				IMultiUserChatWindow *window = findMultiChatWindowForIndex(index);
				IMultiUser *user = window!=NULL ? window->multiUserChat()->findUser(Jid(index->data(RDR_RECENT_REFERENCE).toString()).resource()) : NULL;
				if (user)
					window->contextMenuForUser(user,AMenu);
			}
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
				AToolTips.insert(RTTO_ROSTERSVIEW_INFO_NAME,tr("<big><b>[%1]</b></big> in [%2]").arg(user->nick().toHtmlEscaped(),window->multiUserChat()->roomName().toHtmlEscaped()));
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
					AMenu->addAction(subjectAction, AG_RVCBM_MULTIUSERCHAT_SUBJECT, true);
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
		Jid roomJid = action->data(ADR_ROOM_JID).toString();

		IMultiUserChatWindow *window = findMultiChatWindow(streamJid,roomJid);
		if (window)
		{
			QList<Jid> contacts;
			foreach(const QString &contact, action->data(ADR_CONTACT_JID).toStringList())
				contacts.append(contact);
			window->multiUserChat()->sendInvitation(contacts);
		}
	}
}

void MultiUserChatManager::onInviteDialogFinished(int AResult)
{
	QMessageBox *inviteDialog = qobject_cast<QMessageBox *>(sender());
	if (inviteDialog)
	{
		ChatInvite invite = FInviteDialogs.take(inviteDialog);
		if (AResult == QMessageBox::Yes)
		{
			LOG_STRM_INFO(invite.streamJid,QString("Accepted invite request from=%1 to room=%2").arg(invite.fromJid.full(),invite.roomJid.bare()));
			showJoinMultiChatWizard(invite.streamJid,invite.roomJid,QString::null,invite.password);
		}
		else
		{
			Stanza stanza(STANZA_KIND_MESSAGE);
			stanza.setTo(invite.roomJid.bare()).setId(invite.id);

			QDomElement declElem = stanza.addElement("x",NS_MUC_USER).appendChild(stanza.createElement("decline")).toElement();
			declElem.setAttribute("to",invite.fromJid.full());

			if (FStanzaProcessor && FStanzaProcessor->sendStanzaOut(invite.streamJid,stanza))
				LOG_STRM_INFO(invite.streamJid,QString("Rejected invite request from=%1 to room=%2").arg(invite.fromJid.full(),invite.roomJid.bare()));
			else
				LOG_STRM_WARNING(invite.streamJid,QString("Failed to send invite reject message to=%1").arg(invite.fromJid.full()));
		}
	}
}

void MultiUserChatManager::onNotificationActivated(int ANotifyId)
{
	if (FInviteNotify.contains(ANotifyId))
	{
		ChatInvite invite = FInviteNotify.take(ANotifyId);

		QList<IDiscoIdentity> roomIdent = FDiscovery!=NULL ? FDiscovery->discoInfo(invite.streamJid,invite.roomJid).identity : QList<IDiscoIdentity>();
		int identIndex = !roomIdent.isEmpty() ? FDiscovery->findIdentity(roomIdent,DIC_CONFERENCE,QString::null) : -1;
		QString identName = identIndex>=0 ? roomIdent.value(identIndex).name : QString::null;

		QString roomName = !identName.isEmpty() ? QString("%1 <%2>").arg(identName,invite.roomJid.uBare().toHtmlEscaped()) : invite.roomJid.uBare().toHtmlEscaped();
		QString userName = FNotifications->contactName(invite.streamJid,invite.fromJid).toHtmlEscaped();

		QString msg = tr("You are invited to the conference <b>%1</b> by user <b>%2</b>.").arg(roomName,userName);
		msg += QString("<br>%1<br>").arg(invite.reason.toHtmlEscaped());
		msg += tr("Do you want to join to the conference?");

		QMessageBox *inviteDialog = new QMessageBox(QMessageBox::Question,tr("Invitation to Conference"),msg,QMessageBox::Yes|QMessageBox::No);
		inviteDialog->setAttribute(Qt::WA_DeleteOnClose,true);
		inviteDialog->setEscapeButton(QMessageBox::No);
		inviteDialog->setModal(false);
		connect(inviteDialog,SIGNAL(finished(int)),SLOT(onInviteDialogFinished(int)));

		FInviteDialogs.insert(inviteDialog,invite);
		inviteDialog->show();

		FNotifications->removeNotification(ANotifyId);
	}
}

void MultiUserChatManager::onNotificationRemoved(int ANotifyId)
{
	FInviteNotify.remove(ANotifyId);
}

void MultiUserChatManager::onMessageChatWindowCreated(IMessageChatWindow *AWindow)
{
	if (FDiscovery && AWindow->contactJid().hasNode())
	{
		Menu *inviteMenu = new InviteUsersMenu(AWindow,AWindow->instance());
		inviteMenu->setTitle(tr("Invite to Conversation"));
		inviteMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
		connect(inviteMenu,SIGNAL(inviteAccepted(const QMultiMap<Jid, Jid> &)),SLOT(onConvertMessageChatWindowStart(const QMultiMap<Jid, Jid> &)));
		AWindow->toolBarWidget()->toolBarChanger()->insertAction(inviteMenu->menuAction(),TBG_MWTBW_MULTIUSERCHAT_INVITE)->setPopupMode(QToolButton::InstantPopup);
	}
}

void MultiUserChatManager::onConvertMessageChatWindowStart(const QMultiMap<Jid, Jid> &AAddresses)
{
	Menu *inviteMenu = qobject_cast<Menu *>(sender());
	IMessageChatWindow *window = inviteMenu!=NULL ? qobject_cast<IMessageChatWindow *>(inviteMenu->parent()) : NULL;
	if (window!=NULL && !AAddresses.isEmpty())
	{
		ChatConvert convert;
		convert.streamJid = window->streamJid();
		convert.contactJid = window->contactJid();
		convert.members = AAddresses.values() << window->contactJid();
		convert.threadId = QString("");

		LOG_STRM_INFO(convert.streamJid,QString("Starting conversion chat with=%1 to conference").arg(convert.contactJid.full()));

		QMultiMap<Jid, Jid> addresses = AAddresses;
		addresses.insert(window->streamJid(),window->streamJid());
		addresses.insert(window->streamJid(),window->contactJid());

		QStringList names;
		QSet<Jid> invited;
		for (QMultiMap<Jid,Jid>::const_iterator it=addresses.constBegin(); it!=addresses.constEnd(); ++it)
		{
			if (!invited.contains(it.value().bare()))
			{
				if (FNotifications)
					names.append(FNotifications->contactName(it.key(),it.value()));
				else
					names.append(it->uNode());
				invited += it.value().bare();
			}
		}

		QString roomName;
		if (names.count() > 3)
			roomName = tr("Conference with %1 and %n other contact(s)","",names.count()-2).arg(QStringList(names.mid(0,2)).join(", "));
		else
			roomName = tr("Conference with %1").arg(names.join(", "));

		QMap<QString,QVariant> hints;
		hints["muc#roomconfig_roomname"] = roomName;
		hints["muc#roomconfig_publicroom"] = false;
		hints["muc#roomconfig_persistentroom"] = true;
		hints["muc#roomconfig_whois"] = "anyone";
		hints["muc#roomconfig_changesubject"] = true;
		hints["muc#roomconfig_allowinvites"] = true;

		CreateMultiChatWizard *wizard = new CreateMultiChatWizard(CreateMultiChatWizard::ModeCreate,convert.streamJid,Jid::null,QString::null,QString::null);
		connect(wizard,SIGNAL(wizardAccepted(IMultiUserChatWindow *)),SLOT(onConvertMessageChatWindowWizardAccetped(IMultiUserChatWindow *)));
		connect(wizard,SIGNAL(rejected()),SLOT(onConvertMessageChatWindowWizardRejected()));
		FWizardConvert.insert(wizard,convert);

		wizard->setConfigHints(hints);
		wizard->show();
	}
}

void MultiUserChatManager::onConvertMessageChatWindowWizardAccetped(IMultiUserChatWindow *AWindow)
{
	CreateMultiChatWizard *wizard = qobject_cast<CreateMultiChatWizard *>(sender());
	if (FWizardConvert.contains(wizard))
	{
		ChatConvert convert = FWizardConvert.take(wizard);
		convert.streamJid = AWindow->multiUserChat()->streamJid();
		convert.roomJid = AWindow->multiUserChat()->roomJid();

		LOG_STRM_INFO(convert.streamJid,QString("Accepted conversion chat with=%1 to conference room=%2").arg(convert.contactJid.full(),convert.roomJid.bare()));

		if (FMessageArchiver)
		{
			IArchiveRequest request;
			request.with = convert.contactJid;
			request.maxItems = 1;
			request.openOnly = true;
			request.exactmatch = true;
			request.threadId = convert.threadId;

			QString reqId = FMessageArchiver->loadHeaders(convert.streamJid,request);
			if (!reqId.isEmpty())
			{
				LOG_STRM_INFO(convert.streamJid,QString("Loading history headers for conversion chat with=%1 to conference room=%2, id=%3").arg(convert.contactJid.full(),convert.roomJid.bare(),reqId));
				FHistoryConvert.insert(reqId,convert);
			}
			else
			{
				LOG_STRM_WARNING(convert.streamJid,QString("Failed to load history headers for conversion chat with=%1 to conference room=%2: Request not sent").arg(convert.contactJid.full(),convert.roomJid.bare()));
				onConvertMessageChatWindowFinish(convert);
			}
		}
		else
		{
			onConvertMessageChatWindowFinish(convert);
		}
	}
}

void MultiUserChatManager::onConvertMessageChatWindowWizardRejected()
{
	CreateMultiChatWizard *wizard = qobject_cast<CreateMultiChatWizard *>(sender());
	if (FWizardConvert.contains(wizard))
	{
		ChatConvert convert = FWizardConvert.take(wizard);
		LOG_STRM_INFO(convert.streamJid,QString("User canceled conversion chat with=%1 to conference").arg(convert.contactJid.full()));
	}
}

void MultiUserChatManager::onConvertMessageChatWindowFinish(const ChatConvert &AConvert)
{
	IMultiUserChatWindow *window = findMultiChatWindow(AConvert.streamJid,AConvert.roomJid);
	if (window)
	{
		window->multiUserChat()->sendInvitation(AConvert.members,AConvert.reason,AConvert.threadId);
		LOG_STRM_INFO(AConvert.streamJid,QString("Finished conversion chat with=%1 to conference room=%2").arg(AConvert.contactJid.full(),AConvert.roomJid.bare()));
		REPORT_EVENT(SEVP_MUC_CHAT_CONVERT,1);
	}
	else
	{
		REPORT_ERROR("Failed to finish conversion chat to conference: Conference window not found");
	}
}

void MultiUserChatManager::onMessageArchiverRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHistoryConvert.contains(AId))
	{
		ChatConvert convert = FHistoryConvert.take(AId);
		LOG_STRM_WARNING(convert.streamJid,QString("Failed to load history for conversion chat with=%1 to conference room=%2: %3").arg(convert.contactJid.full(),convert.roomJid.bare(),AError.condition()));
		onConvertMessageChatWindowFinish(convert);
	}
}

void MultiUserChatManager::onMessageArchiverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FHistoryConvert.contains(AId))
	{
		ChatConvert convert = FHistoryConvert.take(AId);
		if (!AHeaders.isEmpty())
		{
			QString reqId = FMessageArchiver->loadCollection(convert.streamJid,AHeaders.first());
			if (!reqId.isEmpty())
			{
				LOG_STRM_INFO(convert.streamJid,QString("Loading history collection for conversion chat with=%1 to conference room=%2, id=%3").arg(convert.contactJid.full(),convert.roomJid.bare(),reqId));
				FHistoryConvert.insert(reqId,convert);
			}
			else
			{
				LOG_STRM_WARNING(convert.streamJid,QString("Failed to load history collection for conversion chat with=%1 to conference room=%2: Request not sent").arg(convert.contactJid.full(),convert.roomJid.bare()));
				onConvertMessageChatWindowFinish(convert);
			}
		}
		else
		{
			LOG_STRM_INFO(convert.streamJid,QString("No current history for conversion chat with=%1 to conference room=%2").arg(convert.contactJid.full(),convert.roomJid.bare()));
			onConvertMessageChatWindowFinish(convert);
		}
	}
}

void MultiUserChatManager::onMessageArchiverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FHistoryConvert.contains(AId))
	{
		ChatConvert convert = FHistoryConvert.take(AId);
		IMultiUserChatWindow *window = findMultiChatWindow(convert.streamJid,convert.roomJid);
		if (window && window->multiUserChat()->isOpen())
		{
			LOG_STRM_INFO(convert.streamJid,QString("Uploading history for conversion chat with=%1 to conference room=%2, messages=%3").arg(convert.contactJid.full(),convert.roomJid.bare()).arg(ACollection.body.messages.count()));
			foreach(Message message, ACollection.body.messages)
			{
				message.setDelayed(message.dateTime(),message.fromJid());
				message.setTo(convert.roomJid.bare()).setType(Message::GroupChat);
				window->multiUserChat()->sendMessage(message);
			}
		}
		onConvertMessageChatWindowFinish(convert);
	}
}
