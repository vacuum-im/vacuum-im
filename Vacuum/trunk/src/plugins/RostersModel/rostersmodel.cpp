#include <QtDebug>
#include "rostersmodel.h"
#include <QTimer>

#define INDEX_CHANGES_FOR_RESET 20

RostersModel::RostersModel(QObject *AParent):IRostersModel(AParent)
{
  FRootIndex = new RosterIndex(RIT_Root,"IT_Root");
  FRootIndex->setParent(this);
  connect(FRootIndex,SIGNAL(dataChanged(IRosterIndex *, int)),
    SLOT(onIndexDataChanged(IRosterIndex *, int)));
  connect(FRootIndex,SIGNAL(childAboutToBeInserted(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childInserted(IRosterIndex *)),
    SLOT(onIndexChildInserted(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
    SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
  connect(FRootIndex,SIGNAL(childRemoved(IRosterIndex *)),
    SLOT(onIndexChildRemoved(IRosterIndex *)));
}

RostersModel::~RostersModel()
{

}

QModelIndex RostersModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());

  IRosterIndex *childIndex = parentIndex->child(ARow);

  if (childIndex)
    return createIndex(ARow,AColumn,childIndex);

  return QModelIndex();
}

QModelIndex RostersModel::parent(const QModelIndex &AIndex) const
{
  if (AIndex.isValid())
  {
    IRosterIndex *curIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
    IRosterIndex *parentIndex = curIndex->parentIndex();

    if (parentIndex != FRootIndex && parentIndex)
      return createIndex(parentIndex->row(),0,parentIndex); 
  }

  return QModelIndex();
}

bool RostersModel::hasChildren(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());

  return parentIndex->childCount() > 0;
}

int RostersModel::rowCount(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = FRootIndex;
  if (AParent.isValid())
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());

  return parentIndex->childCount();
}

int RostersModel::columnCount(const QModelIndex &/*AParent*/) const
{
  return 1;
}

Qt::ItemFlags RostersModel::flags(const QModelIndex &AIndex) const
{
  IRosterIndex *index = FRootIndex;
  if (AIndex.isValid())
    index = static_cast<IRosterIndex *>(AIndex.internalPointer());

  return index->flags();  
}

QVariant RostersModel::data(const QModelIndex &AIndex, int ARole) const 
{
  if (!AIndex.isValid())
    return QVariant();

  IRosterIndex *index = static_cast<IRosterIndex *>(AIndex.internalPointer());
  return index->data(ARole);
}

IRosterIndex *RostersModel::addStream(IRoster *ARoster, IPresence *APresence)
{
  if (!ARoster)
    return NULL;

  Jid streamJid = ARoster->streamJid();
  
  if (FStreams.contains(streamJid.pFull()))
    return FStreams.value(streamJid.pFull()).root;
  
  connect(ARoster->xmppStream()->instance(),SIGNAL(jidChanged(IXmppStream *,const Jid &)),
    SLOT(onStreamJidChanged(IXmppStream *,const Jid &)));
  connect(ARoster->instance(),SIGNAL(received(const IRosterItem &)),SLOT(onRosterItemReceived(const IRosterItem &))); 
  connect(ARoster->instance(),SIGNAL(removed(const IRosterItem &)),SLOT(onRosterItemRemoved(const IRosterItem &)));

  IRosterIndex *index = createRosterIndex(RIT_StreamRoot,streamJid.pFull(),FRootIndex);
  index->setRemoveOnLastChildRemoved(false);
  index->setData(RDR_StreamJid,streamJid.pFull());
  index->setData(RDR_Jid,streamJid.full());
  index->setData(RDR_PJid,streamJid.pFull());

  if (APresence)
  {
    connect(APresence->instance(),SIGNAL(received(const IPresenceItem &)),
      SLOT(onPresenceReceived(const IPresenceItem &))); 
    connect(APresence->instance(),SIGNAL(changed(int, const QString &, int)),
      SLOT(onPresenceChanged(int , const QString &, int)));
    index->setData(RDR_Show, APresence->show());
    index->setData(RDR_Status,APresence->status());
  }

  StreamItem streamItem;
  streamItem.presence = APresence;
  streamItem.roster = ARoster;
  streamItem.root = index;
  FStreams.insert(streamJid.pFull(),streamItem);

  insertRosterIndex(index,FRootIndex);
  emit streamAdded(streamJid);

  return index;
}

