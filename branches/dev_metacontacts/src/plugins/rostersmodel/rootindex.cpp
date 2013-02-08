#include "rootindex.h"

#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include "rosterindex.h"

RootIndex::RootIndex(AdvancedItemModel *AModel)
{
	FModel = AModel;
}

RootIndex::~RootIndex()
{

}

int RootIndex::kind() const
{
	return RIK_ROOT;
}

int RootIndex::row() const
{
	return -1;
}

IRosterIndex *RootIndex::parentIndex() const
{
	return NULL;
}

int RootIndex::childCount() const
{
	return FModel->rowCount();
}

void RootIndex::appendChild(IRosterIndex *AIndex)
{
	FModel->appendRow(AIndex->instance());
}

IRosterIndex *RootIndex::childIndex(int ARow) const
{
	return static_cast<RosterIndex *>(FModel->item(ARow));
}

IRosterIndex *RootIndex::takeIndex(int ARow)
{
	return static_cast<RosterIndex *>(FModel->takeRow(ARow).value(0));
}

void RootIndex::removeChild(int ARow)
{
	FModel->removeRow(ARow);
}

void RootIndex::removeChildren()
{
	FModel->clear();
}

void RootIndex::remove(bool ADestroy)
{
	Q_UNUSED(ADestroy);
}

QMap<int,QVariant> RootIndex::indexData() const
{
	static const QMap<int,QVariant> value;
	return value;
}

QVariant RootIndex::data(int ARole) const
{
	static const QVariant value;
	return ARole==RDR_KIND ? QVariant(RIK_ROOT) : value;
}

void RootIndex::setData(const QVariant &AValue, int ARole)
{
	Q_UNUSED(AValue); Q_UNUSED(ARole);
}

QList<IRosterIndex *> RootIndex::findChilds( const QMultiMap<int, QVariant> &AFindData, bool ARecursive) const
{
	QList<IRosterIndex *> indexes;
	foreach(QStandardItem *item, FModel->findItems(AFindData, NULL, ARecursive ? Qt::MatchRecursive : Qt::MatchExactly))
		indexes.append(static_cast<RosterIndex *>(item));
	return indexes;
}
