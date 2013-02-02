#ifndef ADVANCEDITEMMODEL_H
#define ADVANCEDITEMMODEL_H

#include <QStandardItemModel>
#include "advanceditem.h"
#include "utilsexport.h"

class AdvancedItemModel;

class UTILS_EXPORT AdvancedItemSortHandler
{
public:
	enum SortResult {
		Undefined,
		LessThen,
		NotLessThen
	};
	virtual SortResult advancedItemSort(int AOrder, const QStandardItem *ALeft, const QStandardItem *ARight) const;
};

class UTILS_EXPORT AdvancedItemDataHandler
{
public:
	virtual QList<int> advancedItemDataRoles(int AOrder) const =0;
	virtual QVariant advancedItemData(int ARole, int AOrder, const QStandardItem *AItem) const;
	virtual bool setAdvancedItemData(const QVariant &AValue, int ARole, int AOrder, QStandardItem *AItem);
protected:
	void emitItemDataChanged(AdvancedItemModel *AModel, QStandardItem *AItem, int ARole);
};

class UTILS_EXPORT AdvancedItemModel :
	public QStandardItemModel
{
	Q_OBJECT;
public:
	AdvancedItemModel(QObject *AParent = NULL);
	// QStandardItemModel
	QMap<int, QVariant> itemData(const QModelIndex &AIndex) const;
	// AdvancedItemModel
	QList<QStandardItem *> findItems(const QMultiMap<int, QVariant> &AData, const QStandardItem *AParent=NULL, Qt::MatchFlags AFlags=Qt::MatchExactly, int AColumn=0) const;
public:
	// Sort Handlers
	QMultiMap<int, AdvancedItemSortHandler *> itemSortHandlers() const;
	void insertItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler);
	void removeItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler);
	// Data Handlers
	QMultiMap<int, AdvancedItemDataHandler *> itemDataHandlers(int ARole=AnyRole) const;
	void insertItemDataHandler(int AOrder, AdvancedItemDataHandler *AHandler);
	void removeItemDataHandler(int AOrder, AdvancedItemDataHandler *AHandler);
signals:
	void itemDataChanged(QStandardItem *AItem, int ARole);
public:
	static const int AnyRole = -1;
protected:
	void emitItemDataChanged(QStandardItem *AItem, int ARole);
private:
	friend class AdvancedItem;
	friend class AdvancedItemDataHandler;
	QMultiMap<int, AdvancedItemSortHandler *> FItemSortHandlers;
	QMap<int, QMultiMap<int, AdvancedItemDataHandler *> > FItemDataHandlers;
};

#endif // ADVANCEDITEMMODEL_H
