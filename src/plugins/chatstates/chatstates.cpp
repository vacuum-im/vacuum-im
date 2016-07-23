#include "chatstates.h"

#include <QSet>
#include <QDateTime>
#include <QDataStream>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/namespaces.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/archivehandlerorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosternotifyorders.h>
#include <definitions/multiusernotifyorders.h>
#include <definitions/tabpagenotifypriorities.h>
#include <definitions/sessionnegotiatororders.h>
#include <definitions/multiuserdataroles.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/logger.h>

#define SHC_OUT_MESSAGES          "/message/body"
#define SHC_IN_MESSAGES           "/message/[@xmlns='" NS_CHATSTATES "']"

#define STATE_UNKNIWN             "unknown"
#define STATE_ACTIVE              "active"
#define STATE_COMPOSING           "composing"
#define STATE_PAUSED              "paused"
#define STATE_INACTIVE            "inactive"
#define STATE_GONE                "gone"

#define SFV_MAY_SEND              "may"
#define SFV_MUSTNOT_SEND          "mustnot"

#define PAUSED_TIMEOUT            30
#define INACTIVE_TIMEOUT          2*60
#define GONE_TIMEOUT              10*60

#define UPDATE_TIMEOUT            1000

ChatStates::ChatStates()
{
	FDataForms = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FMessageWidgets = NULL;
	FOptionsManager = NULL;
	FPresenceManager = NULL;
	FStanzaProcessor = NULL;
	FMessageArchiver = NULL;
	FMultiChatManager = NULL;
	FSessionNegotiation = NULL;

	FUpdateTimer.setSingleShot(false);
	FUpdateTimer.setInterval(UPDATE_TIMEOUT);
	connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateSelfStates()));
}

ChatStates::~ChatStates()
{

}

