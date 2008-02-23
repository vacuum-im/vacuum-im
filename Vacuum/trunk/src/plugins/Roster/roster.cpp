#include <QtDebug>
#include "roster.h"
#include <QSet>
#include <QFile>

#define SHC_ROSTER        "/iq/query[@xmlns='" NS_JABBER_ROSTER "']"
#define SHC_PRESENCE      "/presence[@type]"

Roster::Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor) : QObject(AXmppStream->instance())
{
  FStanzaProcessor = AStanzaProcessor;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *))); 
  connect(FXmppStream->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
    SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
 
  FOpen = false;
  setStanzaHandlers();
}

Roster::~Roster()
{
  clearItems();
  removeStanzaHandlers();
}

bool Roster::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FRosterHandler)
  {
    AAccept = true;
    if (AStanza.from().isEmpty() || (AStreamJid && AStanza.from())) 
    {
      bool removeOld = false;
      if (FOpenId == AStanza.id())
      {
        FOpen = true;
        removeOld = true;
        emit opened();
      }

      processItemsElement(AStanza.firstElement("query"),removeOld);

      if (AStanza.type() == "set" && !AStanza.id().isEmpty())
      {
        Stanza result("iq");
        result.setId(AStanza.id()).setType("result");
        FStanzaProcessor->sendStanzaOut(AStreamJid,result);
      }
    }
  }
  else if (AHandlerId == FSubscrHandler)
  {
    QString status = AStanza.firstElement("status").text();
    if (AStanza.type() == SUBSCRIPTION_SUBSCRIBE)
    {
      emit subscription(AStanza.from(),IRoster::Subscribe,status); 
      AAccept = true;
    }
    else if (AStanza.type() == SUBSCRIPTION_SUBSCRIBED)
    {
      emit subscription(AStanza.from(),IRoster::Subscribed,status); 
      AAccept = true;
    }
    else if (AStanza.type() == SUBSCRIPTION_UNSUBSCRIBE)
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribe,status); 
      AAccept = true;
    }
    else if (AStanza.type() == SUBSCRIPTION_UNSUBSCRIBED)
    {
      emit subscription(AStanza.from(),IRoster::Unsubscribed,status); 
      AAccept = true;
    }
  }
  return false;
}

void Roster::iqStanza(const Jid &AStreamJid, const Stanza &AStanza)
{
  if (AStanza.id() == FGroupDelimId)
  {
    QString groupDelim;
    if (AStanza.type() == "result")
      groupDelim = AStanza.firstElement("query","jabber:iq:private").firstChildElement("roster").text();

    if (groupDelim.isEmpty())
    {
      groupDelim = "::";
      Stanza delim("iq");
      delim.setType("set").setId(FStanzaProcessor->newId());
      QDomElement elem = delim.addElement("query","jabber:iq:private");
      elem.appendChild(delim.createElement("roster","roster:delimiter")).appendChild(delim.createTextNode(groupDelim));
      FStanzaProcessor->sendIqStanza(this,AStreamJid,delim,0);
    }
    setGroupDelimiter(groupDelim);
    requestRosterItems();
  }
}

void Roster::iqStanzaTimeOut(const QString &AId)
{
  if (AId==FGroupDelimId || AId == FOpenId)
    FXmppStream->close();
}

IRosterItem Roster::rosterItem(const Jid &AItemJid) const
{
  foreach(IRosterItem ritem, FRosterItems)
    if (AItemJid && ritem.itemJid)
      return ritem; 
  return IRosterItem();
}

QList<IRosterItem> Roster::rosterItems() const
{
  return FRosterItems.values();
}

QSet<QString> Roster::groups() const
{
  QSet<QString> allGroups;
  foreach(IRosterItem ritem, FRosterItems)
    if (!ritem.itemJid.node().isEmpty())
      allGroups += ritem.groups;
  return allGroups;
}

QList<IRosterItem> Roster::groupItems(const QString &AGroup) const
{
  QString groupWithDelim = AGroup+FGroupDelim;
  QList<IRosterItem> ritems;
  foreach(IRosterItem ritem, FRosterItems)
  {
    QSet<QString> allItemGroups = ritem.groups;
    foreach(QString itemGroup,allItemGroups)
    {
      if (itemGroup==AGroup || itemGroup.startsWith(groupWithDelim))
      {
        ritems.append(ritem); 
        break;
      }
    }
  }
  return ritems;
}

QSet<QString> Roster::itemGroups(const Jid &AItemJid) const
{
  return rosterItem(AItemJid).groups;
}

void Roster::setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups)
{
  Stanza query("iq");
  query.setType("set").setId(FStanzaProcessor->newId());
  QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  itemElem.setAttribute("jid", AItemJid.node().isEmpty() ? AItemJid.eFull() : AItemJid.eBare());
  if (!AName.isEmpty()) 
    itemElem.setAttribute("name",AName); 
  foreach (QString groupName,AGroups)
    itemElem.appendChild(query.createElement("group")).appendChild(query.createTextNode(groupName));
  FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,0);
}

