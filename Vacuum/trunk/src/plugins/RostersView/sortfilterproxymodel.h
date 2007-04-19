#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
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
  bool showOffline() const { return FShowOffline; }
  void setShowOffline(bool AShow);
  bool sortByStatus() const { return FSortByStatus; }
  void setSortByStatus(bool ASortByStatus);
protected:
  virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
  bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
  bool FShowOffline;
  bool FSortByStatus;
};

#endif // SORTFILTERPROXYMODEL_H