void ChatStates::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Chat State Notifications");
	APluginInfo->description = tr("Allows to share information about the user's activity in the chat");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRESENCE_UUID);
	APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ChatStates::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if (FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
			connect(FPresenceManager->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
				SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
			connect(FPresenceManager->instance(),SIGNAL(presenceClosed(IPresence *)),SLOT(onPresenceClosed(IPresence *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IMessageChatWindow *)),SLOT(onChatWindowCreated(IMessageChatWindow *)));
			connect(FMessageWidgets->instance(),SIGNAL(chatWindowDestroyed(IMessageChatWindow *)),SLOT(onChatWindowDestroyed(IMessageChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatManager").value(0,NULL);
	if (plugin)
	{
		FMultiChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());
		if (FMultiChatManager)
		{
			connect(FMultiChatManager->instance(),SIGNAL(multiChatWindowCreated(IMultiUserChatWindow *)), SLOT(onMultiChatWindowCreated(IMultiUserChatWindow *)));
			connect(FMultiChatManager->instance(),SIGNAL(multiChatWindowDestroyed(IMultiUserChatWindow *)), SLOT(onMultiChatWindowDestroyed(IMultiUserChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageArchiver").value(0);
	if (plugin)
	{
		FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ISessionNegotiation").value(0,NULL);
	if (plugin)
	{
		FSessionNegotiation = qobject_cast<ISessionNegotiation *>(plugin->instance());
		if (FSessionNegotiation && FDataForms)
		{
			connect(FSessionNegotiation->instance(),SIGNAL(sessionTerminated(const IStanzaSession &)),
				SLOT(onStanzaSessionTerminated(const IStanzaSession &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FPresenceManager!=NULL && FMessageWidgets!=NULL && FStanzaProcessor!=NULL;
}

bool ChatStates::initObjects()
{
	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FMessageArchiver)
	{
		FMessageArchiver->insertArchiveHandler(AHO_DEFAULT,this);
	}
	if (FSessionNegotiation && FDataForms)
	{
		FSessionNegotiation->insertNegotiator(this,SNO_DEFAULT);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_CHATSTATE_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATSTATES_COMPOSING);
		notifyType.title = tr("When contact is typing the message for you");
		notifyType.kindMask = INotification::RosterNotify|INotification::TabPageNotify;
		notifyType.kindDefs = notifyType.kindMask;
		FNotifications->registerNotificationType(NNT_CHATSTATE_TYPING,notifyType);
	}
	return true;
}

bool ChatStates::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_CHATSTATESENABLED,true);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}

	return true;
}

bool ChatStates::startPlugin()
{
	FUpdateTimer.start();
	return true;
}

bool ChatStates::archiveMessageEdit(int AOrder, const Jid &AStreamJid, Message &AMessage, bool ADirectionIn)
{
	Q_UNUSED(AOrder); Q_UNUSED(AStreamJid); Q_UNUSED(ADirectionIn);
	if (!AMessage.stanza().firstElement(QString::null,NS_CHATSTATES).isNull())
	{
		AMessage.detach();
		QDomElement elem = AMessage.stanza().firstElement(QString::null,NS_CHATSTATES);
		elem.parentNode().removeChild(elem);
	}
	return false;
}

QMultiMap<int, IOptionsDialogWidget *> ChatStates::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES_CHATSTATESENABLED, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MESSAGES_CHATSTATESENABLED),tr("Send notifications of your chat activity"),AParent));
	}
	return widgets;
}

int ChatStates::sessionInit(const IStanzaSession &ASession, IDataForm &ARequest)
{
	IDataField chatstates;
	chatstates.var = NS_CHATSTATES;
	chatstates.type = DATAFIELD_TYPE_LISTSINGLE;
	chatstates.required = false;

	bool enabled = isEnabled(ASession.contactJid);
	if (enabled)
	{
		IDataOption maysend;
		maysend.value = SFV_MAY_SEND;
		chatstates.options.append(maysend);
	}
	if (permitStatus(ASession.contactJid) != IChatStates::StatusEnable)
	{
		IDataOption mustnotsend;
		mustnotsend.value = SFV_MUSTNOT_SEND;
		chatstates.options.append(mustnotsend);
	}
	chatstates.value = enabled ? SFV_MAY_SEND : SFV_MUSTNOT_SEND;

	if (ASession.status == IStanzaSession::Init)
	{
		ARequest.fields.append(chatstates);
		return ISessionNegotiator::Auto;
	}
	else if (ASession.status == IStanzaSession::Renegotiate)
	{
		int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ASession.form.fields) : -1;
		if (index<0 || ASession.form.fields.at(index).value!=chatstates.value)
		{
			ARequest.fields.append(chatstates);
			return ISessionNegotiator::Auto;
		}
	}
	return ISessionNegotiator::Skip;
}

int ChatStates::sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit)
{
	int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ARequest.fields) : -1;
	if (index>=0)
	{
		int result = ISessionNegotiator::Auto;
		if (ARequest.type == DATAFORM_TYPE_FORM)
		{
			IDataField chatstates;
			chatstates.var = NS_CHATSTATES;
			chatstates.type = DATAFIELD_TYPE_LISTSINGLE;
			chatstates.value = ARequest.fields.at(index).value;
			chatstates.required = false;

			QStringList options;
			for (int i=0; i<ARequest.fields.at(index).options.count(); i++)
				options.append(ARequest.fields.at(index).options.at(i).value);

			int status = permitStatus(ASession.contactJid);
			bool enabled = isEnabled(ASession.contactJid);
			if ((!enabled && !options.contains(SFV_MUSTNOT_SEND)) || (status==IChatStates::StatusEnable && !options.contains(SFV_MAY_SEND)))
			{
				ASubmit.pages[0].fieldrefs.append(NS_CHATSTATES);
				ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
				result = ISessionNegotiator::Manual;
			}
			ASubmit.fields.append(chatstates);
		}
		else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
		{
			QString value = ARequest.fields.at(index).value.toString();
			int status = permitStatus(ASession.contactJid);
			bool enabled = isEnabled(ASession.contactJid);
			if ((!enabled && value==SFV_MAY_SEND) || (status==IChatStates::StatusEnable && value==SFV_MUSTNOT_SEND))
			{
				ASubmit.pages[0].fieldrefs.append(NS_CHATSTATES);
				ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
				result = ISessionNegotiator::Manual;
			}
		}
		return result;
	}
	return ISessionNegotiator::Skip;
}

int ChatStates::sessionApply(const IStanzaSession &ASession)
{
	int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ASession.form.fields) : -1;
	if (index >= 0)
	{
		QString value = ASession.form.fields.at(index).value.toString();
		FStanzaSessions[ASession.streamJid].insert(ASession.contactJid,value);
		if (value == SFV_MAY_SEND)
		{
			ChatParams &chatParams = FChatParams[ASession.streamJid][ASession.contactJid];
			chatParams.canSendStates = true;
			setSupported(ASession.streamJid,ASession.contactJid,true);
			sendStateMessage(Message::Chat,ASession.streamJid,ASession.contactJid,chatParams.self.state);
		}
		return ISessionNegotiator::Auto;
	}
	return ISessionNegotiator::Skip;
}

void ChatStates::sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm)
{
	Q_UNUSED(ASession);
	int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,AForm.fields) : -1;
	if (index >= 0)
	{
		AForm.fields[index].label = tr("Chat State Notifications");
		QList<IDataOption> &options = AForm.fields[index].options;
		for (int i=0; i<options.count(); i++)
		{
			if (options[i].value == SFV_MAY_SEND)
				options[i].label = tr("Allow Chat State Notifications");
			else if (options[i].value == SFV_MUSTNOT_SEND)
				options[i].label = tr("Disallow Chat State Notifications");
		}
	}
}

