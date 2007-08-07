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
  virtual Jid streamJid() const;
  virtual bool manualHostPort() const;
  virtual QString host() const;
  virtual qint16 port() const;
  virtual QString password() const;
  virtual QString defaultLang() const;
  virtual QStringList proxyTypes() const;
  virtual int proxyType() const;
  virtual QString proxyHost() const;
  virtual qint16 proxyPort() const;
  virtual QString proxyUsername() const;
  virtual QString proxyPassword() const;
  virtual QString pollServer() const;
  virtual bool autoConnect() const;
  virtual bool autoReconnect() const;
  virtual void setActive(bool AActive);
  virtual void setStreamJid(const Jid &AJid);
  virtual void setManualHostPort(bool AManual);
  virtual void setHost(const QString &AHost);
  virtual void setPort(qint16 APort);
  virtual void setPassword(const QString &APassword);
  virtual void setDefaultLang(const QString &ALang);
  virtual void setProxyType(int AProxyType);
  virtual void setProxyHost(const QString &AProxyHost);
  virtual void setProxyPort(qint16 AProxyPort);
  virtual void setProxyUsername(const QString &AProxyUser);
  virtual void setProxyPassword(const QString &AProxyPassword);
  virtual void setPollServer(const QString &APollServer);
  virtual void setAutoConnect(bool AAutoConnect);
  virtual void setAutoReconnect(bool AAutoReconnect);
signals:
  virtual void changed(const QString &AName, const QVariant &AValue);
protected:
  void initXmppStream();
private:
  QString FId;
  ISettings *FSettings;
  IXmppStream *FXmppStream;
};

#endif // ACCOUNT_H
