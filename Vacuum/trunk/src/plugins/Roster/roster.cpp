#include <QtDebug>
#include "roster.h"

#include <QSet>

Roster::Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) 
  : QObject(AXmppStream->instance())
{
  FXmppStream = AXmppStream;
  FStanzaProcessor = AStanzaProcessor;
  FOpen = false;
  FRosterHandler = 0;
  FSubscrHandler = 0;
  connect(FXmppStream->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *))); 
}

Roster::~Roster()
{
  clearItems();
}

bool Roster::readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AStreamJid);
  bool hooked = false;
  if (AHandlerId == FRosterHandler)
  {
    if (!AStanza.from().isEmpty() && !FXmppStream->jid().equals(AStanza.from(),false))
      return false;

    if (AStanza.type() != "error")
    {
      QSet<RosterItem *> befourItems; 
      QSet<RosterItem *> afterItems; 
      if (!FOpenId.isEmpty() && FOpenId == AStanza.id())
      {
        befourItems = FRosterItems.toSet();
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
          Jid itemJid = stanzaItem.attribute("jid");
          RosterItem *rosterItem = (RosterItem *)item(itemJid);
          if (!rosterItem)
          {
            rosterItem = new RosterItem(itemJid.bare(),this);
            FRosterItems.append(rosterItem); 
          }
          rosterItem->setName(stanzaItem.attribute("name"));
          rosterItem->setSubscription(subscr);   
          rosterItem->setAsk(stanzaItem.attribute("ask"));
          
          QSet<QString> allItemGroups;
          QDomElement groupElem = stanzaItem.firstChildElement("group"); 
          while (!groupElem.isNull())
          {
            if (groupElem.firstChild().isText())
              allItemGroups.insert(groupElem.firstChild().toText().data());
            groupElem = groupElem.nextSiblingElement("group"); 
          }
          rosterItem->setGroups(allItemGroups); 

          if (!befourItems.isEmpty())
            afterItems.insert(rosterItem);

          emit itemPush(rosterItem); 
          hooked = true;
        }
        else if (subscr == "remove")
        {
          RosterItem *rosterItem = (RosterItem *)item(stanzaItem.attribute("jid"));
          if (rosterItem)
          {
            emit itemRemoved(rosterItem);
            FRosterItems.removeAt(FRosterItems.indexOf(rosterItem));  
            delete rosterItem;
          }
          hooked = true;
        }
        stanzaItem = stanzaItem.nextSiblingElement("item");  
      }

      if (!befourItems.isEmpty())
      {
        RosterItem *rosterItem;
        QSet<RosterItem *> oldItems = befourItems - afterItems;
        foreach(rosterItem,oldItems)
        {
          emit itemRemoved(rosterItem);
          FRosterItems.removeAt(FRosterItems.indexOf(rosterItem));
          delete rosterItem;
        }
      }

      if (AStanza.type() == "set" && !AStanza.id().isEmpty() && hooked)
      {
        Stanza result("iq");
        result.setId(AStanza.id()).setType("result");  
        FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),result); 
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
    QString status = AStanza.firstElement("status").text();
    if (AStanza.type() == "subscribe")
    {
      emit subscription(AStanza.from(),IRoster::Subscribe,status); 
      hooked = true;
    }
    else if (AStanza.type() == "subscribed")
    {
      emit subscription(AStanza.from(),IRoster::Subscribed,status); 
      hooked = true;
    }
    else if (AStanza.type() == "unsubscribe")
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribe,status); 
      hooked = true;
    }
    else if (AStanza.type() == "unsubscribed")
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribed,status); 
      hooked = true;
    }
  }
  AAccept = AAccept || hooked;
  return hooked;
}

void Roster::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  Q_UNUSED(AStreamJid);
  if (!FGroupDelimId.isEmpty() && AStanza.id() == FGroupDelimId)
  {
    FGroupDelimId.clear();

    QString groupDelim;
    if (AStanza.type() == "result")
      groupDelim = AStanza.firstElement("query","jabber:iq:private").firstChildElement("roster").text();

    if (groupDelim.isEmpty())
    {
      groupDelim = "::";
      Stanza query("iq");
      query.setType("set").setId(FStanzaProcessor->newId());
      QDomNode node = query.addElement("query","jabber:iq:private");
      node = node.appendChild(query.createElement("roster","roster:delimiter"));
      node.appendChild(query.createTextNode(groupDelim));
      FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,0);
    }

    FGroupDelim = groupDelim;

    if (!requestRosterItems())
      close();
  }
}

