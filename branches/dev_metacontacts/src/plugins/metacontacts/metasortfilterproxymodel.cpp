#include "metasortfilterproxymodel.h"

#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/recentitemtypes.h>

MetaSortFilterProxyModel::MetaSortFilterProxyModel(IMetaContacts *AMetaContacts, QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FHideContacts = true;
	FMetaContacts = AMetaContacts;
}

bool MetaSortFilterProxyModel::isHideContacts() const
{
	return FHideContacts;
}

void MetaSortFilterProxyModel::setHideContacts(bool AHide)
{
	if (FHideContacts != AHide)
	{
		FHideContacts = AHide;
		invalidate();
	}
}

bool MetaSortFilterProxyModel::filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const
{
	if (FHideContacts)
	{
		QModelIndex index = sourceModel()->index(AModelRow,0,AModelParent);
		int indexKind = index.data(RDR_KIND).toInt();
		if (indexKind == RIK_CONTACT)
			return index.data(RDR_METACONTACT_ID).isNull();
		else if (indexKind==RIK_RECENT_ITEM && index.data(RDR_RECENT_TYPE).toString()==REIT_CONTACT)
			return index.data(RDR_METACONTACT_ID).isNull();
	}
	return true;
}
