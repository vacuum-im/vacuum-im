#include "account.h"

Account::Account(const QString &AAccountId, ISettings *ASettings, 
                 IXmppStream *AStream, QObject *AParent)
  : QObject(AParent)
{
  FId = AAccountId;
  FSettings = ASettings;
  FXmppStream = AStream;
  
  FSettings->setValueNS("account[]:streamJid",FId,AStream->jid().full());
  initXmppStream();
}

Account::~Account()
{

}

QByteArray Account::encript(const QString &AValue, const QByteArray &AKey) const
{
  return FSettings->encript(AValue,AKey);
}

QString Account::decript(const QByteArray &AValue, const QByteArray &AKey) const
{
  return FSettings->decript(AValue,AKey);
}

QVariant Account::value(const QString &AName, const QVariant &ADefault) const
{
  return FSettings->valueNS(QString("account[]:%1").arg(AName),FId,ADefault);
}

void Account::setValue(const QString &AName, const QVariant &AValue)
{
  FSettings->setValueNS(QString("account[]:%1").arg(AName),FId,AValue);
  emit changed(AName,AValue);
}

void Account::delValue(const QString &AName)
{
  FSettings->deleteValueNS(QString("account[]:%1").arg(AName),FId);
  emit changed(AName,QVariant());
}

void Account::clear()
{
  FSettings->deleteNS(FId);
}

const QString &Account::accountId() const
{
  return FId;
}

QString Account::name() const
{
  return value("name").toString();
}

void Account::setName(const QString &AName)
{
  if (name() != AName)
    setValue("name",AName);
}

IXmppStream *Account::xmppStream() const
{
  return FXmppStream;
}

bool Account::isActive() const
{
  return value("active",false).toBool();
}

Jid Account::streamJid() const
{
  return value("streamJid",FXmppStream->jid().full()).toString();
}

QString Account::password() const
{
  return decript(value("password").toByteArray(),FId.toUtf8());
}

QString Account::defaultLang() const
{
  return value("defLang",FXmppStream->defaultLang()).toString();
}

void Account::setActive(bool AActive)
{
  if (isActive() != AActive)
  {
    setValue("active",AActive);
  }
}

void Account::setStreamJid(const Jid &AJid)
{
  if (streamJid().full() != AJid.full())
  {
    FXmppStream->setJid(AJid);
    setValue("streamJid",AJid.full());
  }
}

void Account::setPassword(const QString &APassword)
{
  if (password() != APassword)
  {
    FXmppStream->setPassword(APassword);
    setValue("password",encript(APassword,FId.toUtf8()));
  }
}

void Account::setDefaultLang(const QString &ALang)
{
  if (defaultLang() != ALang)
  {
    FXmppStream->setDefaultLang(ALang);
    setValue("defaultLang",ALang);
  }
}


void Account::initXmppStream()
{
  FXmppStream->setPassword(password());
  FXmppStream->setDefaultLang(defaultLang()); 
}

