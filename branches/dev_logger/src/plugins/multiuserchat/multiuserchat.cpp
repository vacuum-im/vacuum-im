#include "multiuserchat.h"

#include <definitions/multiuserdataroles.h>
#include <definitions/namespaces.h>
#include <definitions/messageeditororders.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

#define SHC_PRESENCE        "/presence"
#define SHC_MESSAGE         "/message"

#define MUC_IQ_TIMEOUT      30000
#define MUC_LIST_TIMEOUT    60000

MultiUserChat::MultiUserChat(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANickName, const QString &APassword, QObject *AParent) : QObject(AParent)
{
	FPresence = NULL;
	FDataForms = NULL;
	FXmppStream = NULL;
	FStanzaProcessor = NULL;
	FMessageProcessor = NULL;
	FDiscovery = NULL;
	FChatPlugin = AChatPlugin;

	FMainUser = NULL;
	FSHIPresence = -1;
	FSHIMessage = -1;
	FConnected = false;
	FAutoPresence = false;

	FRoomJid = ARoomJid;
	FStreamJid = AStreamJid;
	FNickName = ANickName;
	FPassword = APassword;
	FShow = IPresence::Offline;
	FRoomName = FRoomJid.uBare();

	initialize();
}

MultiUserChat::~MultiUserChat()
{
	if (!FUsers.isEmpty())
		closeChat(IPresence::Offline, QString::null);

	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPresence);
		FStanzaProcessor->removeStanzaHandle(FSHIMessage);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->removeMessageEditor(MEO_MULTIUSERCHAT,this);
	}
	emit chatDestroyed();
}

bool MultiUserChat::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	Jid fromJid = AStanza.from();
	if (AStreamJid==FStreamJid && FRoomJid.pBare()==fromJid.pBare())
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
	Q_UNUSED(AStreamJid);
	if (AStanza.id()==FConfigRequestId && FRoomJid==AStanza.from())
	{
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference configuretion form received, room=%1").arg(FRoomJid.bare()));
			QDomElement formElem = AStanza.firstElement("query",NS_MUC_OWNER).firstChildElement("x");
			while (formElem.namespaceURI() != NS_JABBER_DATA)
				formElem = formElem.nextSiblingElement("x");
			if (FDataForms && !formElem.isNull())
				emit configFormReceived(FDataForms->dataForm(formElem));
			else
				emit chatNotify(tr("Room configuration is not available."));
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive conference configuration form, room=%1: %2").arg(FRoomJid.bare(),err.errorMessage()));
			emit chatError(err.errorMessage());
		}
		FConfigRequestId.clear();
	}
	else if (AStanza.id()==FConfigSubmitId && FRoomJid==AStanza.from())
	{
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference configuration submit accepted, room=%1").arg(FRoomJid.bare()));
			emit configFormAccepted();
			emit chatNotify(tr("Room configuration accepted."));
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to accept conference configuration submit, room=%1: %2").arg(FRoomJid.bare(),err.errorMessage()));
			emit configFormRejected(err);
			emit chatError(err.errorMessage());
		}
		FConfigSubmitId.clear();
	}
	else if (FAffilListRequests.contains(AStanza.id()) && FRoomJid==AStanza.from())
	{
		QString affiliation = FAffilListRequests.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference affiliation list received, room=%1, affiliation=%2, id=%3").arg(FRoomJid.bare(),affiliation,AStanza.id()));

			QList<IMultiUserListItem> listItems;
			QDomElement itemElem = AStanza.firstElement("query",NS_MUC_ADMIN).firstChildElement("item");
			while (!itemElem.isNull())
			{
				if (itemElem.attribute("affiliation") == affiliation)
				{
					IMultiUserListItem listitem;
					listitem.jid = itemElem.attribute("jid");
					listitem.affiliation = itemElem.attribute("affiliation");
					listitem.notes = itemElem.firstChildElement("reason").text();
					listItems.append(listitem);
				}
				itemElem = itemElem.nextSiblingElement("item");
			}
			emit affiliationListReceived(affiliation,listItems);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to receive conference affiliation list, room=%1, affiliation=%2, id=%3: %4").arg(FRoomJid.bare(),affiliation,AStanza.id(),err.errorMessage()));
			emit chatError(tr("Request for list of %1s is failed: %2").arg(affiliation).arg(err.errorMessage()));
		}
	}
	else if (FAffilListSubmits.contains(AStanza.id()) && FRoomJid==AStanza.from())
	{
		QString affiliation = FAffilListSubmits.take(AStanza.id());
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(AStreamJid,QString("Conference affiliation list changes accepted, room=%1, affiliation=%2, id=%3").arg(FRoomJid.bare(),affiliation,AStanza.id()));
			emit chatNotify(tr("Changes in list of %1s was accepted.").arg(affiliation));
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(AStreamJid,QString("Failed to accept conference affiliation list changes, room=%1, affiliation=%2, id=%3: %4").arg(FRoomJid.bare(),affiliation,AStanza.id(),err.errorMessage()));
			emit chatError(tr("Changes in list of %1s was not accepted: %2").arg(affiliation).arg(err.errorMessage()));
		}
	}
	else if (AStanza.type() == "error")
	{
		XmppStanzaError err(AStanza);
		LOG_STRM_WARNING(AStreamJid,QString("Unexpected error received in conference, room=%1, id=%2: %3").arg(FRoomJid.bare(),AStanza.id(),err.errorMessage()));
		emit chatError(err.errorMessage());
	}
}