bool ChatStates::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessagesIn.value(AStreamJid)==AHandlerId && isReady(AStreamJid) && !AStanza.isError())
	{
		Message message(AStanza);
		bool hasBody = !message.body().isEmpty();
		if (!message.isDelayed())
		{
			if (message.type() != Message::GroupChat)
			{
				Jid contactJid = AStanza.from();
				QDomElement elem = AStanza.firstElement(QString::null,NS_CHATSTATES);
				if (!elem.isNull())
				{
					if (hasBody || FChatParams.value(AStreamJid).value(contactJid).canSendStates)
					{
						AAccept = true;
						setSupported(AStreamJid,contactJid,true);
						FChatParams[AStreamJid][contactJid].canSendStates = true;

						int state = stateTagToCode(elem.tagName());
						setChatUserState(AStreamJid,contactJid,state);
					}
				}
				else if (hasBody)
				{
					if (userChatState(AStreamJid,contactJid) != IChatStates::StateUnknown)
						setChatUserState(AStreamJid,contactJid,IChatStates::StateUnknown);
					setSupported(AStreamJid,contactJid,false);
				}
			}
			else
			{
				QDomElement elem = AStanza.firstElement(QString::null,NS_CHATSTATES);
				if (!elem.isNull())
				{
					AAccept = true;
					Jid roomJid = AStanza.from();
					int state = stateTagToCode(elem.tagName());
					setRoomUserState(AStreamJid,roomJid,state);
				}
			}
		}
		return !hasBody;
	}
	else if (FSHIMessagesOut.value(AStreamJid)==AHandlerId && isReady(AStreamJid) && !AStanza.isError())
	{
		Message message(AStanza);
		if (message.type() != Message::GroupChat)
		{
			Jid contactJid = AStanza.to();
			IMessageChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,contactJid,true) : NULL;
			if (window)
			{
				if (isSupported(AStreamJid,contactJid))
				{
					AStanza.addElement(STATE_ACTIVE,NS_CHATSTATES);
					FChatParams[AStreamJid][contactJid].canSendStates = true;
				}
				setChatSelfState(AStreamJid,contactJid,IChatStates::StateActive,false);
			}
		}
		else
		{
			Jid roomJid = AStanza.to();
			IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(AStreamJid,roomJid) : NULL;
			if (window)
			{
				AStanza.addElement(STATE_ACTIVE,NS_CHATSTATES);
				setRoomSelfState(AStreamJid,roomJid,IChatStates::StateActive,false);
			}
		}
	}

	return false;
}

bool ChatStates::isReady( const Jid &AStreamJid ) const
{
	return FChatParams.contains(AStreamJid);
}

int ChatStates::permitStatus(const Jid &AContactJid) const
{
	return FPermitStatus.value(AContactJid.bare(),IChatStates::StatusDefault);
}

void ChatStates::setPermitStatus(const Jid &AContactJid, int AStatus)
{
	if (permitStatus(AContactJid) != AStatus)
	{
		LOG_INFO(QString("Changing contact chat state permit status, contact=%1, status=%2").arg(AContactJid.bare(),AStatus));
		bool wasEnabled = isEnabled(AContactJid);

		Jid bareJid = AContactJid.bare();
		if (AStatus == IChatStates::StatusDisable)
			FPermitStatus.insert(bareJid,AStatus);
		else if (AStatus == IChatStates::StatusEnable)
			FPermitStatus.insert(bareJid,AStatus);
		else
			FPermitStatus.remove(bareJid);

		if (!wasEnabled && isEnabled(AContactJid))
			resetSupported(AContactJid);

		emit permitStatusChanged(bareJid,AStatus);
	}
}

bool ChatStates::isEnabled(const Jid &AContactJid, const Jid &AStreamJid) const
{
	if (AStreamJid.isValid())
	{
		QString svalue = FStanzaSessions.value(AStreamJid).value(AContactJid);
		if (svalue == SFV_MAY_SEND)
			return true;
		else if (svalue == SFV_MUSTNOT_SEND)
			return false;
	}

	int status = permitStatus(AContactJid);
	if (status == IChatStates::StatusDisable)
		return false;
	else if (status==IChatStates::StatusEnable)
		return true;

	return Options::node(OPV_MESSAGES_CHATSTATESENABLED).value().toBool();
}

