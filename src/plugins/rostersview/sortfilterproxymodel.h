#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/ipresence.h"

class SortFilterProxyModel : 
  public QSortFilterProxyModel
{
  Q_OBJECT;
public:
  SortFilterProxyModel(IRostersViewPlugin *ARostersViewPlugin, QObject *AParent = NULL);
  ~SortFilterProxyModel();
  bool checkOption(IRostersView::Option AOption) const;
  void setOption(IRostersView::Option AOption, bool AValue);
protected:
  virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
  virtual bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
  int FOptions;
  IRostersViewPlugin *FRostersViewPlugin;
};

#endif // SORTFILTERPROXYMODEL_H