bool MultiUserChat::messageReadWrite(int AOrder, const Jid &AStreamJid, Message &AMessage, int ADirection)
{
	if (AOrder==MEO_MULTIUSERCHAT && ADirection==IMessageProcessor::MessageIn && AStreamJid==FStreamJid)
	{
		if (FRoomJid.pBare() == Jid(AMessage.from()).pBare())
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
	return FRoomName;
}

bool MultiUserChat::isOpen() const
{
	return FMainUser!=NULL;
}

bool MultiUserChat::isConnected() const
{
	return FConnected;
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

bool MultiUserChat::isUserPresent(const Jid &AContactJid) const
{
	if (FUsers.contains(AContactJid.resource()) && AContactJid.pBare()==FRoomJid.pBare())
		return true;

	foreach(MultiUser *user, FUsers)
		if (AContactJid == user->data(MUDR_REAL_JID).toString())
			return true;

	return false;
}

IMultiUser *MultiUserChat::mainUser() const
{
	return FMainUser;
}

QList<IMultiUser *> MultiUserChat::allUsers() const
{
	QList<IMultiUser *> result;
	foreach(MultiUser *user, FUsers)
		result.append(user);
	return result;
}

IMultiUser *MultiUserChat::userByNick(const QString &ANick) const
{
	return FUsers.value(ANick,NULL);
}

QString MultiUserChat::nickName() const
{
	return FNickName;
}

bool MultiUserChat::setNickName(const QString &ANick)
{
	if (!ANick.isEmpty())
	{
		if (isConnected())
		{
			if (FNickName != ANick)
			{
				Jid userJid(FRoomJid.node(),FRoomJid.domain(),ANick);
				Stanza presence("presence");
				presence.setTo(userJid.full());
				if (FStanzaProcessor->sendStanzaOut(FStreamJid,presence))
				{
					LOG_STRM_INFO(FStreamJid,QString("Change conference nick request sent, room=%1, old=%2, new=%3").arg(FRoomJid.bare(),FNickName,ANick));
					return true;
				}
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send change conference nick request, room=%1").arg(FRoomJid.bare()));
			}
		}
		else
		{
			FNickName = ANick;
			return true;
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
	FPassword = APassword;
}

IMultiUserChatHistory MultiUserChat::history() const
{
	return FHistory;
}

void MultiUserChat::setHistory(const IMultiUserChatHistory &AHistory)
{
	FHistory = AHistory;
}

int MultiUserChat::show() const
{
	return FShow;
}

QString MultiUserChat::status() const
{
	return FStatus;
}

XmppError MultiUserChat::roomError() const
{
	return FRoomError;
}

bool MultiUserChat::sendStreamPresence()
{
	return FPresence!=NULL ? sendPresence(FPresence->show(),FPresence->status()) : false;
}

bool MultiUserChat::sendPresence(int AShow, const QString &AStatus)
{
	if (FStanzaProcessor)
	{
		Jid userJid(FRoomJid.node(),FRoomJid.domain(),FNickName);

		Stanza presence("presence");
		presence.setTo(userJid.full());

		QString showText;
		bool isConnecting = true;
		switch (AShow)
		{
		case IPresence::Online:
			showText = QString::null;
			break;
		case IPresence::Chat:
			showText = "chat";
			break;
		case IPresence::Away:
			showText = "away";
			break;
		case IPresence::DoNotDisturb:
			showText = "dnd";
			break;
		case IPresence::ExtendedAway:
			showText = "xa";
			break;
		default:
			isConnecting = false;
			showText = "unavailable";
		}

		if (!isConnecting)
			presence.setType("unavailable");
		else if (!showText.isEmpty())
			presence.addElement("show").appendChild(presence.createTextNode(showText));

		if (!AStatus.isEmpty())
			presence.addElement("status").appendChild(presence.createTextNode(AStatus));

		if (!isConnected() && isConnecting)
		{
			FRoomError = XmppError::null;
			emit chatAboutToConnect();

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
			if (FDiscovery && !FDiscovery->hasDiscoInfo(streamJid(),roomJid()))
				FDiscovery->requestDiscoInfo(streamJid(),roomJid());
		}
		else if (isConnected() && !isConnecting)
		{
			emit chatAboutToDisconnect();
		}

		if (FStanzaProcessor->sendStanzaOut(FStreamJid,presence))
		{
			LOG_STRM_DEBUG(FStreamJid,QString("Presence sent to conference, room=%1, show=%2").arg(FRoomJid.bare()).arg(AShow));
			FConnected = isConnecting;
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send presence to conference, room=%1, show=%2").arg(FRoomJid.bare()).arg(AShow));
		}
	}
	else
	{
		REPORT_ERROR("Failed to send presence to conference: Required interfaces not found");
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
		message.setTo(toJid.full());
		message.setType(AToNick.isEmpty() ? Message::GroupChat : Message::Chat);

		if (FMessageProcessor)
		{
			if (FMessageProcessor->sendMessage(FStreamJid,message,IMessageProcessor::MessageOut))
				return true;
			else
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send message to conference, room=%1").arg(FRoomJid.bare()));
		}
		else if (FStanzaProcessor)
		{
			if (FStanzaProcessor->sendStanzaOut(FStreamJid, message.stanza()))
			{
				emit messageSent(message);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send message to conference, room=%1").arg(FRoomJid.bare()));
			}
		}
	}
	return false;
}

bool MultiUserChat::requestVoice()
{
	if (FStanzaProcessor && isOpen() && FMainUser->data(MUDR_ROLE).toString()==MUC_ROLE_VISITOR)
	{
		Message message;
		message.setTo(FRoomJid.bare());

		Stanza &mstanza = message.stanza();
		QDomElement formElem = mstanza.addElement("x",NS_JABBER_DATA);
		formElem.setAttribute("type",DATAFORM_TYPE_SUBMIT);

		QDomElement fieldElem =  formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var","FORM_TYPE");
		fieldElem.setAttribute("type",DATAFIELD_TYPE_HIDDEN);
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(MUC_FT_REQUEST));

		fieldElem = formElem.appendChild(mstanza.createElement("field")).toElement();
		fieldElem.setAttribute("var",MUC_FV_ROLE);
		fieldElem.setAttribute("type",DATAFIELD_TYPE_TEXTSINGLE);
		fieldElem.setAttribute("label","Requested role");
		fieldElem.appendChild(mstanza.createElement("value")).appendChild(mstanza.createTextNode(MUC_ROLE_PARTICIPANT));

		if (FStanzaProcessor->sendStanzaOut(FStreamJid, mstanza))
		{
			LOG_STRM_INFO(FStreamJid,QString("Voice request sent to conference, room=%1").arg(FRoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send voice request to conference, room=%1").arg(FRoomJid.bare()));
		}
	}
	return false;
}

bool MultiUserChat::inviteContact(const Jid &AContactJid, const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		Message message;
		message.setTo(FRoomJid.bare());

		Stanza &mstanza = message.stanza();
		QDomElement invElem = mstanza.addElement("x",NS_MUC_USER).appendChild(mstanza.createElement("invite")).toElement();
		invElem.setAttribute("to",AContactJid.full());
		if (!AReason.isEmpty())
			invElem.appendChild(mstanza.createElement("reason")).appendChild(mstanza.createTextNode(AReason));

		if (FStanzaProcessor->sendStanzaOut(FStreamJid, mstanza))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference invite request sent, room=%1, contact=%2").arg(FRoomJid.bare(),AContactJid.full()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference invite request, room=%1, contact=%2").arg(FRoomJid.bare(),AContactJid.full()));
		}
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
			LOG_STRM_INFO(FStreamJid,QString("Conference subject change message sent, room=%1").arg(FRoomJid.bare()));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference subject change message, room=%1").arg(FRoomJid.bare()));
		}
	}
	return false;
}

