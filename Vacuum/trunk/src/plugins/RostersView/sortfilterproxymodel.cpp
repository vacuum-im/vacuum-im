#include <QtDebug>
#include "sortfilterproxymodel.h"

#include <QTimer>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent)
  : QSortFilterProxyModel(parent)
{
  FOptions = 0;
}

SortFilterProxyModel::~SortFilterProxyModel()
{

}

void SortFilterProxyModel::setSourceModel(QAbstractItemModel *ASourceModel)
{
  QSortFilterProxyModel::setSourceModel(ASourceModel);
}

bool SortFilterProxyModel::checkOption(IRostersView::Option AOption) const
{
  return (FOptions & AOption) == AOption;
}

void SortFilterProxyModel::setOption(IRostersView::Option AOption, bool AValue)
{
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
  filterChanged();
  clear();
}

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
  int leftType = ALeft.data(IRosterIndex::DR_Type).toInt();
  int rightType = ARight.data(IRosterIndex::DR_Type).toInt();
  if (leftType == rightType)
  {
    int leftShow = ALeft.data(IRosterIndex::DR_Show).toInt();
    int rightShow = ARight.data(IRosterIndex::DR_Show).toInt();
    bool showOnlineFirst = checkOption(IRostersView::ShowOnlineFirst);
    if (showOnlineFirst && leftType != IRosterIndex::IT_StreamRoot && leftShow != rightShow)
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
  if (checkOption(IRostersView::ShowOfflineContacts))
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