bool ChatStates::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (!FStanzaSessions.value(AStreamJid).contains(AContactJid))
	{
		bool supported = !FNotSupported.value(AStreamJid).contains(AContactJid);
		if (FDiscovery && supported && userChatState(AStreamJid,AContactJid)==IChatStates::StateUnknown)
		{
			IDiscoInfo dinfo = FDiscovery->discoInfo(AStreamJid,AContactJid);
			supported = dinfo.streamJid!=AStreamJid || !dinfo.error.isNull() || dinfo.features.contains(NS_CHATSTATES);
		}
		return supported;
	}
	return true;
}

int ChatStates::userChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FChatParams.value(AStreamJid).value(AContactJid).user.state;
}

int ChatStates::selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FChatParams.value(AStreamJid).value(AContactJid).self.state;
}

int ChatStates::userRoomState(const Jid &AStreamJid, const Jid &AUserJid) const
{
	return FRoomParams.value(AStreamJid).value(AUserJid.bare()).user.value(AUserJid).state;
}

int ChatStates::selfRoomState(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	return FRoomParams.value(AStreamJid).value(ARoomJid.bare()).self.state;
}

void ChatStates::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.var = NS_CHATSTATES;
	dfeature.active = true;
	dfeature.name = tr("Chat State Notifications");
	dfeature.description = tr("Supports the exchanging of the information about the user's activity in the chat");
	FDiscovery->insertDiscoFeature(dfeature);
}

QString ChatStates::stateCodeToTag(int AState) const
{
	QString state;
	if (AState == IChatStates::StateActive)
		state = STATE_ACTIVE;
	else if (AState == IChatStates::StateComposing)
		state = STATE_COMPOSING;
	else if (AState == IChatStates::StatePaused)
		state = STATE_PAUSED;
	else if (AState == IChatStates::StateInactive)
		state = STATE_INACTIVE;
	else if (AState == IChatStates::StateGone)
		state = STATE_GONE;
	return state;
}

int ChatStates::stateTagToCode(const QString &AState) const
{
	int state = IChatStates::StateUnknown;
	if (AState == STATE_ACTIVE)
		state = IChatStates::StateActive;
	else if (AState == STATE_COMPOSING)
		state = IChatStates::StateComposing;
	else if (AState == STATE_PAUSED)
		state = IChatStates::StatePaused;
	else if (AState == STATE_INACTIVE)
		state = IChatStates::StateInactive;
	else if (AState == STATE_GONE)
		state = IChatStates::StateGone;
	return state;
}

bool ChatStates::sendStateMessage(Message::MessageType AType, const Jid &AStreamJid, const Jid &AContactJid, int AState) const
{
	if (FStanzaProcessor)
	{
		QString state = stateCodeToTag(AState);
		if (!state.isEmpty())
		{
			Message message;
			message.setType(AType).setTo(AContactJid.full());
			message.stanza().addElement(state,NS_CHATSTATES);
			return FStanzaProcessor->sendStanzaOut(AStreamJid,message.stanza());
		}
	}
	return false;
}

bool ChatStates::isChatCanSend(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (!isEnabled(AContactJid,AStreamJid))
		return false;
	if (!isSupported(AStreamJid,AContactJid))
		return false;
	if (!FChatParams.value(AStreamJid).value(AContactJid).canSendStates)
		return false;
	return true;
}

void ChatStates::resetSupported(const Jid &AContactJid)
{
	foreach (const Jid &streamJid, FNotSupported.keys())
	{
		foreach (const Jid &contactJid, FNotSupported.value(streamJid))
		{
			if (AContactJid.isEmpty() || AContactJid.pBare()==contactJid.pBare())
				setSupported(streamJid,contactJid,true);
		}
	}
}

void ChatStates::setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported)
{
	if (FNotSupported.contains(AStreamJid))
	{
		QList<Jid> &unsuppotred = FNotSupported[AStreamJid];
		int index = unsuppotred.indexOf(AContactJid);
		if (ASupported != (index<0))
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Changing contact chat state support status, contact=%1, supported=%2").arg(AContactJid.full()).arg(ASupported));
			if (ASupported)
				unsuppotred.removeAt(index);
			else
				unsuppotred.append(AContactJid);
			emit supportStatusChanged(AStreamJid,AContactJid,ASupported);
		}
	}
}

void ChatStates::setChatUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState)
{
	if (isReady(AStreamJid))
	{
		ChatParams &chatParams = FChatParams[AStreamJid][AContactJid];
		if (chatParams.user.state != AState)
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Contact chat state changed, contact=%1, state=%2").arg(AContactJid.full()).arg(AState));
			chatParams.user.state = AState;
			notifyChatState(AStreamJid,AContactJid);
			emit userChatStateChanged(AStreamJid,AContactJid,AState);
		}
	}
}

