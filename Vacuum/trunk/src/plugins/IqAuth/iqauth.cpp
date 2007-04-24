#include <QtDebug>
#include "iqauth.h"

#include "../../utils/sha1.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

IqAuth::IqAuth(IXmppStream *AXmppStream)
  : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),
    SLOT(onStreamClosed(IXmppStream *)));
}

IqAuth::~IqAuth()
{

}

bool IqAuth::start(const QDomElement &/*AElem*/)
{
  bool useDigest = false;
  QByteArray shaData = FXmppStream->streamId().toUtf8()+FXmppStream->password().toUtf8(); 
  SHA1Context sha;
  useDigest = SHA1Hash(&sha, (const unsigned char *)shaData.constData(), shaData.size());

  Stanza setAuth("iq",this);
  setAuth.setType("set").setTo(FXmppStream->jid().domane()).setId("auth"); 
  QDomElement query = setAuth.addElement("query",NS_JABBER_IQ_AUTH);
  query.appendChild(setAuth.createElement("username")).
    appendChild(setAuth.createTextNode(FXmppStream->jid().node()));
  if (useDigest)
  {
    QByteArray shaDigest(40,' ');
    SHA1Hex(&sha,shaDigest.data()); 
    query.appendChild(setAuth.createElement("digest")).
      appendChild(setAuth.createTextNode(shaDigest.toLower().trimmed()));
  }
  else
    query.appendChild(setAuth.createElement("password")).
      appendChild(setAuth.createTextNode(FXmppStream->password()));

  query.appendChild(setAuth.createElement("resource")).
    appendChild(setAuth.createTextNode(FXmppStream->jid().resource()));

  FXmppStream->sendStanza(setAuth);

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
  FCleanupHandler.clear(); 
}

void IqAuthPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Non-SASL Authentication (JEP-0078)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Non-SASL Authentication";
  APluginInfo->uid = IQAUTH_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool IqAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    connect(plugin->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
  return plugin!=NULL;
}

IStreamFeature *IqAuthPlugin::newInstance(IXmppStream *AXmppStream)
{
  IqAuth *iqAuth = new IqAuth(AXmppStream);
  FCleanupHandler.add(iqAuth);
  return iqAuth;
}

void IqAuthPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  AXmppStream->addFeature(newInstance(AXmppStream)); 
}

Q_EXPORT_PLUGIN2(IqAuthPlugin, IqAuthPlugin)
