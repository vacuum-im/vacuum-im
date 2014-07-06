#include "metasortfilterproxymodel.h"

#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>

MetaSortFilterProxyModel::MetaSortFilterProxyModel(IMetaContacts *AMetaContacts, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FMetaContacts = AMetaContacts;
}

bool MetaSortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	Q_UNUSED(ALeft); Q_UNUSED(ARight);
	return true;
}

bool MetaSortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
	QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);
	if (index.data(RDR_KIND).toInt() == RIK_CONTACT)
		return index.data(RDR_METACONTACT_ID).isNull();
	return true;
}
