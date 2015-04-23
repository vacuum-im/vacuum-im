#include "presence.h"

#include <definitions/namespaces.h>
#include <utils/datetime.h>
#include <utils/logger.h>

#define SHC_PRESENCE  "/presence"

Presence::Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
	FStanzaProcessor = AStanzaProcessor;

	FOpened = false;
	FShow = IPresence::Offline;
	FPriority = 0;

	IStanzaHandle shandle;
	shandle.handler = this;
	shandle.order = SHO_DEFAULT;
	shandle.direction = IStanzaHandle::DirectionIn;
	shandle.streamJid = FXmppStream->streamJid();
	shandle.conditions.append(SHC_PRESENCE);
	FSHIPresence = FStanzaProcessor->insertStanzaHandle(shandle);

	connect(AXmppStream->instance(),SIGNAL(error(const XmppError &)),SLOT(onXmppStreamError(const XmppError &)));
	connect(AXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));
}

Presence::~Presence()
{
	FStanzaProcessor->removeStanzaHandle(FSHIPresence);
	emit presenceDestroyed();
}

bool Presence::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId==FSHIPresence && FOpened)
	{
		int show;
		int priority;
		QString status;
		if (AStanza.type().isEmpty())
		{
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

			status = AStanza.firstElement("status").text();
			priority = AStanza.firstElement("priority").text().toInt();
		}
		else if (AStanza.type() == "unavailable")
		{
			show = IPresence::Offline;
			status = AStanza.firstElement("status").text();
			priority = 0;
		}
		else if (AStanza.type() == "error")
		{
			XmppStanzaError err(AStanza);
			show = IPresence::Error;
			status = err.errorMessage();
			priority = 0;
		}
		else
		{
			return false;
		}

		LOG_STRM_DEBUG(streamJid(),QString("Presence received, from=%1, show=%2, status=%3, priority=%4").arg(AStanza.from()).arg(show).arg(status).arg(priority));

		if (AStreamJid != AStanza.from())
		{
			Jid fromJid = AStanza.from();

			QMap<QString, IPresenceItem> &bareItems = FItems[fromJid.bare()];
			IPresenceItem &pitem = bareItems[fromJid.resource()];
			IPresenceItem before = pitem;

			pitem.itemJid = fromJid;
			pitem.show = show;
			pitem.priority = priority;
			pitem.status = status;

			if (pitem != before)
			{
				QDomElement delayElem = AStanza.firstElement("delay",NS_XMPP_DELAY);
				if (!delayElem.isNull())
					pitem.sentTime = DateTime(delayElem.attribute("stamp")).toLocal();
				else
					pitem.sentTime = QDateTime::currentDateTime();

				for (QMap<QString, IPresenceItem>::iterator it = bareItems.begin(); it!=bareItems.end(); )
				{
					if (it->show==IPresence::Error && it.key()!=fromJid.resource())
						it = bareItems.erase(it);
					else
						++it;
				}

				emit itemReceived(pitem,before);
			}
			
			if (show == IPresence::Offline)
			{
				if (bareItems.count() > 1)
					bareItems.remove(fromJid.resource());
				else
					FItems.remove(fromJid.bare());
			}
		}
		else if (show!=IPresence::Offline && (FShow!=show || FStatus!=status || FPriority!=priority))
		{
			FShow = show;
			FStatus = status;
			FPriority = priority;
			emit changed(show,status,priority);
		}
		AAccept = true;
	}
	else if (AHandlerId == FSHIPresence)
	{
		LOG_STRM_WARNING(streamJid(),QString("Failed to process presence, from=%1: Presence not opened").arg(AStanza.from()));
	}
	return false;
}

Jid Presence::streamJid() const
{
	return FXmppStream->streamJid();
}

IXmppStream *Presence::xmppStream() const
{
	return FXmppStream;
}

bool Presence::isOpen() const
{
	return FOpened;
}

int Presence::show() const
{
	return FShow;
}

QString Presence::status() const
{
	return FStatus;
}

bool Presence::setShow(int AShow)
{
	return setPresence(AShow,FStatus,FPriority);
}

bool Presence::setStatus(const QString &AStatus)
{
	return setPresence(FShow,AStatus,FPriority);
}

int Presence::priority() const
{
	return FPriority;
}

bool Presence::setPriority(int APriority)
{
	return setPresence(FShow,FStatus,APriority);
}

bool Presence::setPresence(int AShow, const QString &AStatus, int APriority)
{
	if (FXmppStream->isOpen() && AShow!=IPresence::Error)
	{
		QString show;
		switch (AShow)
		{
		case IPresence::Online:
			show = QString::null;
			break;
		case IPresence::Chat:
			show = "chat";
			break;
		case IPresence::Away:
			show = "away";
			break;
		case IPresence::DoNotDisturb:
			show = "dnd";
			break;
		case IPresence::ExtendedAway:
			show = "xa";
			break;
		case IPresence::Invisible:
			show = QString::null;
			break;
		case IPresence::Offline:
			show = QString::null;
			break;
		default:
			return false;
		}

		Stanza stanza("presence");
		if (AShow == IPresence::Invisible)
		{
			stanza.setType("invisible");
		}
		else if (AShow == IPresence::Offline)
		{
			stanza.setType("unavailable");
		}
		else
		{
			if (!show.isEmpty())
				stanza.addElement("show").appendChild(stanza.createTextNode(show));
			stanza.addElement("priority").appendChild(stanza.createTextNode(QString::number(APriority)));
		}

		if (!AStatus.isEmpty())
			stanza.addElement("status").appendChild(stanza.createTextNode(AStatus));

		if (FOpened && AShow==IPresence::Offline)
			emit aboutToClose(AShow, AStatus);

		if (FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(),stanza))
		{
			FShow = AShow;
			FStatus = AStatus;
			FPriority = APriority;

			LOG_STRM_INFO(streamJid(),QString("Self presence sent, show=%1, status=%2, priority=%3").arg(AShow).arg(AStatus).arg(APriority));

			if (!FOpened && AShow!=IPresence::Offline)
			{
				FOpened = true;
				emit opened();
			}

			emit changed(FShow,FStatus,FPriority);

			if (FOpened && AShow==IPresence::Offline)
			{
				FOpened = false;
				clearPresenceItems();
				emit closed();
			}

			return true;
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to send self presence, show=%1, status=%2, priority=%3").arg(AShow).arg(AStatus).arg(APriority));
		}
	}
	else if (AShow==IPresence::Offline || AShow==IPresence::Error)
	{
		FShow = AShow;
		FStatus = AStatus;
		FPriority = 0;

		LOG_STRM_INFO(streamJid(),QString("Self presence changed, show=%1, status=%2, priority=%3").arg(AShow).arg(AStatus).arg(APriority));

		if (FOpened)
		{
			FOpened = false;
			clearPresenceItems();
			emit closed();
		}

		emit changed(FShow,FStatus,FPriority);
		return true;
	}
	return false;
}

