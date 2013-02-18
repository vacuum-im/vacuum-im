#include "sortfilterproxymodel.h"

SortFilterProxyModel::SortFilterProxyModel(IRostersViewPlugin *ARostersViewPlugin, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FShowOffline = true;
	FSortByStatus = false;
	FRostersViewPlugin = ARostersViewPlugin;
}

SortFilterProxyModel::~SortFilterProxyModel()
{

}

void SortFilterProxyModel::invalidate()
{
	FShowOffline = Options::node(OPV_ROSTER_SHOWOFFLINE).value().toBool();
	FSortByStatus = Options::node(OPV_ROSTER_SORTBYSTATUS).value().toBool();
	QSortFilterProxyModel::invalidate();
}

bool SortFilterProxyModel::compareVariant( const QVariant &ALeft, const QVariant &ARight ) const
{
	switch (ALeft.userType()) 
	{
	case QVariant::Invalid:
		return (ARight.type() != QVariant::Invalid);
	case QVariant::Int:
		return ALeft.toInt() < ARight.toInt();
	case QVariant::UInt:
		return ALeft.toUInt() < ARight.toUInt();
	case QVariant::LongLong:
		return ALeft.toLongLong() < ARight.toLongLong();
	case QVariant::ULongLong:
		return ALeft.toULongLong() < ARight.toULongLong();
	case QMetaType::Float:
		return ALeft.toFloat() < ARight.toFloat();
	case QVariant::Double:
		return ALeft.toDouble() < ARight.toDouble();
	case QVariant::Char:
		return ALeft.toChar() < ARight.toChar();
	case QVariant::Date:
		return ALeft.toDate() < ARight.toDate();
	case QVariant::Time:
		return ALeft.toTime() < ARight.toTime();
	case QVariant::DateTime:
		return ALeft.toDateTime() < ARight.toDateTime();
	case QVariant::String:
	default:
		if (isSortLocaleAware())
			return ALeft.toString().localeAwareCompare(ARight.toString()) < 0;
		else
			return ALeft.toString().compare(ARight.toString(), sortCaseSensitivity()) < 0;
	}
	return false;
}

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	int leftTypeOrder = ALeft.data(RDR_KIND_ORDER).toInt();
	int rightTypeOrder = ARight.data(RDR_KIND_ORDER).toInt();
	if (leftTypeOrder == rightTypeOrder)
	{
		QVariant leftSortOrder = ALeft.data(RDR_SORT_ORDER);
		QVariant rightSortOrder = ARight.data(RDR_SORT_ORDER);
		if (leftSortOrder.isNull() || rightSortOrder.isNull() || leftSortOrder==rightSortOrder)
		{
			if (FSortByStatus && leftTypeOrder!=RIKO_STREAM_ROOT)
			{
				int leftShow = ALeft.data(RDR_SHOW).toInt();
				int rightShow = ARight.data(RDR_SHOW).toInt();
				if (leftShow != rightShow)
				{
					static const int showOrders[] = {6,2,1,3,4,5,7,8};
					static const int showOrdersCount = sizeof(showOrders)/sizeof(showOrders[0]);
					if (leftShow<showOrdersCount && rightShow<showOrdersCount)
						return showOrders[leftShow] < showOrders[rightShow];
				}
			}
			return compareVariant(ALeft.data(Qt::DisplayRole),ARight.data(Qt::DisplayRole));
		}
		return compareVariant(leftSortOrder,rightSortOrder);
	}
	return leftTypeOrder < rightTypeOrder;
}

bool SortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
	QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);
	if (index.data(RDR_ALLWAYS_INVISIBLE).toInt() > 0)
		return false;
	if (index.data(RDR_ALLWAYS_VISIBLE).toInt() > 0)
		return true;

	if (!FShowOffline)
	{
		if (sourceModel()->hasChildren(index))
		{
			for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
				if (filterAcceptsRow(childRow,index))
					return true;
			return false;
		}
		else if (!index.data(RDR_SHOW).isNull())
		{
			int indexShow = index.data(RDR_SHOW).toInt();
			return indexShow!=IPresence::Offline && indexShow!=IPresence::Error;
		}
	}
	return true;
}
