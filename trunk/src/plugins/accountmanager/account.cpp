#include "account.h"

#define SVN_VALUE_PREFIX    "account[]"

Account::Account(IXmppStreams *AXmppStreams, ISettings *ASettings, const QString &AAccountId, QObject *AParent) : QObject(AParent)
{
  FAccountId = AAccountId;
  FSettings = ASettings;
  FXmppStreams = AXmppStreams;
  FXmppStream = NULL;
}

Account::~Account()
{

}

const QString &Account::accountId() const
{
  return FAccountId;
}

IXmppStream *Account::xmppStream() const
{
  return FXmppStream;
}

bool Account::isValid() const
{
  Jid sJid = streamJid();
  bool valid = sJid.isValid();
  valid = valid && !sJid.node().isEmpty();
  valid = valid && !sJid.domain().isEmpty();
  valid = valid && (FXmppStream==FXmppStreams->getStream(sJid) || FXmppStreams->getStream(sJid)==NULL);
  return valid;
}

bool Account::isActive() const
{
  return FXmppStream!=NULL;
}

void Account::setActive(bool AActive)
{
  if (AActive && FXmppStream==NULL && isValid())
  {
    FXmppStream = FXmppStreams->newStream(streamJid());
    FXmppStream->setPassword(password());
    FXmppStream->setDefaultLang(defaultLang()); 
    FXmppStreams->addStream(FXmppStream);
    emit changed(AVN_ACTIVE,true);
  }
  else if (!AActive && FXmppStream!=NULL)
  {
    FXmppStreams->removeStream(FXmppStream);
    emit changed(AVN_ACTIVE,false);
    FXmppStreams->destroyStream(FXmppStream->jid());
    FXmppStream = NULL;
  }
}

QString Account::name() const
{
  return value(AVN_NAME).toString();
}

void Account::setName(const QString &AName)
{
  setValue(AVN_NAME,AName);
}

Jid Account::streamJid() const
{
  return FXmppStream == NULL ? value(AVN_STREAM_JID).toString() : FXmppStream->jid();
}

void Account::setStreamJid(const Jid &AJid)
{
  setValue(AVN_STREAM_JID,AJid.full());
}

QString Account::password() const
{
  return decript(value(AVN_PASSWORD).toByteArray(),FAccountId.toUtf8());
}

void Account::setPassword(const QString &APassword)
{
  setValue(AVN_PASSWORD,encript(APassword,FAccountId.toUtf8()));
}

QString Account::defaultLang() const
{
  return value(AVN_DEFAULT_LANG).toString();
}

void Account::setDefaultLang(const QString &ALang)
{
  setValue(AVN_DEFAULT_LANG,ALang);
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
  return FSettings->valueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId,ADefault);
}

void Account::setValue(const QString &AName, const QVariant &AValue)
{
  if (value(AName) != AValue)
  {
    if (FXmppStream)
    {
      if (AName == AVN_STREAM_JID)
        FXmppStream->setJid(AValue.toString());
      else if (AName == AVN_PASSWORD)
        FXmppStream->setPassword(decript(AValue.toByteArray(),FAccountId.toUtf8()));
      else if (AName == AVN_DEFAULT_LANG)
        FXmppStream->setDefaultLang(AValue.toString());
    }
    FSettings->setValueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId,AValue);
    emit changed(AName,AValue);
  }
}

void Account::delValue(const QString &AName)
{
  FSettings->deleteValueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId);
  emit changed(AName,QVariant());
}

