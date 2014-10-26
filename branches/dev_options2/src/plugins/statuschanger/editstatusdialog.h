#ifndef EDITSTATUSDIALOG_H
#define EDITSTATUSDIALOG_H

#include <QIcon>
#include <QDialog>
#include <QStyledItemDelegate>
#include <interfaces/istatuschanger.h>
#include <interfaces/ipresence.h>
#include "ui_editstatusdialog.h"

using namespace Ui;

struct RowStatus {
	int id;
	QString name;
	int show;
	QString text;
	int priority;
};

class Delegate :
	public QStyledItemDelegate
{
	Q_OBJECT;
public:
	enum DelegateType {
		DelegateName,
		DelegateShow,
		DelegateMessage,
		DelegatePriority
	};
public:
	Delegate(IStatusChanger *AStatusChanger, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
	IStatusChanger *FStatusChanger;
};

class EditStatusDialog :
	public QDialog,
	public EditStatusDialogClass
{
	Q_OBJECT;
public:
	EditStatusDialog(IStatusChanger *AStatusChanger);
	~EditStatusDialog();
protected slots:
	void onAddbutton(bool);
	void onDeleteButton(bool);
	void onDialogButtonsBoxAccepted();
	void onSelectionChanged();
private:
	IStatusChanger *FStatusChanger;
private:
	QList<int> FDeletedStatuses;
	QMap<int, RowStatus *> FStatusItems;
};

#endif // EDITSTATUSDIALOG_H
