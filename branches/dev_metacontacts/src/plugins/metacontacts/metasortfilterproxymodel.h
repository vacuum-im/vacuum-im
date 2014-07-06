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
protected:
	bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
	bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
	IMetaContacts *FMetaContacts;
};

#endif // METASORTFILTERPROXYMODEL_H
