#include "advanceditemmodel.h"

#include <QTimer>

/************************
 *AdvancedItemSortHandler
 ************************/
AdvancedItemSortHandler::SortResult AdvancedItemSortHandler::advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const
{
	Q_UNUSED(AOrder); Q_UNUSED(ALeft); Q_UNUSED(ARight);
	return Undefined;
}


/************************
 *AdvancedItemDataHandler
 ************************/
QVariant AdvancedItemDataHolder::advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const
{
	Q_UNUSED(ARole); Q_UNUSED(AOrder); Q_UNUSED(AItem);
	return QVariant();
}

bool AdvancedItemDataHolder::setAdvancedItemData(int AOrder, const QVariant &AValue, QStandardItem *AItem, int ARole)
{
	Q_UNUSED(AValue); Q_UNUSED(ARole); Q_UNUSED(AOrder); Q_UNUSED(AItem);
	return false;
}

void AdvancedItemDataHolder::emitItemDataChanged(QStandardItem *AItem, int ARole)
{
	AdvancedItemModel *advModel = AItem!=NULL ? qobject_cast<AdvancedItemModel *>(AItem->model()) : NULL;
	if (advModel)
		advModel->emitItemDataChanged(AItem,ARole);
}


/******************
 *AdvancedItemModel
 ******************/
AdvancedItemModel::AdvancedItemModel(QObject *AParent) : QStandardItemModel(AParent)
{
	FDelayedDataChangedSignals = false;
	FRecursiveParentDataChangedSignals = false;

	connect(this,SIGNAL(rowsInserted(const QModelIndex &, int, int)),SLOT(onRowsInserted(const QModelIndex &, int, int)));
	connect(this,SIGNAL(columnsInserted(const QModelIndex &, int, int)),SLOT(onColumnsInserted(const QModelIndex &, int, int)));
	connect(this,SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),SLOT(onRowsAboutToBeRemoved(const QModelIndex &, int, int)));
	connect(this,SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),SLOT(onColumnsAboutToBeRemoved(const QModelIndex &, int, int)));
}

QMap<int, QVariant> AdvancedItemModel::itemData(const QModelIndex &AIndex) const
{
	QStandardItem *item = itemFromIndex(AIndex);
	if (item && item->type()==AdvancedItem::StandardItemTypeValue)
	{
		AdvancedItem *advItem = static_cast<AdvancedItem *>(item);
		return advItem->itemData();
	}
	return QStandardItemModel::itemData(AIndex);
}

bool AdvancedItemModel::isDelayedDataChangedSignals() const
{
	return FDelayedDataChangedSignals;
}

void AdvancedItemModel::setDelayedDataChangedSignals( bool AEnabled )
{
	FDelayedDataChangedSignals = AEnabled;
}

bool AdvancedItemModel::isRecursiveParentDataChangedSignals() const
{
	return FRecursiveParentDataChangedSignals;
}

void AdvancedItemModel::setRecursiveParentDataChangedSignals(bool AEnabled)
{
	FRecursiveParentDataChangedSignals = AEnabled;
}

QList<QStandardItem *> AdvancedItemModel::findItems(const QMultiMap<int, QVariant> &AData, const QStandardItem *AParent, Qt::MatchFlags AFlags, int AColumn) const
{
	QList<QStandardItem *> items;

	uint matchType = AFlags & 0x0F;
	bool recursive = AFlags & Qt::MatchRecursive;
	Qt::CaseSensitivity cs = AFlags & Qt::MatchCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

	int row=0;
	QStandardItem *rowItem=NULL;
	const QStandardItem *parentItem = AParent==NULL ? invisibleRootItem() : AParent;
	while (rowItem = parentItem->child(row++,AColumn))
	{
		int lastRole = -1;
		bool accepted = true;
		for (QMultiMap<int, QVariant>::const_iterator findIt=AData.constBegin(); findIt!=AData.constEnd(); ++findIt)
		{
			int findRole = findIt.key();
			if ((accepted && lastRole!=findRole) || (!accepted && lastRole==findRole))
			{
				QVariant findValue = findIt.value();
				QVariant itemValue = rowItem->data(findRole);
				switch(matchType)
				{
				case Qt::MatchExactly:
					accepted = (findValue==itemValue);
					break;
				case Qt::MatchRegExp:
					accepted = QRegExp(findValue.toString(),cs).exactMatch(itemValue.toString());
					break;
				case Qt::MatchWildcard:
					accepted = QRegExp(findValue.toString(), cs, QRegExp::Wildcard).exactMatch(itemValue.toString());
					break;
				case Qt::MatchStartsWith:
					accepted = itemValue.toString().startsWith(findValue.toString(), cs);
					break;
				case Qt::MatchEndsWith:
					accepted = itemValue.toString().endsWith(findValue.toString(), cs);
					break;
				case Qt::MatchFixedString:
					accepted = (itemValue.toString().compare(findValue.toString(), cs)==0);
					break;
				case Qt::MatchContains:
				default:
					accepted = itemValue.toString().contains(findValue.toString(), cs);
				}
			}
			else if (!accepted)
			{
				break;
			}
			lastRole = findRole;
		}
		if (accepted)
			items.append(rowItem);
		if (recursive && rowItem->hasChildren())
			items += findItems(AData,rowItem,AFlags,AColumn);
	}

	return items;
}

QMultiMap<int, AdvancedItemSortHandler *> AdvancedItemModel::itemSortHandlers() const
{
	return FItemSortHandlers;
}

void AdvancedItemModel::insertItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler)
{
	if (AHandler)
		FItemSortHandlers.insertMulti(AOrder,AHandler);
}

