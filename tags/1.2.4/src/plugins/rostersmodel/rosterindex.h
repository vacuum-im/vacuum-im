#ifndef ROSTERINDEX_H
#define ROSTERINDEX_H

#include <QMap>
#include <QHash>
#include <QMultiHash>
#include <definitions/rosterindextyperole.h>
#include <interfaces/irostersmodel.h>

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
	virtual int type() const;
	virtual IRosterIndex *parentIndex() const;
	virtual void setParentIndex(IRosterIndex *AIndex);
	virtual int row() const;
	virtual void appendChild(IRosterIndex *AIndex);
	virtual IRosterIndex *child(int ARow) const;
	virtual int childRow(const IRosterIndex *AIndex) const;
	virtual int childCount() const;
	virtual bool removeChild(IRosterIndex *AIndex);
	virtual void removeAllChilds();
	virtual Qt::ItemFlags flags() const;
	virtual void setFlags(const Qt::ItemFlags &AFlags);
	virtual void insertDataHolder(IRosterDataHolder *ADataHolder);
	virtual void removeDataHolder(IRosterDataHolder *ADataHolder);
	virtual QVariant data(int ARole) const;
	virtual QMap<int,QVariant> data() const;
	virtual void setData(int ARole, const QVariant &AData);
	virtual QList<IRosterIndex *> findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive = false) const;
	virtual bool removeOnLastChildRemoved() const;
	virtual void setRemoveOnLastChildRemoved(bool ARemove);
	virtual bool removeChildsOnRemoved() const;
	virtual void setRemoveChildsOnRemoved(bool ARemove);
	virtual bool destroyOnParentRemoved() const;
	virtual void setDestroyOnParentRemoved(bool ADestroy);
signals:
	void dataChanged(IRosterIndex *AIndex, int ARole);
	void dataHolderInserted(IRosterDataHolder *ADataHolder);
	void dataHolderRemoved(IRosterDataHolder *ADataHolder);
	void childAboutToBeInserted(IRosterIndex *AIndex);
	void childInserted(IRosterIndex *AIndex);
	void childAboutToBeRemoved(IRosterIndex *AIndex);
	void childRemoved(IRosterIndex *AIndex);
	void indexDestroyed(IRosterIndex *AIndex);
protected slots:
	void onDataHolderChanged(IRosterIndex *AIndex, int ARole);
	void onRemoveByLastChildRemoved();
	void onDestroyByParentRemoved();
private:
	bool FBlokedSetParentIndex;
	bool FRemoveChildsOnRemoved;
	bool FDestroyOnParentRemoved;
	bool FRemoveOnLastChildRemoved;
private:
	Qt::ItemFlags FFlags;
	IRosterIndex *FParentIndex;
	QMap<int, QVariant> FData;
	QList<IRosterIndex *> FChilds;
	QHash<int, QMultiMap<int,IRosterDataHolder *> > FDataHolders;
};

#endif // ROSTERINDEX_H
