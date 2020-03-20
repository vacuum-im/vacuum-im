#include "multiuserchat.h"

#include <definitions/namespaces.h>
#include <definitions/dataformtypes.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/messageeditororders.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

#define SHC_PRESENCE        "/presence"
#define SHC_MESSAGE         "/message"

#define MUC_IQ_TIMEOUT      30000
#define MUC_LIST_TIMEOUT    60000

#define DIC_CONFERENCE      "conference"

MultiUserChat::MultiUserChat(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANickName, const QString &APassword, bool AIsolated, QObject *AParent) : QObject(AParent)
{
	FSHIMessage = -1;
	FSHIPresence = -1;

	FMainUser = NULL;
	FAutoPresence = false;
	FResendPresence = false;
	FState = IMultiUserChat::Closed;

	FIsolated = AIsolated;
	FRoomJid = ARoomJid;
	FStreamJid = AStreamJid;
	FNickname = ANickName;
	FPassword = APassword;

	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_PI_MULTIUSERCHAT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = FStreamJid;
		shandle.conditions.append(SHC_PRESENCE);
		FSHIPresence = FStanzaProcessor->insertStanzaHandle(shandle);
	}

	if (FIsolated && FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_MI_MULTIUSERCHAT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.streamJid = FStreamJid;
		shandle.conditions.append(SHC_MESSAGE);
		FSHIMessage = FStanzaProcessor->insertStanzaHandle(shandle);
	}
	else if (!FIsolated && FMessageProcessor)
	{
		FMessageProcessor->insertMessageEditor(MEO_MULTIUSERCHAT,this);
	}

	if (FPresenceManager)
	{
		connect(FPresenceManager->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),SLOT(onPresenceChanged(IPresence *, int, const QString &, int)));
	}

	if (FXmppStreamManager)
	{
		connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		connect(FXmppStreamManager->instance(),SIGNAL(streamJidChanged(IXmppStream *,const Jid &)),SLOT(onXmppStreamJidChanged(IXmppStream *,const Jid &)));
	}

	if (FDiscovery)
	{
		connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoveryInfoReceived(const IDiscoInfo &)));
	}
}

MultiUserChat::~MultiUserChat()
{
	abortConnection(QString(),false);
	
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIMessage);
		FStanzaProcessor->removeStanzaHandle(FSHIPresence);
	}

	if (FMessageProcessor)
	{
		FMessageProcessor->removeMessageEditor(MEO_MULTIUSERCHAT,this);
	}

	emit chatDestroyed();
}

bool MultiUserChat::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AStreamJid==FStreamJid && FRoomJid.pBare()==Jid(AStanza.from()).pBare())
	{
		AAccept = true;
		if (AHandlerId == FSHIPresence)
			processPresence(AStanza);
		else if (AHandlerId == FSHIMessage)
			processMessage(AStanza);
		return true;
	}
	return false;
}

void MultiUserChat::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FRoleUpdateId.contains(AStanza.id()))
	{
		QString nick = FRoleUpdateId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("User role updated, nick=%1, id=%2, room=%3").arg(nick,AStanza.id(),FRoomJid.bare()));
			emit userRoleUpdated(AStanza.id(), nick);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to update user role, nick=%1, id=%2, room=%3: %4").arg(nick,AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(), err);
		}
	}
	else if (FAffilUpdateId.contains(AStanza.id()))
	{
		QString nick = FAffilUpdateId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("User affiliation updated, nick=%1, id=%2, room=%3").arg(nick,AStanza.id(),FRoomJid.bare()));
			emit userAffiliationUpdated(AStanza.id(),nick);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to update user affiliation, nick=%1, id=%2, room=%3: %4").arg(nick,AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(), err);
		}
	}
	else if (FAffilListLoadId.contains(AStanza.id()))
	{
		QString affiliation = FAffilListLoadId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference affiliation list loaded, affiliation=%1, id=%2, room=%3").arg(affiliation,AStanza.id(),FRoomJid.bare()));

			QList<IMultiUserListItem> items;
			QDomElement itemElem = AStanza.firstElement("query",NS_MUC_ADMIN).firstChildElement("item");
			while (!itemElem.isNull())
			{
				if (itemElem.attribute("affiliation") == affiliation)
				{
					IMultiUserListItem item;
					item.realJid = itemElem.attribute("jid");
					item.affiliation = itemElem.attribute("affiliation");
					item.reason = itemElem.firstChildElement("reason").text();
					items.append(item);
				}
				itemElem = itemElem.nextSiblingElement("item");
			}
			emit affiliationListLoaded(AStanza.id(), items);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load conference affiliation list, affiliation=%1, id=%2, room=%3: %4").arg(affiliation,AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(), err);
		}
	}
	else if (FAffilListUpdateId.contains(AStanza.id()))
	{
		QList<IMultiUserListItem> items = FAffilListUpdateId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference affiliation list updated, id=%1, room=%2").arg(AStanza.id(),FRoomJid.bare()));
			emit affiliationListUpdated(AStanza.id(), items);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to update conference affiliation list, id=%1, room=%2: %3").arg(AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(), err);
		}
	}
	else if (FConfigLoadId.contains(AStanza.id()))
	{
		FConfigLoadId.removeAll(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference configuration loaded, id=%1, room=%2").arg(AStanza.id(),FRoomJid.bare()));
			QDomElement formElem = AStanza.firstElement("query",NS_MUC_OWNER).firstChildElement("x");
			while (formElem.namespaceURI() != NS_JABBER_DATA)
				formElem = formElem.nextSiblingElement("x");
			emit roomConfigLoaded(AStanza.id(),FDataForms->dataForm(formElem));
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load conference configuration, id=%1, room=%2: %3").arg(AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(),err);
		}
	}
	else if (FConfigUpdateId.contains(AStanza.id()))
	{
		IDataForm form = FConfigUpdateId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference configuration updated, id=%1, room=%2").arg(AStanza.id(),FRoomJid.bare()));
			emit roomConfigUpdated(AStanza.id(), form);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to update conference configuration, id=%1, room=%2: %3").arg(AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(),err);
		}
	}
	else if (FRoomDestroyId.contains(AStanza.id()))
	{
		QString reason = FRoomDestroyId.take(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference destroyed, id=%1, room=%2").arg(AStanza.id(),FRoomJid.bare()));
			emit roomDestroyed(AStanza.id(),reason);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to destroy conference, id=%1, room=%2: %3").arg(AStanza.id(),FRoomJid.bare(),err.condition()));
			emit requestFailed(AStanza.id(),err);
		}
	}
}

