#include <QtDebug>
#include "roster.h"

#include <QSet>
#include <QFile>

Roster::Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) 
  : QObject(AXmppStream->instance())
{
  FStanzaProcessor = AStanzaProcessor;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *))); 
  connect(FXmppStream->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
    SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
 
  FOpen = false;
  FRosterHandler = 0;
  FSubscrHandler = 0;
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
    if (!AStanza.from().isEmpty() && !(FXmppStream->jid() && AStanza.from()))
      return false;

    if (AStanza.type() != "error")
    {
      QSet<RosterItem *> befourItems; 
      if (!FOpenId.isEmpty() && FOpenId == AStanza.id())
      {
        befourItems = FRosterItems.toSet();
        FOpenId.clear();
        FOpen = true;
        emit opened();
        hooked = true;
      }

      hooked = processItemsElement(AStanza.firstElement("query")) || hooked;

      if (!befourItems.isEmpty())
      {
        QSet<RosterItem *> oldItems = befourItems - FRosterItems.toSet();
        foreach(RosterItem *rosterItem,oldItems)
          removeRosterItem(rosterItem);
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

    setGroupDelimiter(groupDelim);

    requestRosterItems();
  }
}

void Roster::iqStanzaTimeOut(const QString &AId)
{
  if (!FGroupDelimId.isEmpty() && AId == FGroupDelimId)
    FXmppStream->close();
  if (!FOpenId.isEmpty() && AId == FOpenId)
    FXmppStream->close();
}

IRosterItem *Roster::item(const Jid &AItemJid) const
{
  foreach(RosterItem *rosterItem, FRosterItems)
    if (AItemJid && rosterItem->jid())
      return rosterItem; 
  return NULL;
}

QList<IRosterItem *> Roster::items() const
{
  QList<IRosterItem *> rosterItems;
  foreach(RosterItem *rosterItem, FRosterItems)
    rosterItems.append(rosterItem); 
  return rosterItems;
}

QSet<QString> Roster::groups() const
{
  QSet<QString> allGroups;
  foreach(RosterItem *rosterItem, FRosterItems)
    if (!rosterItem->jid().node().isEmpty())
      allGroups += rosterItem->groups();
  return allGroups;
}

QList<IRosterItem *> Roster::groupItems(const QString &AGroup) const
{
  QList<IRosterItem *> rosterItems;
  foreach(RosterItem *rosterItem, FRosterItems)
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
    itemElem.setAttribute("jid",AItemJid.eBare());
  else
    itemElem.setAttribute("jid",AItemJid.eFull());
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
  subscr.setTo(AItemJid.eBare()).setType(type);
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
    itemElem.setAttribute("jid",AItemJid.eBare());
  else
    itemElem.setAttribute("jid",AItemJid.eFull());
  itemElem.setAttribute("subscription","remove"); 

  FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,0);
}

void Roster::saveRosterItems(const QString &AFileName) const
{
  if (FRosterItems.isEmpty())
    return;

  QDomDocument xml;
  QDomElement elem = xml.appendChild(xml.createElement("roster")).toElement();
  elem.setAttribute("streamJid",streamJid().pBare());
  elem.setAttribute("groupDelimiter",FGroupDelim);
  foreach(IRosterItem *rosterItem, FRosterItems)
  {
    QDomElement itemElem = elem.appendChild(xml.createElement("item")).toElement();
    itemElem.setAttribute("jid",rosterItem->jid().eFull());
    itemElem.setAttribute("name",rosterItem->name());
    itemElem.setAttribute("subscription",rosterItem->subscription());
    itemElem.setAttribute("ask",rosterItem->ask());
    foreach(QString group, rosterItem->groups())
      itemElem.appendChild(xml.createElement("group")).appendChild(xml.createTextNode(group));
  }

  QFile rosterFile(AFileName);
  if (rosterFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    rosterFile.write(xml.toByteArray());
    rosterFile.close();
  }
}