bool MultiUserChat::sendDataFormMessage(const IDataForm &AForm)
{
	if (FStanzaProcessor && FDataForms && isOpen())
	{
		Message message;
		message.setTo(FRoomJid.bare());
		QDomElement elem = message.stanza().element();
		FDataForms->xmlForm(AForm,elem);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,message.stanza(),0))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference data form message sent, room=%1, title=%2").arg(FRoomJid.bare(),AForm.title));
			emit dataFormMessageSent(AForm);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference data form message, room=%1, title=%2").arg(FRoomJid.bare(),AForm.title));
		}
	}
	return false;
}

bool MultiUserChat::setRole(const QString &ANick, const QString &ARole, const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		IMultiUser *user = userByNick(ANick);
		if (user)
		{
			Stanza request("iq");
			request.setTo(FRoomJid.bare()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("role",ARole);
			itemElem.setAttribute("nick",ANick);
			if (!user->data(MUDR_REAL_JID).toString().isEmpty())
				itemElem.setAttribute("jid",user->data(MUDR_REAL_JID).toString());
			if (!AReason.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));
			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,0))
			{
				LOG_STRM_INFO(FStreamJid,QString("User role change request sent, room=%1, user=%2, role=%3").arg(FRoomJid.bare(),ANick,ARole));
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send user role change request, room=%1, user=%1, role=%3").arg(FRoomJid.bare(),ANick,ARole));
			}
		}
		else
		{
			LOG_STRM_ERROR(FStreamJid,QString("Failed to change user role, room=%1, user=%2: User not found").arg(FRoomJid.bare(),ANick));
		}
	}
	return false;
}

