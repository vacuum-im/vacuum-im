#include <QtDebug>
#include "rostermodel.h"

RosterModel::RosterModel(IRoster *ARoster, IPresence *APresence)
{
  FRoster = ARoster;
  FPresence = APresence;
  FRootIndex = new RosterIndex(IRosterIndex::IT_Root,"IT_Root");
  FRootIndex->setParent(this); 
  if (ARoster)
  {
    connect(ARoster->instance(),SIGNAL(itemPush(IRosterItem *)),SLOT(onRosterItemPush(IRosterItem *))); 
    connect(ARoster->instance(),SIGNAL(itemRemoved(IRosterItem *)),SLOT(onRosterItemRemoved(IRosterItem *))); 
    setParent(ARoster->instance());
    FRootIndex->setData(Qt::DisplayRole, ARoster->streamJid().bare());
    FRootIndex->setData(IRosterIndex::DR_StreamJid,ARoster->streamJid().prep().full()); 
  }
  if (APresence)
  {
    connect(APresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *))); 
    connect(APresence->instance(),SIGNAL(selfPresence(IPresence::Show , const QString &, qint8 , const Jid &)),
      SLOT(onSelfPresence(IPresence::Show , const QString &, qint8 , const Jid &)));
    FRootIndex->setData(IRosterIndex::DR_Show,APresence->show());
    FRootIndex->setData(IRosterIndex::DR_Status,APresence->status());
    FRootIndex->setData(IRosterIndex::DR_Priority,APresence->priority());  
  }
}

RosterModel::~RosterModel()
{
  qDebug()<< "~RosterModel";
}

QModelIndex RosterModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
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

QModelIndex RosterModel::parent(const QModelIndex &AIndex) const
{
  if (!AIndex.isValid())
    return QModelIndex();

  IRosterIndex *currentIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
  IRosterIndex *parentIndex = currentIndex->parentIndex(); 
  
  if (parentIndex && parentIndex != FRootIndex)
    return createIndex(parentIndex->row(),0,parentIndex); 

  return QModelIndex();
}

bool RosterModel::hasChildren(const QModelIndex &AParent) const
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

int RosterModel::rowCount(const QModelIndex &AParent) const
{
  if (!AParent.isValid())
    return FRootIndex->childCount();

  IRosterIndex *parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  if (parentIndex)
    return parentIndex->childCount();

  return 0;
}

int RosterModel::columnCount(const QModelIndex &/*AParent*/) const
{
  return 1;
}

QVariant RosterModel::data(const QModelIndex &AIndex, int ARole) const 
{
  if (!AIndex.isValid())
    return QVariant();

  IRosterIndex *index = static_cast<IRosterIndex *>(AIndex.internalPointer());
  if (index)
    return index->data(ARole);

  return QVariant();
}

QVariant RosterModel::headerData(int ASection, Qt::Orientation AOrientation, int ARole) const
{
  if (ASection ==0 && AOrientation == Qt::Horizontal)
    return FRootIndex->data(ARole); 

  return QVariant();
}

//IRosterModel
IRosterIndex *RosterModel::createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent)
{
  IRosterIndex *index = findRosterIndex(AType,AId,AParent);
  if (!index)
  {
    index = new RosterIndex(AType,AId);
    index->setParentIndex(AParent);
  }
  return index;
}

IRosterIndex *RosterModel::createGroup(const QString &AName, IRosterIndex *AParent)
{
  IRosterIndex *index = findGroup(AName,AParent);
  if (!index)
  {
    int i = 0;
    index = AParent;
    IRosterIndex *newIndex;
    QList<QString> groupTree = AName.split("::",QString::SkipEmptyParts); 
    do
    {
      if (newIndex = findGroup(groupTree.at(i),index))
      {
        index = newIndex;
        i++;
      }
    } while (newIndex && i<groupTree.count());

    while (i<groupTree.count())
    {
      index = createRosterIndex(IRosterIndex::IT_Group,groupTree.at(i),index);
      index->setData(Qt::DisplayRole,groupTree.at(i)); 
      i++;
    }
  }
  return index;
}

IRosterIndex *RosterModel::createGroup(const QString &AName, int AType, IRosterIndex *AParent)
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

IRosterIndex *RosterModel::createContact(const Jid &AJid, IRosterIndex *AParent)
{
  IRosterIndex *index = createRosterIndex(IRosterIndex::IT_Contact,AJid.prep().full(),AParent);
  index->setData(Qt::DisplayRole,AJid.full());   
  index->setData(IRosterIndex::DR_Jid,AJid.full());
  return index;
}

IRosterIndex *RosterModel::findRosterIndex(int AType, const QVariant &AId, IRosterIndex *AParent) const
{
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_Type,AType);  
  data.insert(IRosterIndex::DR_Id,AId);
  if (!AParent)
    return FRootIndex->findChild(data).value(0,0) ;
  else
    return AParent->findChild(data).value(0,0);
}