void Roster::iqStanzaTimeOut(const QString &AId)
{
  if (!FGroupDelimId.isEmpty() && AId == FGroupDelimId)
    close();
}

void Roster::open()
{
  if (!FXmppStream->isOpen() || isOpen() || !FOpenId.isEmpty() || !FGroupDelimId.isEmpty())
    return;

  setStanzaHandlers();

  if (!requestGroupDelimiter())
    close();
}

void Roster::close()
{
  if (isOpen())
    emit closed();

  removeStanzaHandlers();

  FOpen = false;
  FOpenId.clear();
  FGroupDelimId.clear();
}

IRosterItem *Roster::item(const Jid &AItemJid) const
{
  RosterItem *rosterItem;
  foreach(rosterItem, FRosterItems)
    if (rosterItem->jid().equals(AItemJid,false))
      return rosterItem; 
  return 0;
}

QList<IRosterItem *> Roster::items() const
{
  QList<IRosterItem *> rosterItems;
  RosterItem *rosterItem;
  foreach(rosterItem, FRosterItems)
    rosterItems.append(rosterItem); 
  return rosterItems;
}

QSet<QString> Roster::groups() const
{
  QSet<QString> allGroups;
  RosterItem *rosterItem;
  foreach(rosterItem, FRosterItems)
    if (!rosterItem->jid().node().isEmpty())
      allGroups += rosterItem->groups();
  return allGroups;
}

QList<IRosterItem *> Roster::groupItems(const QString &AGroup) const
{
  QList<IRosterItem *> rosterItems;
  RosterItem *rosterItem;
  foreach(rosterItem, FRosterItems)
  {
    QString itemGroup;
    QSet<QString> allItemGroups = rosterItem->groups();
    foreach(itemGroup,allItemGroups)
      if (itemGroup==AGroup || itemGroup.startsWith(AGroup+FGroupDelim))
      {
        rosterItems.append(rosterItem); 
        break;
      }
  }
  return rosterItems;
}

QSet<QString> Roster::itemGroups(const Jid &AItemJid) const
{
  IRosterItem *rosterItem = item(AItemJid);
  if (rosterItem)
    return rosterItem->groups();
  return QSet<QString>();
}

void Roster::setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups)
{
  Stanza query("iq");
  query.setId(FStanzaProcessor->newId()).setType("set");
  QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  if (!AItemJid.node().isEmpty())
    itemElem.setAttribute("jid",AItemJid.bare());
  else
    itemElem.setAttribute("jid",AItemJid.full());
  if (!AName.isEmpty()) 
    itemElem.setAttribute("name",AName); 

  QString groupName;
  foreach (groupName,AGroups)
    itemElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(groupName));

  FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,0);
}

void Roster::sendSubscription(const Jid &AItemJid, IRoster::SubsType AType, const QString &AStatus)
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
  if (!AStatus.isEmpty())
    subscr.addElement("status").appendChild(subscr.createTextNode(AStatus));
  FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),subscr);  
}

void Roster::removeItem(const Jid &AItemJid)
{
  Stanza query("iq");
  query.setId(FStanzaProcessor->newId()).setType("set");
  QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  if (!AItemJid.node().isEmpty())
    itemElem.setAttribute("jid",AItemJid.bare());
  else
    itemElem.setAttribute("jid",AItemJid.full());
  itemElem.setAttribute("subscription","remove"); 

  FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,0);
}

void Roster::renameItem(const Jid &AItemJid, const QString &AName)
{
  IRosterItem *rosterItem = item(AItemJid);
  if (rosterItem)
    setItem(AItemJid,AName,rosterItem->groups());
}

void Roster::copyItemToGroup(const Jid &AItemJid, const QString &AGroup)
{
  IRosterItem *rosterItem = item(AItemJid);
  if (rosterItem)
  {
    QSet<QString> allItemGroups = rosterItem->groups();
    setItem(AItemJid,rosterItem->name(),allItemGroups << AGroup);
  }
}

void Roster::moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo)
{
  IRosterItem *rosterItem = item(AItemJid);
  if (rosterItem)
  {
    QSet<QString> allItemGroups = rosterItem->groups();
    allItemGroups -= AGroupFrom;
    setItem(AItemJid,rosterItem->name(),allItemGroups += AGroupTo);
  }
}

void Roster::removeItemFromGroup( const Jid &AItemJid, const QString &AGroup )
{
  IRosterItem *rosterItem = item(AItemJid);
  if (rosterItem)
  {
    QSet<QString> allItemGroups = rosterItem->groups();
    if (allItemGroups.contains(AGroup))
      setItem(AItemJid,rosterItem->name(),allItemGroups -= AGroup);
  }
}

