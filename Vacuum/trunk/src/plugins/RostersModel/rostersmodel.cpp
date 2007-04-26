#include <QtDebug>
#include "rostersmodel.h"

RostersModel::RostersModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  FIndexDataHolder = new IndexDataHolder(this);
  FRootIndex = new RosterIndex(IRosterIndex::IT_Root,"IT_Root");
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

IRosterIndex *RostersModel::appendStream(IRoster *ARoster, IPresence *APresence)
{
  if (!ARoster)
    return NULL;

  Jid streamJid = ARoster->streamJid();
  
  if (FStreams.contains(streamJid.pFull()))
    return FStreams.value(streamJid.pFull()).root;
  
  connect(ARoster->instance(),SIGNAL(itemPush(IRosterItem *)),SLOT(onRosterItemPush(IRosterItem *))); 
  connect(ARoster->instance(),SIGNAL(itemRemoved(IRosterItem *)),SLOT(onRosterItemRemoved(IRosterItem *))); 

  IRosterIndex *index = createRosterIndex(IRosterIndex::IT_StreamRoot,streamJid.pFull(),FRootIndex);
  index->setRemoveOnLastChildRemoved(false);
  index->setData(IRosterIndex::DR_StreamJid,streamJid.pFull());
  index->setData(IRosterIndex::DR_Jid,streamJid.full());

  if (APresence)
  {
    connect(APresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *))); 
    connect(APresence->instance(),SIGNAL(selfPresence(IPresence::Show , const QString &, qint8 , const Jid &)),
      SLOT(onSelfPresence(IPresence::Show , const QString &, qint8 , const Jid &)));
    index->setData(IRosterIndex::DR_Show, APresence->show());
    index->setData(IRosterIndex::DR_Status,APresence->status());
    index->setData(IRosterIndex::DR_Priority,APresence->priority());
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
    disconnect(streamRoster->instance(),SIGNAL(itemPush(IRosterItem *)),this,
      SLOT(onRosterItemPush(IRosterItem *))); 
    disconnect(streamRoster->instance(),SIGNAL(itemRemoved(IRosterItem *)),this,
      SLOT(onRosterItemRemoved(IRosterItem *))); 
    
    if (streamPresence)
    {
      disconnect(streamPresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),
        this, SLOT(onPresenceItem(IPresenceItem *))); 
      disconnect(streamPresence->instance(),SIGNAL(selfPresence(IPresence::Show , const QString &, qint8 , const Jid &)),
        this, SLOT(onSelfPresence(IPresence::Show , const QString &, qint8 , const Jid &)));
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
    if (AType < IRosterIndex::IT_UserDefined)
      index->setDataHolder(FIndexDataHolder);
    if (AParent)
      index->setData(RosterIndex::DR_StreamJid,AParent->data(RosterIndex::DR_StreamJid));
    emit indexCreated(index,AParent);
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
    
    QString groupName;
    if (AParent && AParent->data(IRosterIndex::DR_GroupName).isValid())
      groupName = AParent->data(IRosterIndex::DR_GroupName).toString();
    
    while (newIndex && i<groupTree.count())
    {
      if (groupName.isEmpty())
        groupName = groupTree.at(i);
      else
        groupName += AGroupDelim + groupTree.at(i);

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
      newIndex->setData(IRosterIndex::DR_GroupName,groupName);
      insertRosterIndex(newIndex,index);
      index = newIndex;
      i++;
      groupName += AGroupDelim + groupTree.value(i);
    }
  }
  return index;
}

IRosterIndex *RostersModel::findRosterIndex(int AType, const QVariant &AId, IRosterIndex *AParent) const
{
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_Type,AType);  
  data.insert(IRosterIndex::DR_Id,AId);
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

void RostersModel::insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent)
{
  AIndex->setParentIndex(AParent);
}

void RostersModel::removeRosterIndex(IRosterIndex *AIndex)
{
  AIndex->setParentIndex(0);
}

QModelIndex RostersModel::modelIndexByRosterIndex(IRosterIndex *AIndex)
{
  if (AIndex)
    return createIndex(AIndex->row(),0,AIndex);
  return QModelIndex();
}

