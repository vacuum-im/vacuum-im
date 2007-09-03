#ifndef ACCOUNT_H
#define ACCOUNT_H

#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/isettings.h"
#include "../../utils/jid.h"

class Account : 
  public QObject,
  public IAccount
{
  Q_OBJECT;
  Q_INTERFACES(IAccount);

public:
  Account(const QString &AAccountId, ISettings *ASettings, 
    IXmppStream *AStream, QObject *AParent);
  ~Account();

  virtual QObject *instance() { return this; }

  virtual QByteArray encript(const QString &AValue, const QByteArray &AKey) const;
  virtual QString decript(const QByteArray &AValue, const QByteArray &AKey) const;
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) const; 
  virtual void setValue(const QString &AName, const QVariant &AValue);
  virtual void delValue(const QString &AName);
  virtual void clear();

  virtual const QString &accountId() const;
  virtual QString name() const;
  virtual void setName(const QString &AName);

  virtual IXmppStream *xmppStream() const;
  virtual bool isActive() const;
  virtual void setActive(bool AActive);
  virtual Jid streamJid() const;
  virtual void setStreamJid(const Jid &AJid);
  virtual QString password() const;
  virtual void setPassword(const QString &APassword);
  virtual QString defaultLang() const;
  virtual void setDefaultLang(const QString &ALang);
signals:
  virtual void changed(const QString &AName, const QVariant &AValue);
protected:
  void initXmppStream();
private:
  ISettings *FSettings;
  IXmppStream *FXmppStream;
private:
  QString FId;
};

#endif // ACCOUNT_H