void AdvancedItemModel::removeItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler)
{
	FItemSortHandlers.remove(AOrder,AHandler);
}

QMultiMap<int, AdvancedItemDataHolder *> AdvancedItemModel::itemDataHolders(int ARole) const
{
	return FItemDataHolders.value(ARole);
}

void AdvancedItemModel::insertItemDataHolder(int AOrder, AdvancedItemDataHolder *AHolder)
{
	if (AHolder)
	{
		foreach(int role, AHolder->advancedItemDataRoles(AOrder))
			FItemDataHolders[role].insertMulti(AOrder,AHolder);
		FItemDataHolders[AnyRole].insertMulti(AOrder,AHolder);
	}
}

void AdvancedItemModel::removeItemDataHolder(int AOrder, AdvancedItemDataHolder *AHolder)
{
	if (AHolder)
	{
		foreach(int role, AHolder->advancedItemDataRoles(AOrder))
			FItemDataHolders[role].remove(AOrder,AHolder);
		FItemDataHolders[AnyRole].remove(AOrder,AHolder);
	}
}

void AdvancedItemModel::emitItemInserted(QStandardItem *AItem)
{
	if (AItem)
	{
		for (int row=0; row<AItem->rowCount(); row++)
			for (int col=0; col<AItem->columnCount(); col++)
				emitItemInserted(AItem->child(row,col));
		emit itemInserted(AItem);
	}
}

void AdvancedItemModel::emitItemRemoved(QStandardItem *AItem)
{
	if (AItem)
	{
		for (int row=0; row<AItem->rowCount(); row++)
			for (int col=0; col<AItem->columnCount(); col++)
				emitItemRemoved(AItem->child(row,col));
		emit itemRemoved(AItem);
	}
}

void AdvancedItemModel::emitItemChanged(QStandardItem *AItem)
{
	if (AItem->parent()==NULL && AItem->model()->item(AItem->row(),AItem->column())!=AItem)
	{
		if (horizontalHeaderItem(AItem->column()) == AItem)
			emit headerDataChanged(Qt::Horizontal,AItem->column(),AItem->column());
		else if (verticalHeaderItem(AItem->row()) == AItem)
			emit headerDataChanged(Qt::Vertical,AItem->row(),AItem->row());
	}
	else
	{
		QModelIndex index = indexFromItem(AItem);
		emit dataChanged(index,index);
	}
}

void AdvancedItemModel::emitItemDataChanged(QStandardItem *AItem, int ARole)
{
	if (FDelayedDataChangedSignals)
	{
		if (FChangedItems.isEmpty())
			QTimer::singleShot(0,this,SLOT(onEmitDelayedDataChangedSignals()));
		FChangedItems.insertMulti(AItem,ARole);
	}
	else
	{
		if (ARole > AnyRole)
			emit itemDataChanged(AItem,ARole);
		emitItemChanged(AItem);
	}
	emitRecursiveParentDataChanged(AItem->parent());
}

void AdvancedItemModel::emitRecursiveParentDataChanged(QStandardItem *AParent)
{
	if (FRecursiveParentDataChangedSignals)
	{
		for(QStandardItem *parentItem=AParent; parentItem!=NULL; parentItem = parentItem->parent())
		{
			if (FDelayedDataChangedSignals)
			{
				if (FChangedItems.isEmpty())
					QTimer::singleShot(0,this,SLOT(onEmitDelayedDataChangedSignals()));
				FChangedItems.insertMulti(parentItem,AnyRole);
			}
			else
			{
				emitItemChanged(parentItem);
			}
		}
	}
}

void AdvancedItemModel::onEmitDelayedDataChangedSignals()
{
	QStandardItem *lastItem = NULL;
	for (QMultiMap<QStandardItem *, int>::const_iterator it=FChangedItems.constBegin(); it!=FChangedItems.constEnd(); ++it)
	{
		int role = it.value();
		QStandardItem *item = it.key();

		if (lastItem != item)
			emitItemChanged(item);

		if (role > AnyRole)
			emit itemDataChanged(item,role);
		
		lastItem = item;
	}
	FChangedItems.clear();
}

void AdvancedItemModel::onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
	int colums = columnCount(AParent);
	for (int row=AStart; row<=AEnd; row++)
		for (int col=0; col<colums; col++)
			emitItemInserted(itemFromIndex(index(row,col,AParent)));
	emitRecursiveParentDataChanged(itemFromIndex(AParent));
}

void AdvancedItemModel::onColumnsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
	int rows = rowCount(AParent);
	for (int col=AStart; col<=AEnd; col++)
		for(int row=0; row<rows; row++)
			emitItemInserted(itemFromIndex(index(row,col,AParent)));
	emitRecursiveParentDataChanged(itemFromIndex(AParent));
}

void AdvancedItemModel::onRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd)
{
	int colums = columnCount(AParent);
	for (int row=AStart; row<=AEnd; row++)
	{
		for (int col=0; col<colums; col++)
		{
			QStandardItem *item = itemFromIndex(index(row,col,AParent));
			if (item)
			{
				FChangedItems.remove(item);
				emitItemRemoved(item);
			}
		}
	}
	emitRecursiveParentDataChanged(itemFromIndex(AParent));
}

void AdvancedItemModel::onColumnsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd)
{
	int rows = rowCount(AParent);
	for (int col=AStart; col<=AEnd; col++)
	{
		for(int row=0; row<rows; row++)
		{
			QStandardItem *item = itemFromIndex(index(row,col,AParent));
			if (item)
			{
				FChangedItems.remove(item);
				emitItemRemoved(item);
			}
		}
	}
	emitRecursiveParentDataChanged(itemFromIndex(AParent));
}