bool MultiUserChat::setAffiliation(const QString &ANick, const QString &AAffiliation, const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		IMultiUser *user = userByNick(ANick);
		if (user)
		{
			Stanza request("iq");
			request.setTo(FRoomJid.bare()).setType("set").setId(FStanzaProcessor->newId());
			QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("affiliation",AAffiliation);
			itemElem.setAttribute("nick",ANick);
			if (!user->data(MUDR_REAL_JID).toString().isEmpty())
				itemElem.setAttribute("jid",user->data(MUDR_REAL_JID).toString());
			if (!AReason.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));
			if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,0))
			{
				LOG_STRM_INFO(FStreamJid,QString("User affiliation change request sent, room=%1, user=%2, affiliation=%3").arg(FRoomJid.bare(),ANick,AAffiliation));
				return true;
			}
			else
			{
				LOG_STRM_WARNING(FStreamJid,QString("Failed to send user affiliation change request, room=%1, user=%1, affiliation=%3").arg(FRoomJid.bare(),ANick,AAffiliation));
			}
		}
		else
		{
			LOG_STRM_ERROR(FStreamJid,QString("Failed to change user affiliation, room=%1, user=%2: User not found").arg(FRoomJid.bare(),ANick));
		}
	}
	return false;
}

bool MultiUserChat::requestAffiliationList(const QString &AAffiliation)
{
	if (FAffilListRequests.values().contains(AAffiliation))
	{
		return true;
	}
	else if (FStanzaProcessor && isOpen() && AAffiliation!=MUC_AFFIL_NONE)
	{
		Stanza request("iq");
		request.setTo(FRoomJid.bare()).setType("get").setId(FStanzaProcessor->newId());
		QDomElement itemElem = request.addElement("query",NS_MUC_ADMIN).appendChild(request.createElement("item")).toElement();
		itemElem.setAttribute("affiliation",AAffiliation);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_LIST_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Affiliation list request sent, room=%1, affiliation=%2, id=%3").arg(FRoomJid.bare(),AAffiliation,request.id()));
			FAffilListRequests.insert(request.id(),AAffiliation);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send affiliation list request, room=%1, affiliation=%2").arg(FRoomJid.bare(),AAffiliation));
			emit chatError(tr("Failed to send request for list of %1s.").arg(AAffiliation));
		}
	}
	return false;
}

bool MultiUserChat::changeAffiliationList(const QList<IMultiUserListItem> &ADeltaList)
{
	if (FStanzaProcessor && isOpen() && !ADeltaList.isEmpty())
	{
		Stanza request("iq");
		request.setTo(FRoomJid.bare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement query = request.addElement("query",NS_MUC_ADMIN);
		foreach(const IMultiUserListItem &listItem, ADeltaList)
		{
			QDomElement itemElem = query.appendChild(request.createElement("item")).toElement();
			itemElem.setAttribute("affiliation",listItem.affiliation);
			itemElem.setAttribute("jid",listItem.jid);
			if (!listItem.notes.isEmpty())
				itemElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(listItem.notes));
		}
		QString affiliation = ADeltaList.value(0).affiliation;
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_LIST_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Affiliation list changes sent, room=%1, affiliation=%2, id=%3").arg(FRoomJid.bare(),affiliation,request.id()));
			emit affiliationListChanged(ADeltaList);
			FAffilListSubmits.insert(request.id(),ADeltaList.value(0).affiliation);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send affiliation list changes, room=%1, affiliation=%2").arg(FRoomJid.bare(),affiliation));
			emit chatError(tr("Failed to send changes in list of %1s").arg(ADeltaList.value(0).affiliation));
		}
	}
	return false;
}