void ChatStates::setChatSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend)
{
	if (isReady(AStreamJid))
	{
		ChatParams &chatParams = FChatParams[AStreamJid][AContactJid];

		if (AState==IChatStates::StateActive || AState==IChatStates::StateComposing)
			chatParams.self.lastActive = QDateTime::currentDateTime().toTime_t();

		if (chatParams.self.state != AState)
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Self chat state changed, contact=%1, state=%2").arg(AContactJid.full()).arg(AState));
			chatParams.self.state = AState;
			if (ASend && isChatCanSend(AStreamJid,AContactJid))
				sendStateMessage(Message::Chat,AStreamJid,AContactJid,AState);
			emit selfChatStateChanged(AStreamJid,AContactJid,AState);
		}
	}
}

void ChatStates::notifyChatState(const Jid &AStreamJid, const Jid &AContactJid)
{
	IMessageChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AContactJid) : NULL;
	if (FNotifications && window)
	{
		ChatParams &chatParams = FChatParams[AStreamJid][AContactJid];
		if (chatParams.user.state==IChatStates::StateComposing && chatParams.user.notifyId<=0)
		{
			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_CHATSTATE_TYPING);
			if (notify.kinds > 0)
			{
				notify.typeId = NNT_CHATSTATE_TYPING;
				notify.data.insert(NDR_STREAM_JID,AStreamJid.full());
				notify.data.insert(NDR_CONTACT_JID,AContactJid.full());
				notify.data.insert(NDR_ICON, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATSTATES_COMPOSING));
				notify.data.insert(NDR_TOOLTIP,tr("Typing a message..."));
				notify.data.insert(NDR_ROSTER_ORDER,RNO_CHATSTATE_TYPING);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::AllwaysVisible);
				notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_CHATSTATE_TYPING);
				notify.data.insert(NDR_TABPAGE_ICONBLINK,false);
				chatParams.user.notifyId = FNotifications->appendNotification(notify);
			}
		}
		else if (chatParams.user.state!=IChatStates::StateComposing && chatParams.user.notifyId>0)
		{
			FNotifications->removeNotification(chatParams.user.notifyId);
			chatParams.user.notifyId = 0;
		}
	}
}

bool ChatStates::isRoomCanSend(const Jid &AStreamJid, const Jid &ARoomJid) const
{
	IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(AStreamJid,ARoomJid) : NULL;
	if (window == NULL)
		return false;
	if (!isEnabled(ARoomJid))
		return false;
	if (!window->multiUserChat()->isOpen())
		return false;
	if (window->multiUserChat()->mainUser()->role() == MUC_ROLE_VISITOR)
		return false;
	return true;
}

void ChatStates::setRoomUserState(const Jid &AStreamJid, const Jid &AUserJid, int AState)
{
	if (isReady(AStreamJid) && AUserJid.hasResource())
	{
		RoomParams &roomParams = FRoomParams[AStreamJid][AUserJid.bare()];
		UserParams &userParams = roomParams.user[AUserJid];
		
		if (userParams.state != AState)
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Room user chat state changed, user=%1, state=%2").arg(AUserJid.full()).arg(AState));
			userParams.state = AState;
			notifyUserState(AStreamJid,AUserJid);
			emit userRoomStateChanged(AStreamJid,AUserJid,AState);
		}

		if (roomParams.delayedSend)
		{
			roomParams.waitEcho = sendStateMessage(Message::GroupChat,AStreamJid,AUserJid.bare(),roomParams.self.state);
			roomParams.delayedSend = false;
		}
		else
		{
			roomParams.waitEcho = false;
		}
	}
}

void ChatStates::setRoomSelfState(const Jid &AStreamJid, const Jid &ARoomJid, int AState, bool ASend)
{
	if (isReady(AStreamJid) && !ARoomJid.hasResource())
	{
		RoomParams &roomParams = FRoomParams[AStreamJid][ARoomJid];

		if (AState==IChatStates::StateActive || AState==IChatStates::StateComposing)
			roomParams.self.lastActive = QDateTime::currentDateTime().toTime_t();

		if (roomParams.self.state != AState)
		{
			LOG_STRM_DEBUG(AStreamJid,QString("Room self state changed, room=%1, state=%2").arg(ARoomJid.full()).arg(AState));
			roomParams.self.state = AState;
			if (ASend && isRoomCanSend(AStreamJid,ARoomJid))
			{
				if (!roomParams.waitEcho)
					roomParams.waitEcho = sendStateMessage(Message::GroupChat,AStreamJid,ARoomJid,AState);
				else
					roomParams.delayedSend = true;
			}
			else
			{
				roomParams.delayedSend = false;
			}
			emit selfRoomStateChanged(AStreamJid,ARoomJid,AState);
		}
	}
}