bool MultiUserChat::messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (AOrder==MEO_MULTIUSERCHAT && ADirection==IMessageProcessor::DirectionIn && AStreamJid==FStreamJid)
	{
		if (FRoomJid.pBare() == AMessage.fromJid().pBare())
			return processMessage(AMessage.stanza());
	}
	return false;
}

Jid MultiUserChat::streamJid() const
{
	return FStreamJid;
}

Jid MultiUserChat::roomJid() const
{
	return FRoomJid;
}

QString MultiUserChat::roomName() const
{
	return FRoomJid.uNode();
}

QString MultiUserChat::roomTitle() const
{
	return !FRoomTitle.isEmpty() ? FRoomTitle : roomName();
}

int MultiUserChat::state() const
{
	return FState;
}

bool MultiUserChat::isOpen() const
{
	return FState==IMultiUserChat::Opened;
}

bool MultiUserChat::isIsolated() const
{
	return FIsolated;
}

bool MultiUserChat::autoPresence() const
{
	return FAutoPresence;
}

void MultiUserChat::setAutoPresence(bool AAuto)
{
	FAutoPresence = AAuto;
}

QList<int> MultiUserChat::statusCodes() const
{
	return FStatusCodes;
}

QList<int> MultiUserChat::statusCodes(const Stanza &AStanza) const
{
	QList<int> codes;
	QDomElement statusElem =  AStanza.firstElement("x",NS_MUC_USER).firstChildElement("status");
	while (!statusElem.isNull())
	{
		codes.append(statusElem.attribute("code").toInt());
		statusElem = statusElem.nextSiblingElement("status");
	}
	return codes;
}

XmppError MultiUserChat::roomError() const
{
	return FRoomError;
}

IPresenceItem MultiUserChat::roomPresence() const
{
	return FRoomPresence;
}

IMultiUser *MultiUserChat::mainUser() const
{
	return FMainUser;
}

QList<IMultiUser *> MultiUserChat::allUsers() const
{
	QList<IMultiUser *> users;
	users.reserve(FUsers.count());

	foreach(MultiUser *user, FUsers)
		users.append(user);

	return users;
}

IMultiUser *MultiUserChat::findUser(const QString &ANick) const
{
	return FUsers.value(ANick);
}

IMultiUser *MultiUserChat::findUserByRealJid(const Jid &ARealJid) const
{
	foreach(MultiUser *user, FUsers)
		if (ARealJid == user->realJid())
			return user;
	return NULL;
}

bool MultiUserChat::isUserPresent(const Jid &AContactJid) const
{
	return AContactJid.pBare()!=FRoomJid.pBare() ? findUserByRealJid(AContactJid)!=NULL : FUsers.contains(AContactJid.resource());
}

void MultiUserChat::abortConnection(const QString &AStatus, bool AError)
{
	if (FState != IMultiUserChat::Closed)
	{
		LOG_STRM_INFO(FStreamJid,QString("Aborting conference connection, status=%1, room=%2").arg(AStatus,FRoomJid.bare()));

		IPresenceItem presence;
		presence.itemJid = FMainUser!=NULL ? FMainUser->userJid() : FRoomJid;
		presence.show = AError ? IPresence::Error : IPresence::Offline;
		presence.status = AStatus;
		closeRoom(presence);
	}
}

QString MultiUserChat::nickname() const
{
	return FNickname;
}

