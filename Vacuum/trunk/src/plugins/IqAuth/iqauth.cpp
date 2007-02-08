#include "iqauth.h"
#include <QtDebug>
#include "../../utils/sha1.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

IqAuth::IqAuth(IXmppStream *AStream)
  : QObject(AStream->instance())
{
  FNeedHook = false;
  FStream = AStream;
  connect(FStream->instance(),SIGNAL(closed(IXmppStream *)),
    SLOT(onStreamClosed(IXmppStream *)));
}

IqAuth::~IqAuth()
{
  qDebug() << "~IqAuth";
}

bool IqAuth::start(const QDomElement &AElem)
{
  Q_UNUSED(AElem);
  bool useDigest = false;
  QByteArray shaData = FStream->streamId().toUtf8()+FStream->password().toUtf8(); 
  SHA1Context sha;
  useDigest = SHA1Hash(&sha, (const unsigned char *)shaData.constData(), shaData.size());

  Stanza setAuth("iq",this);
  setAuth.setType("set").setTo(FStream->jid().domane()).setId("auth"); 
  QDomElement query = setAuth.addElement("query",NS_JABBER_IQ_AUTH);
  query.appendChild(setAuth.createElement("username")).
    appendChild(setAuth.createTextNode(FStream->jid().node()));
  if (useDigest)
  {
    QByteArray shaDigest(40,' ');
    SHA1Hex(&sha,shaDigest.data()); 
    query.appendChild(setAuth.createElement("digest")).
      appendChild(setAuth.createTextNode(shaDigest.toLower().trimmed()));
  }
  else
    query.appendChild(setAuth.createElement("password")).
      appendChild(setAuth.createTextNode(FStream->password()));

  query.appendChild(setAuth.createElement("resource")).
    appendChild(setAuth.createTextNode(FStream->jid().resource()));

  FStream->sendStanza(setAuth);

  FNeedHook = true;
  return true;
}

bool IqAuth::needHook(Direction ADirection) const
{
  if (ADirection == DirectionIn) 
    return FNeedHook; 
  
  return false;
}

bool IqAuth::hookElement(QDomElement *AElem,Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->attribute("id") == "auth")
  {
    FNeedHook = false;
    if (AElem->attribute("type") == "result")
    {
      emit finished(false);
      return true;
    }
    else if (AElem->attribute("type") == "error")
    {
      ErrorHandler err(ErrorHandler::DEFAULTNS,*AElem);
      err.setContext(tr("During authorization there was an error:")); 
      emit error(err.message());
      return true;
    };
  };
  return false;
}

void IqAuth::onStreamClosed(IXmppStream *)
{
  FNeedHook = false;
}


//IqAuthPlugin
IqAuthPlugin::IqAuthPlugin()
{

}

IqAuthPlugin::~IqAuthPlugin()
{
  qDebug() << "~IqAuthPlugin";
  FCleanupHandler.clear(); 
}

void IqAuthPlugin::getPluginInfo(PluginInfo *info)
{
  info->author = tr("Potapov S.A. aka Lion");
  info->description = tr("Implementation of Non-SASL Authentication (JEP-0078)");
  info->homePage = "http://jrudevels.org";
  info->name = "Non-SASL Authentication";
  info->uid = IQAUTH_UUID;
  info->version = "0.1";
  info->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool IqAuthPlugin::initPlugin(IPluginManager *APluginManager)
{
  QList<IPlugin *> plugins = APluginManager->getPlugins("IXmppStreams");
  bool connected = false;
  for(int i=0; i<plugins.count(); i++)
  {
    QObject *obj = plugins[i]->instance(); 
    connected = connected || connect(obj,SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
  }
  return connected;
}

bool IqAuthPlugin::startPlugin()
{
  return true;
}

IStreamFeature *IqAuthPlugin::newInstance(IXmppStream *AStream)
{
  IqAuth *iqAuth = new IqAuth(AStream);
  FCleanupHandler.add(iqAuth);
  return iqAuth;
}

void IqAuthPlugin::onStreamAdded(IXmppStream *AStream)
{
  AStream->addFeature(newInstance(AStream)); 
}

Q_EXPORT_PLUGIN2(IqAuthPlugin, IqAuthPlugin)
