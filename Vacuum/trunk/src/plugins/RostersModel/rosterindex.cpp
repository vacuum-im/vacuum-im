#include <QtDebug>
#include "rosterindex.h"
#include <QTimer>

RosterIndex::RosterIndex(int AType, const QString &AId)
{
  FParentIndex = NULL;
  FData.insert(RDR_Type,AType);
  FData.insert(RDR_Id,AId);
  FFlags = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
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
  emit indexDestroyed(this);
}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
  if (FBlokedSetParentIndex || FParentIndex == AIndex)
    return;

  FBlokedSetParentIndex = true;

  if (!AIndex && FRemoveChildsOnRemoved)
    removeAllChilds();

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

    if (FDestroyOnParentRemoved)
      QTimer::singleShot(0,this,SLOT(onDestroyByParentRemoved()));
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

int RosterIndex::childRow(const IRosterIndex *AIndex) const
{
  return FChilds.indexOf((IRosterIndex * const)AIndex); 
}

bool RosterIndex::removeChild(IRosterIndex *AIndex)
{
  if (FChilds.contains(AIndex))
  {
    emit childAboutToBeRemoved(AIndex);
    FChilds.removeAt(FChilds.indexOf(AIndex));
    AIndex->setParentIndex(0);
    disconnect(AIndex->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onChildIndexDestroyed(QObject *))); 
    emit childRemoved(AIndex);

    if (FRemoveOnLastChildRemoved && FChilds.isEmpty())
      QTimer::singleShot(0,this,SLOT(onRemoveByLastChildRemoved()));

    return true;
  }
  return false;
}

void RosterIndex::removeAllChilds()
{
  foreach(IRosterIndex *index,FChilds)
    removeChild(index);
}

void RosterIndex::insertDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  connect(ADataHolder->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    SLOT(onDataHolderChanged(IRosterIndex *, int)));

  foreach (int role, ADataHolder->roles())
  {
    FDataHolders[role].insert(ADataHolder->order(),ADataHolder);
    dataChanged(this,role);
  }
  emit dataHolderInserted(ADataHolder);
}

void RosterIndex::removeDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  disconnect(ADataHolder->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    this,SLOT(onDataHolderChanged(IRosterIndex *, int)));
  
  foreach (int role, ADataHolder->roles())
  {
    FDataHolders[role].remove(ADataHolder->order(),ADataHolder);
    if (FDataHolders[role].isEmpty())
      FDataHolders.remove(role);
    dataChanged(this,role);
  }
  emit dataHolderRemoved(ADataHolder);
}

QVariant RosterIndex::data(int ARole) const
{
  QVariant data = FData.value(ARole);

  if (!data.isValid())
  {
    int i = 0;
    QList<IRosterIndexDataHolder *> dataHolders = FDataHolders.value(ARole).values();
    while (i < dataHolders.count() && !data.isValid())
      data = dataHolders.value(i++)->data(this,ARole);
  }

  return data;
}

void RosterIndex::setData(int ARole, const QVariant &AData)
{
  int i = 0;
  bool dataSeted = false;
  QList<IRosterIndexDataHolder *> dataHolders = FDataHolders.value(ARole).values();
  while (i < dataHolders.count() && !dataSeted)
    dataSeted = dataHolders.value(i++)->setData(this,ARole,AData);
  
  if (!dataSeted)
  {
    if (AData.isValid())
      FData.insert(ARole,AData);
    else
      FData.remove(ARole);
    emit dataChanged(this, ARole);
  }
}

IRosterIndexList RosterIndex::findChild(const QMultiHash<int, QVariant> AData, bool ASearchInChilds) const
{
  IRosterIndexList indexes;
  
  IRosterIndex *index;
  foreach (index, FChilds)
  {
    bool cheked = true;
    QList<int> dataRoles = AData.keys(); 
    QList<int>::const_iterator role = dataRoles.constBegin();
    while (cheked && role!=dataRoles.constEnd())
    {
      cheked = AData.values(*role).contains(index->data(*role));
      role++;
    }
    if (cheked)
      indexes.append(index);
    if (ASearchInChilds)
      indexes += index->findChild(AData,ASearchInChilds); 
  }

  return indexes;  
}

void RosterIndex::onDataHolderChanged(IRosterIndex *AIndex, int ARole)
{
 if (AIndex == this || AIndex == NULL)
   emit dataChanged(this, ARole);
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

void RosterIndex::onRemoveByLastChildRemoved()
{
  if (FChilds.isEmpty())
    setParentIndex(NULL);
}

void RosterIndex::onDestroyByParentRemoved()
{
  if (!FParentIndex)
    deleteLater();
}

