#ifndef METASORTFILTERPROXYMODEL_H
#define METASORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <interfaces/imetacontacts.h>

class MetaSortFilterProxyModel : 
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	MetaSortFilterProxyModel(IMetaContacts *AMetaContacts, QObject *AParent);
	bool isHideContacts() const;
	void setHideContacts(bool AHide);
protected:
	bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
	IMetaContacts *FMetaContacts;
private:
	bool FHideContacts;
};

#endif // METASORTFILTERPROXYMODEL_H
