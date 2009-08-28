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

QUuid Account::accountId() const
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
  valid = valid && (FXmppStream==FXmppStreams->xmppStream(sJid) || FXmppStreams->xmppStream(sJid)==NULL);
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
    FXmppStream = FXmppStreams->newXmppStream(streamJid());
    FXmppStream->setPassword(password());
    FXmppStream->setDefaultLang(defaultLang()); 
    FXmppStreams->addXmppStream(FXmppStream);
    emit changed(AVN_ACTIVE,true);
  }
  else if (!AActive && FXmppStream!=NULL)
  {
    FXmppStreams->removeXmppStream(FXmppStream);
    emit changed(AVN_ACTIVE,false);
    FXmppStreams->destroyXmppStream(FXmppStream->streamJid());
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
  return FXmppStream == NULL ? value(AVN_STREAM_JID).toString() : FXmppStream->streamJid();
}

void Account::setStreamJid(const Jid &AJid)
{
  setValue(AVN_STREAM_JID,AJid.full());
}

QString Account::password() const
{
  return decript(value(AVN_PASSWORD).toByteArray(),FAccountId.toString().toUtf8());
}

void Account::setPassword(const QString &APassword)
{
  setValue(AVN_PASSWORD,encript(APassword,FAccountId.toString().toUtf8()));
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
  return FSettings->valueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId.toString(),ADefault);
}

void Account::setValue(const QString &AName, const QVariant &AValue)
{
  if (value(AName) != AValue)
  {
    if (FXmppStream)
    {
      if (AName == AVN_STREAM_JID)
        FXmppStream->setStreamJid(AValue.toString());
      else if (AName == AVN_PASSWORD)
        FXmppStream->setPassword(decript(AValue.toByteArray(),FAccountId.toString().toUtf8()));
      else if (AName == AVN_DEFAULT_LANG)
        FXmppStream->setDefaultLang(AValue.toString());
    }
    FSettings->setValueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId.toString(),AValue);
    emit changed(AName,AValue);
  }
}

void Account::delValue(const QString &AName)
{
  FSettings->deleteValueNS(QString(SVN_VALUE_PREFIX":%1").arg(AName),FAccountId.toString());
  emit changed(AName,QVariant());
}