bool MultiUserChat::setNickname(const QString &ANick)
{
	if (!ANick.isEmpty())
	{
		if (isOpen())
		{
			if (FNickname!=ANick && FRequestedNick.isEmpty())
			{
				Stanza presence = makePresenceStanza(ANick,FRoomPresence.show,FRoomPresence.status,FRoomPresence.priority);
				if (FStanzaProcessor!=NULL && FStanzaProcessor->sendStanzaOut(FStreamJid,presence))
				{
					FRequestedNick = ANick;
					LOG_STRM_INFO(FStreamJid,QString("Change conference nick request sent, room=%1, old=%2, new=%3").arg(FRoomJid.bare(),FNickname,ANick));
					return true;
				}
				else
				{
					LOG_STRM_WARNING(FStreamJid,QString("Failed to send change conference nick request, room=%1").arg(FRoomJid.bare()));
				}
			}
			else if (FNickname == ANick)
			{
				LOG_STRM_WARNING(FStreamJid,QString("Received mirrored change conference nick request, room=%1").arg(FRoomJid.bare()));
				return true;
			}
			else if (FRequestedNick == ANick)
			{
				LOG_STRM_WARNING(FStreamJid,QString("Received duplicate change conference nick request, room=%1").arg(FRoomJid.bare()));
				return true;
			}
			else if (!FRequestedNick.isEmpty())
			{
				LOG_STRM_ERROR(FStreamJid,QString("Failed to change conference nick, room=%1: Previous change nick request in progress").arg(FRoomJid.bare()));
			}
		}
		else if (FState == IMultiUserChat::Closed)
		{
			FNickname = ANick;
			emit nicknameChanged(FNickname,XmppError::null);
			return true;
		}
		else
		{
			LOG_STRM_ERROR(FStreamJid,QString("Failed to change conference nick, room=%1: Intermediate chat state").arg(FRoomJid.bare()));
		}
	}
	else
	{
		REPORT_ERROR("Failed to change conference nick: Nick is empty");
	}
	return false;
}

QString MultiUserChat::password() const
{
	return FPassword;
}

void MultiUserChat::setPassword(const QString &APassword)
{
	if (FPassword != APassword)
	{
		LOG_STRM_INFO(FStreamJid,QString("Conference password changed, room=%1").arg(FRoomJid.bare()));
		FPassword = APassword;
		emit passwordChanged(FPassword);
	}
}

IMultiUserChatHistory MultiUserChat::historyScope() const
{
	return FHistory;
}

void MultiUserChat::setHistoryScope(const IMultiUserChatHistory &AHistory)
{
	FHistory = AHistory;
}

bool MultiUserChat::sendStreamPresence()
{
	IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(FStreamJid) : NULL;
	return presence!=NULL ? sendPresence(presence->show(),presence->status(),presence->priority()) : false;
}

bool MultiUserChat::sendPresence(int AShow, const QString &AStatus, int APriority)
{
	if (FStanzaProcessor)
	{
		Stanza presence = makePresenceStanza(FNickname,AShow,AStatus,APriority);
		bool isOnline = (presence.type()!=PRESENCE_TYPE_UNAVAILABLE);

		if (FState==IMultiUserChat::Closed && isOnline)
		{
			FRoomError = XmppError::null;

			QDomElement xelem = presence.addElement("x",NS_MUC);
			if (FHistory.empty || FHistory.maxChars>0 || FHistory.maxStanzas>0 || FHistory.seconds>0 || FHistory.since.isValid())
			{
				QDomElement histElem = xelem.appendChild(presence.createElement("history")).toElement();
				if (!FHistory.empty)
				{
					if (FHistory.maxChars > 0)
						histElem.setAttribute("maxchars",FHistory.maxChars);
					if (FHistory.maxStanzas > 0)
						histElem.setAttribute("maxstanzas",FHistory.maxStanzas);
					if (FHistory.seconds > 0)
						histElem.setAttribute("seconds",FHistory.seconds);
					if (FHistory.since.isValid())
						histElem.setAttribute("since",DateTime(FHistory.since).toX85UTC());
				}
				else
				{
					histElem.setAttribute("maxchars",0);
				}
			}

			if (!FPassword.isEmpty())
				xelem.appendChild(presence.createElement("password")).appendChild(presence.createTextNode(FPassword));

			if (FDiscovery)
			{
				if (!FDiscovery->hasDiscoInfo(streamJid(),roomJid()))
					FDiscovery->requestDiscoInfo(streamJid(),roomJid());
				else
					onDiscoveryInfoReceived(FDiscovery->discoInfo(streamJid(),roomJid()));
			}
		}

		if (FState==IMultiUserChat::Closed && !isOnline)
		{
			// Do not send offline presences if room is already closed
		}
		else if (FState==IMultiUserChat::Opening && isOnline)
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send presence to conference, show=%1, room=%2: Room is in opening state").arg(AShow).arg(FRoomJid.bare()));
		}
		else if (FState == IMultiUserChat::Closing)
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send presence to conference, show=%1, room=%2: Room is in closing state").arg(AShow).arg(FRoomJid.bare()));
		}
		else if (!FStanzaProcessor->sendStanzaOut(FStreamJid,presence))
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send presence to conference, show=%1, room=%2").arg(AShow).arg(FRoomJid.bare()));
		}
		else
		{
			LOG_STRM_INFO(FStreamJid,QString("Presence sent to conference, show=%1, room=%2").arg(AShow).arg(FRoomJid.bare()));

			if (FState==IMultiUserChat::Closed && isOnline)
				setState(IMultiUserChat::Opening);
			else if (FState==IMultiUserChat::Opened && !isOnline)
				setState(IMultiUserChat::Closing);

			return true;
		}
	}
	return false;
}