void RostersModel::removeStream(const QString &AStreamJid)
{
  if (FStreams.contains(AStreamJid))
  {
    emit streamRemoved(AStreamJid);
    StreamItem streamItem = FStreams.value(AStreamJid);
    IRoster *streamRoster = streamItem.roster;
    IPresence *streamPresence = streamItem.presence;
    IRosterIndex *streamRoot = streamItem.root;
    disconnect(streamRoster->xmppStream()->instance(),SIGNAL(jidChanged(IXmppStream *,const Jid &)),this,
      SLOT(onStreamJidChanged(IXmppStream *,const Jid &)));
    disconnect(streamRoster->instance(),SIGNAL(received(const IRosterItem &)),this,
      SLOT(onRosterItemReceived(const IRosterItem &))); 
    disconnect(streamRoster->instance(),SIGNAL(removed(const IRosterItem &)),this,
      SLOT(onRosterItemRemoved(const IRosterItem &))); 
    
    if (streamPresence)
    {
      disconnect(streamPresence->instance(),SIGNAL(received(const IPresenceItem &)),
        this, SLOT(onPresenceReceived(const IPresenceItem &))); 
      disconnect(streamPresence->instance(),SIGNAL(changed(int, const QString &, int)),
        this, SLOT(onPresenceChanged(int, const QString &, int)));
    }

    removeRosterIndex(streamRoot);
    FStreams.remove(AStreamJid);
  }
}

IRoster *RostersModel::getRoster(const QString &AStreamJid) const
{
  if (FStreams.contains(AStreamJid))
    return FStreams.value(AStreamJid).roster;

  return NULL;
}

IPresence *RostersModel::getPresence(const QString &AStreamJid) const
{
  if (FStreams.contains(AStreamJid))
    return FStreams.value(AStreamJid).presence;
  return NULL;
}

IRosterIndex *RostersModel::getStreamRoot(const Jid &AStreamJid) const
{
  if (FStreams.contains(AStreamJid.pFull()))
    return FStreams.value(AStreamJid.pFull()).root;
  return NULL;
}

IRosterIndex *RostersModel::createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent)
{
  IRosterIndex *index = findRosterIndex(AType,AId,AParent);
  if (!index)
  {
    index = new RosterIndex(AType,AId);
    connect(index->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onIndexDestroyed(IRosterIndex *)));
    if (AParent)
      index->setData(RDR_StreamJid,AParent->data(RDR_StreamJid));
    emit indexCreated(index,AParent);
    insertDefaultDataHolders(index);
  }
  return index;
}

IRosterIndex *RostersModel::createGroup(const QString &AName, const QString &AGroupDelim, 
                                         int AType, IRosterIndex *AParent)
{
  IRosterIndex *index = findGroup(AName,AGroupDelim,AType,AParent);
  if (!index)
  {
    int i = 0;
    index = AParent;
    IRosterIndex *newIndex = (IRosterIndex *)true;
    QList<QString> groupTree = AName.split(AGroupDelim,QString::SkipEmptyParts); 
    
    QString group;
    if (AParent && AParent->data(RDR_Group).isValid())
      group = AParent->data(RDR_Group).toString();
    
    while (newIndex && i<groupTree.count())
    {
      if (group.isEmpty())
        group = groupTree.at(i);
      else
        group += AGroupDelim + groupTree.at(i);

      newIndex = findGroup(groupTree.at(i),AGroupDelim,AType,index);
      if (newIndex)
      {
        index = newIndex;
        i++;
      }
    }
    
    while (i<groupTree.count())
    {
      newIndex = createRosterIndex(AType,groupTree.at(i),index);
      newIndex->setData(RDR_Group,group);
      insertRosterIndex(newIndex,index);
      index = newIndex;
      i++;
      group += AGroupDelim + groupTree.value(i);
    }
  }
  return index;
}

