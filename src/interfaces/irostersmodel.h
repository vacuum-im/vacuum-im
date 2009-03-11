#ifndef IROSTERMODEL_H
#define IROSTERMODEL_H

#include <QHash>
#include <QVariant>
#include <QAbstractItemModel>
#include "../../utils/jid.h"

#define ROSTERSMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

class IRosterIndex;
typedef QList<IRosterIndex *> IRosterIndexList;

class IRosterIndexDataHolder
{
public:
  virtual QObject *instance() =0;
  virtual int order() const =0;
  virtual QList<int> roles() const =0;
  virtual QList<int> types() const =0;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const =0;
  virtual bool setData(IRosterIndex *AIndex, int ARole, const QVariant &AValue) =0;
signals:
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0) =0;
};

class IRosterIndex
{
public:
  virtual QObject *instance() =0;
  virtual int type() const =0;
  virtual QString id() const =0;
  virtual IRosterIndex *parentIndex() const =0;
  virtual void setParentIndex(IRosterIndex *AIndex) =0;
  virtual int row() const =0;
  virtual void appendChild(IRosterIndex *AIndex) =0;
  virtual IRosterIndex *child(int ARow) const =0;
  virtual int childRow(const IRosterIndex *AIndex) const =0;
  virtual int childCount() const =0;
  virtual bool removeChild(IRosterIndex *AIndex) =0;
  virtual void removeAllChilds() =0;
  virtual void setFlags(const Qt::ItemFlags &AFlags) =0;
  virtual Qt::ItemFlags flags() const =0;
  virtual void insertDataHolder(IRosterIndexDataHolder *ADataHolder) =0;
  virtual void removeDataHolder(IRosterIndexDataHolder *ADataHolder) =0;
  virtual QVariant data(int ARole) const =0;
  virtual void setData(int ARole, const QVariant &) =0;
  virtual IRosterIndexList findChild(const QMultiHash<int, QVariant> AData, bool ASearchInChilds = false) const =0;
  virtual bool removeOnLastChildRemoved() const=0;
  virtual void setRemoveOnLastChildRemoved(bool ARemove) =0;
  virtual bool removeChildsOnRemoved() const =0;
  virtual void setRemoveChildsOnRemoved(bool ARemove) =0;
  virtual bool destroyOnParentRemoved() const =0;
  virtual void setDestroyOnParentRemoved(bool ADestroy) =0;
signals:
  virtual void dataChanged(IRosterIndex *AIndex, int ARole = 0) =0;
  virtual void dataHolderInserted(IRosterIndexDataHolder *ADataHolder) =0;
  virtual void dataHolderRemoved(IRosterIndexDataHolder *ADataHolder) =0;
  virtual void childAboutToBeInserted(IRosterIndex *AIndex) =0;
  virtual void childInserted(IRosterIndex *AIndex) =0;
  virtual void childAboutToBeRemoved(IRosterIndex *AIndex) =0;
  virtual void childRemoved(IRosterIndex *AIndex) =0;
  virtual void indexDestroyed(IRosterIndex *AIndex) =0;
};

class IRostersModel
{
public:
  virtual QAbstractItemModel *instance() =0;
  virtual IRosterIndex *addStream(const Jid &AStreamJid) =0;
  virtual QList<Jid> streams() const =0;
  virtual void removeStream(const Jid &AStreamJid) =0;
  virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const =0;
  virtual IRosterIndex *rootIndex() const =0;
  virtual IRosterIndex *createRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) =0;
  virtual IRosterIndex *createGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent) =0;
  virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
  virtual void removeRosterIndex(IRosterIndex *AIndex) =0;
  virtual IRosterIndexList getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false) =0;
  virtual IRosterIndex *findRosterIndex(int AType, const QString &AId, IRosterIndex *AParent) const =0;
  virtual IRosterIndex *findGroup(const QString &AName, const QString &AGroupDelim, int AType, IRosterIndex *AParent) const =0;
  virtual void insertDefaultDataHolder(IRosterIndexDataHolder *ADataHolder) =0;
  virtual void removeDefaultDataHolder(IRosterIndexDataHolder *ADataHolder) =0;
  virtual QModelIndex modelIndexByRosterIndex(IRosterIndex *AIndex) const =0;
  virtual IRosterIndex *rosterIndexByModelIndex(const QModelIndex &AIndex) const =0;
  virtual QString blankGroupName() const =0;
  virtual QString agentsGroupName() const =0;
  virtual QString myResourcesGroupName() const =0;
  virtual QString notInRosterGroupName() const =0;
signals:
  virtual void streamAdded(const Jid &AStreamJid) =0;
  virtual void streamRemoved(const Jid &AStreamJid) =0;
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAfter) =0;
  virtual void indexCreated(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
  virtual void indexInserted(IRosterIndex *AIndex) =0;
  virtual void indexDataChanged(IRosterIndex *AIndex, int ARole) =0;
  virtual void indexRemoved(IRosterIndex *AIndex) =0;
  virtual void indexDestroyed(IRosterIndex *AIndex) =0;
  virtual void defaultDataHolderInserted(IRosterIndexDataHolder *ADataHolder) =0;
  virtual void defaultDataHolderRemoved(IRosterIndexDataHolder *ADataHolder) =0;
};

Q_DECLARE_INTERFACE(IRosterIndexDataHolder,"Vacuum.Plugin.IRosterIndexDataHolder/1.0");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.0");
Q_DECLARE_INTERFACE(IRostersModel,"Vacuum.Plugin.IRostersModel/1.0");

#endif
