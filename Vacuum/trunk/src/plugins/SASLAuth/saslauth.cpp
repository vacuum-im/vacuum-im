#include <QtDebug>
#include "saslauth.h"

#include <QMultiHash>
#include <QStringList>
#include "../../utils/stanza.h"
#include "../../utils/errorhandler.h"
#include "../../utils/md5.h"


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
    else
      qDebug() << "Wrong challenge format:" << chl;
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


SASLAuth::SASLAuth(IXmppStream *AStream)
: QObject(AStream->instance())
{
  FNeedHook = false;
  chlNumber = 0;
  FStream = AStream;
  connect(FStream->instance(),SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *)));
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
      FStream->sendStanza(auth);   
      return true;
    }
    else if (mech == "PLAIN")
    {
      FMechanism = mech;
      FNeedHook = true;
      QByteArray resp;
      resp.append('\0')
        .append(FStream->jid().node().toUtf8())
        .append('\0')
        .append(FStream->password().toUtf8());
      Stanza auth("auth");
      auth.setAttribute("xmlns",NS_FEATURE_SASL).setAttribute("mechanism",mech);
      auth.element().appendChild(auth.createTextNode(resp.toBase64()));    
      FStream->sendStanza(auth);   
      return true;
    }
  }
  return false;
}

bool SASLAuth::needHook(Direction ADirection) const
{
  if (ADirection == DirectionIn) 
    return FNeedHook; 
  
  return false;
}

bool SASLAuth::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection != DirectionIn || AElem->namespaceURI() != NS_FEATURE_SASL)
    return false;

  if (AElem->tagName() == "success")
  {
    FNeedHook = false;
    emit finished(true);
    return true;
  }
  else if (AElem->tagName() == "failure")
  {
    FNeedHook = false;
    ErrorHandler err(NS_FEATURE_SASL,AElem->firstChild().toElement().tagName());
    err.setContext(tr("During SASL authorization there was an error:"));
    emit error(err.message()); 
    return true;
  }
  else if (AElem->tagName() == "abort")
  {
    FNeedHook = false;
    ErrorHandler err(NS_FEATURE_SASL,"aborted");
    err.setContext(tr("During SASL authorization there was an error:"));
    emit error(err.message()); 
    return true;
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
        realm = FStream->jid().domane();
      QByteArray _realm = realm.toUtf8(); 
      QString user = FStream->jid().node();
      QString pass = FStream->password();
      nonce = params.value("nonce");
      QByteArray randBytes(32,' ');
      for(int i=0; i<31; i++)
        randBytes[i] = (char) (256.0 * rand() / (RAND_MAX + 1.0));
      cnonce = randBytes.toBase64();
      QString nc = "00000001";
      qop = params.value("qop");
      uri = "xmpp/" + FStream->jid().domane();
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
      FStream->sendStanza(response);
      return true;
    }
    else if (chlNumber == 1)
    {
      chlNumber--;
      Stanza response("response");
      response.setAttribute("xmlns",NS_FEATURE_SASL);
      FStream->sendStanza(response);   
      return true;
    }
  }
  else
  {
    FNeedHook = false;
    emit error("unexpected-request"); 
    return true;
  }
  return false;
}

void SASLAuth::onStreamClosed(IXmppStream *)
{
  FNeedHook = false;
  FMechanism = "";
  chlNumber = 0;
}


//SASLAuthPlugin
SASLAuthPlugin::SASLAuthPlugin()
{

}

SASLAuthPlugin::~SASLAuthPlugin()
{
  FCleanupHandler.clear(); 
}

void SASLAuthPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of SASL Authentication (XMPP-Core)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "SASL Authentication";
  APluginInfo->uid = SASLAUTH_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool SASLAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    connect(plugin->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
  return plugin!=NULL;
}

bool SASLAuthPlugin::initObjects()
{
  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "aborted", 
    ErrorHandler::CANCEL, ErrorHandler::FORBIDDEN, tr("Authorization Aborted"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "incorrect-encoding", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("Incorrect Encoding"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "invalid-authzid", 
    ErrorHandler::CANCEL, ErrorHandler::FORBIDDEN, tr("Invalid Authzid"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "invalid-mechanism", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("Invalid Mechanism"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "mechanism-too-weak", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("Mechanism Too Weak"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "not-authorized", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_AUTHORIZED, tr("Not Authorized"));

  ErrorHandler::addErrorItem(NS_FEATURE_SASL, "temporary-auth-failure", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_AUTHORIZED, tr("Temporary Auth Failure"));

  return true;
}

//ISASLAuth
IStreamFeature *SASLAuthPlugin::newInstance(IXmppStream *AStream)
{
  SASLAuth *saslAuth = new SASLAuth(AStream);
  FCleanupHandler.add(saslAuth);
  return saslAuth;
}

void SASLAuthPlugin::onStreamAdded(IXmppStream *AStream)
{
  AStream->addFeature(newInstance(AStream)); 
}

Q_EXPORT_PLUGIN2(SASLAuthPlugin, SASLAuthPlugin)
