#ifndef IROSTERMODEL_H
#define IROSTERMODEL_H

#include <QVariant>
#include <QAbstractItemModel>
#include <utils/jid.h>
#include <utils/advanceditem.h>
#include <utils/advanceditemmodel.h>

#define ROSTERSMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

class IRosterIndex;
class IRosterDataHolder
{
public:
	virtual QObject *instance() =0;
	virtual QList<int> rosterDataRoles(int AOrder) const =0;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const =0;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole) =0;
protected:
	virtual void rosterDataChanged(IRosterIndex *AIndex, int ARole) =0;
};

class IRosterIndex
{
public:
	virtual AdvancedItem *instance() =0;
	virtual int kind() const =0;
	virtual int row() const =0;
	virtual bool isRemoved() const =0;
	virtual IRosterIndex *parentIndex() const =0;
	virtual int childCount() const =0;
	virtual void appendChild(IRosterIndex *AIndex) =0;
	virtual IRosterIndex *childIndex(int ARow) const =0;
	virtual IRosterIndex *takeIndex(int ARow) =0;
	virtual void removeChild(int ARow) =0;
	virtual void removeChildren() =0;
	virtual void remove(bool ADestroy = true) =0;
	virtual QMap<int,QVariant> indexData() const =0;
	virtual QVariant data(int ARole = Qt::UserRole+1) const =0;
	virtual void setData(const QVariant &AValue, int ARole = Qt::UserRole+1) =0;
	virtual QList<IRosterIndex *> findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive = false) const =0;
public:
	static const int StandardItemTypeValue = QStandardItem::UserType+110;
};

class IRostersModel
{
public:
	enum StreamsLayout {
		LayoutMerged,
		LayoutSeparately
	};
public:
	virtual AdvancedItemModel *instance() =0;
	virtual QList<Jid> streams() const =0;
	virtual IRosterIndex *addStream(const Jid &AStreamJid) =0;
	virtual void removeStream(const Jid &AStreamJid) =0;
	virtual int streamsLayout() const =0;
	virtual void setStreamsLayout(StreamsLayout ALayout) =0;
	virtual IRosterIndex *rootIndex() const =0;
	virtual IRosterIndex *contactsRoot() const =0;
	virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const =0;
	virtual IRosterIndex *streamIndex(const Jid &AStreamJid) const =0;
	virtual IRosterIndex *newRosterIndex(int AKind) =0;
	virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent) =0;
	virtual void removeRosterIndex(IRosterIndex *AIndex, bool ADestroy = true) =0;
	virtual IRosterIndex *findGroupIndex(int AKind, const QString &AGroup, IRosterIndex *AParent) const =0;
	virtual IRosterIndex *getGroupIndex(int AKind, const QString &AGroup, IRosterIndex *AParent) =0;
	virtual QList<IRosterIndex *> findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, IRosterIndex *AParent = NULL) const =0;
	virtual QList<IRosterIndex *> getContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, IRosterIndex *AParent = NULL) =0;
	virtual QModelIndex modelIndexFromRosterIndex(IRosterIndex *AIndex) const =0;
	virtual IRosterIndex *rosterIndexFromModelIndex(const QModelIndex &AIndex) const =0;
	virtual bool isGroupKind(int AKind) const =0;
	virtual QList<int> singleGroupKinds() const =0;
	virtual QString singleGroupName(int AKind) const =0;
	virtual void registerSingleGroup(int AKind, const QString &AName) =0;
	virtual QMultiMap<int, IRosterDataHolder *> rosterDataHolders() const =0;
	virtual void insertRosterDataHolder(int AOrder, IRosterDataHolder *AHolder) =0;
	virtual void removeRosterDataHolder(int AOrder, IRosterDataHolder *AHolder) =0;
protected:
	virtual void streamAdded(const Jid &AStreamJid) =0;
	virtual void streamRemoved(const Jid &AStreamJid) =0;
	virtual void streamJidChanged(const Jid &ABefore, const Jid &AAfter) =0;
	virtual void streamsLayoutAboutToBeChanged(int AAfter) =0;
	virtual void streamsLayoutChanged(int ABefore) =0;
	virtual void indexCreated(IRosterIndex *AIndex) =0;
	virtual void indexInserted(IRosterIndex *AIndex) =0;
	virtual void indexRemoving(IRosterIndex *AIndex) =0;
	virtual void indexDestroyed(IRosterIndex *AIndex) =0;
	virtual void indexDataChanged(IRosterIndex *AIndex, int ARole) =0;
};

Q_DECLARE_INTERFACE(IRosterDataHolder,"Vacuum.Plugin.IRosterDataHolder/1.1");
Q_DECLARE_INTERFACE(IRosterIndex,"Vacuum.Plugin.IRosterIndex/1.3");
Q_DECLARE_INTERFACE(IRostersModel,"Vacuum.Plugin.IRostersModel/1.3");

#endif // IROSTERMODEL_H
