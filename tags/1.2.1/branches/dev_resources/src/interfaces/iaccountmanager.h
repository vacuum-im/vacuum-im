#ifndef IACCOUNTMANAGER_H
#define IACCOUNTMANAGER_H

#include <QString>
#include <QVariant>
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"

#define ACCOUNTMANAGER_UUID "{56F1AA4C-37A6-4007-ACFE-557EEBD86AF8}"

class IAccount
{
public:
  virtual QObject *instance() = 0;
  virtual const QString &accountId() const =0;
  virtual IXmppStream *xmppStream() const =0;
  virtual bool isValid() const =0;
  virtual bool isActive() const =0;
  virtual void setActive(bool AActive) =0;
  virtual QString name() const =0;
  virtual void setName(const QString &AName) =0;
  virtual Jid streamJid() const =0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual QString password() const =0;
  virtual void setPassword(const QString &APassword) =0;
  virtual QString defaultLang() const =0;
  virtual void setDefaultLang(const QString &ADefLang) =0;
  //AccountSettings
  virtual QByteArray encript(const QString &AValue, const QByteArray &AKey) const =0;
  virtual QString decript(const QByteArray &AValue, const QByteArray &AKey) const =0;
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) const =0;
  virtual void setValue(const QString &AName, const QVariant &AValue) =0;
  virtual void delValue(const QString &AName) =0;
signals:
  virtual void changed(const QString &AName, const QVariant &AValue) =0;
};

class IAccountManager
{
public:
  virtual QObject *instance() = 0;
  virtual IAccount *insertAccount(const QString &AAccountId = "") =0;
  virtual QList<IAccount *> accounts() const =0;
  virtual IAccount *accountById(const QString &AAcoountId) const =0;
  virtual IAccount *accountByStream(const Jid &AStreamJid) const =0;
  virtual void showAccount(const QString &AAccountId) =0;
  virtual void hideAccount(const QString &AAccountId) =0;
  virtual void removeAccount(const QString &AAccountId) =0;
  virtual void destroyAccount(const QString &AAccountId) =0;
signals:
  virtual void inserted(IAccount *AAccount) =0;
  virtual void shown(IAccount *AAccount) =0;
  virtual void hidden(IAccount *AAccount) =0;
  virtual void removed(IAccount *AAccount) =0;
  virtual void destroyed(const QString &AAccountId) =0;
};

Q_DECLARE_INTERFACE(IAccount,"Vacuum.Plugin.IAccount/1.0")
Q_DECLARE_INTERFACE(IAccountManager,"Vacuum.Plugin.IAccountManager/1.0")

#endif
