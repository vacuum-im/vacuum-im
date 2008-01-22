#include "presence.h"

#define SHC_PRESENCE  "/presence"

Presence::Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
  FStanzaProcessor = AStanzaProcessor;

  FOpened = false;
  FShow = Offline;
  FStatus = tr("Disconnected");
  FPriority = 0;

  connect(AXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *))); 
  connect(AXmppStream->instance(),SIGNAL(error(IXmppStream *, const QString &)),
    SLOT(onStreamError(IXmppStream *, const QString &))); 

  FPresenceHandler = FStanzaProcessor->insertHandler(this,SHC_PRESENCE,IStanzaProcessor::DirectionIn,SHP_DEFAULT,streamJid());   
}

Presence::~Presence()
{
  FStanzaProcessor->removeHandler(FPresenceHandler);
}

bool Presence::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FPresenceHandler)
  {
    Show show;
    QString status;
    int priority;
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
        show = DoNotDistrib;
      else if (showText == "xa")
        show = ExtendedAway;
      else show = Error;
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
      PresenceItem *pitem = (PresenceItem *)item(AStanza.from());
      if (!pitem)
      {
        pitem = new PresenceItem(AStanza.from(),this);
        FPresenceItems.append(pitem); 
      }
      pitem->setShow(show);
      pitem->setStatus(status);
      pitem->setPriority(priority);
      emit presenceItem(pitem);

      if (show == Offline)
      {
        FPresenceItems.removeAt(FPresenceItems.indexOf(pitem));
        delete pitem;
      }
    }
    else if (FShow != show || FStatus != status || FPriority != priority)
    {
      FShow = show;
      FStatus = status;
      FPriority = priority;
      emit selfPresence(show,status,priority,Jid());
    }
    AAccept = true;
    return true;
  }
  return false;
}

IPresenceItem *Presence::item(const Jid &AItemJid) const
{
  PresenceItem *pItem;
  foreach(pItem, FPresenceItems)
    if (pItem->jid() == AItemJid)
      return pItem;
  return 0;
}

QList<IPresenceItem *> Presence::items() const
{
  QList<IPresenceItem *> pItems;
  PresenceItem *pItem;
  foreach(pItem, FPresenceItems)
    pItems.append(pItem); 
  return pItems;
}

QList<IPresenceItem *> Presence::items(const Jid &AItemJid) const
{
  QList<IPresenceItem *> pItems;
  PresenceItem *pItem;
  foreach(pItem, FPresenceItems)
    if (AItemJid && pItem->jid())
      pItems.append(pItem); 
  return pItems;
}

bool Presence::setShow(Show AShow, const Jid &AToJid)
{
  return setPresence(AShow,FStatus,FPriority,AToJid);
}

bool Presence::setStatus(const QString &AStatus, const Jid &AToJid)
{
  return setPresence(FShow,AStatus,FPriority,AToJid);
}

bool Presence::setPriority(qint8 APriority, const Jid &AToJid)
{
  return setPresence(FShow,FStatus,APriority,AToJid);
}

bool Presence::setPresence(Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid)
{
  if (FXmppStream->isOpen() && AShow != Error)
  {
    QString show;
    switch (AShow)
    {
    case Online: show = ""; break; 
    case Chat: show = "chat"; break; 
    case Away: show = "away"; break;
    case DoNotDistrib: show = "dnd"; break; 
    case ExtendedAway: show = "xa"; break;
    case Invisible: show=""; break;
    case Offline: show=""; break;
    default: return false;
    }

    Stanza pres("presence");
    if (AToJid.isValid())
      pres.setTo(AToJid.eFull());

    if (AShow == Invisible)
      pres.setType("invisible");
    else if (AShow == Offline)
      pres.setType("unavailable");
    else
    {
      if (!show.isEmpty())
        pres.addElement("show").appendChild(pres.createTextNode(show));
      pres.addElement("priority").appendChild(pres.createTextNode(QString::number(APriority)));  
    } 

    if (!AStatus.isEmpty()) 
      pres.addElement("status").appendChild(pres.createTextNode(AStatus));

    if (!AToJid.isValid() && FOpened && (AShow == Offline || AShow==Error))
      emit aboutToClose(AShow, AStatus);

    if (FStanzaProcessor->sendStanzaOut(FXmppStream->jid(), pres))
    {
      if (!AToJid.isValid())
      {
        FShow = AShow;
        FStatus = AStatus;
        FPriority = APriority;
        if (!FOpened && AShow!=Offline && AShow!=Error)
        {
          FOpened = true;
          emit opened();
        }
      }
      emit selfPresence(FShow,FStatus,FPriority,AToJid);
      if (FOpened && (AShow==Offline || AShow==Error))
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
    if (!AToJid.isValid())
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
    }
    emit selfPresence(FShow,FStatus,FPriority,AToJid);
    return true;
  }
  return false; 
}

void Presence::clearItems()
{
  while (FPresenceItems.count() > 0)
  {
    PresenceItem *item = FPresenceItems.at(0);
    item->setShow(Offline);
    item->setStatus(tr("Disconnected"));
    item->setPriority(0);
    emit presenceItem(item);
    delete item;
    FPresenceItems.removeAt(0); 
  }
}

void Presence::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  if (isOpen())
    setPresence(Offline,tr("Disconnected"),0);
}

void Presence::onStreamError(IXmppStream * /*AXmppStream*/, const QString &AError)
{
  setPresence(IPresence::Error,AError,0);
}

