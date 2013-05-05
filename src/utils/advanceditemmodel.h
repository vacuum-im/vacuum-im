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
	friend class AdvancedItem;
	friend class AdvancedItemDataHolder;
public:
	enum AdvancedRole {
		AnyRole = -1,
		FlagsRole = Qt::UserRole-1
	};
public:
	AdvancedItemModel(QObject *AParent = NULL);
	// QStandardItemModel
	QMap<int, QVariant> itemData(const QModelIndex &AIndex) const;
	// AdvancedItemModel
	bool isDelayedDataChangedSignals() const;
	void setDelayedDataChangedSignals(bool AEnabed);
	bool isRecursiveParentDataChangedSignals() const;
	void setRecursiveParentDataChangedSignals(bool AEnabled);
	bool isRemovedItem(const QStandardItem *AItem) const;
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
	void itemRemoving(QStandardItem *AItem);
	void itemDataChanged(QStandardItem *AItem, int ARole);
protected:
	void insertChangedItem(QStandardItem *AItem, int ARole);
	void removeChangedItem(QStandardItem *AItem);
protected:
	void emitItemInserted(QStandardItem *AItem);
	void emitItemRemoving(QStandardItem *AItem);
	void emitItemChanged(QStandardItem *AItem);
	void emitItemDataChanged(QStandardItem *AItem, int ARole);
	void emitRecursiveParentDataChanged(QStandardItem *AParent);
protected slots:
	void onEmitDelayedDataChangedSignals();
	void onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onColumnsInserted(const QModelIndex &AParent, int AStart, int AEnd);
	void onRowsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
	void onColumnsAboutToBeRemoved(const QModelIndex &AParent, int AStart, int AEnd);
	void onRowsOrColumnsRemoved(const QModelIndex &AParent, int AStart, int AEnd);
private:
	bool FDelayedDataChangedSignals;
	bool FRecursiveParentDataChangedSignals;
	QList<const QStandardItem *> FRemovingItems;
	QMultiMap<QStandardItem *, int> FChangedItems;
	QMultiMap<int, AdvancedItemSortHandler *> FItemSortHandlers;
	QMap<int, QMultiMap<int, AdvancedItemDataHolder *> > FItemDataHolders;
};

#endif // ADVANCEDITEMMODEL_H
