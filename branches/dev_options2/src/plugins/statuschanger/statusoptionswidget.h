#ifndef STATUSOPTIONSWIDGET_H
#define STATUSOPTIONSWIDGET_H

#include <QPushButton>
#include <QTableWidget>
#include <QStyledItemDelegate>
#include <interfaces/ipresencemanager.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>

struct RowData {
	int show;
	QString name;
	QString text;
	int priority;
};

class StatusDelegate :
	public QStyledItemDelegate
{
	Q_OBJECT;
public:
	StatusDelegate(IStatusChanger *AStatusChanger, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
	IStatusChanger *FStatusChanger;
};

class StatusOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	StatusOptionsWidget(IStatusChanger *AStatusChanger, QWidget *AParent);
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onAddButtonClicked();
	void onDeleteButtonClicked();
	void onStatusItemSelectionChanged();
private:
	IStatusChanger *FStatusChanger;
private:
	QPushButton *pbtAdd;
	QPushButton *pbtDelete;
	QTableWidget *tbwStatus;
private:
	QList<int> FDeletedStatuses;
	QMap<int, RowData> FStatusItems;
};

#endif // STATUSOPTIONSWIDGET_H
