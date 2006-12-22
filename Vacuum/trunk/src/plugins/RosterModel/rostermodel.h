#ifndef ROSTERMODEL_H
#define ROSTERMODEL_H

#include "interfaces/irostermodel.h"
#include "interfaces/iroster.h"
#include "rosterindex.h"

class RosterModel : 
  public IRosterModel
{
  Q_OBJECT;
  Q_INTERFACES(IRosterModel);
public:
  RosterModel(IRoster *ARoster);
  ~RosterModel();

  //QAbstractItemModel
  virtual QModelIndex index(int ARow, int AColumn, const QModelIndex &AParent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex &AIndex) const;
  virtual bool hasChildren(const QModelIndex &AParent) const;
  virtual int rowCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual int columnCount(const QModelIndex &AParent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex &AIndex, int ARole = Qt::DisplayRole) const;
private:
  IRoster *FRoster;
private:
  RosterIndex *FRootIndex;
};

#endif // ROSTERMODEL_H