void Roster::renameGroup(const QString &AGroup, const QString &AGroupTo)
{
  IRosterItem *rosterItem;
  QList<IRosterItem *> allGroupItems = groupItems(AGroup);
  foreach(rosterItem, allGroupItems)
  {
    QString group;
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = rosterItem->groups();
    foreach(group,oldItemGroups)
    {
      QString newGroup = group;
      if (newGroup.startsWith(AGroup))
      {
        newGroup.remove(0,AGroup.size());
        newGroup.prepend(AGroupTo);
      }
      newItemGroups += newGroup;
    }
    setItem(rosterItem->jid(),rosterItem->name(),newItemGroups);
  }
}

void Roster::copyGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
  IRosterItem *rosterItem;
  QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
  foreach(rosterItem, FRosterItems)
  {
    QString group;
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = rosterItem->groups();
    foreach(group,oldItemGroups)
    {
      if (group.startsWith(AGroup))
      {
        QString newGroup = group;
        newGroup.remove(0,AGroup.size());
        if (!AGroupTo.isEmpty())
          newGroup.prepend(AGroupTo + FGroupDelim + groupName);
        else
          newGroup.prepend(groupName);
        
        newItemGroups += newGroup;
      }
    }
    setItem(rosterItem->jid(),rosterItem->name(),oldItemGroups+newItemGroups);
  }
}

void Roster::moveGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
  IRosterItem *rosterItem;
  QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
  foreach(rosterItem, FRosterItems)
  {
    QString group;
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = rosterItem->groups();
    foreach(group,oldItemGroups)
    {
      if (group.startsWith(AGroup))
      {
        QString newGroup = group;
        newGroup.remove(0,AGroup.size());
        if (!AGroupTo.isEmpty())
          newGroup.prepend(AGroupTo + FGroupDelim + groupName);
        else
          newGroup.prepend(groupName);
        newItemGroups += newGroup;
        oldItemGroups -= group;
      }
    }
    setItem(rosterItem->jid(),rosterItem->name(),oldItemGroups+newItemGroups);
  }
}

void Roster::removeGroup( const QString &AGroup )
{
  IRosterItem *rosterItem;
  QList<IRosterItem *> allGroupItems = groupItems(AGroup);
  foreach(rosterItem, allGroupItems)
  {
    QSet<QString> oldItemGroups = rosterItem->groups();
    QSet<QString>::iterator group = oldItemGroups.begin();
    while (group != oldItemGroups.end())
    {
      if ((*group).startsWith(AGroup))
        group = oldItemGroups.erase(group);
      else
        group++;
    }
    if (oldItemGroups.isEmpty())
      removeItem(rosterItem->jid());
    else
      setItem(rosterItem->jid(),rosterItem->name(),oldItemGroups);
  }
}

void Roster::clearItems()
{
  RosterItem *rosterItem;
  while (FRosterItems.count() >0)
  {
    rosterItem = FRosterItems.at(0);
    emit itemRemoved(rosterItem);
    FRosterItems.removeAt(0);
    delete rosterItem; 
  }
}

bool Roster::requestGroupDelimiter()
{
  FGroupDelimId = FStanzaProcessor->newId();
  Stanza query("iq");
  query.setType("get").setId(FGroupDelimId);
  query.addElement("query","jabber:iq:private").appendChild(query.createElement("roster","roster:delimiter"));
  return FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,30000);
}

bool Roster::requestRosterItems()
{
  FOpenId = FStanzaProcessor->newId();
  Stanza query("iq");
  query.setType("get").setId(FOpenId);
  query.addElement("query","jabber:iq:roster"); 
  return FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),query);
}

void Roster::setStanzaHandlers()
{
  FRosterHandler = FStanzaProcessor->setHandler(this,"/iq/query[@xmlns='jabber:iq:roster']",
    IStanzaProcessor::DirectionIn,0,FXmppStream->jid()); 

  FSubscrHandler = FStanzaProcessor->setHandler(this,"/presence[@type]",
    IStanzaProcessor::DirectionIn,0,FXmppStream->jid());
}

void Roster::removeStanzaHandlers()
{
  FStanzaProcessor->removeHandler(FRosterHandler); 
  FRosterHandler = 0;

  FStanzaProcessor->removeHandler(FSubscrHandler); 
  FSubscrHandler = 0;
}

void Roster::onStreamOpened(IXmppStream *)
{
  open();
}

void Roster::onStreamClosed(IXmppStream *)
{
  close();
}

