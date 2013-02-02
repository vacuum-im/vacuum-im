#include "advanceditemmodel.h"


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
QVariant AdvancedItemDataHandler::advancedItemData(int ARole, int AOrder, const QStandardItem *AItem) const
{
	Q_UNUSED(ARole); Q_UNUSED(AOrder); Q_UNUSED(AItem);
	return QVariant();
}

bool AdvancedItemDataHandler::setAdvancedItemData(const QVariant &AValue, int ARole, int AOrder, QStandardItem *AItem)
{
	Q_UNUSED(AValue); Q_UNUSED(ARole); Q_UNUSED(AOrder); Q_UNUSED(AItem);
	return false;
}

void AdvancedItemDataHandler::emitItemDataChanged(AdvancedItemModel *AModel, QStandardItem *AItem, int ARole)
{
	 AModel->emitItemDataChanged(AItem,ARole);
}


/******************
 *AdvancedItemModel
 ******************/
AdvancedItemModel::AdvancedItemModel(QObject *AParent) : QStandardItemModel(AParent)
{
}

QMap<int, QVariant> AdvancedItemModel::itemData(const QModelIndex &AIndex) const
{
	QStandardItem *item = itemFromIndex(AIndex);
	if (item && item->type()==AdvancedItem::ItemTypeValue)
	{
		AdvancedItem *advItem = static_cast<AdvancedItem *>(item);
		return advItem->itemData();
	}
	return QStandardItemModel::itemData(AIndex);
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

QMultiMap<int, AdvancedItemDataHandler *> AdvancedItemModel::itemDataHandlers(int ARole) const
{
	return FItemDataHandlers.value(ARole);
}

void AdvancedItemModel::insertItemDataHandler(int AOrder, AdvancedItemDataHandler *AHandler)
{
	if (AHandler)
	{
		foreach(int role, AHandler->advancedItemDataRoles(AOrder))
			FItemDataHandlers[role].insertMulti(AOrder,AHandler);
		FItemDataHandlers[AnyRole].insertMulti(AOrder,AHandler);
	}
}

void AdvancedItemModel::removeItemDataHandler(int AOrder, AdvancedItemDataHandler *AHandler)
{
	if (AHandler)
	{
		foreach(int role, AHandler->advancedItemDataRoles(AOrder))
			FItemDataHandlers[role].remove(AOrder,AHandler);
		FItemDataHandlers[AnyRole].remove(AOrder,AHandler);
	}
}

void AdvancedItemModel::emitItemDataChanged(QStandardItem *AItem, int ARole)
{
	emit itemDataChanged(AItem,ARole);
}
