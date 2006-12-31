#include "presence.h"
#include "utils/errorhandler.h"

Presence::Presence(IXmppStream *AStream, IStanzaProcessor *AStanzaProcessor, QObject *parent)
  : QObject(parent)
{
  connect(AStream->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *))); 
  connect(AStream->instance(),SIGNAL(aboutToClose(IXmppStream *)),SLOT(onStreamAboutToClose(IXmppStream *))); 
  connect(AStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *))); 
  FStream = AStream;
  FStanzaProcessor = AStanzaProcessor;
  FPresenceHandler = 0;
  FShow = Offline;
  FPriority = 0;
}

Presence::~Presence()
{

}

bool Presence::stanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AStreamJid);
  bool hooked = false;
  if (AHandlerId == FPresenceHandler)
  {
    if (AStanza.from().isEmpty())
      return false;

    PresenceItem *pItem = (PresenceItem *)item(AStanza.from());
    if (!pItem)
    {
      pItem = new PresenceItem(AStanza.from(),this);
      FItems.append(pItem); 
    }
     
    if (AStanza.type() == "error")
    {
      ErrorHandler err(ErrorHandler::DEFAULTNS,AStanza.element(),this);
      pItem->setShow(Error);
      pItem->setStatus(err.meaning());
      pItem->setPriority(0);
      emit presenceItem(pItem);
      hooked = true;
    }
    else if (AStanza.type() == "unavailable")
    {
      pItem->setShow(Offline);
      pItem->setStatus(AStanza.firstElement("status").firstChild().toText().data());
      pItem->setPriority(0);
      emit presenceItem(pItem);
      FItems.removeAt(FItems.indexOf(pItem));
      delete pItem;
      hooked = true;
    }
    else if (AStanza.type().isEmpty())
    {
      QString showText = AStanza.firstElement("show").firstChild().toText().data();
      Show show;
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
      pItem->setShow(show);
      pItem->setStatus(AStanza.firstElement("status").firstChild().toText().data());
      pItem->setPriority(AStanza.firstElement("status").firstChild().toText().data().toInt());  
      emit presenceItem(pItem);
      hooked = true;
    }
  }
  AAccept = AAccept || hooked;
  return hooked;
}

IPresenceItem *Presence::item(const Jid &AItemJid) const
{
  PresenceItem *pItem;
  foreach(pItem, FItems)
    if (pItem->jid() == AItemJid)
      return pItem;
  return 0;
}

QList<IPresenceItem *> Presence::items() const
{
  QList<IPresenceItem *> pItems;
  PresenceItem *pItem;
  foreach(pItem, FItems)
    pItems.append(pItem); 
  return pItems;
}

QList<IPresenceItem *> Presence::items(const Jid &AItemJid) const
{
  QList<IPresenceItem *> pItems;
  PresenceItem *pItem;
  foreach(pItem, FItems)
    if (pItem->jid().equals(AItemJid,false))
      pItems.append(pItem); 
  return pItems;
}

bool Presence::setPresence(Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid)
{

  if (FStream->isOpen())
  {
    QString show;
    switch (AShow)
    {
    case Online: show = ""; break; 
    case Chat: show = "chat"; break; 
    case Away: show = "away"; break;
    case DoNotDistrib: show = "dnd"; break; 
    case ExtendedAway: show = "xa"; break;
    case Offline: break;
    default: return false;
    }

    Stanza pres("presence");
    if (AToJid.isValid())
      pres.setTo(AToJid.full());

    if (AShow != Offline)
    {
      if (!show.isEmpty())
        pres.addElement("show").appendChild(pres.createTextNode(show));
      pres.addElement("priority").appendChild(pres.createTextNode(QString::number(APriority)));  
    } 
    else pres.setType("unavailable");

    if (!AStatus.isEmpty()) 
      pres.addElement("status").appendChild(pres.createTextNode(AStatus));
    
    if (FStanzaProcessor->sendStanzaOut(FStream->jid(), pres))
    {
      if (!AToJid.isValid())
      {
        FShow = AShow;
        FStatus = AStatus;
        FPriority = APriority;
        emit selfPresence(FShow,FStatus,FPriority,AToJid);
      }
      return true;
    }
  } 
  else 
  {
    FShow = AShow;
    FStatus = AStatus;
    FPriority = APriority;
  }

  return false; 
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

void Presence::onStreamOpened(IXmppStream *)
{
  if (!FPresenceHandler)
    FPresenceHandler = FStanzaProcessor->setHandler(this,"/presence",IStanzaProcessor::DirectionIn,0,FStream->jid());   
  emit opened();
  //setPresence(IPresence::Online,"Hellow world",5); 
}

void Presence::onStreamAboutToClose(IXmppStream *)
{
  if (FShow != Offline)
    setPresence(IPresence::Offline,"Disconnected",0); 
}

void Presence::onStreamClosed(IXmppStream *)
{
  FStanzaProcessor->removeHandler(FPresenceHandler);
  FPresenceHandler = 0;
  emit closed();
}