void ChatStates::notifyUserState(const Jid &AStreamJid, const Jid &AUserJid)
{
	IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(AStreamJid,AUserJid.bare()) : NULL;
	if (window)
	{
		IMultiUser *user = window->multiUserChat()->findUser(AUserJid.resource());
		if (user != window->multiUserChat()->mainUser())
		{
			UserParams &userParams = FRoomParams[window->streamJid()][window->contactJid()].user[AUserJid];
			if (userParams.state==IChatStates::StateComposing && userParams.notifyId<=0)
			{
				QStandardItem *userItem = window->multiUserView()->findUserItem(user);
				if (userItem)
				{
					IMultiUserViewNotify notify;
					notify.order = MUNO_CHATSTATE_TYPING;
					notify.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATSTATES_COMPOSING);
					userParams.notifyId = window->multiUserView()->insertItemNotify(notify,userItem);
					notifyRoomState(AStreamJid,AUserJid.bare());
				}
			}
			else if (userParams.state!=IChatStates::StateComposing && userParams.notifyId>0)
			{
				window->multiUserView()->removeItemNotify(userParams.notifyId);
				userParams.notifyId = 0;
				notifyRoomState(AStreamJid,AUserJid.bare());
			}
		}
	}
}

void ChatStates::notifyRoomState(const Jid &AStreamJid, const Jid &ARoomJid)
{
	IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(AStreamJid,ARoomJid) : NULL;
	if (FNotifications && window)
	{
		RoomParams &roomParams = FRoomParams[AStreamJid][ARoomJid];

		bool hasNotify = false;
		if (!Options::node(OPV_MUC_GROUPCHAT_ITEM,ARoomJid.pBare()).node("notify-silence").value().toBool())
		{
			foreach(const UserParams &userParams, roomParams.user)
			{
				if (userParams.notifyId > 0)
				{
					hasNotify = true;
					break;
				}
			}
		}

		if (hasNotify && roomParams.notifyId<=0)
		{
			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_CHATSTATE_TYPING);
			if (notify.kinds > 0)
			{
				QMap<QString,QVariant> searchData;
				searchData.insert(QString::number(RDR_STREAM_JID),AStreamJid.pFull());
				searchData.insert(QString::number(RDR_KIND),RIK_MUC_ITEM);
				searchData.insert(QString::number(RDR_PREP_BARE_JID),ARoomJid.pBare());

				notify.typeId = NNT_CHATSTATE_TYPING;
				notify.data.insert(NDR_ROSTER_SEARCH_DATA,searchData);
				notify.data.insert(NDR_ICON, IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHATSTATES_COMPOSING));
				notify.data.insert(NDR_TOOLTIP,tr("Typing a message..."));
				notify.data.insert(NDR_ROSTER_ORDER,RNO_CHATSTATE_TYPING);
				notify.data.insert(NDR_ROSTER_FLAGS,IRostersNotify::AllwaysVisible);
				notify.data.insert(NDR_TABPAGE_WIDGET,(qint64)window->instance());
				notify.data.insert(NDR_TABPAGE_PRIORITY,TPNP_CHATSTATE_TYPING);
				notify.data.insert(NDR_TABPAGE_ICONBLINK,false);
				roomParams.notifyId = FNotifications->appendNotification(notify);
			}
		}
		else if (!hasNotify && roomParams.notifyId>0)
		{
			FNotifications->removeNotification(roomParams.notifyId);
			roomParams.notifyId = 0;
		}
	}
}

void ChatStates::onPresenceOpened(IPresence *APresence)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = APresence->streamJid();

		shandle.direction = IStanzaHandle::DirectionOut;
		shandle.order = SHO_MO_CHATSTATES;
		shandle.conditions = QList<QString>() << SHC_OUT_MESSAGES;
		FSHIMessagesOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.order = SHO_MI_CHATSTATES;
		shandle.conditions = QList<QString>() << SHC_IN_MESSAGES;
		FSHIMessagesIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	FNotSupported[APresence->streamJid()].clear();
	FChatParams[APresence->streamJid()].clear();
	FRoomParams[APresence->streamJid()].clear();
}

void ChatStates::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	bool isNowOnline = AItem.show!=IPresence::Offline && AItem.show!=IPresence::Error;
	bool isWasOnline = ABefore.show!=IPresence::Offline && ABefore.show!=IPresence::Error;
	if (isNowOnline && !isWasOnline)
	{
		setSupported(APresence->streamJid(),AItem.itemJid,true);
	}
	else if (!isNowOnline && isWasOnline)
	{
		if (FChatParams.value(APresence->streamJid()).contains(AItem.itemJid))
			setChatUserState(APresence->streamJid(),AItem.itemJid,IChatStates::StateGone);
	}
}