bool MultiUserChat::sendMessage(const Message &AMessage, const QString &AToNick)
{
	if (isOpen())
	{
		Jid toJid = FRoomJid;
		toJid.setResource(AToNick);

		Message message = AMessage;
		message.setTo(toJid.full()).setType(AToNick.isEmpty() ? Message::GroupChat : Message::Chat);

		if (FIsolated && FStanzaProcessor && FStanzaProcessor->sendStanzaOut(FStreamJid,message.stanza()))
		{
			emit messageSent(message);
			return true;
		}
		else if (!FIsolated && FMessageProcessor && FMessageProcessor->sendMessage(FStreamJid,message,IMessageProcessor::DirectionOut))
		{
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send message to conference, room=%1").arg(FRoomJid.bare()));
		}
	}
	else
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send message to conference, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return false;
}

bool MultiUserChat::sendInvitation(const QList<Jid> &AContacts, const QString &AReason, const QString &AThread)
{
	if (FStanzaProcessor && isOpen() && !AContacts.isEmpty())
	{
		// NOTE: Sending each invite in separate message due to ejabberd is not supported multi invite

		Stanza invite(STANZA_KIND_MESSAGE);
		invite.setTo(FRoomJid.bare());

		QDomElement xElem = invite.addElement("x",NS_MUC_USER);
		QDomElement inviteElem = xElem.appendChild(invite.createElement("invite")).toElement();
		
		if (!AReason.isEmpty())
			inviteElem.appendChild(invite.createElement("reason")).appendChild(invite.createTextNode(AReason));
	
		if (!AThread.isEmpty())
			inviteElem.appendChild(invite.createElement("continue")).toElement().setAttribute("thread",AThread);
		else if (!AThread.isNull())
			inviteElem.appendChild(invite.createElement("continue"));

		QList<Jid> invited;
		foreach(const Jid &contact, AContacts)
		{
			if (!invited.contains(contact) && !isUserPresent(contact))
			{
				inviteElem.setAttribute("to",contact.full());
				if (FStanzaProcessor->sendStanzaOut(FStreamJid, invite))
					invited.append(contact);
				else
					LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference invite to=%1, room=%2").arg(contact.full(),FRoomJid.bare()));
			}
		}

		if (!invited.isEmpty())
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference invite sent to room=%1, contacts=%2").arg(FRoomJid.bare()).arg(invited.count()));
			emit invitationSent(invited,AReason,AThread);
			return true;
		}
	}
	else if (FStanzaProcessor && !isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference invite to room=%1, contact=%2: Conference is closed").arg(FRoomJid.bare()).arg(AContacts.count()));
	}
	return false;
}

bool MultiUserChat::sendDirectInvitation(const QList<Jid> &AContacts, const QString &AReason, const QString &AThread)
{
	if (FStanzaProcessor && isOpen() && !AContacts.isEmpty())
	{
		Stanza invite(STANZA_KIND_MESSAGE);

		QDomElement xElem = invite.addElement("x",NS_JABBER_X_CONFERENCE);
		xElem.setAttribute("jid",FRoomJid.bare());

		if (!AReason.isEmpty())
			xElem.setAttribute("reason",AReason);

		if (!FPassword.isEmpty())
			xElem.setAttribute("password",FPassword);

		if (!AThread.isEmpty())
		{
			xElem.setAttribute("continue",true);
			xElem.setAttribute("thread",AThread);
		}
		else if (!AThread.isNull())
		{
			xElem.setAttribute("continue",true);
		}

		QList<Jid> invited;
		foreach(const Jid &contactJid, AContacts)
		{
			if (!invited.contains(contactJid))
			{
				invite.setTo(contactJid.full());
				if (FStanzaProcessor->sendStanzaOut(FStreamJid, invite))
					invited.append(contactJid);
				else
					LOG_STRM_WARNING(FStreamJid,QString("Failed to send direct conference invite to=%1, room=%2").arg(contactJid.full(),FRoomJid.bare()));
			}
		}

		if (!invited.isEmpty())
		{
			LOG_STRM_INFO(FStreamJid,QString("Direct conference invite sent to room=%1, contacts=%2").arg(FRoomJid.bare()).arg(invited.count()));
			emit invitationSent(invited,AReason,AThread);
			return true;
		}
	}
	else if (FStanzaProcessor && !isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send direct conference invite to room=%1, contact=%2: Conference is closed").arg(FRoomJid.bare()).arg(AContacts.count()));
	}
	return false;
}