void RostersModel::onRosterItemPush(IRosterItem *ARosterItem)
{
  QString streamJid = ARosterItem->roster()->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  QString rosterJid = ARosterItem->jid().pBare();
  int itemType = IRosterIndex::IT_Contact;
  if (ARosterItem->jid().node().isEmpty())
    itemType = IRosterIndex::IT_Agent;

  QString groupDelim = ARosterItem->roster()->groupDelimiter();
  int groupType; 
  QString groupDisplay;
  QSet<QString> itemGroups; 
  if (itemType == IRosterIndex::IT_Agent)
  {
    groupType = IRosterIndex::IT_AgentsGroup;
    groupDisplay = agentsGroupName();
    itemGroups.insert(""); 
  }
  else if (ARosterItem->groups().isEmpty())
  {
    groupType = IRosterIndex::IT_BlankGroup; 
    groupDisplay = blankGroupName();
    itemGroups.insert(""); 
  }
  else
  {
    groupType = IRosterIndex::IT_Group;
    itemGroups = ARosterItem->groups();
  }

  IRosterIndex *index;
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_Type,itemType);
  data.insert(IRosterIndex::DR_RosterJid,rosterJid);     
  IRosterIndexList curItemList = streamRoot->findChild(data,true);
  QSet<QString> curGroups;
  foreach(index,curItemList)
    curGroups.insert(index->data(IRosterIndex::DR_RosterGroup).toString());
  QSet<QString> newGroups = itemGroups - curGroups;
  QSet<QString> oldGroups = curGroups - itemGroups; 

  IRosterIndexList itemList; 
  QString group;
  foreach(group,itemGroups)
  {
    IRosterIndex *groupIndex;
    if (groupType != IRosterIndex::IT_Group)
      groupIndex = createGroup(groupDisplay,groupDelim,groupType,streamRoot);
    else 
      groupIndex = createGroup(group,groupDelim,groupType,streamRoot);

    IRosterIndexList groupItemList;
    if (newGroups.contains(group) && !oldGroups.isEmpty())
    {
      IRosterIndex *oldGroupIndex;
      QString oldGroup = oldGroups.values().value(0);
      if (oldGroup.isEmpty())
        oldGroupIndex = findGroup(blankGroupName(),groupDelim,IRosterIndex::IT_BlankGroup,streamRoot);
      else
        oldGroupIndex = findGroup(oldGroup,groupDelim,IRosterIndex::IT_Group,streamRoot);
      if (oldGroupIndex)
      {
        groupItemList = oldGroupIndex->findChild(data);
        foreach(index,groupItemList)
        {
          index->setData(IRosterIndex::DR_RosterGroup,group);  
          index->setParentIndex(groupIndex);
        }
      }
    }
    else
      groupItemList = groupIndex->findChild(data);

    if (groupItemList.isEmpty())
    {
      int presIndex = 0;
      QList<IPresenceItem *> presItems = streamPresence->items(ARosterItem->jid());
      do 
      {
        index = createRosterIndex(itemType,rosterJid,groupIndex);
        index->setData(IRosterIndex::DR_Jid,ARosterItem->jid().bare());
        index->setData(IRosterIndex::DR_StreamJid,streamJid);     
        index->setData(IRosterIndex::DR_RosterJid,rosterJid);
        index->setData(IRosterIndex::DR_RosterName,ARosterItem->name()); 
        index->setData(IRosterIndex::DR_Subscription,ARosterItem->subscription());
        index->setData(IRosterIndex::DR_Ask,ARosterItem->ask());
        index->setData(IRosterIndex::DR_RosterGroup,group); 

        IPresenceItem *presItem = presItems.value(presIndex++,NULL);
        if (presItem)
        {
          index->setData(IRosterIndex::DR_Id,presItem->jid().pFull());
          index->setData(IRosterIndex::DR_Jid,presItem->jid().full());
          index->setData(IRosterIndex::DR_Show,presItem->show());
          index->setData(IRosterIndex::DR_Status,presItem->status());
          index->setData(IRosterIndex::DR_Priority,presItem->priority());
        }
        itemList.append(index);
        insertRosterIndex(index,groupIndex);
      } while(presIndex < presItems.count());
    }
    else foreach(index,groupItemList)
    {
      index->setData(IRosterIndex::DR_RosterName,ARosterItem->name()); 
      index->setData(IRosterIndex::DR_Subscription,ARosterItem->subscription());
      index->setData(IRosterIndex::DR_Ask,ARosterItem->ask());
      itemList.append(index);
    }
  }

  foreach(index,curItemList)
    if (!itemList.contains(index))
      removeRosterIndex(index);
}

