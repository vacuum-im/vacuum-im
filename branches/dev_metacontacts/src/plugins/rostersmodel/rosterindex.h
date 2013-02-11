#ifndef ROSTERINDEX_H
#define ROSTERINDEX_H

#include <interfaces/irostersmodel.h>

class RostersModel;

class RosterIndex :
	public AdvancedItem,
	public IRosterIndex
{
	Q_INTERFACES(IRosterIndex);
public:
	RosterIndex(int AKind, RostersModel *AModel);
	~RosterIndex();
	// AdvancedItem
	virtual int type() const;
	// IRosterIndex
	virtual AdvancedItem *instance() { return this; }
	virtual int kind() const;
	virtual int row() const;
	virtual bool isRemoved() const;
	virtual IRosterIndex *parentIndex() const;
	virtual int childCount() const;
	virtual void appendChild(IRosterIndex *AIndex);
	virtual IRosterIndex *childIndex(int ARow) const;
	virtual IRosterIndex *takeIndex(int ARow);
	virtual void removeChild(int ARow);
	virtual void removeChildren();
	virtual void remove(bool ADestroy = true);
	virtual QMap<int,QVariant> indexData() const;
	virtual QVariant data(int ARole = Qt::UserRole+1) const;
	virtual void setData(const QVariant &AValue, int ARole = Qt::UserRole+1);
	virtual QList<IRosterIndex *> findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive = false) const;
private:
	RostersModel *FModel;
};

#endif // ROSTERINDEX_H
