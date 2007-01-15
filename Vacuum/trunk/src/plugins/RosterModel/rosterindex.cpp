#include <QtDebug>
#include "rosterindex.h"

RosterIndex::RosterIndex(int AType, const QString &AId)
{
  FParentIndex =0;
  FData.insert(DR_Type,AType);
  FData.insert(DR_Id,AId);
  FFlags = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  FRemoveOnLastChildRemoved = false;
  //qDebug() << "RosterIndex" << id();
}

RosterIndex::~RosterIndex()
{
  //qDebug() << "~RosterIndex" << id();
}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
  if (FParentIndex == AIndex)
    return;

  if (FParentIndex)
  {
    IRosterIndex *oldParent = FParentIndex;
    FParentIndex = 0;
    oldParent->removeChild(this); 
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
    childAboutToBeInserted(AIndex);
    FChilds.append(AIndex); 
    AIndex->setParentIndex(this);
    connect(AIndex->instance(),SIGNAL(destroyed(QObject *)),SLOT(onChildIndexDestroyed(QObject *)));
    childInserted(AIndex);
  }
}

bool RosterIndex::removeChild(IRosterIndex *AIndex, bool ARecurse)
{
  bool removed = false;

  if (FChilds.contains(AIndex))
  {
    childAboutToBeRemoved(AIndex);
    FChilds.removeAt(FChilds.indexOf(AIndex));
    AIndex->setParentIndex(0);
    disconnect(AIndex->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onChildIndexDestroyed(QObject *))); 
    childRemoved(AIndex);
    if (FRemoveOnLastChildRemoved && FChilds.isEmpty())
    {
      setParentIndex(0);
      this->deleteLater();
    }
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
  bool dataSet = false;
  QVariant oldData = data(ARole);

  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);
  if (!dataHolder)
    dataHolder = FDataHolders.value(DR_AnyRole,0);

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
    emit dataChanged(this);

  return dataSet;
}

QVariant RosterIndex::data(int ARole) const
{
  QVariant data;

  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);
  if (!dataHolder)
    dataHolder = FDataHolders.value(DR_AnyRole,0);

  if (dataHolder)
    data = dataHolder->data(this,ARole);

  if (!data.isValid())
    data = FData.value(ARole,QVariant());
  
  return data;
}

IRosterIndexList RosterIndex::findChild(const QHash<int, QVariant> AData, bool ARecurse) const
{
  IRosterIndexList indexes;
  
  IRosterIndex *index;
  foreach (index, FChilds)
  {
    bool cheked = true;
    QHash<int,QVariant>::const_iterator i = AData.begin();
    while (cheked && i!=AData.end())
    {
      cheked = (i.value() == index->data(i.key()));
      i++;
    }
    if (cheked)
      indexes.append(index);
    if (ARecurse)
      indexes += index->findChild(AData,ARecurse); 
  }

  return indexes;  
}

void RosterIndex::onChildIndexDestroyed(QObject *AIndex)
{
  IRosterIndex *index = qobject_cast<IRosterIndex *>(AIndex);
  if (FChilds.contains(index))
  {
    emit childAboutToBeRemoved(index);
    FChilds.removeAt(FChilds.indexOf(index));  
    emit childRemoved(index);
  }
}

void RosterIndex::onDataHolderChanged()
{
  emit dataChanged(this);
}