void ChatStates::onPresenceClosed(IPresence *APresence)
{
	foreach(const Jid &contactJid, FChatParams.value(APresence->streamJid()).keys())
	{
		setChatUserState(APresence->streamJid(),contactJid,IChatStates::StateUnknown);
		setChatSelfState(APresence->streamJid(),contactJid,IChatStates::StateUnknown,false);
	}

	foreach(const Jid &roomJid, FRoomParams.value(APresence->streamJid()).keys())
	{
		foreach(const Jid &userJid, FRoomParams.value(APresence->streamJid()).value(roomJid).user.keys())
			setRoomUserState(APresence->streamJid(),userJid,IChatStates::StateUnknown);
		setRoomSelfState(APresence->streamJid(),roomJid,IChatStates::StateUnknown,false);
	}

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMessagesIn.take(APresence->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessagesOut.take(APresence->streamJid()));
	}

	FNotSupported.remove(APresence->streamJid());
	FChatParams.remove(APresence->streamJid());
	FRoomParams.remove(APresence->streamJid());
	FStanzaSessions.remove(APresence->streamJid());
}

void ChatStates::onChatWindowCreated(IMessageChatWindow *AWindow)
{
	StateWidget *widget = new StateWidget(this,AWindow,AWindow->toolBarWidget()->toolBarChanger()->toolBar());
	AWindow->toolBarWidget()->toolBarChanger()->insertWidget(widget,TBG_MWTBW_CHATSTATES);
	widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	widget->setPopupMode(QToolButton::InstantPopup);

	connect(AWindow->instance(),SIGNAL(tabPageActivated()),SLOT(onChatWindowActivated()));
	connect(AWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onChatWindowTextChanged()));
	FChatByEditor.insert(AWindow->editWidget()->textEdit(),AWindow);
}