void Roster::sendSubscription(const Jid &AItemJid, int ASubsType, const QString &AText)
{
  QString type;
  if (ASubsType == IRoster::Subscribe)
    type = SUBSCRIPTION_SUBSCRIBE;
  else if (ASubsType == IRoster::Subscribed)
    type = SUBSCRIPTION_SUBSCRIBED;
  else if (ASubsType == IRoster::Unsubscribe)
    type = SUBSCRIPTION_UNSUBSCRIBE;
  else if (ASubsType == IRoster::Unsubscribed)
    type = SUBSCRIPTION_UNSUBSCRIBED;
  else return;

  Stanza subscr("presence");
  subscr.setTo(AItemJid.eBare()).setType(type);
  if (!AText.isEmpty())
    subscr.addElement("status").appendChild(subscr.createTextNode(AText));
  FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),subscr);  
}

void Roster::removeItem(const Jid &AItemJid)
{
  Stanza query("iq");
  query.setId(FStanzaProcessor->newId()).setType("set");
  QDomElement itemElem = query.addElement("query",NS_JABBER_ROSTER).appendChild(query.createElement("item")).toElement();
  itemElem.setAttribute("jid", AItemJid.node().isEmpty() ? AItemJid.eFull() : AItemJid.eBare());
  itemElem.setAttribute("subscription",SUBSCRIPTION_REMOVE); 
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
  foreach(IRosterItem ritem, FRosterItems)
  {
    QDomElement itemElem = elem.appendChild(xml.createElement("item")).toElement();
    itemElem.setAttribute("jid",ritem.itemJid.eFull());
    itemElem.setAttribute("name",ritem.name);
    itemElem.setAttribute("subscription",ritem.subscription);
    itemElem.setAttribute("ask",ritem.ask);
    foreach(QString group, ritem.groups)
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
  if (!isOpen())
  {
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
          processItemsElement(itemsElem,true);
        }
      }
      rosterFile.close();
    }
  }
}

void Roster::renameItem(const Jid &AItemJid, const QString &AName)
{
  IRosterItem ritem = rosterItem(AItemJid);
  if (ritem.isValid)
    setItem(AItemJid,AName,ritem.groups);
}

void Roster::copyItemToGroup(const Jid &AItemJid, const QString &AGroup)
{
  IRosterItem ritem = rosterItem(AItemJid);
  if (ritem.isValid)
  {
    QSet<QString> allItemGroups = ritem.groups;
    setItem(AItemJid,ritem.name,allItemGroups += AGroup);
  }
}

void Roster::moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo)
{
  IRosterItem ritem = rosterItem(AItemJid);
  if (ritem.isValid)
  {
    QSet<QString> allItemGroups = ritem.groups;
    allItemGroups -= AGroupFrom;
    setItem(AItemJid,ritem.name,allItemGroups += AGroupTo);
  }
}

void Roster::removeItemFromGroup(const Jid &AItemJid, const QString &AGroup)
{
  IRosterItem ritem = rosterItem(AItemJid);
  if (ritem.isValid)
  {
    QSet<QString> allItemGroups = ritem.groups;
    if (allItemGroups.contains(AGroup))
      setItem(AItemJid,ritem.name,allItemGroups -= AGroup);
  }
}

void Roster::renameGroup(const QString &AGroup, const QString &AGroupTo)
{
  QList<IRosterItem> allGroupItems = groupItems(AGroup);
  foreach(IRosterItem ritem, allGroupItems)
  {
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = ritem.groups;
    foreach(QString group,oldItemGroups)
    {
      QString newGroup = group;
      if (newGroup.startsWith(AGroup))
      {
        newGroup.remove(0,AGroup.size());
        newGroup.prepend(AGroupTo);
      }
      newItemGroups += newGroup;
    }
    setItem(ritem.itemJid,ritem.name,newItemGroups);
  }
}

void Roster::copyGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
  QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
  foreach(IRosterItem ritem, FRosterItems)
  {
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = ritem.groups;
    foreach(QString group,oldItemGroups)
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
    setItem(ritem.itemJid,ritem.name,oldItemGroups+newItemGroups);
  }
}

void Roster::moveGroupToGroup(const QString &AGroup, const QString &AGroupTo)
{
  QString groupName = AGroup.split(FGroupDelim,QString::SkipEmptyParts).last();
  foreach(IRosterItem ritem, FRosterItems)
  {
    QSet<QString> newItemGroups;
    QSet<QString> oldItemGroups = ritem.groups;
    foreach(QString group,oldItemGroups)
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
    setItem(ritem.itemJid,ritem.name,oldItemGroups+newItemGroups);
  }
}

