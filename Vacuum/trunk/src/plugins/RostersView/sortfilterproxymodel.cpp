#include <QtDebug>
#include "sortfilterproxymodel.h"

#include <QTimer>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  FShowOffline = true;
  FSortByStatus = true;
}

SortFilterProxyModel::~SortFilterProxyModel()
{

}

void SortFilterProxyModel::setSourceModel(QAbstractItemModel *ASourceModel)
{
  QSortFilterProxyModel::setSourceModel(ASourceModel);
}

void SortFilterProxyModel::setShowOffline(bool AShow)
{
  if (FShowOffline != AShow)
  {
    FShowOffline = AShow;
    filterChanged();
    clear();
  }
}

void SortFilterProxyModel::setSortByStatus(bool ASortByStatus)
{
  if (FSortByStatus != ASortByStatus)
  {
    FSortByStatus = ASortByStatus;
    clear();
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
    if (FSortByStatus && leftType != IRosterIndex::IT_StreamRoot && leftShow != rightShow)
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
    case IRosterIndex::IT_AgentsGroup:
    case IRosterIndex::IT_BlankGroup:
    case IRosterIndex::IT_NotInRosterGroup:
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