IRosterIndex *RosterModel::findGroup(const QString &AName, IRosterIndex *AParent) const
{
  IRosterIndex *index =0;
  QList<QString> groupTree = AName.split("::",QString::SkipEmptyParts);
  int i = 0;
  index = AParent;
  do {
    index = findRosterIndex(IRosterIndex::IT_Group,groupTree.at(i++),index);  
  } while (index && i<groupTree.count());
  return index;
}

IRosterIndex *RosterModel::findGroup(const QString &AName, int AType, IRosterIndex *AParent) const 
{
  IRosterIndex *index =0;
  QList<QString> groupTree = AName.split("::",QString::SkipEmptyParts);
  int i = 0;
  index = AParent;
  do {
    index = findRosterIndex(AType,groupTree.at(i++),index);  
  } while (index && i<groupTree.count());
  return index;
}
bool RosterModel::removeRosterIndex(IRosterIndex *AIndex)
{
  RosterIndex *index = qobject_cast<RosterIndex *>(AIndex->instance());
  if (index && index != FRootIndex)
  {
    index->setParentIndex(0);
    delete index;
    return true;
  }
  return false;
}

void RosterModel::onRosterItemPush(IRosterItem *ARosterItem)
{
  QString itemId = ARosterItem->jid().prep().full();
  int itemType = IRosterIndex::IT_Contact;
  if (ARosterItem->jid().node().isEmpty())
    itemType = IRosterIndex::IT_Transport;
  QString itemDisplay = ARosterItem->name();
  if (itemDisplay.isEmpty())
    itemDisplay = ARosterItem->jid().full();  
  
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
  data.insert(IRosterIndex::DR_RosterJid,ARosterItem->jid().prep().bare());     
  IRosterIndexList curItemList = FRootIndex->findChild(data,true);
  QSet<QString> curGroups;
  foreach(index,curItemList)
    curGroups.insert(index->data(IRosterIndex::DR_RosterGroup).toString());
  QSet<QString> newGroups = itemGroups - curGroups;
  QSet<QString> oldGroups = curGroups - itemGroups; 
  
  IRosterIndexList itemList; 
  QSet<QString>::const_iterator group = itemGroups.begin();
  while (group!=itemGroups.end())
  {
    IRosterIndex *groupIndex;
    if (groupType != IRosterIndex::IT_Group)
      groupIndex = createGroup(groupDisplay,groupType,FRootIndex);
    else 
      groupIndex = createGroup(*group,groupType,FRootIndex);

    IRosterIndexList groupItemList;
    if (newGroups.contains(*group) && !oldGroups.isEmpty())
    {
      IRosterIndex *oldGroupIndex;
      QString oldGroup = oldGroups.values().value(0);
      if (oldGroup.isEmpty())
        oldGroupIndex = findGroup(blankGroupName(),IRosterIndex::IT_BlankGroup,FRootIndex);
      else
        oldGroupIndex = findGroup(oldGroup,IRosterIndex::IT_Group,FRootIndex);
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
      index->setData(Qt::DisplayRole,itemDisplay); 
      index->setData(IRosterIndex::DR_StreamJid,ARosterItem->roster()->streamJid().prep().full());     
      index->setData(IRosterIndex::DR_RosterJid,ARosterItem->jid().prep().bare());
      index->setData(IRosterIndex::DR_RosterGroup,*group); 
      groupItemList.append(index); 
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

 emit layoutChanged();
}

void RosterModel::onRosterItemRemoved(IRosterItem *ARosterItem)
{
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_StreamJid,ARosterItem->roster()->streamJid().prep().full());      
  data.insert(IRosterIndex::DR_RosterJid,ARosterItem->jid().prep().bare());     
  IRosterIndexList indexList = FRootIndex->findChild(data,true);
  IRosterIndex *index;
  foreach(index, indexList)
    removeRosterIndex(index);    
}

void RosterModel::onSelfPresence(IPresence::Show AShow, const QString &AStatus, 
                                 qint8 APriority, const Jid &AToJid)
{
  if (!AToJid.isValid())
  {
    FRootIndex->setData(IRosterIndex::DR_Show, AShow);
    FRootIndex->setData(IRosterIndex::DR_Status, AStatus);
    FRootIndex->setData(IRosterIndex::DR_Priority, APriority);  
  }
}

void RosterModel::onPresenceItem(IPresenceItem *APresenceItem)
{
  QHash<int,QVariant> data;
  data.insert(IRosterIndex::DR_Type,IRosterIndex::IT_Contact);
  data.insert(IRosterIndex::DR_Id, APresenceItem->jid().prep().full());   
  IRosterIndexList indexList = FRootIndex->findChild(data,true);
  IRosterIndex *index;
  foreach(index, indexList)
  {
    index->setData(IRosterIndex::DR_Show,APresenceItem->show());
    index->setData(IRosterIndex::DR_Status,APresenceItem->status());
    index->setData(IRosterIndex::DR_Priority,APresenceItem->priority());   
  }
}

void RosterModel::onRosterIndexDataChanged(IRosterIndex *AIndex)
{
  emit dataChanged(createIndex(AIndex->row(),0,AIndex),createIndex(AIndex->row(),0,AIndex)); 
}