bool MultiUserChat::sendVoiceRequest()
{
	if (FStanzaProcessor && isOpen() && FMainUser->role()==MUC_ROLE_VISITOR)
	{
		Message message;
		message.setTo(FRoomJid.bare()).setRandomId();

		Stanza &mstanza = message.stanza();
		QDomElement formElem = mstanza.addElement("x",NS_JABBER_DATA);
		formElem.setAttribute("type",DATAFORM_TYPE_SUBMIT);

		QDomElement fieldElem =  formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var","FORM_TYPE");
		fieldElem.setAttribute("type",DATAFIELD_TYPE_HIDDEN);
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(DFT_MUC_REQUEST));

		fieldElem = formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var","muc#role");
		fieldElem.setAttribute("type",DATAFIELD_TYPE_TEXTSINGLE);
		fieldElem.setAttribute("label","Requested role");
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(MUC_ROLE_PARTICIPANT));

		if (FStanzaProcessor->sendStanzaOut(FStreamJid, message.stanza()))
		{
			LOG_STRM_INFO(FStreamJid,QString("Voice request sent to conference, room=%1").arg(FRoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send voice request to conference, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send voice request to conference, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return false;
}

QString MultiUserChat::subject() const
{
	return FSubject;
}

bool MultiUserChat::sendSubject(const QString &ASubject)
{
	if (FStanzaProcessor && isOpen())
	{
		Message message;
		message.setTo(FRoomJid.bare()).setType(Message::GroupChat).setSubject(ASubject);
		if (FStanzaProcessor->sendStanzaOut(FStreamJid,message.stanza()))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference subject message sent, room=%1").arg(FRoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference subject message, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference subject message, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return false;
}

bool MultiUserChat::sendVoiceApproval(const Message &AMessage)
{
	if (FStanzaProcessor && isOpen())
	{
		Message message = AMessage;
		message.setTo(FRoomJid.bare());

		if (FStanzaProcessor->sendStanzaOut(FStreamJid,message.stanza()))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference voice approval sent, room=%1").arg(FRoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference voice approval, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference voice approval, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return false;
}

QString MultiUserChat::loadAffiliationList(const QString &AAffiliation)
{
	if (FStanzaProcessor && isOpen() && AAffiliation!=MUC_AFFIL_NONE)
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_GET).setTo(FRoomJid.bare()).setUniqueId();

		QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
		itemElem.setAttribute("affiliation",AAffiliation);

		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_LIST_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Load affiliation list request sent, affiliation=%1, id=%2, room=%3").arg(AAffiliation,request.id(),FRoomJid.bare()));
			FAffilListLoadId.insert(request.id(), AAffiliation);
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send load affiliation list request, affiliation=%1, room=%2").arg(AAffiliation,FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to load affiliation list, affiliation=%1, room=%2: Conference is closed").arg(AAffiliation,FRoomJid.bare()));
	}
	else if (AAffiliation == MUC_AFFIL_NONE)
	{
		REPORT_ERROR("Failed to load affiliation list: Affiliation is none");
	}
	return QString();
}

QString MultiUserChat::updateAffiliationList(const QList<IMultiUserListItem> &AItems)
{
	if (FStanzaProcessor && isOpen() && !AItems.isEmpty())
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_SET).setTo(FRoomJid.bare()).setUniqueId();

		QDomElement query = request.addElement("query",NS_MUC_ADMIN);
		foreach(const IMultiUserListItem &item, AItems)
		{
			QDomElement itemElem = query.appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("jid",item.realJid.full());
			if (!item.reason.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(item.reason));
			itemElem.setAttribute("affiliation",item.affiliation);
		}

		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_LIST_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Update affiliation list request sent, id=%1, room=%2").arg(request.id(),FRoomJid.bare()));
			FAffilListUpdateId.insert(request.id(),AItems);
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send update affiliation list request, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to update affiliation list, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return QString();
}

QString MultiUserChat::setUserRole(const QString &ANick, const QString &ARole, const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		IMultiUser *user = findUser(ANick);
		if (user)
		{
			Stanza request(STANZA_KIND_IQ);
			request.setType(STANZA_TYPE_SET).setTo(FRoomJid.bare()).setUniqueId();

			QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("role",ARole);
			itemElem.setAttribute("nick",ANick);

			if (!AReason.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));

			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
			{
				LOG_STRM_INFO(FStreamJid,QString("Update role request sent, nick=%1, role=%2, id=%3, room=%4").arg(ANick,ARole,request.id(),FRoomJid.bare()));
				FRoleUpdateId.insert(request.id(),ANick);
				return request.id();
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send update role request, nick=%1, role=%2, room=%3").arg(ANick,ARole,FRoomJid.bare()));
			}
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to change user role, nick=%1, room=%2: User not found").arg(ANick,FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to change user role, nick=%1, room=%2: Conference is closed").arg(ANick,FRoomJid.bare()));
	}
	return QString();
}

QString MultiUserChat::setUserAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		IMultiUser *user = findUser(ANick);
		if (user)
		{
			Stanza request(STANZA_KIND_IQ);
			request.setType(STANZA_TYPE_SET).setTo(FRoomJid.bare()).setUniqueId();

			QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("affiliation",AAffiliation);
			itemElem.setAttribute("nick",ANick);

			if (user->realJid().isValid())
				itemElem.setAttribute("jid",user->realJid().bare());

			if (!AReason.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));

			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
			{
				LOG_STRM_INFO(FStreamJid,QString("Update affiliation request sent, nick=%1, affiliation=%2, id=%3, room=%4").arg(ANick,AAffiliation,request.id(),FRoomJid.bare()));
				FAffilUpdateId.insert(request.id(),ANick);
				return request.id();
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send update affiliation request, nick=%1, affiliation=%2, room=%3").arg(ANick,AAffiliation,FRoomJid.bare()));
			}
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to change user affiliation, nick=%1, room=%2: User not found").arg(ANick,FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to change user affiliation, nick=%1, room=%2: Conference is closed").arg(ANick,FRoomJid.bare()));
	}
	return QString();
}

QString MultiUserChat::loadRoomConfig()
{
	if (FStanzaProcessor && isOpen())
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_GET).setTo(FRoomJid.bare()).setUniqueId();
		request.addElement("query",NS_MUC_OWNER);

		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference configuration load request sent, id=%1, room=%2").arg(request.id(),FRoomJid.bare()));
			FConfigLoadId.append(request.id());
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send load conference configuration request, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to load conference configuration, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return QString();
}

