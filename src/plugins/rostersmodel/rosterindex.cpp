#include "rosterindex.h"

#include <definitions/rosterindexroles.h>

#include "rostersmodel.h"

RosterIndex::RosterIndex(int AKind, RostersModel *AModel)
{
	FKind = AKind;
	FModel = AModel;
	AdvancedItem::setData(AKind,RDR_KIND);
	AdvancedItem::setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsEditable);
}

RosterIndex::~RosterIndex()
{
	FModel->emitIndexDestroyed(this);
}

int RosterIndex::type() const
{
	return IRosterIndex::RosterItemTypeValue;
}

int RosterIndex::kind() const
{
	return FKind;
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
	QStandardItem *pItem = AdvancedItem::parent();
	if (pItem == NULL)
		return model()!=NULL ? FModel->rootIndex() : NULL;
	else if (pItem->type() == IRosterIndex::RosterItemTypeValue)
		return static_cast<RosterIndex *>(pItem);
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
	IRosterIndex *pIndex = parentIndex();
	if (pIndex)
	{
		if (ADestroy)
			pIndex->removeChild(row());
		else
			pIndex->takeIndex(row());
	}
}

QMap<int,QVariant> RosterIndex::indexData() const
{
	return AdvancedItem::itemData();
}

QVariant RosterIndex::data(int ARole) const
{
	return ARole!=RDR_KIND ? AdvancedItem::data(ARole) : FKind;
}

void RosterIndex::setData(const QVariant &AValue, int ARole)
{
	if (ARole != RDR_KIND)
		AdvancedItem::setData(AValue,ARole);
}

QList<IRosterIndex *> RosterIndex::findChilds(const QMultiMap<int, QVariant> &AFindData, bool ARecursive) const
{
	QList<IRosterIndex *> indexes;
	foreach(QStandardItem *item, AdvancedItem::findChilds(AFindData, ARecursive ? Qt::MatchRecursive : Qt::MatchExactly))
		if (item->type() == IRosterIndex::RosterItemTypeValue)
			indexes.append(static_cast<RosterIndex *>(item));
	return indexes;
}
