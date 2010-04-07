#include "presence.h"

#define SHC_PRESENCE  "/presence"

Presence::Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
  FStanzaProcessor = AStanzaProcessor;

  FOpened = false;
  FShow = Offline;
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

bool Presence::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FSHIPresence)
  {
    int show;
    int priority;
    QString status;
    if (AStanza.type().isEmpty())
    {
      QString showText = AStanza.firstElement("show").text();
      if (showText.isEmpty())
        show = Online;
      else if (showText == "chat")
        show = Chat;
      else if (showText == "away")
        show = Away;
      else if (showText == "dnd")
        show = DoNotDisturb;
      else if (showText == "xa")
        show = ExtendedAway;
      else 
        show = Online;    // остыль под кривые клиенты и транспорты

      status = AStanza.firstElement("status").text();
      priority = AStanza.firstElement("priority").text().toInt();
    }
    else if (AStanza.type() == "unavailable")
    {
      show = Offline;
      status = AStanza.firstElement("status").text();
      priority = 0;
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      show = Error;
      status = err.message();
      priority = 0;
    }
    else
      return false;

    if (!AStanza.from().isEmpty() && AStreamJid != AStanza.from())
    {
      Jid fromJid = AStanza.from();

      IPresenceItem &pitem = FItems[fromJid];
      pitem.isValid = true;
      pitem.itemJid = fromJid;
      pitem.show = show;
      pitem.priority = priority;
      pitem.status = status;
      emit received(pitem);

      if (show == Offline)
        FItems.remove(fromJid);
    }
    else if (show!=IPresence::Offline && (FShow != show || FStatus != status || FPriority != priority))
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
    foreach(IPresenceItem pitem, FItems)
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
  if (FXmppStream->isOpen() && AShow != Error)
  {
    QString show;
    switch (AShow)
    {
    case Online: 
      show = ""; 
      break; 
    case Chat: 
      show = "chat"; 
      break; 
    case Away: 
      show = "away"; 
      break;
    case DoNotDisturb: 
      show = "dnd"; 
      break; 
    case ExtendedAway: 
      show = "xa"; 
      break;
    case Invisible: 
      show=""; 
      break;
    case Offline: 
      show=""; 
      break;
    default: 
      return false;
    }

    Stanza pres("presence");
    if (AShow == Invisible)
    {
      pres.setType("invisible");
    }
    else if (AShow == Offline)
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

    if (FOpened && AShow==Offline)
      emit aboutToClose(AShow, AStatus);

    if (FStanzaProcessor->sendStanzaOut(FXmppStream->streamJid(), pres))
    {
      FShow = AShow;
      FStatus = AStatus;
      FPriority = APriority;
      if (!FOpened && AShow!=Offline)
      {
        FOpened = true;
        emit opened();
      }
      emit changed(FShow,FStatus,FPriority);
      if (FOpened && AShow==Offline)
      {
        clearItems();
        FOpened = false;
        emit closed();
      }
      return true;
    }
  } 
  else if (AShow == Offline || AShow == Error)
  {
    FShow = AShow;
    FStatus = AStatus;
    FPriority = 0;
    if (FOpened)
    {
      emit aboutToClose(AShow,AStatus);
      clearItems();
      FOpened = false;
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
      case Online: 
        show = ""; 
        break; 
      case Chat: 
        show = "chat"; 
        break; 
      case Away: 
        show = "away"; 
        break;
      case DoNotDisturb: 
        show = "dnd"; 
        break; 
      case ExtendedAway: 
        show = "xa"; 
        break;
      case Invisible: 
        show=""; 
        break;
      case Offline: 
        show=""; 
        break;
      default: 
        return false;
    }

    Stanza pres("presence");
    pres.setTo(AContactJid.eFull());
    if (AShow == Invisible)
    {
      pres.setType("invisible");
    }
    else if (AShow == Offline)
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
      emit sent(AContactJid,AShow,AStatus,APriority);
      return true;
    }
  }
  return false;
}

void Presence::clearItems()
{
  QList<Jid> items = FItems.keys();
  foreach(Jid itemJid, items)
  {
    IPresenceItem &pitem = FItems[itemJid];
    pitem.show = Offline;
    pitem.priority = 0;
    pitem.status.clear();
    emit received(pitem);
    FItems.remove(itemJid);
  }
}

void Presence::onStreamClosed()
{
  if (isOpen())
    setPresence(Offline,"",0);
}

void Presence::onStreamError(const QString &AError)
{
  setPresence(Error,AError,0);
}
