#ifndef SHORTCUTOPTIONSWIDGET_H
#define SHORTCUTOPTIONSWIDGET_H

#include <QTimer>
#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/ioptionsmanager.h>
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
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
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
	void setItemRed(QStandardItem *AItem, bool ARed) const;
	void setItemBold(QStandardItem *AItem, bool ABold) const;
protected slots:
	void onDefaultClicked();
	void onClearClicked();
	void onRestoreDefaultsClicked();
	void onShowConflictsTimerTimeout();
	void onModelItemChanged(QStandardItem *AItem);
	void onIndexDoubleClicked(const QModelIndex &AIndex);
private:
	Ui::ShortcutOptionsWidgetClass ui;
private:
	int FBlockChangesCheck;
	QTimer FConflictTimer;
	QStandardItemModel FModel;
	SortFilterProxyModel FSortModel;
	QList<QStandardItem *> FGlobalItems;
	QHash<QString, QStandardItem *> FShortcutItem;
	QMap<QStandardItem *, QKeySequence> FItemKeys;
};

#endif // SHORTCUTOPTIONSWIDGET_H
