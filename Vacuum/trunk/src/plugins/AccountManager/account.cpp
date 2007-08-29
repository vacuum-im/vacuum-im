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

bool Account::manualHostPort() const
{
  return value("manualHostPort",false).toBool();
}

QString Account::host() const
{
  return value("host",streamJid().domane()).toString();
}

qint16 Account::port() const
{
  return value("port",5222).toInt();
}

QString Account::password() const
{
  return decript(value("password").toByteArray(),FId.toUtf8());
}

QString Account::defaultLang() const
{
  return value("defLang",FXmppStream->defaultLang()).toString();
}

QStringList Account::proxyTypes() const
{
  return FXmppStream->connection()->proxyTypes();
}

int Account::proxyType() const
{
  return value("proxyType",FXmppStream->connection()->proxyType()).toInt();
}

QString Account::proxyHost() const
{
  return value("proxyHost",FXmppStream->connection()->proxyHost()).toString();
}

qint16 Account::proxyPort() const
{
  return value("proxyPort",FXmppStream->connection()->proxyPort()).toInt();
}

QString Account::proxyUsername() const
{
  return value("proxyUser",FXmppStream->connection()->proxyUsername()).toString();
}

QString Account::proxyPassword() const
{
  return decript(value("proxyPassword").toByteArray(),FId.toUtf8());
}

QString Account::pollServer() const
{
  return value("pollServer",FXmppStream->connection()->pollServer()).toString();
}

bool Account::autoConnect() const
{
  return value("autoConnect",false).toBool();
}

bool Account::autoReconnect() const
{
  return value("autoReconnect",true).toBool();
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

void Account::setManualHostPort(bool AManual)
{
  if (manualHostPort() != AManual)
  {
    setValue("manualHostPort",AManual);
    if (AManual)
    {
      FXmppStream->setHost(value("host",FXmppStream->jid().domane()).toString());
      FXmppStream->setPort(value("port",5222).toInt());
    }
    else
    {
      FXmppStream->setHost(FXmppStream->jid().domane());
      FXmppStream->setPort(5222);
    }
  }
}

void Account::setHost(const QString &AHost)
{
  if (host() != AHost)
  {
    if (manualHostPort())
      FXmppStream->setHost(AHost);
    setValue("host",AHost);
  }
}

void Account::setPort(qint16 APort)
{
  if (port() != APort)
  {
    if (manualHostPort())
      FXmppStream->setPort(APort);
    setValue("port",APort);
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

void Account::setProxyType(int AProxyType)
{
  if (proxyType() != AProxyType)
  {
    FXmppStream->connection()->setProxyType(AProxyType);
    setValue("proxyType",AProxyType);
  }
}

void Account::setProxyHost(const QString &AProxyHost)
{
  if (proxyHost() != AProxyHost)
  {
    FXmppStream->connection()->setProxyHost(AProxyHost);
    setValue("proxyHost",AProxyHost);
  }
}

void Account::setProxyPort(qint16 AProxyPort)
{
  if (proxyPort() != AProxyPort)
  {
    FXmppStream->connection()->setProxyPort(AProxyPort);
    setValue("proxyPort",AProxyPort);
  }
}

void Account::setProxyUsername(const QString &AProxyUser)
{
  if (proxyUsername() != AProxyUser)
  {
    FXmppStream->connection()->setProxyUsername(AProxyUser);
    setValue("proxyUser",AProxyUser);
  }
}

void Account::setProxyPassword(const QString &AProxyPassword)
{
  if (proxyPassword() != AProxyPassword)
  {
    FXmppStream->connection()->setProxyPassword(AProxyPassword);
    setValue("proxyPassword",encript(AProxyPassword, FId.toUtf8()));
  }
}

void Account::setPollServer(const QString &APollServer)
{
  if (pollServer() != APollServer)
  {
    FXmppStream->connection()->setPollServer(APollServer);
    setValue("pollServer",APollServer);
  }
}

void Account::setAutoConnect(bool AAutoConnect)
{
  if (autoConnect() != AAutoConnect)
  {
    setValue("autoConnect",AAutoConnect);
  }
}

void Account::setAutoReconnect(bool AAutoReconnect)
{
  if (autoReconnect() != AAutoReconnect)
  {
    setValue("autoReconnect",AAutoReconnect);
  }
}

void Account::initXmppStream()
{
  if (manualHostPort())
  {
    FXmppStream->setHost(host());
    FXmppStream->setPort(port());  
  }
  FXmppStream->setPassword(password());
  FXmppStream->setDefaultLang(defaultLang()); 
  FXmppStream->connection()->setProxyType(proxyType());
  FXmppStream->connection()->setProxyHost(proxyHost());
  FXmppStream->connection()->setProxyPort(proxyPort());
  FXmppStream->connection()->setProxyUsername(proxyUsername());
  FXmppStream->connection()->setProxyPassword(proxyPassword());
  FXmppStream->connection()->setPollServer(pollServer());
}

