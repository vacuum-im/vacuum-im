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
  virtual QVariant value(const QString &AName, const QVariant &ADefault=QVariant()) const =0; 
  virtual void setValue(const QString &AName, const QVariant &AValue) =0;
  virtual void delValue(const QString &AName) =0;
  virtual void clear() =0;

  virtual const QString &accountId() const =0;
  virtual QString name() const =0;
  virtual void setName(const QString &AName) =0;

  virtual IXmppStream *xmppStream() const =0;
  virtual bool isActive() const =0;
  virtual Jid streamJid() const =0;
  virtual bool manualHostPort() const =0;
  virtual QString host() const =0;
  virtual qint16 port() const =0;
  virtual QString password() const =0;
  virtual QString defaultLang() const =0;
  virtual QStringList proxyTypes() const =0;
  virtual int proxyType() const =0;
  virtual QString proxyHost() const =0;
  virtual qint16 proxyPort() const =0;
  virtual QString proxyUsername() const =0;
  virtual QString proxyPassword() const =0;
  virtual QString pollServer() const=0;
  virtual bool autoConnect() const =0;
  virtual bool autoReconnect() const =0;
  virtual void setActive(bool AActive) =0;
  virtual void setStreamJid(const Jid &) =0;
  virtual void setManualHostPort(bool AManual) =0;
  virtual void setHost(const QString &) =0;
  virtual void setPort(qint16) =0;
  virtual void setPassword(const QString &) =0;
  virtual void setDefaultLang(const QString &) =0;
  virtual void setProxyType(int) =0;
  virtual void setProxyHost(const QString &) =0;
  virtual void setProxyPort(qint16) =0;
  virtual void setProxyUsername(const QString &) =0;
  virtual void setProxyPassword(const QString &) =0;
  virtual void setPollServer(const QString &) =0;
  virtual void setAutoConnect(bool) =0;
  virtual void setAutoReconnect(bool) =0;
signals:
  virtual void changed(const QString &AName, const QVariant &AValue) =0;
};

class IAccountManager
{
public:
  virtual QObject *instance() = 0;
  virtual IAccount *addAccount(const QString &AName, const Jid &AStreamJid) =0;
  virtual IAccount *addAccount(const QString &AAccountId, 
    const QString &AName = QString(), const Jid &AStreamJid= Jid()) =0;
  virtual void showAccount(IAccount *AAccount) =0;
  virtual IAccount *accountById(const QString &AAcoountId) const =0;
  virtual IAccount *accountByName(const QString &AName) const =0;
  virtual IAccount *accountByStream(const Jid &AStreamJid) const =0;
  virtual void removeAccount(IAccount *AAccount) =0;
  virtual void destroyAccount(const QString &AAccountId) =0;
signals:
  virtual void added(IAccount *) =0;
  virtual void shown(IAccount *) =0;
  virtual void hidden(IAccount *) =0;
  virtual void removed(IAccount *) =0;
  virtual void destroyed(IAccount *) =0;
};

Q_DECLARE_INTERFACE(IAccount,"Vacuum.Plugin.IAccount/1.0")
Q_DECLARE_INTERFACE(IAccountManager,"Vacuum.Plugin.IAccountManager/1.0")

#endif