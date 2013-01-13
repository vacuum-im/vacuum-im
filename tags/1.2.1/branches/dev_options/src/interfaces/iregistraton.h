#ifndef IREGISTRATION_H
#define IREGISTRATION_H

#include <QUrl>
#include <interfaces/idataforms.h>
#include <utils/jid.h>

#define REGISTRATION_UUID         "{441F0DD4-C2DF-4417-B2F7-1D180C125EE3}"

struct IRegisterFields {
  enum Fields {
    Username  = 1,
    Password  = 2,
    Email     = 4,
    URL       = 8
  };
  int fieldMask;
  bool registered;
  Jid serviceJid;
  QString instructions;
  QString username;
  QString password;
  QString email;
  QString key;
  QUrl url;
  IDataForm form;
};

struct IRegisterSubmit {
  int fieldMask;
  Jid serviceJid;
  QString username;
  QString password;
  QString email;
  QString key;
  IDataForm form;
};

class IRegistration {
public:
  enum RegisterOperation {
    Register,
    Unregister,
    ChangePassword
  };
public:
  virtual QObject *instance() =0;
  virtual QString sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
  virtual QString sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid) =0;
  virtual QString sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AUserName, const QString &APassword) =0;
  virtual QString sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit) =0;
  virtual bool showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent = NULL) =0;
protected:
  virtual void registerFields(const QString &AId, const IRegisterFields &AFields) =0;
  virtual void registerSuccessful(const QString &AId) =0;
  virtual void registerError(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IRegistration,"Vacuum.Plugin.IRegistration/1.0")

#endif
