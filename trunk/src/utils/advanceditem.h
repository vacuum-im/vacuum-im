#ifndef ADVANCEDITEM_H
#define ADVANCEDITEM_H

#include <QStandardItem>
#include "utilsexport.h"

class UTILS_EXPORT AdvancedItem :
	public QStandardItem
{
public:
	AdvancedItem();
	AdvancedItem(const QString &AText);
	AdvancedItem(const QIcon &AIcon, const QString &AText);
	// QStandardItem
	int type() const;
	QStandardItem *clone() const;
	void read(QDataStream &AIn);
	void write(QDataStream &AOut) const;
	QVariant data(int ARole = Qt::UserRole+1) const;
	void setData(const QVariant &AValue, int ARole = Qt::UserRole+1);
	bool operator<(const QStandardItem &AOther) const;
	// AdvancedItem
	bool isRemoved() const;
	virtual QMap<int, QVariant> itemData() const;
	virtual void setItemData(const QMap<int, QVariant> &AData);
	virtual QList<QStandardItem *> findChilds(const QMultiMap<int, QVariant> &AData, Qt::MatchFlags AFlags=Qt::MatchExactly, int AColumn=0) const;
public:
	static const int StandardItemTypeValue = QStandardItem::UserType+100;
private:
	QMap<int, QVariant> FData;
};

#endif // ADVANCEDITEM_H
