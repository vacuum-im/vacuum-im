#include "saslauth.h"

#include <QMultiHash>
#include <QStringList>

static QMultiHash<QString, QString> parseChallenge(const QString &chl)
{
  QMultiHash<QString, QString> hash;
  QStringList params = chl.split(",",QString::SkipEmptyParts); 
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

static QByteArray getRespValue(const QByteArray &realm,
                               const QByteArray &user,
                               const QByteArray &pass,
                               const QByteArray &nonce,
                               const QByteArray &cnonce,
                               const QByteArray &nc,
                               const QByteArray &qop,
                               const QByteArray &uri,
                               const QByteArray &auth)
{
  unsigned char Y[16];
  unsigned char A1[16];
  unsigned char A2[16];
  unsigned char colon = ':';
  QByteArray buf(32,' ');

  md5_context context;
  md5_starts(&context);
  md5_update(&context, (uint8 *) user.data(), user.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) realm.data(), realm.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) pass.data(), pass.length());
  md5_finish(&context, Y);

  md5_starts(&context);
  md5_update(&context, Y, sizeof(Y));
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) nonce.data(), nonce.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) cnonce.data(), cnonce.length());
  md5_finish(&context, A1);

  md5_starts(&context);
  if (!auth.isEmpty())
    md5_update(&context, (uint8 *) auth.data(), auth.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) uri.data(), uri.length());
  md5_finish(&context, A2);


  md5_starts(&context);
  md5_hex(A1,buf.data());
  md5_update(&context, (uint8 *) buf.data(), buf.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) nonce.data(), nonce.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) nc.data(), nc.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) cnonce.data(), cnonce.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_update(&context, (uint8 *) qop.data(), qop.length());
  md5_update(&context, &colon, sizeof(colon));
  md5_hex(A2,buf.data());
  md5_update(&context, (uint8 *) buf.data(), buf.length());
  md5_finish(&context, A1);

  md5_hex(A1,buf.data()); 

  return buf;
}


SASLAuth::SASLAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  chlNumber = 0;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
}

SASLAuth::~SASLAuth()
{

}

bool SASLAuth::start(const QDomElement &AElem)
{
  FNeedHook = false;
  FMechanism = "";
  int i = 0;
  QString mech;
  while (i<AElem.childNodes().count())
  {
    mech = AElem.childNodes().at(i++).firstChild().toText().data();
    if (mech == "DIGEST-MD5")
    {
      FMechanism = mech;
      FNeedHook = true;
      Stanza auth("auth");
      auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",mech); 
      FXmppStream->sendStanza(auth);   
      return true;
    }
    else if (mech == "PLAIN")
    {
      FMechanism = mech;
      FNeedHook = true;
      QByteArray resp;
      resp.append('\0')
        .append(FXmppStream->jid().eNode().toUtf8())
        .append('\0')
        .append(FXmppStream->password().toUtf8());
      Stanza auth("auth");
      auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",mech);
      auth.element().appendChild(auth.createTextNode(resp.toBase64()));    
      FXmppStream->sendStanza(auth);   
      return true;
    }
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
      if (chlNumber == 0)
      {
        chlNumber++;
        QString chl = QByteArray::fromBase64(AElem->text().toAscii()); 
        QMultiHash<QString, QString> params = parseChallenge(chl);
        realm = params.value("realm");
        if (realm.isEmpty())
          realm = FXmppStream->jid().domane();
        QByteArray _realm = realm.toUtf8(); 
        QString user = FXmppStream->jid().eNode();
        QString pass = FXmppStream->password();
        nonce = params.value("nonce");
        QByteArray randBytes(32,' ');
        for(int i=0; i<31; i++)
          randBytes[i] = (char) (256.0 * rand() / (RAND_MAX + 1.0));
        cnonce = randBytes.toBase64();
        QString nc = "00000001";
        qop = params.value("qop");
        uri = "xmpp/" + FXmppStream->jid().domane();
        QByteArray respValue = getRespValue(realm.toUtf8(),
          user.toUtf8(),
          pass.toUtf8(),
          nonce.toAscii(), cnonce.toAscii(),
          nc.toAscii(),	qop.toAscii(), 
          uri.toAscii(), "AUTHENTICATE");
        QString resp = "username=\"%1\",realm=\"%2\",nonce=\"%3\",cnonce=\"%4\","
          "nc=%5,qop=%6,digest-uri=\"%7\",response=%8,charset=utf-8";
        resp = resp.arg(user.toUtf8().data())
          .arg(realm)
          .arg(nonce)
          .arg(cnonce)
          .arg(nc)
          .arg(qop)
          .arg(uri)
          .arg(respValue.data());
        Stanza response("response");
        response.setAttribute("xmlns",NS_FEATURE_SASL);
        response.element().appendChild(response.createTextNode(resp.toAscii().toBase64())); 
        FXmppStream->sendStanza(response);
      }
      else if (chlNumber == 1)
      {
        chlNumber--;
        Stanza response("response");
        response.setAttribute("xmlns",NS_FEATURE_SASL);
        FXmppStream->sendStanza(response);   
      }
      return true;
    }
    else
    {
      emit error("unexpected-request"); 
    }
    FNeedHook = false;
    return true;
  }
  return false;
}

void SASLAuth::onStreamClosed(IXmppStream * /*AXmppStream*/)
{
  FNeedHook = false;
  FMechanism = "";
  chlNumber = 0;
}
