#include "dataholder.h"

#include "rosterindex.h"

DataHolder::DataHolder(IRosterDataHolder *ADataHolder, IRostersModel *AModel) : QObject(ADataHolder->instance())
{
	FModel = AModel;
	FDataHolder = ADataHolder;
	connect(ADataHolder->instance(),SIGNAL(rosterDataChanged(IRosterIndex *,int)),SLOT(onRosterDataChanged(IRosterIndex *,int)));
}

DataHolder::~DataHolder()
{

}

QList<int> DataHolder::advancedItemDataRoles(int AOrder) const
{
	return FDataHolder->rosterDataRoles(AOrder);
}

QVariant DataHolder::advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const
{
	if (AItem->type() == IRosterIndex::StandardItemTypeValue)
	{
		const IRosterIndex *index = static_cast<const RosterIndex *>(AItem);
		return FDataHolder->rosterData(AOrder,index,ARole);
	}
	return QVariant();
}

bool DataHolder::setAdvancedItemData(int AOrder, const QVariant &AValue, QStandardItem *AItem, int ARole)
{
	if (AItem->type() == IRosterIndex::StandardItemTypeValue)
	{
		IRosterIndex *index = static_cast<RosterIndex *>(AItem);
		return FDataHolder->setRosterData(AOrder,AValue,index,ARole);
	}
	return false;
}

void DataHolder::onRosterDataChanged(IRosterIndex *AIndex, int ARole)
{
	if (AIndex == NULL)
	{
		foreach(QStandardItem *item, FModel->instance()->findItems(QMultiMap<int, QVariant>(),NULL,Qt::MatchRecursive))
			emitItemDataChanged(item,ARole);
	}
	else
	{
		emitItemDataChanged(AIndex->instance(),ARole);
	}
}
