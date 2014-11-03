#include "advanceditem.h"

#include <QSet>
#include "advanceditemmodel.h"

#define FIRST_VALID_ROLE 0

static const qint32 DefaultItemFlags = Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsEditable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled;

AdvancedItem::AdvancedItem() : QStandardItem()
{
	FData.insert(AdvancedItemModel::FlagsRole,DefaultItemFlags);
}

AdvancedItem::AdvancedItem(const QString &AText) : QStandardItem(AText)
{
	FData.insert(AdvancedItemModel::FlagsRole,DefaultItemFlags);
}

AdvancedItem::AdvancedItem(const QIcon &AIcon, const QString &AText) : QStandardItem(AIcon,AText)
{
	FData.insert(AdvancedItemModel::FlagsRole,DefaultItemFlags);
}

int AdvancedItem::type() const
{
	return AdvancedItemTypeValue;
}

QStandardItem *AdvancedItem::clone() const
{
	AdvancedItem *item = new AdvancedItem;
	item->FData = FData;
	return item;
}

void AdvancedItem::read(QDataStream &AIn)
{
	qint32 flags;
	AIn >> FData;
	AIn >> flags;
	setFlags(Qt::ItemFlags(flags));
}

void AdvancedItem::write(QDataStream &AOut) const
{
	AOut << FData;
	AOut << flags();
}

QVariant AdvancedItem::data(int ARole) const
{
	QVariant value;
	if (ARole >= FIRST_VALID_ROLE)
	{
		const AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
		if (advModel)
		{
			const QMultiMap<int, AdvancedItemDataHolder *> holders = advModel->itemDataHolders(ARole);
			for(QMultiMap<int, AdvancedItemDataHolder *>::const_iterator it=holders.constBegin(); value.isNull() && it!=holders.constEnd(); ++it)
				value = it.value()->advancedItemData(it.key(),this,ARole);
		}
		return value.isNull() ? FData.value(ARole) : value;
	}
	return value;
}

void AdvancedItem::setData(const QVariant &AValue, int ARole)
{
	if (ARole >= FIRST_VALID_ROLE)
	{
		AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
		if (advModel)
		{
			const QMultiMap<int, AdvancedItemDataHolder *> holders = advModel->itemDataHolders(ARole);
			for(QMultiMap<int, AdvancedItemDataHolder *>::const_iterator it=holders.constBegin(); it!=holders.constEnd(); ++it)
				if (it.value()->setAdvancedItemData(it.key(),AValue,this,ARole))
					return;
		}

		bool changed = false;
		QMap<int, QVariant>::iterator valueIt = FData.find(ARole);
		if (!AValue.isNull())
		{
			if (valueIt == FData.end())
			{
				changed = true;
				FData.insert(ARole,AValue);
			}
			else if (AValue != valueIt.value())
			{
				changed = true;
				*valueIt = AValue;
			}
		}
		else if (valueIt != FData.end())
		{
			changed = true;
			FData.erase(valueIt);
		}

		if (changed && advModel)
			advModel->emitItemDataChanged(this,ARole);
	}
}

bool AdvancedItem::operator <(const QStandardItem &AOther) const
{
	const AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
	if (advModel)
	{
		const QMultiMap<int, AdvancedItemSortHandler *> handlers = advModel->itemSortHandlers();
		for(QMultiMap<int, AdvancedItemSortHandler *>::const_iterator it=handlers.constBegin(); it!=handlers.constEnd(); ++it)
		{
			AdvancedItemSortHandler::SortResult res = it.value()->advancedItemSort(it.key(),this,&AOther);
			if (res != AdvancedItemSortHandler::Undefined)
				return res==AdvancedItemSortHandler::LessThen;
		}
	}
	return QStandardItem::operator<(AOther);
}

bool AdvancedItem::isRemoved() const
{
	const AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
	if (advModel)
		return advModel->isRemovedItem(this);
	return false;
}

QMap<int, QVariant> AdvancedItem::itemData() const
{
	QMap<int, QVariant> values = FData;
	const AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
	if (advModel)
	{
		QMultiMap<int, AdvancedItemDataHolder *> allHolders = advModel->itemDataHolders(AdvancedItemModel::AllRoles);
		const QMultiMap<int, AdvancedItemDataHolder *> anyHolders = advModel->itemDataHolders(AdvancedItemModel::AnyRole);

		QSet<int> presentRoles;
		for(QMultiMap<int, AdvancedItemDataHolder *>::const_iterator anyIt=anyHolders.constBegin(); anyIt!=anyHolders.constEnd(); ++anyIt)
		{
			int anyOrder = anyIt.key();
			AdvancedItemDataHolder *anyHolder = anyIt.value();
			foreach(int role, anyHolder->advancedItemDataRoles(anyOrder))
			{
				if (role>=FIRST_VALID_ROLE && !presentRoles.contains(role))
				{
					QMultiMap<int, AdvancedItemDataHolder *>::iterator tmpIt = allHolders.insert(anyOrder,anyHolder);
					for(QMultiMap<int, AdvancedItemDataHolder *>::const_iterator allIt=allHolders.constBegin(); allIt!=allHolders.constEnd(); ++allIt)
					{
						QVariant value = allIt.value()->advancedItemData(allIt.key(),this,role);
						if (!value.isNull())
						{
							values.insert(role,value);
							presentRoles +=  role;
							break;
						}
					}
					allHolders.erase(tmpIt);
				}
			}
		}
	}
	return values;
}

void AdvancedItem::setItemData(const QMap<int, QVariant> &AData)
{
	for (QMap<int, QVariant>::const_iterator it=AData.constBegin(); it!=AData.constEnd(); ++it)
		setData(it.value(),it.key());
}

QList<QStandardItem *> AdvancedItem::findChilds(const QMultiMap<int, QVariant> &AData, Qt::MatchFlags AFlags, int AColumn) const
{
	const AdvancedItemModel *advModel = qobject_cast<AdvancedItemModel *>(model());
	return advModel!=NULL ? advModel->findItems(AData,this,AFlags,AColumn) : QList<QStandardItem *>();
}
