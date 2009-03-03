#include "rosterindex.h"

#include <QTimer>

RosterIndex::RosterIndex(int AType, const QString &AId)
{
  FParentIndex = NULL;
  FData.insert(RDR_TYPE,AType);
  FData.insert(RDR_INDEX_ID,AId);
  FFlags = (Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  FRemoveOnLastChildRemoved = true;
  FRemoveChildsOnRemoved = true;
  FDestroyOnParentRemoved = true;
  FBlokedSetParentIndex = false;
}

RosterIndex::~RosterIndex()
{
  setParentIndex(NULL);
  emit indexDestroyed(this);
}

void RosterIndex::setParentIndex(IRosterIndex *AIndex)
{
  if (FBlokedSetParentIndex || FParentIndex == AIndex)
    return;

  FBlokedSetParentIndex = true;

  if (FParentIndex)
  {
    FParentIndex->removeChild(this);
    setParent(NULL);
  }

  if (AIndex)
  {
    FParentIndex = AIndex;
    FParentIndex->appendChild(this); 
    setParent(FParentIndex->instance());
  } 
  else
  {
    FParentIndex = NULL;
    if (FDestroyOnParentRemoved)
      QTimer::singleShot(0,this,SLOT(onDestroyByParentRemoved()));
  }

  FBlokedSetParentIndex = false;
}

int RosterIndex::row() const
{
  return FParentIndex ? FParentIndex->childRow(this) : -1;
}

void RosterIndex::appendChild(IRosterIndex *AIndex)
{
  if (AIndex && !FChilds.contains(AIndex))
  {
    FChilds.append(AIndex); 
    AIndex->setParentIndex(this);
    emit childAboutToBeInserted(AIndex);
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
    if (AIndex->removeChildsOnRemoved())
      AIndex->removeAllChilds();

    emit childAboutToBeRemoved(AIndex);
    FChilds.removeAt(FChilds.indexOf(AIndex));
    AIndex->setParentIndex(NULL);
    emit childRemoved(AIndex);

    if (FRemoveOnLastChildRemoved && FChilds.isEmpty())
      QTimer::singleShot(0,this,SLOT(onRemoveByLastChildRemoved()));

    return true;
  }
  return false;
}

void RosterIndex::removeAllChilds()
{
  while (FChilds.count()>0)
    removeChild(FChilds.value(0));
}

void RosterIndex::insertDataHolder(IRosterIndexDataHolder *ADataHolder)
{
  connect(ADataHolder->instance(),SIGNAL(dataChanged(IRosterIndex *, int)),
    SLOT(onDataHolderChanged(IRosterIndex *, int)));

  foreach(int role, ADataHolder->roles())
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
  
  foreach(int role, ADataHolder->roles())
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
  foreach (IRosterIndex *index, FChilds)
  {
    bool cheked = true;
    QList<int> dataRoles = AData.keys();
    for (int i=0; cheked && i<dataRoles.count(); i++)
      cheked = AData.values(dataRoles.at(i)).contains(index->data(dataRoles.at(i)));
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

