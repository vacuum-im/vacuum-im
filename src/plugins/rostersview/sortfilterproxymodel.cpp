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

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	int leftType = ALeft.data(RDR_TYPE).toInt();
	int rightType = ARight.data(RDR_TYPE).toInt();
	if (leftType == rightType)
	{
		int leftShow = ALeft.data(RDR_SHOW).toInt();
		int rightShow = ARight.data(RDR_SHOW).toInt();
		if (FSortByStatus && leftType!=RIT_STREAM_ROOT && leftShow!=rightShow)
		{
			const static int showOrders[] = {6,2,1,3,4,5,7,8};
			return showOrders[leftShow] < showOrders[rightShow];
		}
		else
			return QSortFilterProxyModel::lessThan(ALeft,ARight);
	}
	else
		return leftType < rightType;
}

bool SortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
	if (FShowOffline)
		return true;

	QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);

	if (index.isValid())
	{
		int indexType = index.data(RDR_TYPE).toInt();
		switch (indexType)
		{
		case RIT_AGENT:
			return true;
		case RIT_CONTACT:
		{
			QList<QVariant> labelFlags = index.data(RDR_LABEL_FLAGS).toList();
			foreach(QVariant flag, labelFlags)
				if ((flag.toInt() & IRostersView::LabelVisible) > 0)
					return true;
			int indexShow = index.data(RDR_SHOW).toInt();
			return indexShow!=IPresence::Offline && indexShow!=IPresence::Error;
		}
		case RIT_GROUP:
		case RIT_GROUP_AGENTS:
		case RIT_GROUP_BLANK:
		case RIT_GROUP_NOT_IN_ROSTER:
		{
			for (int childRow = 0; index.child(childRow,0).isValid(); childRow++)
				if (filterAcceptsRow(childRow,index))
					return true;
			return false;
		}
		}
	}
	return true;
}
