#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <definitions/optionvalues.h>
#include <definitions/rosterindextyperole.h>
#include <interfaces/irostersview.h>
#include <interfaces/ipresence.h>
#include <utils/options.h>

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
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
	virtual bool filterAcceptsRow(int AModelRow, const QModelIndex &AModelParent) const;
private:
	IRostersViewPlugin *FRostersViewPlugin;
private:
	bool FShowOffline;
	bool FSortByStatus;
};

#endif // SORTFILTERPROXYMODEL_H