bool MultiUserChat::requestConfigForm()
{
	if (!FConfigRequestId.isEmpty())
	{
		return true;
	}
	else if (FStanzaProcessor && isOpen())
	{
		Stanza request("iq");
		request.setTo(FRoomJid.bare()).setType("get").setId(FStanzaProcessor->newId());
		request.addElement("query",NS_MUC_OWNER);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference configuration request sent, room=%1, id=%2").arg(FRoomJid.bare(),FConfigRequestId));
			FConfigRequestId = request.id();
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference configuration request, room=%1").arg(FRoomJid.bare()));
			emit chatError(tr("Room configuration request failed."));
		}
	}
	return false;
}

bool MultiUserChat::sendConfigForm(const IDataForm &AForm)
{
	if (!FConfigSubmitId.isEmpty())
	{
		return true;
	}
	else if (FStanzaProcessor && FDataForms && isOpen())
	{
		Stanza request("iq");
		request.setTo(FRoomJid.bare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement queryElem = request.addElement("query",NS_MUC_OWNER).toElement();
		FDataForms->xmlForm(AForm,queryElem);
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference configuration changes sent, room=%1, id=%2").arg(FRoomJid.bare(),FConfigSubmitId));
			FConfigSubmitId = request.id();
			emit configFormSent(AForm);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference configuration changes, room=%1").arg(FRoomJid.bare()));
			emit chatError(tr("Room configuration submit failed."));
		}
	}
	return false;
}

bool MultiUserChat::destroyRoom(const QString &AReason)
{
	if (FStanzaProcessor && isOpen())
	{
		Stanza request("iq");
		request.setTo(FRoomJid.bare()).setType("set").setId(FStanzaProcessor->newId());
		QDomElement destroyElem = request.addElement("query",NS_MUC_OWNER).appendChild(request.createElement("destroy")).toElement();
		destroyElem.setAttribute("jid",FRoomJid.bare());
		if (!AReason.isEmpty())
			destroyElem.appendChild(request.createElement("reason")).appendChild(request.createTextNode(AReason));
		if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,request,MUC_IQ_TIMEOUT))
		{
			LOG_STRM_INFO(FStreamJid,QString("conference destruction request sent, room=%1").arg(FRoomJid.bare()));
			emit chatNotify(tr("Room destruction request was sent."));
			return true;
		}
		else
		{
			LOG_STRM_WARNING(FStreamJid,QString("Failed to send conference destruction request, room=%1").arg(FRoomJid.bare()));
		}
	}
	return false;
}

void MultiUserChat::initialize()
{
	IPlugin *plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
		if (FMessageProcessor)
			FMessageProcessor->insertMessageEditor(MEO_MULTIUSERCHAT,this);
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
		if (FStanzaProcessor)
		{
			IStanzaHandle shandle;
			shandle.handler = this;
			shandle.order = SHO_PI_MULTIUSERCHAT;
			shandle.direction = IStanzaHandle::DirectionIn;
			shandle.streamJid = FStreamJid;
			shandle.conditions.append(SHC_PRESENCE);
			FSHIPresence = FStanzaProcessor->insertStanzaHandle(shandle);

			if (FMessageProcessor==NULL || !FMessageProcessor->isActiveStream(streamJid()))
			{
				shandle.conditions.clear();
				shandle.order = SHO_MI_MULTIUSERCHAT;
				shandle.conditions.append(SHC_MESSAGE);
				FSHIMessage = FStanzaProcessor->insertStanzaHandle(shandle);
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (presencePlugin)
		{
			FPresence = presencePlugin->findPresence(FStreamJid);
			if (FPresence)
			{
				connect(FPresence->instance(),SIGNAL(changed(int, const QString &, int)),SLOT(onPresenceChanged(int, const QString &, int)));
				connect(FPresence->instance(),SIGNAL(aboutToClose(int, const QString &)),SLOT(onPresenceAboutToClose(int , const QString &)));
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (xmppStreams)
		{
			FXmppStream = xmppStreams->xmppStream(FStreamJid);
			if (FXmppStream)
			{
				connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
				connect(FXmppStream->instance(),SIGNAL(jidChanged(const Jid &)),SLOT(onStreamJidChanged(const Jid &)));
			}
		}
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = FChatPlugin->pluginManager()->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			onDiscoveryInfoReceived(FDiscovery->discoInfo(streamJid(),roomJid()));
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoveryInfoReceived(const IDiscoInfo &)));
		}
	}
}