void Roster::removeGroup(const QString &AGroup)
{
  QList<IRosterItem> allGroupItems = groupItems(AGroup);
  foreach(IRosterItem ritem, allGroupItems)
  {
    QSet<QString> oldItemGroups = ritem.groups;
    QSet<QString>::iterator group = oldItemGroups.begin();
    while (group != oldItemGroups.end())
    {
      if ((*group).startsWith(AGroup))
        group = oldItemGroups.erase(group);
      else
        group++;
    }
    if (oldItemGroups.isEmpty())
      removeItem(ritem.itemJid);
    else
      setItem(ritem.itemJid,ritem.name,oldItemGroups);
  }
}

bool Roster::processItemsElement(const QDomElement &AItemsElem, bool ARemoveOld)
{
  bool processed = false;

  QSet<Jid> oldItems; 
  if (ARemoveOld)
    oldItems = FRosterItems.keys().toSet();

  QDomElement itemElem = AItemsElem.firstChildElement("item");
  while (!itemElem.isNull())
  {
    QString subs = itemElem.attribute("subscription");
    if (subs == SUBSCRIPTION_BOTH || subs == SUBSCRIPTION_TO || 
        subs == SUBSCRIPTION_FROM || subs == SUBSCRIPTION_NONE)
    {
      Jid itemJid = itemElem.attribute("jid");
      itemJid.setResource("");
      IRosterItem &ritem = FRosterItems[itemJid];
      ritem.isValid = true;
      ritem.itemJid = itemJid;
      ritem.name = itemElem.attribute("name");
      ritem.subscription = subs;   
      ritem.ask = itemElem.attribute("ask");
      oldItems -= ritem.itemJid;

      QSet<QString> allItemGroups;
      QDomElement groupElem = itemElem.firstChildElement("group"); 
      while (!groupElem.isNull())
      {
        allItemGroups += groupElem.text();
        groupElem = groupElem.nextSiblingElement("group"); 
      }
      ritem.groups = allItemGroups; 

      emit received(ritem); 
      processed = true;
    }
    else if (subs == SUBSCRIPTION_REMOVE)
    {
      removeRosterItem(itemElem.attribute("jid"));
      processed = true;
    }
    itemElem = itemElem.nextSiblingElement("item");  
  }

  foreach(Jid itemJid,oldItems)
    removeRosterItem(itemJid);

  return processed;
}

void Roster::removeRosterItem(const Jid &AItemJid)
{
  if (FRosterItems.contains(AItemJid))
  {
    IRosterItem ritem = FRosterItems.take(AItemJid);
    emit removed(ritem);
  }
}

void Roster::clearItems()
{
  QList<Jid> items = FRosterItems.keys();
  foreach(Jid itemJid,items)
    removeRosterItem(itemJid);
}

void Roster::requestGroupDelimiter()
{
  Stanza query("iq");
  query.setType("get").setId(FStanzaProcessor->newId());
  query.addElement("query",NS_JABBER_PRIVATE).appendChild(query.createElement("roster","roster:delimiter"));
  if (FStanzaProcessor->sendIqStanza(this,FXmppStream->jid(),query,30000))
    FGroupDelimId = query.id();
}

void Roster::setGroupDelimiter(const QString &ADelimiter)
{
  if (FGroupDelim != ADelimiter)
    clearItems();
  FGroupDelim = ADelimiter;
}

void Roster::requestRosterItems()
{
  Stanza query("iq");
  query.setType("get").setId(FStanzaProcessor->newId());
  query.addElement("query",NS_JABBER_ROSTER); 
  if (FStanzaProcessor->sendStanzaOut(FXmppStream->jid(),query))
    FOpenId = query.id();
}

void Roster::setStanzaHandlers()
{
  FRosterHandler = FStanzaProcessor->insertHandler(this,SHC_ROSTER,
    IStanzaProcessor::DirectionIn,SHP_DEFAULT,FXmppStream->jid()); 

  FSubscrHandler = FStanzaProcessor->insertHandler(this,SHC_PRESENCE,
    IStanzaProcessor::DirectionIn,SHP_DEFAULT,FXmppStream->jid());
}

void Roster::removeStanzaHandlers()
{
  FStanzaProcessor->removeHandler(FRosterHandler); 
  FRosterHandler = 0;

  FStanzaProcessor->removeHandler(FSubscrHandler); 
  FSubscrHandler = 0;
}

void Roster::onStreamOpened(IXmppStream * /*AXmppStream*/)
{
  requestGroupDelimiter();
}

void Roster::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  if (isOpen())
  {
    FOpen = false;
    emit closed();
  }
}

void Roster::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  if (!(AAfter && AXmppStream->jid()))
    clearItems();
  emit jidAboutToBeChanged(AAfter);
}

