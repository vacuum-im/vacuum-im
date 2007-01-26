#include <QtDebug>
#include "rostersmodel.h"

RostersModel::RostersModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  qDebug() << "RostersModel";
  FRootIndex = new RosterIndex(IRosterIndex::IT_Root,"IT_Root");
  FRootIndex->setParent(this);
  connect(FRootIndex,SIGNAL(dataChanged(IRosterIndex *)),
    SLOT(onIndexDataChanged(IRosterIndex *)));
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
  qDebug() << "~RostersModel";
  delete FRootIndex;
}

QModelIndex RostersModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex;
  if (!AParent.isValid())
    parentIndex = FRootIndex;
  else
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());

  IRosterIndex *childIndex = parentIndex->child(ARow);

  if (childIndex)
    return createIndex(ARow,AColumn,childIndex);

  return QModelIndex();
}

QModelIndex RostersModel::parent(const QModelIndex &AIndex) const
{
  if (!AIndex.isValid())
    return QModelIndex();

  IRosterIndex *curIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
  IRosterIndex *parentIndex = curIndex->parentIndex();

  if (parentIndex && parentIndex != FRootIndex)
    return createIndex(parentIndex->row(),0,parentIndex); 

  return QModelIndex();
}

bool RostersModel::hasChildren(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex;
  if (!AParent.isValid())
    parentIndex = FRootIndex;
  else
    parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  if (parentIndex)
    return parentIndex->childCount() > 0;
  return false;
}

int RostersModel::rowCount(const QModelIndex &AParent) const
{
  if (!AParent.isValid())
    return FRootIndex->childCount();

  IRosterIndex *parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  if (parentIndex)
    return parentIndex->childCount();

  return 0;
}

int RostersModel::columnCount(const QModelIndex &/*AParent*/) const
{
  return 1;
}

Qt::ItemFlags RostersModel::flags(const QModelIndex &AIndex) const
{
  IRosterIndex *index = (IRosterIndex *)AIndex.internalPointer(); 
  return index->flags();  
}

QVariant RostersModel::data(const QModelIndex &AIndex, int ARole) const 
{
  if (!AIndex.isValid())
    return QVariant();

  IRosterIndex *index = static_cast<IRosterIndex *>(AIndex.internalPointer());
  if (index)
    return index->data(ARole);

  return QVariant();
}

IRosterIndex *RostersModel::appendStream( IRoster *ARoster, IPresence *APresence )
{
  if (!ARoster)
    return NULL;

  QString streamJid = ARoster->streamJid().pFull();
  
  if (FStreams.contains(streamJid))
    return FStreams.value(streamJid).root;
  
  connect(ARoster->instance(),SIGNAL(itemPush(IRosterItem *)),SLOT(onRosterItemPush(IRosterItem *))); 
  connect(ARoster->instance(),SIGNAL(itemRemoved(IRosterItem *)),SLOT(onRosterItemRemoved(IRosterItem *))); 

  IRosterIndex *index = createRosterIndex(IRosterIndex::IT_StreamRoot,streamJid,FRootIndex);
  index->setData(Qt::DisplayRole, ARoster->streamJid().full());
  index->setData(IRosterIndex::DR_StreamJid,streamJid);
  index->setData(IRosterIndex::DR_Jid,ARoster->streamJid().full());

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
  FStreams.insert(streamJid,streamItem);

  emit streamAdded(streamJid);

  return index;
}

bool RostersModel::removeStream( const QString &AStreamJid )
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
    return true;
  }
  return false;
}

IRoster *RostersModel::getRoster( const QString &AStreamJid ) const
{
  if (FStreams.contains(AStreamJid))
    return FStreams.value(AStreamJid).roster;

  return NULL;
}

IPresence *RostersModel::getPresence( const QString &AStreamJid ) const
{
  if (FStreams.contains(AStreamJid))
    return FStreams.value(AStreamJid).presence;
  return NULL;
}

IRosterIndex *RostersModel::getStreamRoot( const Jid &AStreamJid ) const
{
  if (FStreams.contains(AStreamJid.pFull()))
    return FStreams.value(AStreamJid.pFull()).root;
  return NULL;
}

