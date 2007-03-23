#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  FShowOffline = true;
}

SortFilterProxyModel::~SortFilterProxyModel()
{

}

void SortFilterProxyModel::setSourceModel(QAbstractItemModel *ASourceModel)
{
  if (sourceModel())
  {
    disconnect(sourceModel(),SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      this, SLOT(onSourseDataChanged(const QModelIndex &, const QModelIndex &)));
    disconnect(sourceModel(),SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      this, SLOT(onSourseRowsInserted(const QModelIndex &,int,int)));
    disconnect(sourceModel(),SIGNAL(rowsRemoved(const QModelIndex &,int,int)),
      this, SLOT(onSourseRowsRemoved(const QModelIndex &,int,int)));
  }

  QSortFilterProxyModel::setSourceModel(ASourceModel);
  
  if (sourceModel())
  {
    connect(sourceModel(),SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
      SLOT(onSourseDataChanged(const QModelIndex &, const QModelIndex &)));
    connect(sourceModel(),SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      SLOT(onSourseRowsInserted(const QModelIndex &,int,int)));
    connect(sourceModel(),SIGNAL(rowsRemoved(const QModelIndex &,int,int)),
      SLOT(onSourseRowsRemoved(const QModelIndex &,int,int)));
  }
}

void SortFilterProxyModel::setShowOffline(bool AShow)
{
  if (FShowOffline != AShow)
  {
    FShowOffline = AShow;
    clear();    //filterChanged();
  }
}

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
  int leftType = ALeft.data(IRosterIndex::DR_Type).toInt();
  int rightType = ARight.data(IRosterIndex::DR_Type).toInt();
  if (leftType == rightType)
  {
    int leftShow = ALeft.data(IRosterIndex::DR_Show).toInt();
    int rightShow = ARight.data(IRosterIndex::DR_Show).toInt();
    if (leftShow != rightShow)
    {
      if (leftShow == IPresence::Offline)
        return true;
      else if (rightShow == IPresence::Offline)
        return  false;
      else
        return leftShow > rightShow;
    }
    else
      return QString::localeAwareCompare(ALeft.data().toString(),ARight.data().toString()) >= 0;
  }
  else
    return leftType > rightType;
}

bool SortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
  if (FShowOffline)
    return true;

  QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);

  if (index.isValid())
  {
    int indexType = index.data(IRosterIndex::DR_Type).toInt();
    switch(indexType)
    {
    case IRosterIndex::IT_Contact:
    case IRosterIndex::IT_Agent:
      {
        int indexShow = index.data(IRosterIndex::DR_Show).toInt();
        return indexShow != IPresence::Offline && indexShow != IPresence::Error;
      }
    case IRosterIndex::IT_Group:
    case IRosterIndex::IT_BlankGroup:
    case IRosterIndex::IT_AgentsGroup:
      {
        int childRow = 0;
        QModelIndex childIndex;
        while ((childIndex = index.child(childRow,0)).isValid())
        {
          if (filterAcceptsRow(childRow,index))
            return true;
          childRow++;
        }
        return false;
      }
    default:
      return true;
    }
  }
  return false;
}

void SortFilterProxyModel::onSourseDataChanged(const QModelIndex &/*ATopLeft*/, const QModelIndex &ABottomRight)
{
  if (hasFilteredParent(ABottomRight))
    filterChanged();
}

void SortFilterProxyModel::onSourseRowsInserted(const QModelIndex &AParent, int /*AStart*/, int /*AEnd*/)
{
  if (hasFilteredParent(AParent))
    filterChanged();
}

void SortFilterProxyModel::onSourseRowsRemoved(const QModelIndex &AParent, int /*AStart*/, int /*AEnd*/)
{
  if (hasFilteredParent(AParent))
    filterChanged();
}

bool SortFilterProxyModel::hasFilteredParent(const QModelIndex &/*AModelIndex*/)
{
  return !FShowOffline;
}