QString MultiUserChat::updateRoomConfig(const IDataForm &AForm)
{
	if (FStanzaProcessor && FDataForms && isOpen())
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_SET).setTo(FRoomJid.bare()).setUniqueId();

		QDomElement queryElem = request.addElement("query",NS_MUC_OWNER).toElement();
		FDataForms->xmlForm(AForm,queryElem);

		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference configuration update request sent, id=%1, room=%2").arg(request.id(),FRoomJid.bare()));
			FConfigUpdateId.insert(request.id(),AForm);
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send update conference configuration request, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to update conference configuration, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}
	return QString();
}

QString MultiUserChat::destroyRoom(const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		Stanza request(STANZA_KIND_IQ);
		request.setType(STANZA_TYPE_SET).setTo(FRoomJid.bare()).setUniqueId();

		QDomElement destroyElem = request.addElement("query",NS_MUC_OWNER).appendChild(request.createElement("destroy")).toElement();
		destroyElem.setAttribute("jid",FRoomJid.bare());

		if (!AReason.isEmpty())
			destroyElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));

		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference destruction request sent, id=%1, room=%2").arg(request.id(),FRoomJid.bare()));
			FRoomDestroyId.insert(request.id(),AReason);
			return request.id();
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference destruction request, room=%1").arg(FRoomJid.bare()));
		}
	}
	else if (!isOpen())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Failed to destroy conference, room=%1: Conference is closed").arg(FRoomJid.bare()));
	}

	return QString();
}

void MultiUserChat::setState(ChatState AState)
{
	if (FState != AState)
	{
		LOG_STRM_INFO(FStreamJid,QString("Conference state changed from=%1 to=%2, room=%3").arg(FState).arg(AState).arg(FRoomJid.bare()));
		FState = AState;

		if (FState == IMultiUserChat::Opened)
		{
			if (FResendPresence)
				sendStreamPresence();
		}
		else if (FState == IMultiUserChat::Closed)
		{
			FResendPresence = false;
		}

		emit stateChanged(AState);
	}
}

