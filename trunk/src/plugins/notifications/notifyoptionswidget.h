#ifndef NOTIFYOPTIONSWIDGET_H
#define NOTIFYOPTIONSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_notifyoptionswidget.h"

class SortFilterProxyModel : 
	public QSortFilterProxyModel
{
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class NotifyOptionsWidget : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	NotifyOptionsWidget(INotifications *ANotifications, QWidget *AParent = NULL);
	~NotifyOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void createTreeModel();
	void setItemGray(QStandardItem *AItem, bool AGray) const;
	void setItemBold(QStandardItem *AItem, bool ABold) const;
	void setItemItalic(QStandardItem *AItem, bool AItalic) const;
protected slots:
	void onRestoreDefaultsClicked();
	void onModelItemChanged(QStandardItem *AItem);
private:
	Ui::NotifyOptionsWidgetClass ui;
private:
	INotifications *FNotifications;
private:
	int FBlockChangesCheck;
	QStandardItemModel FModel;
	SortFilterProxyModel FSortModel;
	QMap<QString, QStandardItem *> FTypeItems;
};

#endif // NOTIFYOPTIONSWIDGET_H