bool MultiUserChat::processMessage(const Stanza &AStanza)
{
	bool hooked = true;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();

	QDomElement statusElem =  AStanza.firstElement("x",NS_MUC_USER).firstChildElement("status");
	while (!statusElem.isNull())
	{
		FStatusCodes.append(statusElem.attribute("code").toInt());
		statusElem = statusElem.nextSiblingElement("status");
	}

	Message message(AStanza);
	if (AStanza.type() == "error")
	{
		XmppStanzaError err(AStanza);
		LOG_STRM_WARNING(FStreamJid,QString("Error message received in conefernce, room=%1, from=%2: %3").arg(FRoomJid.bare(),fromNick,err.errorMessage()));
		emit chatError(!fromNick.isEmpty() ? QString("%1 - %2").arg(fromNick).arg(err.errorMessage()) : err.errorMessage());
	}
	else if (message.type()==Message::GroupChat && !message.stanza().firstElement("subject").isNull())
	{
		QString newSubject = message.subject();
		if (FSubject != newSubject)
		{
			LOG_STRM_INFO(FStreamJid,QString("Conference subject changed, room=%1, user=%2").arg(FRoomJid.bare(),fromNick));
			FSubject = newSubject;
			emit subjectChanged(fromNick, FSubject);
		}
	}
	else if (fromNick.isEmpty())
	{
		if (!AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline").isNull())
		{
			QDomElement declElem = AStanza.firstElement("x",NS_MUC_USER).firstChildElement("decline");
			Jid contactJid = declElem.attribute("from");
			QString reason = declElem.firstChildElement("reason").text();
			LOG_STRM_INFO(FStreamJid,QString("Conference invite declined, room=%1, user=%2: %3").arg(FRoomJid.bare(),contactJid.full(),reason));
			emit inviteDeclined(contactJid,reason);
		}
		else if (!AStanza.firstElement("x",NS_JABBER_DATA).isNull())
		{
			LOG_STRM_INFO(FStreamJid,QString("Data form message received, room=%1: %2").arg(FRoomJid.bare(),message.body()));
			if (FMessageProcessor == NULL)
				emit dataFormMessageReceived(message);
			else
				hooked = false;
		}
		else
		{
			LOG_STRM_INFO(FStreamJid,QString("Service message received, room=%1: %2").arg(FRoomJid.bare(),message.body()));
			emit serviceMessageReceived(message);
		}
	}
	else if (!message.body().isEmpty())
	{
		if (FMessageProcessor == NULL)
			emit messageReceived(fromNick,message);
		else
			hooked = false;
	}

	FStatusCodes.clear();
	return hooked;
}

bool MultiUserChat::processPresence(const Stanza &AStanza)
{
	bool accepted = false;

	Jid fromJid = AStanza.from();
	QString fromNick = fromJid.resource();

	QDomElement xelem = AStanza.firstElement("x",NS_MUC_USER);
	QDomElement itemElem = xelem.firstChildElement("item");

	QDomElement statusElem = xelem.firstChildElement("status");
	while (!statusElem.isNull())
	{
		FStatusCodes.append(statusElem.attribute("code").toInt());
		statusElem = statusElem.nextSiblingElement("status");
	}

	if (AStanza.type().isEmpty())
	{
		if (!fromNick.isEmpty() && !itemElem.isNull())
		{
			int show;
			QString showText = AStanza.firstElement("show").text();
			if (showText.isEmpty())
				show = IPresence::Online;
			else if (showText == "chat")
				show = IPresence::Chat;
			else if (showText == "away")
				show = IPresence::Away;
			else if (showText == "dnd")
				show = IPresence::DoNotDisturb;
			else if (showText == "xa")
				show = IPresence::ExtendedAway;
			else
				show = IPresence::Online;

			QString status = AStanza.firstElement("status").text();
			Jid realJid = itemElem.attribute("jid");
			QString role = itemElem.attribute("role");
			QString affiliation = itemElem.attribute("affiliation");

			MultiUser *user = FUsers.value(fromNick);
			if (!user)
			{
				user = new MultiUser(FRoomJid,fromNick,this);
				user->setData(MUDR_STREAM_JID,FStreamJid.full());
				LOG_STRM_DEBUG(FStreamJid,QString("User entered the conference, room=%1, user=%2, role=%3, affiliation=%4").arg(FRoomJid.bare(),fromNick,role,affiliation));
			}
			else
			{
				if (user->data(MUDR_ROLE).toString() != role)
					LOG_STRM_DEBUG(FStreamJid,QString("User role changed, room=%1, user=%2, role=%3").arg(FRoomJid.bare(),fromNick,role));
				if (user->data(MUDR_AFFILIATION).toString() != affiliation)
					LOG_STRM_DEBUG(FStreamJid,QString("User affiliation changed, room=%1, user=%2, role=%3").arg(FRoomJid.bare(),fromNick,affiliation));
			}

			user->setData(MUDR_SHOW,show);
			user->setData(MUDR_STATUS,status);
			user->setData(MUDR_REAL_JID,realJid.full());
			user->setData(MUDR_ROLE,role);
			user->setData(MUDR_AFFILIATION,affiliation);

			if (!FUsers.contains(fromNick))
			{
				connect(user->instance(),SIGNAL(dataChanged(int,const QVariant &, const QVariant &)),
					SLOT(onUserDataChanged(int,const QVariant &, const QVariant &)));
				FUsers.insert(fromNick,user);
			}

			if (!isOpen() && (fromNick==FNickName || FStatusCodes.contains(MUC_SC_ROOM_ENTER)))
			{
				LOG_STRM_INFO(FStreamJid,QString("You entered the conference, room=%1, nick=%2, role=%3, affiliation=%4").arg(FRoomJid.bare(),fromNick,role,affiliation));
				FNickName = fromNick;
				FMainUser = user;
				emit chatOpened();
			}

			LOG_STRM_DEBUG(FStreamJid,QString("User changed status in conference, room=%1, user=%2, show=%3, status=%4").arg(FRoomJid.bare(),fromNick).arg(show).arg(status));

			if (user == FMainUser)
			{
				FShow = show;
				FStatus = status;
				emit presenceChanged(FShow,FStatus);
			}

			emit userPresence(user,show,status);
			accepted = true;
		}
	}
	else if (AStanza.type() == "unavailable")
	{
		MultiUser *user = FUsers.value(fromNick);
		if (user)
		{
			bool applyPresence = true;
			int show = IPresence::Offline;
			QString status = AStanza.firstElement("status").text();
			QString role = itemElem.attribute("role");
			QString affiliation = itemElem.attribute("affiliation");

			if (FStatusCodes.contains(MUC_SC_NICK_CHANGED))
			{
				QString newNick = itemElem.attribute("nick");
				if (!newNick.isEmpty())
				{
					LOG_STRM_DEBUG(FStreamJid,QString("Changing user nick from=%1 to=%2 in room=%3").arg(fromNick,newNick,FRoomJid.bare()));
					if (!FUsers.contains(newNick))
					{
						applyPresence = false;
						FUsers.remove(fromNick);
						FUsers.insert(newNick,user);
					}
					user->setNickName(newNick);
					emit userNickChanged(user,fromNick,newNick);

					if (user == FMainUser)
					{
						FNickName = newNick;
						sendPresence(FShow,FStatus);
					}
				}
				else
				{
					LOG_STRM_ERROR(FStreamJid,QString("Failed to changes user nick, room=%1, user=%2: New nick is empty").arg(FRoomJid.bare(),fromNick));
				}
			}
			else if (FStatusCodes.contains(MUC_SC_USER_KICKED))
			{
				QString byUser = itemElem.firstChildElement("actor").attribute("jid");
				QString reason = itemElem.firstChildElement("reason").text();
				LOG_STRM_DEBUG(FStreamJid,QString("User was kicked from conference, room=%1, user=%2, by=%3: %4").arg(FRoomJid.bare(),fromNick,byUser,reason));
				emit userKicked(fromNick,reason,byUser);
			}
			else if (FStatusCodes.contains(MUC_SC_USER_BANNED))
			{
				QString byUser = itemElem.firstChildElement("actor").attribute("jid");
				QString reason = itemElem.firstChildElement("reason").text();
				LOG_STRM_DEBUG(FStreamJid,QString("User was banned in conference, room=%1, user=%2, by=%3: %4").arg(FRoomJid.bare(),fromNick,byUser,reason));
				emit userBanned(fromNick,reason,byUser);
			}
			else if (!xelem.firstChildElement("destroy").isNull())
			{
				QString reason = xelem.firstChildElement("destroy").firstChildElement("reason").text();
				LOG_STRM_DEBUG(FStreamJid,QString("Conference was destroyed by owner, room=%1: %2").arg(FRoomJid.bare(),reason));
				emit roomDestroyed(reason);
			}

			if (applyPresence)
			{
				user->setData(MUDR_SHOW,show);
				user->setData(MUDR_STATUS,status);
				user->setData(MUDR_ROLE,role);
				user->setData(MUDR_AFFILIATION,affiliation);
				emit userPresence(user,show,status);

				if (user != FMainUser)
				{
					LOG_STRM_DEBUG(FStreamJid,QString("User leave the conference, room=%1, user=%2, show=%3, status=%4").arg(FRoomJid.bare(),fromNick).arg(show).arg(status));
					FUsers.remove(fromNick);
					delete user;
				}
				else
				{
					LOG_STRM_INFO(FStreamJid,QString("You leave the conference, room=%1, show=%2, status=%3").arg(FRoomJid.bare(),role,affiliation));
					closeChat(show,status);
				}
			}

			accepted = true;
		}
	}
	else if (AStanza.type() == "error")
	{
		XmppStanzaError err(AStanza);
		LOG_STRM_WARNING(FStreamJid,QString("Error precence received in conference, room=%1, user=%2: %3").arg(FRoomJid.bare(),fromNick,err.errorMessage()));
		emit chatError(!fromNick.isEmpty() ? QString("%1 - %2").arg(fromNick).arg(err.errorMessage()) : err.errorMessage());

		if (fromNick == FNickName)
		{
			FRoomError = err;
			closeChat(IPresence::Error,err.errorMessage());
		}

		accepted = true;
	}

	FStatusCodes.clear();
	return accepted;
}

void MultiUserChat::closeChat(int AShow, const QString &AStatus)
{
	FConnected = false;
	LOG_STRM_INFO(FStreamJid,QString("Closing conference, room=%1").arg(FRoomJid.bare()));

	if (FMainUser)
	{
		FMainUser->setData(MUDR_SHOW,AShow);
		FMainUser->setData(MUDR_STATUS,AStatus);
		emit userPresence(FMainUser,AShow,AStatus);
		delete FMainUser;
	}
	FMainUser = NULL;
	FUsers.remove(FNickName);

	foreach(MultiUser *user, FUsers)
	{
		user->setData(MUDR_SHOW,IPresence::Offline);
		user->setData(MUDR_STATUS,QString());
		emit userPresence(user,IPresence::Offline,QString::null);
	}
	qDeleteAll(FUsers);
	FUsers.clear();

	FShow = AShow;
	FStatus = AStatus;
	emit presenceChanged(FShow,FStatus);

	emit chatClosed();
}

void MultiUserChat::onUserDataChanged(int ARole, const QVariant &ABefore, const QVariant &AAfter)
{
	IMultiUser *user = qobject_cast<IMultiUser *>(sender());
	if (user)
		emit userDataChanged(user,ARole,ABefore,AAfter);
}

void MultiUserChat::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
	Q_UNUSED(APriority);
	if (FAutoPresence)
		sendPresence(AShow,AStatus);
}

