#ifndef AUTORULESOPTIONSDIALOG_H
#define AUTORULESOPTIONSDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QTableWidget>
#include <QDialogButtonBox>
#include <QStyledItemDelegate>
#include <interfaces/ipresence.h>
#include <interfaces/iautostatus.h>
#include <interfaces/istatuschanger.h>

class AutoRuleDelegate :
	public QStyledItemDelegate
{
	Q_OBJECT;
public:
	AutoRuleDelegate(IStatusChanger *AStatusChanger, QObject *AParent = NULL);
	QWidget *createEditor(QWidget *AParent, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
	void setEditorData(QWidget *AEditor, const QModelIndex &AIndex) const;
	void setModelData(QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex) const;
	void updateEditorGeometry(QWidget *AEditor, const QStyleOptionViewItem &AOption, const QModelIndex &AIndex) const;
private:
	IStatusChanger *FStatusChanger;
};

class AutoRulesOptionsDialog :
	public QDialog
{
	Q_OBJECT;
public:
	AutoRulesOptionsDialog(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent = NULL);
	~AutoRulesOptionsDialog();
protected:
	int appendTableRow(const QUuid &ARuleId, const IAutoStatusRule &ARule);
protected slots:
	void onRuledItemSelectionChanged();
	void onDialogButtonBoxClicked(QAbstractButton *AButton);
private:
	IAutoStatus *FAutoStatus;
	IStatusChanger *FStatusChanger;
private:
	QPushButton *pbtAdd;
	QPushButton *pbtDelete;
	QTableWidget *tbwRules;
	QDialogButtonBox *dbbButtonBox;
};

#endif // AUTORULESOPTIONSDIALOG_H