void RostersModel::onRosterItemRemoved(IRosterItem *ARosterItem)
{
  QString streamJid = ARosterItem->roster()->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IRosterIndex *streamRoot = streamItem.root;

  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_RosterJid,ARosterItem->jid().pBare());     
  IRosterIndexList itemList = streamRoot->findChild(data,true);
  IRosterIndex *index;
  foreach(index, itemList)
    removeRosterIndex(index);    
}

void RostersModel::onSelfPresence(IPresence::Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid)
{
  IPresence *presence = dynamic_cast<IPresence *>(sender());
  QString streamJid = presence->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  if (!AToJid.isValid())
  {
    streamRoot->setData(IRosterIndex::DR_Show, AShow);
    streamRoot->setData(IRosterIndex::DR_Status, AStatus);
    streamRoot->setData(IRosterIndex::DR_Priority, APriority);
  }
  else                   
  {
    QHash<int, QVariant> data;
    data.insert(IRosterIndex::DR_RosterJid, AToJid.pBare());
    IRosterIndexList indexList = streamRoot->findChild(data,true);
    IRosterIndex *index;
    foreach(index,indexList)
    {
      if (streamPresence->show() == AShow)
        index->setData(IRosterIndex::DR_Self_Show,QVariant());
      else
        index->setData(IRosterIndex::DR_Self_Show,AShow);

      if (streamPresence->status() == AStatus)
        index->setData(IRosterIndex::DR_Self_Status,QVariant());
      else
        index->setData(IRosterIndex::DR_Self_Status,AStatus); 

      if (streamPresence->priority() == APriority)
        index->setData(IRosterIndex::DR_Self_Priority,QVariant());
      else
        index->setData(IRosterIndex::DR_Self_Priority,APriority);  
    }
  }
}

