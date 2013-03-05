#ifndef IROSTERMODEL_H
#define IROSTERMODEL_H

#include <QVariant>
#include <QAbstractItemModel>
#include <utils/jid.h>

#define ROSTERSMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

class IRosterIndex;

class IRosterDataHolder
{
public:
	virtual QObject *instance() =0;
	virtual int rosterDataOrder() const =0;
	virtual QList<int> rosterDataRoles() const =0;
	virtual QList<int> rosterDataTypes() const =0;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const =0;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue) =0;
protected:
	virtual void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0) =0;
};

class IRosterIndex
{
public:
	virtual QObject *instance() =0;
	virtual int type() const =0;
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
	virtual void insertDataHolder(IRosterDataHolder *ADataHolder) =0;
	virtual void removeDataHolder(IRosterDataHolder *ADataHolder) =0;
	virtual QVariant data(int ARole) const =0;
	virtual QMap<int, QVariant> data() const =0;
	virtual void setData(int ARole, const QVariant &) =0;
	virtual QList<IRosterIndex *> findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive = false) const =0;
	virtual bool removeOnLastChildRemoved() const =0;
	virtual void setRemoveOnLastChildRemoved(bool ARemove) =0;
	virtual bool removeChildsOnRemoved() const =0;
	virtual void setRemoveChildsOnRemoved(bool ARemove) =0;
	virtual bool destroyOnParentRemoved() const =0;
	virtual void setDestroyOnParentRemoved(bool ADestroy) =0;
protected:
	virtual void dataChanged(IRosterIndex *AIndex, int ARole = 0) =0;
	virtual void dataHolderInserted(IRosterDataHolder *ADataHolder) =0;
	virtual void dataHolderRemoved(IRosterDataHolder *ADataHolder) =0;
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
	virtual IRosterIndex *rootIndex() const =0;
	virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const =0;
	virtual IRosterIndex *createRosterIndex(int AType, IRosterIndex *AParent) =0;
	virtual IRosterIndex *findGroupIndex(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) const =0;
	virtual IRosterIndex *createGroupIndex(int AType, const QString &AGroup, const QString &AGroupDelim, IRosterIndex *AParent) =0;
	virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
	virtual void removeRosterIndex(IRosterIndex *AIndex) =0;
	virtual QList<IRosterIndex *> getContactIndexList(const Jid &AStreamJid, const Jid &AContactJid, bool ACreate = false) =0;
	virtual QModelIndex modelIndexByRosterIndex(IRosterIndex *AIndex) const =0;
	virtual IRosterIndex *rosterIndexByModelIndex(const QModelIndex &AIndex) const =0;
	virtual QString singleGroupName(int AType) const =0;
	virtual void registerSingleGroup(int AType, const QString &AName) =0;
	virtual void insertDefaultDataHolder(IRosterDataHolder *ADataHolder) =0;
	virtual void removeDefaultDataHolder(IRosterDataHolder *ADataHolder) =0;
protected:
	virtual void streamAdded(const Jid &AStreamJid) =0;
	virtual void streamRemoved(const Jid &AStreamJid) =0;
	virtual void streamJidChanged(const Jid &ABefore, const Jid &AAfter) =0;
	virtual void indexCreated(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
	virtual void indexAboutToBeInserted(IRosterIndex *AIndex) =0;
	virtual void indexInserted(IRosterIndex *AIndex) =0;
	virtual void indexDataChanged(IRosterIndex *AIndex, int ARole) =0;
	virtual void indexAboutToBeRemoved(IRosterIndex *AIndex) =0;
	virtual void indexRemoved(IRosterIndex *AIndex) =0;
	virtual void indexDestroyed(IRosterIndex *AIndex) =0;
	virtual void defaultDataHolderInserted(IRosterDataHolder *ADataHolder) =0;
	virtual void defaultDataHolderRemoved(IRosterDataHolder *ADataHolder) =0;
};

Q_DECLARE_INTERFACE(IRosterDataHolder,"Vacuum.Plugin.IRosterDataHolder/1.0");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.1");
Q_DECLARE_INTERFACE(IRostersModel,"Vacuum.Plugin.IRostersModel/1.1");

#endif