void RostersModel::insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent)
{
  AIndex->setParentIndex(AParent);
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex)
{
  AIndex->setParentIndex(0);
}

void RostersModel::insertDefaultDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  if (ADataHolder && !FDataHolders.contains(ADataHolder))
  {
    QMultiHash<int,QVariant> data;
    foreach(int type, ADataHolder->types())
      data.insertMulti(RDR_Type,type);
    
    QList<IRosterIndex *> indexes = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index,indexes)
      index->insertDataHolder(ADataHolder);
    
    FDataHolders.append(ADataHolder);
    emit defaultDataHolderInserted(ADataHolder);
  }
}

void RostersModel::removeDefaultDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  if (ADataHolder && FDataHolders.contains(ADataHolder))
  {
    QMultiHash<int,QVariant> data;
    foreach(int type, ADataHolder->types())
      data.insertMulti(RDR_Type,type);

    QList<IRosterIndex *> indexes = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index,indexes)
      index->removeDataHolder(ADataHolder);

    FDataHolders.removeAt(FDataHolders.indexOf(ADataHolder));
    emit defaultDataHolderRemoved(ADataHolder);
  }
}

IRosterIndexList RostersModel::getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate)
{
  IRosterIndexList indexList;
  IRosterIndex *streamRoot = getStreamRoot(AStreamJid);
  if (streamRoot)
  {
    int type = RIT_Contact;
    if (AContactJid.node().isEmpty())
      type = RIT_Agent;
    else if (AContactJid && AStreamJid)
      type = RIT_MyResource;

    QHash<int,QVariant> data;
    data.insert(RDR_Type, type);
    if (AContactJid.resource().isEmpty())
      data.insert(RDR_BareJid, AContactJid.pBare());
    else
      data.insert(RDR_PJid, AContactJid.pFull());
    indexList = streamRoot->findChild(data,true);

    if (indexList.isEmpty() && !AContactJid.resource().isEmpty())
    {
      data.insert(RDR_PJid, AContactJid.pBare());
      indexList = streamRoot->findChild(data,true);
    }

    if (indexList.isEmpty() && ACreate)
    {
      IRoster *roster = getRoster(AStreamJid.pFull());
      IRosterIndex *group;
      if (type == RIT_MyResource)
        group = createGroup(myResourcesGroupName(),roster->groupDelimiter(),RIT_MyResourcesGroup,streamRoot);
      else
        group = createGroup(notInRosterGroupName(),roster->groupDelimiter(),RIT_NotInRosterGroup,streamRoot);
      IRosterIndex *index = createRosterIndex(type,AContactJid.pFull(),group);
      index->setData(RDR_Jid,AContactJid.full());
      index->setData(RDR_PJid,AContactJid.pFull());
      index->setData(RDR_BareJid,AContactJid.pBare());
      index->setData(RDR_Group,group->data(RDR_Group));
      insertRosterIndex(index,group);
      indexList.append(index);
    }
  }
  return indexList;
}

IRosterIndex *RostersModel::findRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) const
{
  QHash<int,QVariant> data;
  data.insert(RDR_Type,AType);  
  data.insert(RDR_Id,AId);
  if (AParent)
    return AParent->findChild(data).value(0,NULL);
  return FRootIndex->findChild(data).value(0,NULL) ;
}

