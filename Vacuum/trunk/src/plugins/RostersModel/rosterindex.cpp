#include <QtDebug>
#include "rosterindex.h"

RosterIndex::RosterIndex(int AType, const QString &AId)
{
  FParentIndex = NULL;
  FData.insert(DR_Type,AType);
  FData.insert(DR_Id,AId);
  FFlags = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  FItemDelegate = NULL;
  FRemoveOnLastChildRemoved = true;
  FRemoveChildsOnRemoved = true;
  FDestroyOnParentRemoved = true;
  FBlokedSetParentIndex = false;
}

RosterIndex::~RosterIndex()
{
  if (FChilds.count() > 0)
    removeAllChilds();
  if (FParentIndex)
    FParentIndex->removeChild(this);
}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
  if (FBlokedSetParentIndex || FParentIndex == AIndex)
    return;

  FBlokedSetParentIndex = true;

  if (FParentIndex)
  {
    FParentIndex->removeChild(this);
    setParent(0);
  }

  if (AIndex)
  {
    FParentIndex = AIndex;
    FParentIndex->appendChild(this); 
    setParent(FParentIndex->instance());
  } 
  else
  {
    FParentIndex = AIndex;

    if (FRemoveChildsOnRemoved)
      removeAllChilds();

    if (FDestroyOnParentRemoved)
      deleteLater();
  }

  FBlokedSetParentIndex = false;
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
    emit childAboutToBeInserted(AIndex);
    connect(AIndex->instance(),SIGNAL(destroyed(QObject *)),SLOT(onChildIndexDestroyed(QObject *)));
    emit childInserted(AIndex);
  }
}

bool RosterIndex::removeChild(IRosterIndex *AIndex)
{
  bool removed = false;

  if (FChilds.contains(AIndex))
  {
    emit childAboutToBeRemoved(AIndex);
    FChilds.removeAt(FChilds.indexOf(AIndex));
    AIndex->setParentIndex(0);
    disconnect(AIndex->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onChildIndexDestroyed(QObject *))); 
    emit childRemoved(AIndex);

    if (FRemoveOnLastChildRemoved && FChilds.isEmpty())
      setParentIndex(0);

    removed = true;
  }
  return removed;
}

void RosterIndex::removeAllChilds()
{
  foreach(IRosterIndex *index,FChilds)
    removeChild(index);
}

int RosterIndex::childRow(const IRosterIndex *AIndex) const
{
  return FChilds.indexOf((IRosterIndex * const)AIndex); 
}

IRosterIndexDataHolder *RosterIndex::setDataHolder(int ARole, IRosterIndexDataHolder *ADataHolder)
{
  IRosterIndexDataHolder *oldDataHolder = FDataHolders.value(ARole,0);
  if (oldDataHolder != ADataHolder)
  {
    if (ADataHolder)
      FDataHolders.insert(ARole,ADataHolder);
    else
      FDataHolders.remove(ARole);
  }
  return oldDataHolder;
}

QHash<int,IRosterIndexDataHolder *> RosterIndex::setDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  QHash<int,IRosterIndexDataHolder *> oldDataHolders;
  int role;
  foreach(role, ADataHolder->roles())
    oldDataHolders.insert(role,setDataHolder(role,ADataHolder));
  return oldDataHolders;
}

bool RosterIndex::setData(int ARole, const QVariant &AData)
{
  bool dataSeted = false;
  QVariant oldData = data(ARole);

  IRosterIndexDataHolder *dataHolder = FDataHolders.value(ARole,0);
  if (!dataHolder)
    dataHolder = FDataHolders.value(DR_AnyRole,0);

  if (dataHolder)
    dataSeted = dataHolder->setData(this,ARole,AData);

  if (!dataSeted)
  {
    if (AData.isValid()) 
      FData.insert(ARole,AData);
    else
      FData.remove(ARole); 
    dataSeted = true;
  }

  if (dataSeted && oldData != AData)
    emit dataChanged(this);

  return dataSeted;
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

void RosterIndex::onDataChanged(int /*ARole*/)
{
  emit dataChanged(this);
}

void RosterIndex::onChildIndexDestroyed(QObject *AIndex)
{
// TODO:   Не работает преобразование
  IRosterIndex *index = dynamic_cast<IRosterIndex *>(AIndex);
  if (FChilds.contains(index))
  {
    emit childAboutToBeRemoved(index);
    FChilds.removeAt(FChilds.indexOf(index));  
    emit childRemoved(index);
  }
}
