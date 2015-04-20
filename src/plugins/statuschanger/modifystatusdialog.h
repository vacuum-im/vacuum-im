#ifndef MODIFYSTATUSDIALOG_H
#define MODIFYSTATUSDIALOG_H

#include <QDialog>
#include <interfaces/istatuschanger.h>
#include <interfaces/ipresencemanager.h>
#include "ui_modifystatusdialog.h"

class ModifyStatusDialog :
	public QDialog
{
	Q_OBJECT;
public:
	ModifyStatusDialog(IStatusChanger *AStatusChanger, int AStatusId, const Jid &AStreamJid, QWidget *AParent = NULL);
	~ModifyStatusDialog();
	void modifyStatus();
protected slots:
	void onDialogButtonBoxClicked(QAbstractButton *AButton);
private:
	Ui::ModifyStatusDialogClass ui;
private:
	IStatusChanger *FStatusChanger;
private:
	int FStatusId;
	Jid FStreamJid;
};

#endif // MODIFYSTATUSDIALOG_H