IRosterIndex *RostersModel::findGroup(const QString &AName, const QString &AGroupDelim, 
                                      int AType, IRosterIndex *AParent) const
{
  int i = 0;
  IRosterIndex *index = AParent;
  QList<QString> groupTree = AName.split(AGroupDelim,QString::SkipEmptyParts);
  do {
    index = findRosterIndex(AType,groupTree.at(i++),index);  
  } while (index && i<groupTree.count());
  return index;
}

QModelIndex RostersModel::modelIndexByRosterIndex(IRosterIndex *AIndex)
{
  if (AIndex)
    return createIndex(AIndex->row(),0,AIndex);
  return QModelIndex();
}

void RostersModel::emitDelayedDataChanged(IRosterIndex *AIndex)
{
  FChangedIndexes -= AIndex;
  if (AIndex!=FRootIndex)
  {
    QModelIndex modelIndex = createIndex(AIndex->row(),0,AIndex);
    emit dataChanged(modelIndex,modelIndex);
  }

  IRosterIndexList childs;
  foreach(IRosterIndex *index, FChangedIndexes)
    if (index->parentIndex() == AIndex)
      childs.append(index);

  foreach(IRosterIndex *index, childs)
    emitDelayedDataChanged(index);
}

void RostersModel::insertDefaultDataHolders(IRosterIndex *AIndex)
{
  foreach(IRosterIndexDataHolder *dataHolder, FDataHolders)
    if (dataHolder->types().contains(AIndex->type()))
      AIndex->insertDataHolder(dataHolder);
}

void RostersModel::onRosterItemReceived(const IRosterItem &ARosterItem)
{
  IRoster *roster = qobject_cast<IRoster *>(sender());
  QString streamJid = roster->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  QString bareJid = ARosterItem.itemJid.bare();
  QString pbareJid = ARosterItem.itemJid.pBare();

  int itemType = RIT_Contact;
  if (ARosterItem.itemJid.node().isEmpty())
    itemType = RIT_Agent;

  QString groupDelim = roster->groupDelimiter();
  int groupType; 
  QString groupDisplay;
  QSet<QString> itemGroups; 
  if (itemType == RIT_Agent)
  {
    groupType = RIT_AgentsGroup;
    groupDisplay = agentsGroupName();
    itemGroups.insert(""); 
  }
  else if (ARosterItem.groups.isEmpty())
  {
    groupType = RIT_BlankGroup; 
    groupDisplay = blankGroupName();
    itemGroups.insert(""); 
  }
  else
  {
    groupType = RIT_Group;
    itemGroups = ARosterItem.groups;
  }

  QHash<int,QVariant> data;
  data.insert(RDR_Type,itemType);
  data.insert(RDR_BareJid,pbareJid);     
  IRosterIndexList curItemList = streamRoot->findChild(data,true);
  QSet<QString> curGroups;
  foreach(IRosterIndex *index,curItemList)
    curGroups.insert(index->data(RDR_Group).toString());
  QSet<QString> newGroups = itemGroups - curGroups;
  QSet<QString> oldGroups = curGroups - itemGroups; 

  IRosterIndexList itemList; 
  foreach(QString group,itemGroups)
  {
    IRosterIndex *groupIndex = createGroup(groupType==RIT_Group ? group : groupDisplay,groupDelim,groupType,streamRoot);

    IRosterIndexList groupItemList;
    if (newGroups.contains(group) && !oldGroups.isEmpty())
    {
      IRosterIndex *oldGroupIndex;
      QString oldGroup = oldGroups.values().value(0);
      if (oldGroup.isEmpty())
        oldGroupIndex = findGroup(blankGroupName(),groupDelim,RIT_BlankGroup,streamRoot);
      else
        oldGroupIndex = findGroup(oldGroup,groupDelim,RIT_Group,streamRoot);
      if (oldGroupIndex)
      {
        groupItemList = oldGroupIndex->findChild(data);
        foreach(IRosterIndex *index,groupItemList)
        {
          index->setData(RDR_Group,group);  
          index->setParentIndex(groupIndex);
        }
      }
    }
    else
      groupItemList = groupIndex->findChild(data);

    if (groupItemList.isEmpty())
    {
      int presIndex = 0;
      QList<IPresenceItem> pitems = streamPresence->presenceItems(ARosterItem.itemJid);
      do 
      {
        IRosterIndex *index;
        IPresenceItem pitem = pitems.value(presIndex++);
        if (pitem.isValid)
        {
          index = createRosterIndex(itemType,pitem.itemJid.pFull(),groupIndex);
          index->setData(RDR_Jid,pitem.itemJid.full());
          index->setData(RDR_PJid,pitem.itemJid.pFull());
          index->setData(RDR_Show,pitem.show);
          index->setData(RDR_Status,pitem.status);
          index->setData(RDR_Priority,pitem.priority);
        }
        else
        {
          index = createRosterIndex(itemType,pbareJid,groupIndex);
          index->setData(RDR_Jid,bareJid);
          index->setData(RDR_PJid,pbareJid);
        }

        index->setData(RDR_BareJid,pbareJid);
        index->setData(RDR_Name,ARosterItem.name); 
        index->setData(RDR_Subscription,ARosterItem.subscription);
        index->setData(RDR_Ask,ARosterItem.ask);
        index->setData(RDR_Group,group); 

        itemList.append(index);
        insertRosterIndex(index,groupIndex);
      } 
      while(presIndex < pitems.count());
    }
    else foreach(IRosterIndex *index,groupItemList)
    {
      index->setData(RDR_Name,ARosterItem.name); 
      index->setData(RDR_Subscription,ARosterItem.subscription);
      index->setData(RDR_Ask,ARosterItem.ask);
      itemList.append(index);
    }
  }

  foreach(IRosterIndex *index,curItemList)
    if (!itemList.contains(index))
      removeRosterIndex(index);
}

