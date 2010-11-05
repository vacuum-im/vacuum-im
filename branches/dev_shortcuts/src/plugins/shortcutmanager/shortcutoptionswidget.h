#ifndef SHORTCUTOPTIONSWIDGET_H
#define SHORTCUTOPTIONSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/ioptionsmanager.h>
#include <utils/shortcuts.h>
#include "shortcutoptionsdelegate.h"
#include "ui_shortcutoptionswidget.h"

class SortFilterProxyModel : 
	public QSortFilterProxyModel
{
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class ShortcutOptionsWidget : 
	public QWidget,
	public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	ShortcutOptionsWidget(QWidget *AParent);
	~ShortcutOptionsWidget();
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
	QStandardItem *createTreeRow(const QString &AId, QStandardItem *AParent, bool AGroup);
	void setItemBold(QStandardItem *AItem, bool ABold) const;
protected slots:
	void onDefaultClicked();
	void onClearClicked();
	void onRestoreDefaultsClicked();
	void onModelItemChanged(QStandardItem *AItem);
private:
	Ui::ShortcutOptionsWidgetClass ui;
private:
	QStandardItemModel FModel;
	SortFilterProxyModel FSortModel;
	QHash<QString, QStandardItem *> FShortcutItem;
};

#endif // SHORTCUTOPTIONSWIDGET_H