bool Presence::sendPresence(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority)
{
	if (FXmppStream->isOpen() && AContactJid.isValid() && AContactJid!=FXmppStream->streamJid().domain())
	{
		QString show;
		switch (AShow)
		{
		case IPresence::Online:
			show = QString::null;
			break;
		case IPresence::Chat:
			show = "chat";
			break;
		case IPresence::Away:
			show = "away";
			break;
		case IPresence::DoNotDisturb:
			show = "dnd";
			break;
		case IPresence::ExtendedAway:
			show = "xa";
			break;
		case IPresence::Invisible:
			show = QString::null;
			break;
		case IPresence::Offline:
			show = QString::null;
			break;
		default:
			return false;
		}

		Stanza pres("presence");
		pres.setTo(AContactJid.full());
		if (AShow == IPresence::Invisible)
		{
			pres.setType("invisible");
		}
		else if (AShow == IPresence::Offline)
		{
			pres.setType("unavailable");
		}
		else
		{
			if (!show.isEmpty())
				pres.addElement("show").appendChild(pres.createTextNode(show));
			pres.addElement("priority").appendChild(pres.createTextNode(QString::number(APriority)));
		}

		if (!AStatus.isEmpty())
			pres.addElement("status").appendChild(pres.createTextNode(AStatus));

		if (FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(), pres))
		{
			LOG_STRM_INFO(streamJid(),QString("Direct presence sent, to=%1, show=%2, status=%3, priority=%4").arg(AContactJid.full()).arg(AShow).arg(AStatus).arg(APriority));
			emit directSent(AContactJid,AShow,AStatus,APriority);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(streamJid(),QString("Failed to send direct presence, to=%1, show=%2, status=%3, priority=%4").arg(AContactJid.full()).arg(AShow).arg(AStatus).arg(APriority));
		}
	}
	else if (!FXmppStream->isOpen())
	{
		LOG_STRM_WARNING(streamJid(),QString("Failed to send direct presence, to=%1, show=%2, status=%3, priority=%4: Stream not opened").arg(AContactJid.full()).arg(AShow).arg(AStatus).arg(APriority));
	}
	else if (AContactJid == FXmppStream->streamJid().domain())
	{
		REPORT_ERROR("Failed to send direct presence: Invalid destination");
	}
	else
	{
		REPORT_ERROR("Failed to send direct presence: Invalid params");
	}
	return false;
}

QList<Jid> Presence::itemsJid() const
{
	QList<Jid> pitemsJid;
	pitemsJid.reserve(FItems.count());

	for (QHash<Jid, QMap<QString, IPresenceItem> >::const_iterator bareIt=FItems.constBegin(); bareIt!=FItems.constEnd(); ++bareIt)
		for (QMap<QString, IPresenceItem>::const_iterator fullIt=bareIt->constBegin(); fullIt!=bareIt->constEnd(); ++fullIt)
			pitemsJid.append(fullIt->itemJid);

	return pitemsJid;
}

QList<IPresenceItem> Presence::items() const
{
	QList<IPresenceItem> pitems;
	for (QHash<Jid, QMap<QString, IPresenceItem> >::const_iterator it=FItems.constBegin(); it!=FItems.constEnd(); ++it)
		pitems += it->values();
	return pitems;
}

IPresenceItem Presence::findItem(const Jid &AItemFullJid) const
{
	return FItems.value(AItemFullJid.bare()).value(AItemFullJid.resource());
}

QList<IPresenceItem> Presence::findItems(const Jid &AItemBareJid) const
{
	return FItems.value(AItemBareJid.bare()).values();
}

void Presence::clearPresenceItems()
{
	for (QHash<Jid, QMap<QString, IPresenceItem> >::iterator bareIt=FItems.begin(); bareIt!=FItems.end(); bareIt = FItems.erase(bareIt))
	{
		for (QMap<QString, IPresenceItem>::iterator fullIt=bareIt->begin(); fullIt!=bareIt->end(); fullIt=bareIt->erase(fullIt))
		{
			IPresenceItem before = fullIt.value();
			fullIt->priority = 0;
			fullIt->status = QString::null;
			fullIt->show = IPresence::Offline;
			emit itemReceived(fullIt.value(),before);
		}
	}
}

void Presence::onXmppStreamError(const XmppError &AError)
{
	setPresence(IPresence::Error,AError.errorMessage(),0);
}

void Presence::onXmppStreamClosed()
{
	if (isOpen())
		setPresence(IPresence::Error,tr("XMPP stream closed unexpectedly"),0);
}
