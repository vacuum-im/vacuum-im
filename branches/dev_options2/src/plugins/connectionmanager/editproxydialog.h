#ifndef EDITPROXYDIALOG_H
#define EDITPROXYDIALOG_H

#include <QDialog>
#include <interfaces/iconnectionmanager.h>
#include "ui_editproxydialog.h"

class EditProxyDialog :
	public QDialog
{
	Q_OBJECT;
public:
	EditProxyDialog(IConnectionManager *AManager, QWidget *AParent = NULL);
	~EditProxyDialog();
protected:
	QListWidgetItem *createProxyItem(const QUuid &AId, const IConnectionProxy &AProxy) const;
	void updateProxyItem(QListWidgetItem *AItem);
	void updateProxyWidgets(QListWidgetItem *AItem);
protected slots:
	void onAddButtonClicked(bool);
	void onDeleteButtonClicked(bool);
	void onCurrentProxyItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious);
	void onDialogButtonBoxAccepted();
private:
	Ui::EditProxyDialogClass ui;
private:
	IConnectionManager *FManager;
};

#endif // EDITPROXYDIALOG_H
