#include "rostermodel.h"

RosterModel::RosterModel(IRoster *ARoster)
{
  setParent(ARoster->instance()); 
  FRoster = ARoster;
  FRootIndex = new RosterIndex(IRosterIndex::IT_Root);
  FRootIndex->setParent(this); 
}

RosterModel::~RosterModel()
{

}

QModelIndex RosterModel::index(int ARow, int AColumn, const QModelIndex &AParent) const
{
  if (!AParent.isValid())
    return createIndex(ARow,AColumn,FRootIndex);

  IRosterIndex *parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
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
  
  if (parentIndex)
    return createIndex(parentIndex->row(),0,parentIndex); 

  return QModelIndex();
}

bool RosterModel::hasChildren(const QModelIndex &AParent) const
{
  IRosterIndex *parentIndex = static_cast<IRosterIndex *>(AParent.internalPointer());
  return parentIndex->childCount() > 0;
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

  IRosterIndex *currentIndex = static_cast<IRosterIndex *>(AIndex.internalPointer());
  if (currentIndex)
    return currentIndex->data(ARole);

  return QVariant();
}
