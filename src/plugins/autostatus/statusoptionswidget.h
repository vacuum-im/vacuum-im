#ifndef STATUSOPTIONSWIDGET_H
#define STATUSOPTIONSWIDGET_H

#include <QWidget>
#include <QItemDelegate>
#include <interfaces/ipresence.h>
#include <interfaces/iautostatus.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_statusoptionswidget.h"

class Delegate :
			public QItemDelegate
{
	Q_OBJECT;
public:
	Delegate(IStatusChanger *AStatusChanger, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
	IStatusChanger *FStatusChanger;
};

class StatusOptionsWidget :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	StatusOptionsWidget(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent);
	~StatusOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	int appendTableRow(const QUuid &ARuleId, const IAutoStatusRule &ARule);
protected slots:
	void onHelpButtonClicked(bool);
	void onAddButtonClicked(bool);
	void onDeleteButtonClicked(bool);
private:
	Ui::StatusOptionsWidgetClass ui;
private:
	IAutoStatus *FAutoStatus;
	IStatusChanger *FStatusChanger;
};

#endif // STATUSOPTIONSWIDGET_H
