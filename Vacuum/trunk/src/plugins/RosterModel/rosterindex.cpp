#include "rosterindex.h"

RosterIndex::RosterIndex(int AType)
{
  FType = AType;
}

RosterIndex::~RosterIndex()
{

}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
  if (FParentIndex == AIndex)
    return;

  if (FParentIndex)
  {
    FParentIndex = 0;
    FParentIndex->removeChild(this); 
    setParent(0);
  }

  if (AIndex)
  {
    FParentIndex = AIndex;
    FParentIndex->appendChild(this); 
    setParent(FParentIndex->instance());
  }
}

int RosterIndex::row() const
{
  if (FParentIndex)
    return FParentIndex->childRow(this);

  return 0;
}

void RosterIndex::appendChild(IRosterIndex *AIndex)
{
  if (AIndex && !FChilds.contains(AIndex))
  {
    FChilds.append(AIndex); 
    AIndex->setParentIndex(this);
    connect(AIndex->instance(),SIGNAL(destroyed(QObject *)),SLOT(onChildIndexDestroyed(QObject *))); 
  }
}

bool RosterIndex::removeChild(IRosterIndex *AIndex, bool ARecurse)
{
  bool removed = false;

  if (FChilds.contains(AIndex))
  {
    FChilds.removeAt(FChilds.indexOf(AIndex));
    AIndex->setParentIndex(0);
    disconnect(AIndex->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onChildIndexDestroyed(QObject *))); 
    removed = true;
  }
  else if (ARecurse)
  {
    int row = 0;
    while (!removed && row < FChilds.count())
      removed = FChilds.at(row++)->removeChild(AIndex,true); 
  }
  return removed;
}

int RosterIndex::childRow(const IRosterIndex *AIndex) const
{
  return FChilds.indexOf((IRosterIndex * const)AIndex); 
}

IRosterIndexDataHolder *RosterIndex::setDataHolder(int ARole, IRosterIndexDataHolder *ADataHolder)
{
  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);

  if (ADataHolder)
    FDataHolders.insert(ARole,ADataHolder);
  else
    FDataHolders.remove(ARole);

  return dataHolder;
}

bool RosterIndex::setData(int ARole, const QVariant &AData)
{
  QVariant oldData = data(ARole);
  bool dataSet = false;

  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);
  if (!dataHolder)
    dataHolder = FDataHolders.value(-1,0);

  if (!dataHolder)
  {
    if (AData.isValid()) 
      FData.insert(ARole,AData);
    else
      FData.remove(ARole); 
    dataSet = true;
  }
  else
    dataSet = dataHolder->setData(this,ARole,AData);

  if (dataSet && oldData != AData)
    emit dataChanged();

  return dataSet;
}

QVariant RosterIndex::data(int ARole) const
{
  QVariant data;

  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);
  if (!dataHolder)
    dataHolder = FDataHolders.value(-1,0);

  if (dataHolder)
    data = dataHolder->data(this,ARole);

  if (!data.isValid() && FData.contains(ARole))
    data = FData.value(ARole);
  
  return data;
}

void RosterIndex::onChildIndexDestroyed(QObject *AIndex)
{
  if (FChilds.contains((IRosterIndex *)AIndex))
    FChilds.removeAt(FChilds.indexOf((IRosterIndex *)AIndex));  
}

void RosterIndex::onDataHolderChanged()
{
  emit dataChanged();
}
