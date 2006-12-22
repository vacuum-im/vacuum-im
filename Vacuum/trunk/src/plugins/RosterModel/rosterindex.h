#ifndef ROSTERINDEX_H
#define ROSTERINDEX_H

#include <QList>
#include <QHash>
#include "interfaces/irostermodel.h"

class RosterIndex : 
  public QObject,
  public IRosterIndex
{
  Q_OBJECT;
  Q_INTERFACES(IRosterIndex);

public:
  RosterIndex(int AType);
  ~RosterIndex();

  QObject *instance() { return this; }

  //IRosterIndex
  virtual int type() const { return FType; }
  virtual void setParentIndex(IRosterIndex *AIndex);
  virtual IRosterIndex *parentIndex() const { return FParentIndex; } 
  virtual int row() const;
  virtual void appendChild(IRosterIndex *AIndex);
  virtual bool removeChild(IRosterIndex *Aindex, bool ARecurse = false);
  virtual int childCount() const { return FChilds.count(); }
  virtual IRosterIndex *child(int ARow) const { return FChilds.at(ARow); }
  virtual int childRow(const IRosterIndex *AIndex) const; 
  virtual IRosterIndexDataHolder *setDataHolder(int ARole, IRosterIndexDataHolder *ADataHolder);
  virtual bool setData(int ARole, const QVariant &AData);
  virtual QVariant data(int ARole) const;
signals:
  virtual void dataChanged();
protected slots:
  virtual void onChildIndexDestroyed(QObject *AIndex);
  virtual void onDataHolderChanged();
private:
  int FType;
  IRosterIndex *FParentIndex;
  QList<IRosterIndex *> FChilds;
  QHash<int, IRosterIndexDataHolder *> FDataHolders;
  QHash<int, QVariant> FData;
};

#endif // ROSTERINDEX_H