void Roster::loadRosterItems(const QString &AFileName)
{
  if (FXmppStream->isOpen())
    return;

  QFile rosterFile(AFileName);
  if (rosterFile.exists() && rosterFile.open(QIODevice::ReadOnly))
  {
    QDomDocument xml;
    if (xml.setContent(rosterFile.readAll()))
    {
      QDomElement itemsElem = xml.firstChildElement("roster");
      if (!itemsElem.isNull() && itemsElem.attribute("streamJid")==streamJid().pBare())
      {
        setGroupDelimiter(itemsElem.attribute("groupDelimiter"));
        processItemsElement(itemsElem);
      }
    }
    rosterFile.close();
  }
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

bool Roster::processItemsElement(const QDomElement &AItemsElem)
{
  bool processed = false;
  QDomElement itemElem = AItemsElem.firstChildElement("item");
  while (!itemElem.isNull())
  {
    QString subscr = itemElem.attribute("subscription");
    if (subscr == "both" || subscr == "to" || subscr == "from" || subscr == "none")
    {
      Jid itemJid = itemElem.attribute("jid");
      itemJid.setResource("");
      RosterItem *rosterItem = (RosterItem *)item(itemJid);
      if (!rosterItem)
        rosterItem = new RosterItem(itemJid,this);
      rosterItem->setName(itemElem.attribute("name"));
      rosterItem->setSubscription(subscr);   
      rosterItem->setAsk(itemElem.attribute("ask"));

      QSet<QString> allItemGroups;
      QDomElement groupElem = itemElem.firstChildElement("group"); 
      while (!groupElem.isNull())
      {
        if (groupElem.firstChild().isText())
          allItemGroups.insert(groupElem.firstChild().toText().data());
        groupElem = groupElem.nextSiblingElement("group"); 
      }
      rosterItem->setGroups(allItemGroups); 
      
      insertRosterItem(rosterItem);
      processed = true;
    }
    else if (subscr == "remove")
    {
      removeRosterItem((RosterItem *)item(itemElem.attribute("jid")));
      processed = true;
    }
    itemElem = itemElem.nextSiblingElement("item");  
  }
  return processed;
}

void Roster::insertRosterItem(RosterItem *AItem)
{
  if (!FRosterItems.contains(AItem))
    FRosterItems.append(AItem);
  emit itemPush(AItem); 
}

void Roster::removeRosterItem(RosterItem *AItem)
{
  if (FRosterItems.contains(AItem))
  {
    emit itemRemoved(AItem);
    FRosterItems.removeAt(FRosterItems.indexOf(AItem));  
    delete AItem;
  }
}

void Roster::clearItems()
{
  while (FRosterItems.count()>0)
    removeRosterItem(FRosterItems.value(0));
}

void Roster::requestGroupDelimiter()
{
  FGroupDelimId = FStanzaProcessor->newId();
  Stanza query("iq");
  query.setType("get").setId(FGroupDelimId);
  query.addElement("query","jabber:iq:private").appendChild(query.createElement("roster","roster:delimiter"));
  FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,30000);
}

void Roster::setGroupDelimiter(const QString &ADelimiter)
{
  if (!FRosterItems.isEmpty() && FGroupDelim!=ADelimiter)
    clearItems();
  FGroupDelim = ADelimiter;
}

void Roster::requestRosterItems()
{
  FOpenId = FStanzaProcessor->newId();
  Stanza query("iq");
  query.setType("get").setId(FOpenId);
  query.addElement("query","jabber:iq:roster"); 
  FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),query);
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
  setStanzaHandlers();
  requestGroupDelimiter();
}

void Roster::onStreamClosed(IXmppStream *)
{
  if (isOpen())
  {
    FOpen = false;
    emit closed();
  }
  removeStanzaHandlers();
  FOpenId.clear();
  FGroupDelimId.clear();
}

void Roster::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  if (!(AAfter && AXmppStream->jid()))
    clearItems();
  emit jidAboutToBeChanged(AAfter);
}

