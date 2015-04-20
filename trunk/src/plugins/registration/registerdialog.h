#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <interfaces/iregistraton.h>
#include <interfaces/idataforms.h>
#include "ui_registerdialog.h"

class RegisterDialog :
	public QDialog
{
	Q_OBJECT;
public:
	RegisterDialog(IRegistration *ARegistration, IDataForms *ADataForms, const Jid &AStremJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL);
	~RegisterDialog();
	Jid streamJid() const;
	Jid serviceJid() const;
protected:
	void resetDialog();
	void doRegisterOperation();
	void doRegister();
	void doUnregister();
	void doChangePassword();
protected slots:
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterSuccess(const QString &AId);
	void onRegisterError(const QString &AId, const XmppError &AError);
	void onDialogButtonsClicked(QAbstractButton *AButton);
private:
	Ui::RegisterDialogClass ui;
private:
	IDataForms *FDataForms;
	IRegistration *FRegistration;
private:
	Jid FStreamJid;
	Jid FServiceJid;
	int FOperation;
	QString FRequestId;
	IRegisterSubmit FSubmit;
	IDataFormWidget *FCurrentForm;
};

#endif // REGISTERDIALOG_H
