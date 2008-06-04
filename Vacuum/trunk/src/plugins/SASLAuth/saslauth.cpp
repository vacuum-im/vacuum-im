#include "saslauth.h"

#include <QMultiHash>
#include <QStringList>
#include <QCryptographicHash>

static QMultiHash<QString, QString> parseChallenge(const QString &AChallenge)
{
  QMultiHash<QString, QString> hash;
  QStringList params = AChallenge.split(",",QString::SkipEmptyParts); 
  QStringList param;
  QString value;
  for(int i = 0;i<params.count();i++)
  {
    param = params.at(i).split("=");
    if (param.count() == 2)
    {
      value = param.at(1);
      if (value.endsWith("\""))
        value.remove(0,1).chop(1);  
      hash.insert(param.at(0),value);
    }
  }
  return hash;
}

static QByteArray getRespValue(const QByteArray &realm, const QByteArray &user, const QByteArray &pass,const QByteArray &nonce,
                               const QByteArray &cnonce, const QByteArray &nc, const QByteArray &qop, const QByteArray &uri,
                               const QByteArray &auth)
{
  QByteArray yHash =  QCryptographicHash::hash(user+':'+realm+':'+pass,QCryptographicHash::Md5);
  QByteArray a1Hash = QCryptographicHash::hash(yHash+':'+nonce+':'+cnonce,QCryptographicHash::Md5);
  QByteArray a2Hash = QCryptographicHash::hash(auth+':'+uri,QCryptographicHash::Md5);
  QByteArray value =  QCryptographicHash::hash(a1Hash.toHex()+':'+nonce+':'+nc+':'+cnonce+':'+qop+':'+a2Hash.toHex(),QCryptographicHash::Md5);
  return value.toHex();
}

SASLAuth::SASLAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FAuthorized = false;
  FNeedHook = false;
  FChallengeStep = 0;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
}

SASLAuth::~SASLAuth()
{

}

bool SASLAuth::start(const QDomElement &AElem)
{
  if (!FAuthorized)
  {
    QDomElement mechElem = AElem.firstChildElement("mechanism");
    while (!mechElem.isNull())
    {
      FMechanism = mechElem.text();
      if (FMechanism == "DIGEST-MD5")
      {
        FNeedHook = true;
        Stanza auth("auth");
        auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",FMechanism); 
        FXmppStream->sendStanza(auth);   
        return true;
      }
      else if (FMechanism == "PLAIN")
      {
        FNeedHook = true;
        QByteArray resp;
        resp.append('\0').append(FXmppStream->jid().pNode().toUtf8()).append('\0').append(FXmppStream->password().toUtf8());
        Stanza auth("auth");
        auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",FMechanism);
        auth.element().appendChild(auth.createTextNode(resp.toBase64()));    
        FXmppStream->sendStanza(auth);   
        return true;
      }
      mechElem = mechElem.nextSiblingElement("mechanism");
    }
    FMechanism.clear();
  }
  return false;
}

bool SASLAuth::needHook(Direction ADirection) const
{
  return ADirection == DirectionIn ? FNeedHook : false;
}

bool SASLAuth::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->namespaceURI() == NS_FEATURE_SASL)
  {
    if (AElem->tagName() == "success")
    {
      FAuthorized = true;
      emit ready(true);
    }
    else if (AElem->tagName() == "failure")
    {
      ErrorHandler err(*AElem,NS_FEATURE_SASL);
      emit error(err.message()); 
    }
    else if (AElem->tagName() == "abort")
    {
      ErrorHandler err("aborted",NS_FEATURE_SASL);
      emit error(err.message()); 
    }
    else if (AElem->tagName() == "challenge")
    {
      if (FChallengeStep == 0)
      {
        FChallengeStep++;
        QString chl = QByteArray::fromBase64(AElem->text().toAscii()); 
        QMultiHash<QString, QString> params = parseChallenge(chl);
        
        QString realm = params.value("realm");
        if (realm.isEmpty())
          realm = FXmppStream->jid().pDomain();
        QString user = FXmppStream->jid().pNode();
        QString pass = FXmppStream->password();
        QString nonce = params.value("nonce");
        QByteArray randBytes(32,' ');
        for(int i=0; i<31; i++)
          randBytes[i] = (char) (256.0 * rand() / (RAND_MAX + 1.0));
        QString cnonce = randBytes.toBase64();
        QString nc = "00000001";
        QString qop = params.value("qop");
        QString uri = "xmpp/" + FXmppStream->jid().pDomain();
        QByteArray respValue = getRespValue(realm.toUtf8(),user.toUtf8(),pass.toUtf8(),nonce.toAscii(), 
          cnonce.toAscii(),nc.toAscii(),qop.toAscii(), uri.toAscii(), "AUTHENTICATE");
        
        QString resp = "username=\"%1\",realm=\"%2\",nonce=\"%3\",cnonce=\"%4\",nc=%5,qop=%6,digest-uri=\"%7\",response=%8,charset=utf-8";
        resp = resp.arg(user.toUtf8().data()).arg(realm).arg(nonce).arg(cnonce).arg(nc).arg(qop).arg(uri).arg(respValue.data());
        
        Stanza response("response");
        response.setAttribute("xmlns",NS_FEATURE_SASL);
        response.element().appendChild(response.createTextNode(resp.toAscii().toBase64())); 
        FXmppStream->sendStanza(response);
      }
      else if (FChallengeStep == 1)
      {
        FChallengeStep--;
        Stanza response("response");
        response.setAttribute("xmlns",NS_FEATURE_SASL);
        FXmppStream->sendStanza(response);   
      }
      return true;
    }
    else
    {
      emit error(ErrorHandler("bad-request").message()); 
    }
    FNeedHook = false;
    return true;
  }
  return false;
}

void SASLAuth::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  FMechanism = "";
  FNeedHook = false;
  FAuthorized = false;
  FChallengeStep = 0;
}
