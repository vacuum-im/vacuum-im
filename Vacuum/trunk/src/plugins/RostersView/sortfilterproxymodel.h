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
protected:
  virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
  bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
  bool hasFilteredParent(const QModelIndex &AModelIndex);
protected slots:
  void onSourseDataChanged(const QModelIndex &ATopLeft, const QModelIndex &ABottomRight);
  void onSourseRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onSourseRowsRemoved(const QModelIndex &AParent, int AStart, int AEnd);
private:
  bool FShowOffline;    
};

#endif // SORTFILTERPROXYMODEL_H