IRosterIndex *RostersModel::createRosterIndex( int AType, const QString &AId, IRosterIndex *AParent )
{
  IRosterIndex *index = findRosterIndex(AType,AId,AParent);
  if (!index)
  {
    index = new RosterIndex(AType,AId);
    index->setParentIndex(AParent);
    if (AParent)
      index->setData(RosterIndex::DR_StreamJid,AParent->data(RosterIndex::DR_StreamJid));
  }
  return index;
}

IRosterIndex *RostersModel::createGroup( const QString &AName, int AType, IRosterIndex *AParent )
{
  IRosterIndex *index = findGroup(AName,AType,AParent);
  if (!index)
  {
    int i = 0;
    index = AParent;
    IRosterIndex *newIndex;
    QList<QString> groupTree = AName.split("::",QString::SkipEmptyParts); 
    do
    {
      if (newIndex = findGroup(groupTree.at(i),AType,index))
      {
        index = newIndex;
        i++;
      }
    } while (newIndex && i<groupTree.count());

    while (i<groupTree.count())
    {
      index = createRosterIndex(AType,groupTree.at(i),index);
      index->setData(Qt::DisplayRole,groupTree.at(i)); 
      i++;
    }
  }
  return index;
}

IRosterIndex *RostersModel::findRosterIndex( int AType, const QVariant &AId, IRosterIndex *AParent ) const
{
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_Type,AType);  
  data.insert(IRosterIndex::DR_Id,AId);
  if (AParent)
    return AParent->findChild(data).value(0,0);
  return FRootIndex->findChild(data).value(0,0) ;
}

IRosterIndex *RostersModel::findGroup( const QString &AName, int AType, IRosterIndex *AParent ) const
{
  IRosterIndex *index = 0;
  QList<QString> groupTree = AName.split("::",QString::SkipEmptyParts);
  int i = 0;
  index = AParent;
  do {
    index = findRosterIndex(AType,groupTree.at(i++),index);  
  } while (index && i<groupTree.count());
  return index;
}

bool RostersModel::removeRosterIndex( IRosterIndex *AIndex )
{
  if (AIndex && AIndex != FRootIndex)
  {
    AIndex->setParentIndex(0);
    return true;
  }
  return false;
}

