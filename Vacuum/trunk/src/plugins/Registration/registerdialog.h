#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include "../../interfaces/iregistraton.h"
#include "../../interfaces/idataforms.h"
#include "../../utils/skin.h"
#include "ui_registerdialog.h"

class RegisterDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  RegisterDialog(IRegistration *ARegistration, IDataForms *ADataForms, const Jid &AStremJid, 
     const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL);
  ~RegisterDialog();
  virtual const Jid &streamJid() const { return FStreamJid; }
  virtual const Jid &serviceJid() const { return FServiceJid; }
protected:
  void resetDialog();
  void doRegisterOperation();
  void doRegister();
  void doUnregister();
  void doChangePassword();
protected slots:
  void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
  void onRegisterSuccessful(const QString &AId);
  void onRegisterError(const QString &AId, const QString &AError);
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
