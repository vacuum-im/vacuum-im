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

  Stanza setAuth("iq");
  setAuth.setType("set").setTo(FXmppStream->jid().domane()).setId("auth"); 
  QDomElement query = setAuth.addElement("query",NS_JABBER_IQ_AUTH);
  query.appendChild(setAuth.createElement("username")).
    appendChild(setAuth.createTextNode(FXmppStream->jid().eNode()));
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
      ErrorHandler err(*AElem);
      emit error(err.meaning());
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
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);  
}

bool IqAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
    connect(plugin->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
  }
  return plugin!=NULL;
}

//IStreamFeature
IStreamFeature *IqAuthPlugin::addFeature(IXmppStream *AXmppStream)
{
  IqAuth *iqAuth = (IqAuth *)getFeature(AXmppStream->jid());
  if (!iqAuth)
  {
    iqAuth = new IqAuth(AXmppStream);
    connect(iqAuth,SIGNAL(destroyed(QObject *)),SLOT(onIqAuthDestroyed(QObject *)));
    FFeatures.append(iqAuth);
    FCleanupHandler.add(iqAuth);
    AXmppStream->addFeature(iqAuth);
  }
  return iqAuth;
}

IStreamFeature *IqAuthPlugin::getFeature(const Jid &AStreamJid) const
{
  foreach(IqAuth *feature, FFeatures)
    if (feature->xmppStream()->jid() == AStreamJid)
      return feature;
  return NULL;
}

void IqAuthPlugin::removeFeature(IXmppStream *AXmppStream)
{
  IqAuth *iqAuth = (IqAuth *)getFeature(AXmppStream->jid());
  if (iqAuth)
  {
    disconnect(iqAuth,SIGNAL(destroyed(QObject *)),this,SLOT(onIqAuthDestroyed(QObject *)));
    FFeatures.removeAt(FFeatures.indexOf(iqAuth));
    AXmppStream->removeFeature(iqAuth);
    delete iqAuth;
  }
}

void IqAuthPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = addFeature(AXmppStream); 
  emit featureAdded(feature);
}

void IqAuthPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = getFeature(AXmppStream->jid());
  if (feature)
  {
    emit featureRemoved(feature);
    removeFeature(AXmppStream);
  }
}

void IqAuthPlugin::onIqAuthDestroyed(QObject *AObject)
{
  IqAuth *iqAuth = qobject_cast<IqAuth *>(AObject);
  if (FFeatures.contains(iqAuth))
    FFeatures.removeAt(FFeatures.indexOf(iqAuth));
}
Q_EXPORT_PLUGIN2(IqAuthPlugin, IqAuthPlugin)
