#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QObjectCleanupHandler>
#include <interfaces/ioptionsmanager.h>
#include "ui_optionsdialog.h"

class SortFilterProxyModel :
	public QSortFilterProxyModel
{
	Q_OBJECT;
public:
	SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent) {};
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class OptionsDialog :
	public QDialog
{
	Q_OBJECT;
public:
	OptionsDialog(IOptionsManager *AOptionsManager, const QString &ARootId = QString::null, QWidget *AParent = NULL);
	~OptionsDialog();
public:
	void showNode(const QString &ANodeId);
signals:
	void applied();
	void reseted();
protected:
	QWidget *createNodeWidget(const QString &ANodeId);
	QStandardItem *getNodeModelItem(const QString &ANodeId);
	bool canExpandVertically(const QWidget *AWidget) const;
protected slots:
	void onOptionsWidgetModified();
	void onOptionsDialogNodeInserted(const IOptionsDialogNode &ANode);
	void onOptionsDialogNodeRemoved(const IOptionsDialogNode &ANode);
	void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &APrevious);
	void onDialogButtonClicked(QAbstractButton *AButton);
private:
	Ui::OptionsDialogClass ui;
private:
	IOptionsManager *FOptionsManager;
private:
	QStandardItemModel *FItemsModel;
	SortFilterProxyModel *FProxyModel;
private:
	QString FRootNodeId;
	QObjectCleanupHandler FCleanupHandler;
	QMap<QString, QStandardItem *> FNodeItems;
	QMap<QStandardItem *, QWidget *> FItemWidgets;
};

#endif // OPTIONSDIALOG_H
