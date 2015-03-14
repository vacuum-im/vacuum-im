#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>

class SortFilterProxyModel :
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	SortFilterProxyModel(IRostersViewPlugin *ARostersViewPlugin, QObject *AParent = NULL);
	~SortFilterProxyModel();
public slots:
	void invalidate();
protected:
	bool compareVariant(const QVariant &ALeft, const QVariant &ARight) const;
protected:
	bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
	bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
	IRostersView *FRostersView;
private:
	int FSortMode;
	bool FShowOffline;
};

#endif // SORTFILTERPROXYMODEL_H
