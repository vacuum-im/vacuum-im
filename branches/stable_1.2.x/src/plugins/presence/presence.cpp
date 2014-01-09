#include "presence.h"

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

	connect(AXmppStream->instance(),SIGNAL(error(const QString &)),SLOT(onStreamError(const QString &)));
	connect(AXmppStream->instance(),SIGNAL(closed()),SLOT(onStreamClosed()));
}

Presence::~Presence()
{
	FStanzaProcessor->removeStanzaHandle(FSHIPresence);
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
			return false;

		if (AStreamJid != AStanza.from())
		{
			Jid fromJid = AStanza.from();

			IPresenceItem &pitem = FItems[fromJid];
			IPresenceItem before = pitem;

			pitem.isValid = true;
			pitem.itemJid = fromJid;
			pitem.show = show;
			pitem.priority = priority;
			pitem.status = status;

			if (pitem != before)
			{
				for (QHash<Jid,IPresenceItem>::iterator it = FItems.begin(); it!=FItems.end(); )
				{
					if (it->show==IPresence::Error && it.key()==fromJid.bare() && it.key()!=fromJid)
						it = FItems.erase(it);
					else
						++it;
				}
				emit itemReceived(pitem,before);
			}
			
			if (show == IPresence::Offline)
				FItems.remove(fromJid);
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
	return false;
}

QList<IPresenceItem> Presence::presenceItems(const Jid &AItemJid) const
{
	if (!AItemJid.isEmpty())
	{
		QList<IPresenceItem> pitems;
		foreach(const IPresenceItem &pitem, FItems)
			if (AItemJid && pitem.itemJid)
				pitems.append(pitem);
		return pitems;
	}
	return FItems.values();
}

bool Presence::setShow(int AShow)
{
	return setPresence(AShow,FStatus,FPriority);
}

bool Presence::setStatus(const QString &AStatus)
{
	return setPresence(FShow,AStatus,FPriority);
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

			if (!FOpened && AShow!=IPresence::Offline)
			{
				FOpened = true;
				emit opened();
			}

			emit changed(FShow,FStatus,FPriority);

			if (FOpened && AShow==IPresence::Offline)
			{
				FOpened = false;
				clearItems();
				emit closed();
			}

			return true;
		}
	}
	else if (AShow==IPresence::Offline || AShow==IPresence::Error)
	{
		FShow = AShow;
		FStatus = AStatus;
		FPriority = 0;

		if (FOpened)
		{
			FOpened = false;
			clearItems();
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
			emit directSent(AContactJid,AShow,AStatus,APriority);
			return true;
		}
	}
	return false;
}

void Presence::clearItems()
{
	QList<Jid> items = FItems.keys();
	foreach(const Jid &itemJid, items)
	{
		IPresenceItem &pitem = FItems[itemJid];
		IPresenceItem before = pitem;
		pitem.show = Offline;
		pitem.priority = 0;
		pitem.status.clear();
		emit itemReceived(pitem,before);
		FItems.remove(itemJid);
	}
}

void Presence::onStreamClosed()
{
	if (isOpen())
		setPresence(IPresence::Error,tr("XMPP stream closed unexpectedly"),0);
}

void Presence::onStreamError(const QString &AError)
{
	setPresence(IPresence::Error,AError,0);
}