void RostersModel::onRosterItemPush( IRosterItem *ARosterItem )
{
  QString streamJid = ARosterItem->roster()->streamJid().pFull();
  Q_ASSERT(FStreams.contains(streamJid));
  StreamItem streamItem = FStreams.value(streamJid);
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;


  QString itemId = ARosterItem->jid().pFull();
  int itemType = IRosterIndex::IT_Contact;
  if (ARosterItem->jid().node().isEmpty())
    itemType = IRosterIndex::IT_Transport;

  int groupType; 
  QString groupDisplay;
  QSet<QString> itemGroups; 
  if (itemType == IRosterIndex::IT_Transport)
  {
    groupType = IRosterIndex::IT_TransportsGroup;
    groupDisplay = transportsGroupName();
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
  data.insert(IRosterIndex::DR_RosterJid,ARosterItem->jid().pBare());     
  IRosterIndexList curItemList = streamRoot->findChild(data,true);
  QSet<QString> curGroups;
  foreach(index,curItemList)
    curGroups.insert(index->data(IRosterIndex::DR_RosterGroup).toString());
  QSet<QString> newGroups = itemGroups - curGroups;
  QSet<QString> oldGroups = curGroups - itemGroups; 

  bool resendPresence = false;
  IRosterIndexList itemList; 
  QSet<QString>::const_iterator group = itemGroups.begin();
  while (group!=itemGroups.end())
  {
    IRosterIndex *groupIndex;
    if (groupType != IRosterIndex::IT_Group)
      groupIndex = createGroup(groupDisplay,groupType,streamRoot);
    else 
      groupIndex = createGroup(*group,groupType,streamRoot);

    IRosterIndexList groupItemList;
    if (newGroups.contains(*group) && !oldGroups.isEmpty())
    {
      IRosterIndex *oldGroupIndex;
      QString oldGroup = oldGroups.values().value(0);
      if (oldGroup.isEmpty())
        oldGroupIndex = findGroup(blankGroupName(),IRosterIndex::IT_BlankGroup,streamRoot);
      else
        oldGroupIndex = findGroup(oldGroup,IRosterIndex::IT_Group,streamRoot);
      if (oldGroupIndex)
      {
        groupItemList = oldGroupIndex->findChild(data);
        foreach(index,groupItemList)
        {
          index->setParentIndex(groupIndex);
          index->setData(IRosterIndex::DR_RosterGroup,*group);  
        }
      }
    }

    if (groupItemList.isEmpty())
    {
      index = createRosterIndex(itemType,itemId,groupIndex);
      index->setData(Qt::DisplayRole,ARosterItem->name()); 
      index->setData(IRosterIndex::DR_Jid,ARosterItem->jid().bare());
      index->setData(IRosterIndex::DR_StreamJid,ARosterItem->roster()->streamJid().pFull());     
      index->setData(IRosterIndex::DR_RosterJid,ARosterItem->jid().pBare());
      index->setData(IRosterIndex::DR_RosterGroup,*group); 
      groupItemList.append(index); 
      resendPresence = true;
    }

    foreach(index,groupItemList)
    {
      index->setData(IRosterIndex::DR_Subscription,ARosterItem->subscription());
      index->setData(IRosterIndex::DR_Ask,ARosterItem->ask());
      itemList.append(index);
    }
    group++;
  }

  foreach(index,curItemList)
    if (!itemList.contains(index))
      removeRosterIndex(index);

  if (resendPresence && streamPresence)
  {
    IPresenceItem *presItem;
    QList<IPresenceItem *> presItems = streamPresence->items(ARosterItem->jid());
    foreach(presItem,presItems)
      onPresenceItem(presItem);
  }
}

void RostersModel::onRosterItemRemoved( IRosterItem *ARosterItem )
{
  QString streamJid = ARosterItem->roster()->streamJid().pFull();
  Q_ASSERT(FStreams.contains(streamJid));
  StreamItem streamItem = FStreams.value(streamJid);
  IRosterIndex *streamRoot = streamItem.root;

  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_RosterJid,ARosterItem->jid().pBare());     
  IRosterIndexList itemList = streamRoot->findChild(data,true);
  IRosterIndex *index;
  foreach(index, itemList)
    removeRosterIndex(index);    
}