void RostersModel::onRosterItemRemoved(const IRosterItem &ARosterItem)
{
  IRoster *roster = qobject_cast<IRoster *>(sender());
  QString streamJid = roster->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IRosterIndex *streamRoot = streamItem.root;

  QMultiHash<int,QVariant> data;
  data.insert(RDR_Type,RIT_Contact);     
  data.insert(RDR_Type,RIT_Agent);     
  data.insert(RDR_BareJid,ARosterItem.itemJid.pBare());     
  IRosterIndexList itemList = streamRoot->findChild(data,true);
  foreach(IRosterIndex *index, itemList)
    removeRosterIndex(index);    
}

void RostersModel::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  IRosterIndex *streamRoot = getStreamRoot(ABefour);
  if (streamRoot)
  {
    Jid newStreamJid = AXmppStream->jid();

    QHash<int,QVariant> data;
    data.insert(RDR_StreamJid,ABefour.pFull());
    IRosterIndexList itemList = FRootIndex->findChild(data,true);
    foreach(IRosterIndex *index, itemList)
      index->setData(RDR_StreamJid,newStreamJid.pFull());

    streamRoot->setData(RDR_Id,newStreamJid.pFull());
    streamRoot->setData(RDR_Jid,newStreamJid.full());
    streamRoot->setData(RDR_PJid,newStreamJid.pFull());

    StreamItem streamItem = FStreams.take(ABefour.pFull());
    FStreams.insert(newStreamJid.pFull(),streamItem);

    emit streamJidChanged(ABefour,newStreamJid);
  }
}

void RostersModel::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
  IPresence *presence = qobject_cast<IPresence *>(sender());
  QString streamJid = presence->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IRosterIndex *streamRoot = streamItem.root;

  streamRoot->setData(RDR_Show, AShow);
  streamRoot->setData(RDR_Status, AStatus);
  if (AShow != IPresence::Offline && AShow != IPresence::Error)
    streamRoot->setData(RDR_Priority, APriority);
  else
    streamRoot->setData(RDR_Priority, QVariant());
}

