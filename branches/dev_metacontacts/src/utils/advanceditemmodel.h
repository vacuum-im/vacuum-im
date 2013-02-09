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

class UTILS_EXPORT AdvancedItemDataHolder
{
public:
	virtual QList<int> advancedItemDataRoles(int AOrder) const =0;
	virtual QVariant advancedItemData(int AOrder, const QStandardItem *AItem, int ARole) const;
	virtual bool setAdvancedItemData(int AOrder, const QVariant &AValue, QStandardItem *AItem, int ARole);
protected:
	void emitItemDataChanged(QStandardItem *AItem, int ARole);
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
	bool isDelayedDataChangedSignals() const;
	void setDelayedDataChangedSignals(bool AEnabed);
	bool isRecursiveParentDataChangedSignals() const;
	void setRecursiveParentDataChangedSignals(bool AEnabled);
	QList<QStandardItem *> findItems(const QMultiMap<int, QVariant> &AData, const QStandardItem *AParent=NULL, Qt::MatchFlags AFlags=Qt::MatchExactly, int AColumn=0) const;
public:
	// Sort Handlers
	QMultiMap<int, AdvancedItemSortHandler *> itemSortHandlers() const;
	void insertItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler);
	void removeItemSortHandler(int AOrder, AdvancedItemSortHandler *AHandler);
	// Data Handlers
	QMultiMap<int, AdvancedItemDataHolder *> itemDataHolders(int ARole=AnyRole) const;
	void insertItemDataHolder(int AOrder, AdvancedItemDataHolder *AHandler);
	void removeItemDataHolder(int AOrder, AdvancedItemDataHolder *AHandler);
signals:
	void itemInserted(QStandardItem *AItem);
	void itemRemoved(QStandardItem *AItem);
	void itemDataChanged(QStandardItem *AItem, int ARole);
public:
	static const int AnyRole = -1;
	static const int FlagsRole = Qt::UserRole-1;
protected:
	void emitItemInserted(QStandardItem *AItem);
	void emitItemRemoved(QStandardItem *AItem);
	void emitItemChanged(QStandardItem *AItem);
	void emitItemDataChanged(QStandardItem *AItem, int ARole);
	void emitRecursiveParentDataChanged(QStandardItem *AParent);
protected slots:
	void onEmitDelayedDataChangedSignals();
	void onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onColumnsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
	void onColumnsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
private:
	friend class AdvancedItem;
	friend class AdvancedItemDataHolder;
private:
	bool FDelayedDataChangedSignals;
	bool FRecursiveParentDataChangedSignals;
	QMultiMap<QStandardItem *, int> FChangedItems;
	QMultiMap<int, AdvancedItemSortHandler *> FItemSortHandlers;
	QMap<int, QMultiMap<int, AdvancedItemDataHolder *> > FItemDataHolders;
};

#endif // ADVANCEDITEMMODEL_H