void RostersModel::onSelfPresence( IPresence::Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid )
{
  IPresence *presence = dynamic_cast<IPresence *>(sender());
  Q_ASSERT(presence);
  QString streamJid = presence->streamJid().pFull();
  Q_ASSERT(FStreams.contains(streamJid));
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

void RostersModel::onPresenceItem( IPresenceItem *APresenceItem )
{
  QString streamJid = APresenceItem->presence()->streamJid().pFull();
  Q_ASSERT(FStreams.contains(streamJid));
  StreamItem streamItem = FStreams.value(streamJid);
  IRoster *streamRoster = streamItem.roster;
  IPresence *streamPresence = streamItem.presence;
  IRosterIndex *streamRoot = streamItem.root;

  IRosterItem *rosterItem = streamRoster->item(APresenceItem->jid());

  int itemType = IRosterIndex::IT_Contact;
  if (APresenceItem->jid().equals(streamPresence->streamJid(),false))
    itemType = IRosterIndex::IT_MyResource; 
  else if (APresenceItem->jid().node().isEmpty())
    itemType = IRosterIndex::IT_Transport; 

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
      else if (presItemCount == 1)
      {
        if (rosterItem)
          index->setData(Qt::DisplayRole,rosterItem->name()); 
        else
          index->setData(Qt::DisplayRole,APresenceItem->jid().bare());
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
    data.insert(IRosterIndex::DR_Id,APresenceItem->jid().pFull());
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
      foreach(group,itemGroups)
      {
        IRosterIndex *groupIndex = 0;
        if (itemType == IRosterIndex::IT_MyResource)
          groupIndex = createGroup(myResourcesGroupName(),IRosterIndex::IT_MyResourcesGroup,streamRoot);
        else if (itemType == IRosterIndex::IT_Transport)
          groupIndex = findGroup(transportsGroupName(),IRosterIndex::IT_Group,streamRoot);
        else if (group.isEmpty()) 
          groupIndex = findGroup(blankGroupName(),IRosterIndex::IT_BlankGroup,streamRoot);
        else
          groupIndex = findGroup(group,IRosterIndex::IT_Group,streamRoot);

        if (groupIndex)
        {
          IRosterIndex *index = findRosterIndex(itemType,APresenceItem->jid().pBare(),groupIndex);   
          if (!index)
          {
            index = createRosterIndex(itemType,APresenceItem->jid().pFull(),groupIndex);
            index->setData(IRosterIndex::DR_RosterJid,APresenceItem->jid().pBare());
            index->setData(IRosterIndex::DR_RosterGroup,group); 
          }
          else
            index->setData(IRosterIndex::DR_Id,APresenceItem->jid().pFull());

          if (itemType == IRosterIndex::IT_MyResource)
            index->setData(Qt::DisplayRole,APresenceItem->jid().resource());  
          else if (rosterItem)
            index->setData(Qt::DisplayRole,rosterItem->name()+"/"+APresenceItem->jid().resource()); 
          else
            index->setData(Qt::DisplayRole,APresenceItem->jid().full()); 
          index->setData(IRosterIndex::DR_Jid,APresenceItem->jid().full());

          indexList.append(index); 
        }
      }
    }

    IRosterIndex *index;
    foreach(index,indexList)
    {
      index->setData(IRosterIndex::DR_Show,APresenceItem->show());
      index->setData(IRosterIndex::DR_Status,APresenceItem->status());
      index->setData(IRosterIndex::DR_Priority,APresenceItem->priority()); 
    }
  }
}

void RostersModel::onIndexDataChanged( IRosterIndex *AIndex )
{
  emit dataChanged(createIndex(AIndex->row(),0,AIndex),createIndex(AIndex->row(),0,AIndex));
  emit indexChanged(AIndex);
}

void RostersModel::onIndexChildAboutToBeInserted( IRosterIndex *AIndex )
{
  IRosterIndex *parentIndex = AIndex->parentIndex();
  if (parentIndex)
  {
    beginInsertRows(createIndex(parentIndex->row(),0,parentIndex),AIndex->row(),AIndex->row());

    connect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *)),
      SLOT(onIndexDataChanged(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childAboutToBeInserted(IRosterIndex *)),
      SLOT(onIndexChildAboutToBeInserted(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childInserted(IRosterIndex *)),
      SLOT(onIndexChildInserted(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childAboutToBeRemoved(IRosterIndex *)),
      SLOT(onIndexChildAboutToBeRemoved(IRosterIndex *)));
    connect(AIndex->instance(),SIGNAL(childRemoved(IRosterIndex *)),
      SLOT(onIndexChildRemoved(IRosterIndex *)));
    emit indexInsert(AIndex);
  }
}

void RostersModel::onIndexChildInserted( IRosterIndex *AIndex)
{
  endInsertRows();
}

void RostersModel::onIndexChildAboutToBeRemoved( IRosterIndex *AIndex )
{
  IRosterIndex *parentIndex = AIndex->parentIndex();
  //qDebug() << "Remove from" << parentIndex->id() << parentIndex->row() << "index" << AIndex->id() << AIndex->row();
  if (parentIndex)
  {
    beginRemoveRows(createIndex(parentIndex->row(),0,parentIndex),AIndex->row(),AIndex->row());
    emit indexRemove(AIndex);
  }
}

void RostersModel::onIndexChildRemoved(IRosterIndex *AIndex)
{
  disconnect(AIndex->instance(),SIGNAL(dataChanged(IRosterIndex *)),
    this,SLOT(onIndexDataChanged(IRosterIndex *)));
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