void ChatStates::onChatWindowActivated()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		int state = selfChatState(window->streamJid(),window->contactJid());
		if (state == IChatStates::StatePaused)
			setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
		else if (state != IChatStates::StateComposing)
			setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onChatWindowTextChanged()
{
	QTextEdit *editor = qobject_cast<QTextEdit *>(sender());
	IMessageChatWindow *window = FChatByEditor.value(editor);
	if (editor && window)
	{
		if (!editor->document()->isEmpty())
			setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
		else
			setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onChatWindowDestroyed(IMessageChatWindow *AWindow)
{
	setChatSelfState(AWindow->streamJid(),AWindow->contactJid(),IChatStates::StateGone);
	FChatByEditor.remove(AWindow->editWidget()->textEdit());
}

void ChatStates::onMultiChatWindowCreated(IMultiUserChatWindow *AWindow)
{
	StateWidget *widget = new StateWidget(this,AWindow,AWindow->toolBarWidget()->toolBarChanger()->toolBar());
	AWindow->toolBarWidget()->toolBarChanger()->insertWidget(widget,TBG_MWTBW_CHATSTATES);
	widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	widget->setPopupMode(QToolButton::InstantPopup);

	connect(AWindow->instance(),SIGNAL(tabPageActivated()),SLOT(onMultiChatWindowActivated()));
	connect(AWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onMultiChatWindowTextChanged()));
	connect(AWindow->multiUserChat()->instance(),SIGNAL(userChanged(IMultiUser *, int, const QVariant &)),
		SLOT(onMultiChatUserChanged(IMultiUser *, int, const QVariant &)));
	FRoomByEditor.insert(AWindow->editWidget()->textEdit(),AWindow);
}

void ChatStates::onMultiChatWindowActivated()
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window)
	{
		int state = selfRoomState(window->streamJid(),window->contactJid());
		if (state == IChatStates::StatePaused)
			setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
		else if (state != IChatStates::StateComposing)
			setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onMultiChatWindowTextChanged()
{
	QTextEdit *editor = qobject_cast<QTextEdit *>(sender());
	IMultiUserChatWindow *window = FRoomByEditor.value(editor);
	if (editor && window)
	{
		if (!editor->document()->isEmpty())
			setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
		else
			setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
	}
}

void ChatStates::onMultiChatUserChanged(IMultiUser *AUser, int AData, const QVariant &ABefore)
{
	if (AData == MUDR_PRESENCE)
	{
		if (AUser->presence().show==IPresence::Offline || AUser->presence().show==IPresence::Error)
		{
			IMultiUserChat *multiChat = qobject_cast<IMultiUserChat *>(sender());
			if (multiChat!=NULL && isEnabled(multiChat->streamJid()))
			{
				setChatUserState(multiChat->streamJid(),AUser->userJid(),IChatStates::StateUnknown);
				setChatSelfState(multiChat->streamJid(),AUser->userJid(),IChatStates::StateUnknown,false);
				FChatParams[multiChat->streamJid()].remove(AUser->userJid());

				setRoomUserState(multiChat->streamJid(),AUser->userJid(),IChatStates::StateUnknown);
				FRoomParams[multiChat->streamJid()][multiChat->roomJid()].user.remove(AUser->userJid());
			}
		}
	}
	else if (AData == MUDR_NICK)
	{
		Jid beforeJid = AUser->userJid();
		beforeJid.setResource(ABefore.toString());
		IMultiUserChat *multiChat = qobject_cast<IMultiUserChat *>(sender());
		if (multiChat!=NULL && FRoomParams.value(multiChat->streamJid()).value(multiChat->roomJid()).user.contains(beforeJid))
		{
			UserParams userParam = FRoomParams[multiChat->streamJid()][multiChat->roomJid()].user.take(beforeJid);
			FRoomParams[multiChat->streamJid()][multiChat->roomJid()].user.insert(AUser->userJid(),userParam);
		}
	}
}

void ChatStates::onMultiChatWindowDestroyed(IMultiUserChatWindow *AWindow)
{
	if (isEnabled(AWindow->streamJid()))
	{
		setRoomSelfState(AWindow->streamJid(),AWindow->contactJid(),IChatStates::StateUnknown,false);
		FRoomParams[AWindow->streamJid()].remove(AWindow->contactJid());
	}
	FRoomByEditor.remove(AWindow->editWidget()->textEdit());
}

void ChatStates::onUpdateSelfStates()
{
	QList<IMessageChatWindow *> chatWindows = FMessageWidgets!=NULL ? FMessageWidgets->chatWindows() : QList<IMessageChatWindow *>();
	foreach (IMessageChatWindow *window, chatWindows)
	{
		if (FChatParams.value(window->streamJid()).contains(window->contactJid()))
		{
			ChatParams &chatParams = FChatParams[window->streamJid()][window->contactJid()];
			uint timePassed = QDateTime::currentDateTime().toTime_t() - chatParams.self.lastActive;
			if (chatParams.self.state==IChatStates::StateActive && window->isActiveTabPage())
			{
				setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
			}
			else if (chatParams.self.state==IChatStates::StateComposing && timePassed>=PAUSED_TIMEOUT)
			{
				setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StatePaused);
			}
			else if (chatParams.self.state==IChatStates::StateActive && timePassed>=INACTIVE_TIMEOUT)
			{
				setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
			}
			else if (chatParams.self.state==IChatStates::StatePaused && timePassed>=INACTIVE_TIMEOUT)
			{
				setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
			}
			else if (chatParams.self.state==IChatStates::StateInactive && timePassed>GONE_TIMEOUT)
			{
				setChatSelfState(window->streamJid(),window->contactJid(),IChatStates::StateGone);
			}
		}
	}

	QList<IMultiUserChatWindow *> roomWindows = FMultiChatManager!=NULL ? FMultiChatManager->multiChatWindows() : QList<IMultiUserChatWindow *>();
	foreach(IMultiUserChatWindow *window, roomWindows)
	{
		if (FRoomParams.value(window->streamJid()).contains(window->contactJid()))
		{
			RoomParams &roomParams = FRoomParams[window->streamJid()][window->contactJid()];
			uint timePassed = QDateTime::currentDateTime().toTime_t() - roomParams.self.lastActive;
			if (roomParams.self.state==IChatStates::StateActive && window->isActiveTabPage())
			{
				setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
			}
			else if (roomParams.self.state==IChatStates::StateComposing && timePassed>=PAUSED_TIMEOUT)
			{
				setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StatePaused);
			}
			else if (roomParams.self.state==IChatStates::StateActive && timePassed>=INACTIVE_TIMEOUT)
			{
				setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
			}
			else if (roomParams.self.state==IChatStates::StatePaused && timePassed>=INACTIVE_TIMEOUT)
			{
				setRoomSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
			}
		}
	}
}

void ChatStates::onOptionsOpened()
{
	QByteArray data = Options::fileValue("messages.chatstates.permit-status").toByteArray();
	QDataStream stream(data);
	stream >> FPermitStatus;

	onOptionsChanged(Options::node(OPV_MESSAGES_CHATSTATESENABLED));
}

void ChatStates::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FPermitStatus;
	Options::setFileValue(data,"messages.chatstates.permit-status");
}

void ChatStates::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_CHATSTATESENABLED)
	{
		if (ANode.value().toBool())
			resetSupported();
	}
}

void ChatStates::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
	FStanzaSessions[ASession.streamJid].remove(ASession.contactJid);
}
