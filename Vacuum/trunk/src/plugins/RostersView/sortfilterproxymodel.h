#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/ipresence.h"

class SortFilterProxyModel : 
  public QSortFilterProxyModel
{
  Q_OBJECT;

public:
  SortFilterProxyModel(QObject *parent = NULL);
  ~SortFilterProxyModel();
  virtual void setSourceModel(QAbstractItemModel *ASourceModel);
  bool checkOption(IRostersView::Option AOption) const;
  void setOption(IRostersView::Option AOption, bool AValue);
protected:
  virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
  bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
  int FOptions;
};

#endif // SORTFILTERPROXYMODEL_H