void MultiUserChat::onDiscoveryInfoReceived(const IDiscoInfo &AInfo)
{
	if (AInfo.streamJid==streamJid() && AInfo.contactJid==roomJid())
	{
		int index = FDiscovery->findIdentity(AInfo.identity,"conference","text");
		if (index>=0 && !AInfo.identity.at(index).name.isEmpty())
		{
			FRoomName = AInfo.identity.at(index).name;
			LOG_STRM_INFO(FStreamJid,QString("Conference name changed, room=%1: %2").arg(FRoomJid.bare(),FRoomName));
			emit roomNameChanged(FRoomName);
		}
	}
}

void MultiUserChat::onPresenceAboutToClose(int AShow, const QString &AStatus)
{
	if (isConnected())
		sendPresence(AShow,AStatus);
}

void MultiUserChat::onStreamClosed()
{
	if (!FUsers.isEmpty())
		closeChat(IPresence::Offline,QString::null);
}

void MultiUserChat::onStreamJidChanged(const Jid &ABefore)
{
	IXmppStream *xmppStream = qobject_cast<IXmppStream *>(sender());
	if (xmppStream)
	{
		FStreamJid = xmppStream->streamJid();
		foreach(MultiUser *user, FUsers)
			user->setData(MUDR_STREAM_JID,FStreamJid.full());
		emit streamJidChanged(ABefore,FStreamJid);
	}
}
