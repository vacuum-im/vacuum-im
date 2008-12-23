#ifndef ROSTERINDEX_H
#define ROSTERINDEX_H

#include <QHash>
#include <QMultiHash>
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/irostersmodel.h"

class RosterIndex : 
  public QObject,
  public IRosterIndex
{
  Q_OBJECT;
  Q_INTERFACES(IRosterIndex);
public:
  RosterIndex(int AType, const QString &AId);
  ~RosterIndex();
  QObject *instance() { return this; }
  //IRosterIndex
  virtual int type() const { return data(RDR_Type).toInt(); }
  virtual QString id() const { return data(RDR_Id).toString(); }
  virtual IRosterIndex *parentIndex() const { return FParentIndex; } 
  virtual void setParentIndex(IRosterIndex *AIndex);
  virtual int row() const;
  virtual void appendChild(IRosterIndex *AIndex);
  virtual IRosterIndex *child(int ARow) const { return FChilds.value(ARow,0); }
  virtual int childRow(const IRosterIndex *AIndex) const; 
  virtual int childCount() const { return FChilds.count(); }
  virtual bool removeChild(IRosterIndex *AIndex);
  virtual void removeAllChilds();
  virtual Qt::ItemFlags flags() const { return FFlags; }
  virtual void setFlags(const Qt::ItemFlags &AFlags) { FFlags = AFlags; } 
  virtual void insertDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual void removeDataHolder(IRosterIndexDataHolder *ADataHolder);
  virtual QVariant data(int ARole) const;
  virtual void setData(int ARole, const QVariant &AData);
  virtual IRosterIndexList findChild(const QMultiHash<int, QVariant> AData, bool ASearchInChilds = false) const;
  virtual bool removeOnLastChildRemoved() const { return FRemoveOnLastChildRemoved; }
  virtual void setRemoveOnLastChildRemoved(bool ARemove) { FRemoveOnLastChildRemoved = ARemove; }
  virtual bool removeChildsOnRemoved() const { return FRemoveChildsOnRemoved; }
  virtual void setRemoveChildsOnRemoved(bool ARemove) { FRemoveChildsOnRemoved = ARemove; }
  virtual bool destroyOnParentRemoved() const { return FDestroyOnParentRemoved; }
  virtual void setDestroyOnParentRemoved(bool ADestroy) {FDestroyOnParentRemoved = ADestroy; }
signals:
  virtual void dataChanged(IRosterIndex *AIndex, int ARole);
  virtual void dataHolderInserted(IRosterIndexDataHolder *ADataHolder);
  virtual void dataHolderRemoved(IRosterIndexDataHolder *ADataHolder);
  virtual void childAboutToBeInserted(IRosterIndex *AIndex);
  virtual void childInserted(IRosterIndex *AIndex);
  virtual void childAboutToBeRemoved(IRosterIndex *AIndex);
  virtual void childRemoved(IRosterIndex *AIndex);
  virtual void indexDestroyed(IRosterIndex *AIndex);
protected slots:
  void onDataHolderChanged(IRosterIndex *AIndex, int ARole);
  void onRemoveByLastChildRemoved();
  void onDestroyByParentRemoved();
private:
  IRosterIndex *FParentIndex;
  QList<IRosterIndex *> FChilds;
  QHash<int, QMultiMap<int,IRosterIndexDataHolder *> > FDataHolders;
  QHash<int, QVariant> FData;
  Qt::ItemFlags FFlags;
  bool FRemoveOnLastChildRemoved;
  bool FRemoveChildsOnRemoved;
  bool FDestroyOnParentRemoved;
  bool FBlokedSetParentIndex;
};

#endif // ROSTERINDEX_H
