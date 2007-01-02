#include "roster.h"
#include <QtDebug>

Roster::Roster(IXmppStream *AStream, IStanzaProcessor *AStanzaProcessor, QObject *parent) 
  : QObject(parent)
{
  connect(AStream->instance(),SIGNAL(opened(IXmppStream *)),SLOT(open()));
  connect(AStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(close())); 
  FStream = AStream;
  FStanzaProcessor = AStanzaProcessor;
  FOpen = false;
  FRosterHandler = 0;
  FSubscrHandler = 0;
}

Roster::~Roster()
{
  qDebug() << "~Roster";
}

bool Roster::stanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AStreamJid);
  bool hooked = false;
  if (AHandlerId == FRosterHandler)
  {
    if (!AStanza.from().isEmpty() && !FStream->jid().equals(AStanza.from(),false))
      return false;

    if (AStanza.type() != "error")
    {
      if (!FOpenId.isEmpty() && FOpenId == AStanza.id())
      {
        FOpenId.clear();
        FOpen = true;
        emit opened();
        hooked = true;
      }

      QDomElement stanzaItem = AStanza.element().firstChildElement("query").firstChildElement("item");
      while (!stanzaItem.isNull())
      {
        QString subscr = stanzaItem.attribute("subscription");
        if (subscr == "both" || subscr == "to" || subscr == "from" || subscr == "none")
        {
          Jid itemJid(stanzaItem.attribute("jid"));
          RosterItem *rosterItem = (RosterItem *)item(itemJid);
          if (!rosterItem)
          {
            if (!itemJid.node().isEmpty())  
              rosterItem = new RosterItem(itemJid.bare(),this);
            else
              rosterItem = new RosterItem(itemJid,this);
            FItems.append(rosterItem); 
          }
          rosterItem->setName(stanzaItem.attribute("name"));
          rosterItem->setSubscription(subscr);   
          rosterItem->setAsk(stanzaItem.attribute("ask"));
          
          QSet<QString> groups;
          QDomElement groupElem = stanzaItem.firstChildElement("group"); 
          while (!groupElem.isNull())
          {
            if (groupElem.firstChild().isText())
              groups.insert(groupElem.firstChild().toText().data());
            groupElem = groupElem.nextSiblingElement("group"); 
          }
          rosterItem->setGroups(groups); 

          emit itemPush(rosterItem); 
          hooked = true;
        }
        else if (subscr == "remove")
        {
          RosterItem *rosterItem = (RosterItem *)item(stanzaItem.attribute("jid"));
          if (rosterItem)
          {
            emit itemRemoved(rosterItem);
            FItems.removeAt(FItems.indexOf(rosterItem));  
            delete rosterItem;
          }
          hooked = true;
        }
        stanzaItem = stanzaItem.nextSiblingElement("item");  
      }

      if (AStanza.type() == "set" && !AStanza.id().isEmpty() && hooked)
      {
        Stanza result("iq");
        result.setId(AStanza.id()).setType("result");  
        FStanzaProcessor->sendStanzaOut(FStream->jid(),result); 
      }
    }
    else
    {
      if (FOpenId == AStanza.id())
        FOpenId.clear(); 
      hooked = true;
    }
  }
  else if (AHandlerId == FSubscrHandler)
  {
    if (AStanza.type() == "subscribe")
    {
      emit subscription(AStanza.from(),IRoster::Subscribe); 
      hooked = true;
    }
    else if (AStanza.type() == "subscribed")
    {
      emit subscription(AStanza.from(),IRoster::Subscribed); 
      hooked = true;
    }
    else if (AStanza.type() == "unsubscribe")
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribe); 
      hooked = true;
    }
    else if (AStanza.type() == "unsubscribed")
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribed); 
      hooked = true;
    }
  }
  AAccept = AAccept || hooked;
  return hooked;
}

void Roster::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  Q_UNUSED(AStreamJid);
  Q_UNUSED(AStanza);
}