void RostersModel::onPresenceItem(IPresenceItem *APresenceItem)
{
  QString streamJid = APresenceItem->presence()->streamJid().pFull();
  StreamItem streamItem = FStreams.value(streamJid);
  IRoster *streamRoster = streamItem.roster;
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  IRosterItem *rosterItem = streamRoster->item(APresenceItem->jid());

  int itemType = IRosterIndex::IT_Contact;
  if (APresenceItem->jid().equals(streamPresence->streamJid(),false))
    itemType = IRosterIndex::IT_MyResource; 
  else if (APresenceItem->jid().node().isEmpty())
    itemType = IRosterIndex::IT_Agent; 

  if (APresenceItem->show() == IPresence::Offline)
  {
    QHash<int,QVariant> data;
    data.insert(IRosterIndex::DR_Type,itemType);
    data.insert(IRosterIndex::DR_Id,APresenceItem->jid().pFull());
    IRosterIndexList indexList = streamRoot->findChild(data,true);
    IRosterIndex *index;
    int presItemCount = streamPresence->items(APresenceItem->jid()).count();
    foreach (index,indexList)
    {
      if (itemType == IRosterIndex::IT_MyResource || presItemCount > 1)
      {
        removeRosterIndex(index);
      }
      else
      {
        index->setData(IRosterIndex::DR_Id,APresenceItem->jid().pBare());
        index->setData(IRosterIndex::DR_Jid,APresenceItem->jid().bare());
        index->setData(IRosterIndex::DR_Show,APresenceItem->show());
        index->setData(IRosterIndex::DR_Status,APresenceItem->status());
        index->setData(IRosterIndex::DR_Priority,QVariant()); 
      }
    }
  }
  else if (APresenceItem->show() == IPresence::Error)
  {
    QHash<int,QVariant> data;
    data.insert(IRosterIndex::DR_Type,itemType);
    data.insert(IRosterIndex::DR_RosterJid,APresenceItem->jid().pBare());
    IRosterIndexList indexList = streamRoot->findChild(data,true);
    IRosterIndex *index;
    foreach(index,indexList)
    {
      index->setData(IRosterIndex::DR_Show,APresenceItem->show());
      index->setData(IRosterIndex::DR_Status,APresenceItem->status());
      index->setData(IRosterIndex::DR_Priority,QVariant()); 
    }
  }
  else
  {
    QHash<int,QVariant> data;
    data.insert(IRosterIndex::DR_Type,itemType);
    data.insert(IRosterIndex::DR_Id,APresenceItem->jid().pFull());
    IRosterIndexList indexList = streamRoot->findChild(data,true);

    if (indexList.isEmpty())
    {
      QSet<QString> itemGroups;
      if (rosterItem)
        itemGroups = rosterItem->groups();
      if (itemGroups.isEmpty())
        itemGroups.insert(""); 

      QString group;
      QString groupDelim = streamRoster->groupDelimiter();
      foreach(group,itemGroups)
      {
        IRosterIndex *groupIndex = 0;
        if (itemType == IRosterIndex::IT_MyResource)
          groupIndex = createGroup(myResourcesGroupName(),groupDelim,IRosterIndex::IT_MyResourcesGroup,streamRoot);
        else if (!rosterItem)
          groupIndex = createGroup(notInRosterGroupName(),groupDelim,IRosterIndex::IT_NotInRosterGroup,streamRoot);
        else if (itemType == IRosterIndex::IT_Agent)
          groupIndex = findGroup(agentsGroupName(),groupDelim,IRosterIndex::IT_AgentsGroup,streamRoot);
        else if (group.isEmpty()) 
          groupIndex = findGroup(blankGroupName(),groupDelim,IRosterIndex::IT_BlankGroup,streamRoot);
        else
          groupIndex = findGroup(group,groupDelim,IRosterIndex::IT_Group,streamRoot);

        if (groupIndex)
        {
          IRosterIndex *index = findRosterIndex(itemType,APresenceItem->jid().pBare(),groupIndex);   
          if (!index)
          {
            index = createRosterIndex(itemType,APresenceItem->jid().pFull(),groupIndex);
            index->setData(IRosterIndex::DR_StreamJid,streamJid);
            index->setData(IRosterIndex::DR_RosterJid,APresenceItem->jid().pBare());
            index->setData(IRosterIndex::DR_RosterGroup,group); 
            if (rosterItem)
            {
              index->setData(IRosterIndex::DR_RosterName,rosterItem->name()); 
              index->setData(IRosterIndex::DR_Subscription,rosterItem->subscription());
              index->setData(IRosterIndex::DR_Ask,rosterItem->ask());
            }
          }
          else
            index->setData(IRosterIndex::DR_Id,APresenceItem->jid().pFull());

          index->setData(IRosterIndex::DR_Jid,APresenceItem->jid().full());
          index->setData(IRosterIndex::DR_Show,APresenceItem->show());
          index->setData(IRosterIndex::DR_Status,APresenceItem->status());
          index->setData(IRosterIndex::DR_Priority,APresenceItem->priority());
          insertRosterIndex(index,groupIndex);
        }
      }
    }
    else foreach(IRosterIndex *index,indexList)
    {
      index->setData(IRosterIndex::DR_Show,APresenceItem->show());
      index->setData(IRosterIndex::DR_Status,APresenceItem->status());
      index->setData(IRosterIndex::DR_Priority,APresenceItem->priority());
    }
  }
}

void RostersModel::onIndexDataChanged(IRosterIndex *AIndex, int ARole)
{
  QModelIndex modelIndex = createIndex(AIndex->row(),0,AIndex);
  emit dataChanged(modelIndex,modelIndex);
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