void RostersModel::onPresenceReceived(const IPresenceItem &APresenceItem)
{
  IPresence *presence = qobject_cast<IPresence *>(sender());
  QString streamJid = presence->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IRoster *streamRoster = streamItem.roster;
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  IRosterItem ritem = streamRoster->rosterItem(APresenceItem.itemJid);

  int itemType = RIT_Contact;
  if (APresenceItem.itemJid && streamPresence->streamJid())
    itemType = RIT_MyResource; 
  else if (APresenceItem.itemJid.node().isEmpty())
    itemType = RIT_Agent; 

  if (APresenceItem.show == IPresence::Offline)
  {
    QHash<int,QVariant> data;
    data.insert(RDR_Type,itemType);
    data.insert(RDR_PJid,APresenceItem.itemJid.pFull());
    IRosterIndexList indexList = streamRoot->findChild(data,true);
    int pitemsCount = streamPresence->presenceItems(APresenceItem.itemJid).count();
    foreach (IRosterIndex *index,indexList)
    {
      if (itemType == RIT_MyResource || pitemsCount > 1)
      {
        removeRosterIndex(index);
      }
      else
      {
        index->setData(RDR_Id,APresenceItem.itemJid.pBare());
        index->setData(RDR_Jid,APresenceItem.itemJid.bare());
        index->setData(RDR_PJid,APresenceItem.itemJid.pBare());
        index->setData(RDR_Show,APresenceItem.show);
        index->setData(RDR_Status,APresenceItem.status);
        index->setData(RDR_Priority,QVariant()); 
      }
    }
  }
  else if (APresenceItem.show == IPresence::Error)
  {
    QHash<int,QVariant> data;
    data.insert(RDR_Type,itemType);
    data.insert(RDR_BareJid,APresenceItem.itemJid.pBare());
    IRosterIndexList indexList = streamRoot->findChild(data,true);
    foreach(IRosterIndex *index,indexList)
    {
      index->setData(RDR_Show,APresenceItem.show);
      index->setData(RDR_Status,APresenceItem.status);
      index->setData(RDR_Priority,QVariant()); 
    }
  }
  else
  {
    QHash<int,QVariant> data;
    data.insert(RDR_Type,itemType);
    data.insert(RDR_PJid,APresenceItem.itemJid.pFull());
    IRosterIndexList indexList = streamRoot->findChild(data,true);

    if (indexList.isEmpty())
    {
      QSet<QString> itemGroups;
      if (ritem.isValid)
        itemGroups = ritem.groups;
      if (itemGroups.isEmpty())
        itemGroups.insert(""); 

      QString groupDelim = streamRoster->groupDelimiter();
      foreach(QString group,itemGroups)
      {
        IRosterIndex *groupIndex = NULL;
        if (itemType == RIT_MyResource)
          groupIndex = createGroup(myResourcesGroupName(),groupDelim,RIT_MyResourcesGroup,streamRoot);
        else if (!ritem.isValid)
          groupIndex = createGroup(notInRosterGroupName(),groupDelim,RIT_NotInRosterGroup,streamRoot);
        else if (itemType == RIT_Agent)
          groupIndex = findGroup(agentsGroupName(),groupDelim,RIT_AgentsGroup,streamRoot);
        else if (group.isEmpty()) 
          groupIndex = findGroup(blankGroupName(),groupDelim,RIT_BlankGroup,streamRoot);
        else
          groupIndex = findGroup(group,groupDelim,RIT_Group,streamRoot);

        if (groupIndex)
        {
          IRosterIndex *index = findRosterIndex(itemType,APresenceItem.itemJid.pBare(),groupIndex);   
          if (!index)
          {
            index = createRosterIndex(itemType,APresenceItem.itemJid.pFull(),groupIndex);
            index->setData(RDR_BareJid,APresenceItem.itemJid.pBare());
            index->setData(RDR_Group,group); 
            if (ritem.isValid)
            {
              index->setData(RDR_Name,ritem.name); 
              index->setData(RDR_Subscription,ritem.subscription);
              index->setData(RDR_Ask,ritem.ask);
            }
          }
          else
            index->setData(RDR_Id,APresenceItem.itemJid.pFull());

          index->setData(RDR_Jid,APresenceItem.itemJid.full());
          index->setData(RDR_PJid,APresenceItem.itemJid.pFull());
          index->setData(RDR_Show,APresenceItem.show);
          index->setData(RDR_Status,APresenceItem.status);
          index->setData(RDR_Priority,APresenceItem.priority);
          insertRosterIndex(index,groupIndex);
        }
      }
    }
    else foreach(IRosterIndex *index,indexList)
    {
      index->setData(RDR_Show,APresenceItem.show);
      index->setData(RDR_Status,APresenceItem.status);
      index->setData(RDR_Priority,APresenceItem.priority);
    }
  }
}