void Roster::iqStanzaTimeOut(const QString &AId)
{
  Q_UNUSED(AId);
}

IRosterItem *Roster::item(const Jid &AItemJid) const
{
  RosterItem *item;
  foreach(item, FItems)
    if (item->jid().equals(AItemJid,false))
      return item; 
  return 0;
}

QList<IRosterItem *> Roster::items() const
{
  QList<IRosterItem *> items;
  RosterItem *item;
  foreach(item, FItems)
    items.append(item); 
  return items;
}

QList<IRosterItem *> Roster::groupItems(const QString &AGroup) const
{
  QList<IRosterItem *> items;
  RosterItem *item;
  foreach(item, FItems)
    if (item->groups().contains(AGroup))
      items.append(item); 
  return items;
}

QSet<QString> Roster::groups() const
{
  QSet<QString> groups;
  RosterItem *item;
  foreach(item, FItems)
    groups += item->groups();
  return groups;
}

void Roster::open()
{
  if (!FStream->isOpen() || isOpen() || !FOpenId.isEmpty())
    return;

  FRosterHandler = FStanzaProcessor->setHandler(this,"/iq/query[@xmlns='jabber:iq:roster']",
    IStanzaProcessor::DirectionIn,0,FStream->jid()); 
  FSubscrHandler = FStanzaProcessor->setHandler(this,"/presence[@type]",
    IStanzaProcessor::DirectionIn,0,FStream->jid());
  FOpenId = FStanzaProcessor->newId();
  Stanza query("iq");
  query.setId(FOpenId).setType("get");
  query.addElement("query","jabber:iq:roster"); 
  if (!FStanzaProcessor->sendStanzaOut(FStream->jid(),query))
    close();  
}

void Roster::close()
{
  if (isOpen())
    emit closed();
  
  if (FRosterHandler!=0)
  {
    FStanzaProcessor->removeHandler(FRosterHandler); 
    FRosterHandler = 0;
  }
  if (FSubscrHandler!=0)
  {
    FStanzaProcessor->removeHandler(FSubscrHandler); 
    FSubscrHandler = 0;
  }

  FOpen = false;
  FOpenId.clear();
  qDeleteAll(FItems);
}

void Roster::setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups)
{
  Stanza query("iq");
  query.setId(FStanzaProcessor->newId()).setType("set");
  QDomElement item = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  if (!AItemJid.node().isEmpty())
    item.setAttribute("jid",AItemJid.bare());
  else
    item.setAttribute("jid",AItemJid.full());
  if (!AName.isEmpty()) 
    item.setAttribute("name",AName); 
  
  QSet<QString>::const_iterator i = AGroups.begin();
  while (i!=AGroups.end())
    item.appendChild(query.createElement("group").appendChild(query.createTextNode(*i++)));

  FStanzaProcessor->sendIqStanza(this,FStream->jid(),query,0);
}

void Roster::removeItem(const Jid &AItemJid)
{
  Stanza query("iq");
  query.setId(FStanzaProcessor->newId()).setType("set");
  QDomElement item = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  if (!AItemJid.node().isEmpty())
    item.setAttribute("jid",AItemJid.bare());
  else
    item.setAttribute("jid",AItemJid.full());
  item.setAttribute("subscription","remove"); 

  FStanzaProcessor->sendIqStanza(this,FStream->jid(),query,0);
}

void Roster::sendSubscription(const Jid &AItemJid, IRoster::SubscriptionType AType)
{
  QString type;
  if (AType == IRoster::Subscribe)
    type = "subscribe";
  else if (AType == IRoster::Subscribed)
    type = "subscribed";
  else if (AType == IRoster::Unsubscribe)
    type = "unsubscribe";
  else if (AType == IRoster::Unsubscribed)
    type = "unsubscribed";
  else return;
  
  Stanza subscr("presence");
  subscr.setTo(AItemJid.bare()).setType(type); 
  FStanzaProcessor->sendStanzaOut(FStream->jid(),subscr);  
}

