#include "rosterindex.h"

#include <definitions/rosterindexroles.h>

#include "rostersmodel.h"

RosterIndex::RosterIndex(int AKind, RostersModel *AModel)
{
	FModel = AModel;
	setData(AKind,RDR_KIND);
	setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
}

RosterIndex::~RosterIndex()
{
	FModel->emitIndexDestroyed(this);
}

int RosterIndex::type() const
{
	return IRosterIndex::StandardItemTypeValue;
}

int RosterIndex::kind() const
{
	return data(RDR_KIND).toInt();
}

int RosterIndex::row() const
{
	return AdvancedItem::row();
}

bool RosterIndex::isRemoved() const
{
	return AdvancedItem::isRemoved();
}

IRosterIndex *RosterIndex::parentIndex() const
{
	QStandardItem *pitem = AdvancedItem::parent();
	if (pitem == NULL)
		return model()!=NULL ? FModel->rootIndex() : NULL;
	else if (pitem->type() == IRosterIndex::StandardItemTypeValue)
		return static_cast<RosterIndex *>(pitem);
	return NULL;
}

int RosterIndex::childCount() const
{
	return AdvancedItem::rowCount();
}

void RosterIndex::appendChild(IRosterIndex *AIndex)
{
	appendRow(AIndex->instance());
}

IRosterIndex *RosterIndex::childIndex(int ARow) const
{
	return static_cast<RosterIndex *>(AdvancedItem::child(ARow));
}

IRosterIndex *RosterIndex::takeIndex(int ARow)
{
	return static_cast<RosterIndex *>(AdvancedItem::takeRow(ARow).value(0));
}

void RosterIndex::removeChild(int ARow)
{
	AdvancedItem::removeRow(ARow);
}

void RosterIndex::removeChildren()
{
	AdvancedItem::setRowCount(0);
}

void RosterIndex::remove(bool ADestroy)
{
	IRosterIndex *pindex = parentIndex();
	if (pindex)
	{
		if (ADestroy)
			pindex->removeChild(row());
		else
			pindex->takeIndex(row());
	}
}

QMap<int,QVariant> RosterIndex::indexData() const
{
	return AdvancedItem::itemData();
}

QVariant RosterIndex::data(int ARole) const
{
	return AdvancedItem::data(ARole);
}

void RosterIndex::setData(const QVariant &AValue, int ARole)
{
	return AdvancedItem::setData(AValue,ARole);
}

QList<IRosterIndex *> RosterIndex::findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive) const
{
	QList<IRosterIndex *> indexes;
	foreach(QStandardItem *item, AdvancedItem::findChilds(AFindData, ARecursive ? Qt::MatchRecursive : Qt::MatchExactly))
		if (item->type() == IRosterIndex::StandardItemTypeValue)
			indexes.append(static_cast<RosterIndex *>(item));
	return indexes;
}
