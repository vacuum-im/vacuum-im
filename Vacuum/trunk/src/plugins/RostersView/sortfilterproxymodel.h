#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class SortFilterProxyModel : 
  public QSortFilterProxyModel
{
  Q_OBJECT;

public:
  SortFilterProxyModel(QObject *parent = NULL);
  ~SortFilterProxyModel();

private:
    
};

#endif // SORTFILTERPROXYMODEL_H