bool MultiUserChat::processMessage(const Stanza &AStanza)
{
	bool hooked = false;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();
	FStatusCodes = statusCodes(AStanza);

	Message message(AStanza);
	if (message.type()==Message::GroupChat && !message.stanza().firstElement("subject").isNull())
	{
		hooked = true;
		QString newSubject = message.subject();
		if (FSubject != newSubject)
		{
			FSubject = newSubject;
			LOG_STRM_INFO(FStreamJid,QString("Conference subject changed, nick=%1, room=%2").arg(fromNick,FRoomJid.bare()));
			emit subjectChanged(fromNick, FSubject);
		}
	}
	else if (message.type()!=Message::Error && fromNick.isEmpty())
	{
		if (!AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline").isNull())
		{
			hooked = true;
			QDomElement declElem = AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline");
			Jid contactJid = declElem.attribute("from");
			QString reason = declElem.firstChildElement("reason").text();
			LOG_STRM_INFO(FStreamJid,QString("Conference invite declined, contact=%1, room=%2: %3").arg(contactJid.full(),FRoomJid.bare(),reason));
			emit invitationDeclined(contactJid,reason);
		}
		else if (FDataForms && !AStanza.firstElement("x",NS_JABBER_DATA).isNull())
		{
			IDataForm form = FDataForms->dataForm(AStanza.firstElement("x",NS_JABBER_DATA));
			if (FDataForms->fieldValue("FORM_TYPE",form.fields) == DFT_MUC_REQUEST)
			{
				hooked = true;
				LOG_STRM_INFO(FStreamJid,QString("Conference voice request received, room=%1").arg(FRoomJid.bare()));
				emit voiceRequestReceived(message);
			}
		}
	}
	else if (message.type()==Message::Error && fromNick.isEmpty())
	{
		if (!AStanza.firstElement("x",NS_MUC_USER).firstChildElement("invite").isNull())
		{
			hooked = true;
			XmppStanzaError err(message.stanza());
			
			QList<Jid> contacts;
			QDomElement inviteElem = AStanza.firstElement("x",NS_MUC_USER).firstChildElement("invite");
			while (!inviteElem.isNull())
			{
				contacts.append(inviteElem.attribute("to"));
				inviteElem = inviteElem.nextSiblingElement("invite");
			}

			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference invite to room=%1, contacts=%2: %3").arg(FRoomJid.bare()).arg(contacts.count()).arg(err.condition()));
			emit invitationFailed(contacts,err);
		}
	}

	if (!hooked && FIsolated)
	{
		hooked = true;
		emit messageReceived(message);
	}

	FStatusCodes.clear();
	return hooked;
}

bool MultiUserChat::processPresence(const Stanza &AStanza)
{
	bool accepted = false;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();
	MultiUser *fromUser = FUsers.value(fromNick);

	QDomElement xelem = AStanza.firstElement("x",NS_MUC_USER);
	QDomElement itemElem = xelem.firstChildElement("item");

	FStatusCodes = statusCodes(AStanza);

	if (fromNick.isEmpty())
	{
		LOG_STRM_WARNING(FStreamJid,QString("Received unexpected presence from conference bare JID, type=%1: %2").arg(AStanza.type(),AStanza.firstElement("status").text()));
	}
	else if (AStanza.type()==PRESENCE_TYPE_AVAILABLE && !itemElem.isNull())
	{
		QString role = itemElem.attribute("role");
		QString affiliation = itemElem.attribute("affiliation");

		IPresenceItem presence;
		presence.itemJid = fromJid;
		presence.status = AStanza.firstElement("status").text();
		presence.priority = AStanza.firstElement("priority").text().toInt();

		QString showText = AStanza.firstElement("show").text();
		if (showText == PRESENCE_SHOW_ONLINE)
			presence.show = IPresence::Online;
		else if (showText == PRESENCE_SHOW_CHAT)
			presence.show = IPresence::Chat;
		else if (showText == PRESENCE_SHOW_AWAY)
			presence.show = IPresence::Away;
		else if (showText == PRESENCE_SHOW_DND)
			presence.show = IPresence::DoNotDisturb;
		else if (showText == PRESENCE_SHOW_XA)
			presence.show = IPresence::ExtendedAway;
		else
			presence.show = IPresence::Online;

		QDomElement delayElem = AStanza.firstElement("delay",NS_XMPP_DELAY);
		if (!delayElem.isNull())
			presence.sentTime = DateTime(delayElem.attribute("stamp")).toLocal();
		else
			presence.sentTime = QDateTime::currentDateTime();

		if (fromUser == NULL)
		{
			LOG_STRM_DEBUG(FStreamJid,QString("User has joined the conference, nick=%1, role=%2, affiliation=%3, room=%4").arg(fromNick,role,affiliation,FRoomJid.bare()));
			fromUser = new MultiUser(FStreamJid,fromJid,itemElem.attribute("jid"),this);
		}

		fromUser->setRole(role);
		fromUser->setAffiliation(affiliation);
		fromUser->setPresence(presence);

		if (!FUsers.contains(fromNick))
		{
			FUsers.insert(fromNick,fromUser);
			connect(fromUser->instance(),SIGNAL(changed(int, const QVariant &)),SLOT(onUserChanged(int, const QVariant &)));
			emit userChanged(fromUser,MUDR_PRESENCE,QVariant());
		}

		if (FState == IMultiUserChat::Opening)
		{
			if (fromNick == FNickname)
			{
				FMainUser = fromUser;
				setState(IMultiUserChat::Opened);
			}
			else if (FStatusCodes.contains(MUC_SC_SELF_PRESENCE))
			{
				LOG_STRM_WARNING(FStreamJid,QString("Main user nick was changed by server, from=%1, to=%2, room=%3").arg(FNickname,fromNick,FRoomJid.bare()));
				FNickname = fromNick;

				FMainUser = fromUser;
				setState(IMultiUserChat::Opened);
			}
		}

		if (fromUser == FMainUser)
		{
			LOG_STRM_DEBUG(FStreamJid,QString("Conference presence changed from=%1 to=%2, room=%3").arg(FRoomPresence.show).arg(presence.show).arg(FRoomJid.bare()));
			FRoomPresence = presence;
			emit presenceChanged(FRoomPresence);
		}

		accepted = true;
	}
	else if (AStanza.type()==PRESENCE_TYPE_UNAVAILABLE && fromUser!=NULL)
	{
		bool applyPresence = true;

		IPresenceItem presence;
		presence.itemJid = fromJid;
		presence.status = AStanza.firstElement("status").text();

		QString role = itemElem.attribute("role");
		QString affiliation = itemElem.attribute("affiliation");

		if (FStatusCodes.contains(MUC_SC_NICK_CHANGED))
		{
			applyPresence = false;
			QString newNick = itemElem.attribute("nick");
			if (!newNick.isEmpty() && !FUsers.contains(newNick))
			{
				fromUser->setNick(newNick);
				FUsers.insert(newNick,FUsers.take(fromNick));

				if (fromUser == FMainUser)
				{
					FNickname = newNick;
					FRequestedNick.clear();
					emit nicknameChanged(FNickname, XmppError::null);
				}
			}
			else if (newNick.isEmpty())
			{
				LOG_STRM_ERROR(FStreamJid,QString("Failed to changes user nick from=%1, room=%2: New nick is empty").arg(fromNick,FRoomJid.bare()));
			}
			else
			{
				LOG_STRM_ERROR(FStreamJid,QString("Failed to changes user nick from=%1 to=%2, room=%3: Nick occupied by another user").arg(fromNick,newNick,FRoomJid.bare()));
			}
		}
		else if (FStatusCodes.contains(MUC_SC_USER_KICKED))
		{
			QString actor = itemElem.firstChildElement("actor").attribute("nick");
			QString reason = itemElem.firstChildElement("reason").text();
			LOG_STRM_INFO(FStreamJid,QString("User was kicked from conference, nick=%1, by=%2, room=%3: %4").arg(fromNick,actor,FRoomJid.bare(),reason));
			emit userKicked(fromNick,reason,actor);
		}
		else if (FStatusCodes.contains(MUC_SC_USER_BANNED))
		{
			QString actor = itemElem.firstChildElement("actor").attribute("nick");
			QString reason = itemElem.firstChildElement("reason").text();
			LOG_STRM_INFO(FStreamJid,QString("User was banned in conference, nick=%1, by=%2, room=%3: %4").arg(fromNick,actor,FRoomJid.bare(),reason));
			emit userBanned(fromNick,reason,actor);
		}
		else if (!xelem.firstChildElement("destroy").isNull())
		{
			QString reason = xelem.firstChildElement("destroy").firstChildElement("reason").text();
			LOG_STRM_INFO(FStreamJid,QString("Conference was destroyed by owner, room=%1: %2").arg(FRoomJid.bare(),reason));
			emit roomDestroyed(QString(),reason);
		}

		if (applyPresence)
		{
			LOG_STRM_DEBUG(FStreamJid,QString("User has left the conference, nick=%1, show=%2, status=%3, room=%4").arg(fromNick).arg(presence.show).arg(presence.status).arg(FRoomJid.bare()));
			if (fromUser != FMainUser)
			{
				fromUser->setPresence(presence);
				delete FUsers.take(fromNick);
			}
			else
			{
				closeRoom(presence);
			}
		}

		accepted = true;
	}
	else if (AStanza.isError())
	{
		XmppStanzaError err(AStanza);
		LOG_STRM_DEBUG(FStreamJid,QString("User has left the conference due to error, nick=%1, room=%2: %3").arg(fromNick,FRoomJid.bare(),err.errorMessage()));

		IPresenceItem presence;
		presence.itemJid = fromJid;
		presence.show = IPresence::Error;
		presence.status = err.errorMessage();

		if (fromNick == FNickname)
		{
			FRoomError = err;
			closeRoom(presence);
		}
		else if (fromNick == FRequestedNick)
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to changes user nick from=%1 to=%2, room=%3: %4").arg(FNickname,FRequestedNick,FRoomJid.bare(),err.errorMessage()));
			FRequestedNick.clear();
			emit nicknameChanged(fromNick,err);
		}
		else if (fromUser != NULL)
		{
			fromUser->setPresence(presence);
			delete FUsers.take(fromNick);
		}

		accepted = true;
	}

	FStatusCodes.clear();
	return accepted;
}