void RostersModel::onIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
  if (FChangedIndexes.isEmpty())
    QTimer::singleShot(0,this,SLOT(onDelayedDataChanged()));
  FChangedIndexes+=AIndex;
  emit indexDataChanged(AIndex, ARole);
}

void RostersModel::onIndexChildAboutToBeInserted(IRosterIndex *AIndex)
{
  IRosterIndex *parentIndex = AIndex->parentIndex();
  if (parentIndex)
  {
    if (parentIndex == FRootIndex)
      beginInsertRows(QModelIndex(),AIndex->row(),AIndex->row());
    else
      beginInsertRows(createIndex(parentIndex->row(),0,parentIndex),AIndex->row(),AIndex->row());

    connect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
      SLOT(onIndexDataChanged(IRosterIndex *, int)));
    connect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
      SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
      SLOT(onIndexChildInserted(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
      SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
      SLOT(onIndexChildRemoved(IRosterIndex *)));
  }
}

void RostersModel::onIndexChildInserted(IRosterIndex *AIndex)
{
  endInsertRows();
  emit indexInserted(AIndex);
}

void RostersModel::onIndexChildAboutToBeRemoved(IRosterIndex *AIndex)
{
  FChangedIndexes-=AIndex;
  emit indexRemoved(AIndex);
  IRosterIndex *parentIndex = AIndex->parentIndex();
  if (parentIndex)
  { 
    if (parentIndex == FRootIndex)
      beginRemoveRows(QModelIndex(),AIndex->row(),AIndex->row());
    else
      beginRemoveRows(createIndex(parentIndex->row(),0,parentIndex),AIndex->row(),AIndex->row());
  }
}

void RostersModel::onIndexChildRemoved(IRosterIndex *AIndex)
{
  disconnect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    this,SLOT(onIndexDataChanged(IRosterIndex *, int)));
  disconnect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
    this,SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
    this,SLOT(onIndexChildInserted(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
    this,SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
  disconnect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
    this,SLOT(onIndexChildRemoved(IRosterIndex *)));
  endRemoveRows();
}

void RostersModel::onIndexDestroyed(IRosterIndex *AIndex)
{
  emit indexDestroyed(AIndex);
}

void RostersModel::onDelayedDataChanged()
{
  //Замена сигналов изменения большого числа индексов на сброс модели
  //из-за тормозов в сортировке в QSortFilterProxyModel
  if (FChangedIndexes.count() < INDEX_CHANGES_FOR_RESET)
  {
    //Вызывает dataChanged у всех родителей для поддержки SortFilterProxyModel
    QSet<IRosterIndex *> childIndexes = FChangedIndexes;
    foreach(IRosterIndex *index,childIndexes)
    {
      IRosterIndex *parentIndex = index->parentIndex();
      while (parentIndex && !FChangedIndexes.contains(parentIndex))
      {
        FChangedIndexes+=parentIndex;
        parentIndex = parentIndex->parentIndex();
      }
    }
    emitDelayedDataChanged(FRootIndex);
  }
  else
    reset();

  FChangedIndexes.clear();
}