void MultiUserChat::closeRoom(const IPresenceItem &APresence)
{
	if (FState != IMultiUserChat::Closed)
	{
		FRequestedNick.clear();

		if (FMainUser != NULL)
		{
			FMainUser->setPresence(APresence);
			delete FMainUser;
			FMainUser = NULL;
		}
		FUsers.remove(FNickname);

		foreach(MultiUser *user, FUsers)
			user->setPresence(IPresenceItem());
		qDeleteAll(FUsers);
		FUsers.clear();

		FRoomPresence = APresence;
		emit presenceChanged(FRoomPresence);

		setState(IMultiUserChat::Closed);
	}
}

Stanza MultiUserChat::makePresenceStanza(const QString &ANick, int AShow, const QString &AStatus, int APriority) const
{
	Stanza presence(STANZA_KIND_PRESENCE);
	presence.setTo(Jid(FRoomJid.node(),FRoomJid.domain(),ANick).full());

	QString showText;
	bool isOnline = true;
	switch (AShow)
	{
	case IPresence::Online:
		showText = PRESENCE_SHOW_ONLINE;
		break;
	case IPresence::Chat:
		showText = PRESENCE_SHOW_CHAT;
		break;
	case IPresence::Away:
		showText = PRESENCE_SHOW_AWAY;
		break;
	case IPresence::DoNotDisturb:
		showText = PRESENCE_SHOW_DND;
		break;
	case IPresence::ExtendedAway:
		showText = PRESENCE_SHOW_XA;
		break;
	default:
		isOnline = false;
	}

	if (!AStatus.isEmpty())
		presence.addElement("status").appendChild(presence.createTextNode(AStatus));

	if (isOnline)
	{
		if (!showText.isEmpty())
			presence.addElement("show").appendChild(presence.createTextNode(showText));
		presence.addElement("priority").appendChild(presence.createTextNode(QString::number(APriority)));
	}
	else
	{
		presence.setType(PRESENCE_TYPE_UNAVAILABLE);
	}

	return presence;
}

void MultiUserChat::onUserChanged(int AData, const QVariant &ABefore)
{
	IMultiUser *user = qobject_cast<IMultiUser *>(sender());
	if (user)
		emit userChanged(user,AData,ABefore);
}

void MultiUserChat::onDiscoveryInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.streamJid==streamJid() && AInfo.contactJid==roomJid())
	{
		int index = FDiscovery->findIdentity(AInfo.identity,DIC_CONFERENCE,QString());
		QString name = index>=0 ? AInfo.identity.at(index).name : QString();
		if (!name.isEmpty() && FRoomTitle!=name)
		{
			FRoomTitle = name;
			LOG_STRM_DEBUG(FStreamJid,QString("Conference title changed, room=%1: %2").arg(FRoomJid.bare(),FRoomTitle));
			emit roomTitleChanged(FRoomTitle);
		}
	}
}

void MultiUserChat::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (AXmppStream->streamJid() == FStreamJid)
		abortConnection(AXmppStream->error().errorMessage(),!AXmppStream->error().isNull());
}

void MultiUserChat::onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	if (ABefore == FStreamJid)
	{
		FStreamJid = AXmppStream->streamJid();
		emit streamJidChanged(ABefore,FStreamJid);
	}
}

void MultiUserChat::onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority)
{
	if (FAutoPresence && APresence->streamJid()==FStreamJid)
	{
		if (FState==IMultiUserChat::Opening && AShow!=IPresence::Offline)
			FResendPresence = true;
		else if (AShow != IPresence::Error)
			sendPresence(AShow,AStatus,APriority);
	}
}
