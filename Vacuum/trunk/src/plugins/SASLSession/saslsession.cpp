#include <QtDebug>
#include "saslsession.h"

#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

SASLSession::SASLSession(IXmppStream *AXmppStream) 
 : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
}

SASLSession::~SASLSession()
{

}

bool SASLSession::start(const QDomElement &AElem)
{
  Q_UNUSED(AElem);
  FNeedHook = true;
  Stanza session("iq");
  session.setType("set").setId("session"); 
  session.addElement("session",NS_FEATURE_SESSION);
  FXmppStream->sendStanza(session); 
  return true;
}

bool SASLSession::needHook(Direction ADirection) const
{
  if (ADirection == DirectionIn) 
    return FNeedHook; 
  
  return false;
}

bool SASLSession::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->attribute("id") == "session")
  {
    FNeedHook = false;
    if (AElem->attribute("type") == "result")
    {
      emit finished(false);
      return true;
    }
    else if (AElem->attribute("type") == "error")
    {
      ErrorHandler err(ErrorHandler::DEFAULTNS, *AElem);
      emit error(err.message());
      return true;
    }
  }
  return false;
}

void SASLSession::onStreamClosed(IXmppStream *)
{
  FNeedHook = false;
}


//SASLSessionPlugin
SASLSessionPlugin::SASLSessionPlugin()
{

}

SASLSessionPlugin::~SASLSessionPlugin()
{

}

void SASLSessionPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Session Establishment (XMPP-IM)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Session Establishment";
  APluginInfo->uid = SASLSESSION_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
}

bool SASLSessionPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
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
IStreamFeature *SASLSessionPlugin::addFeature(IXmppStream *AXmppStream)
{
  SASLSession *saslSession = (SASLSession *)getFeature(AXmppStream->jid());
  if (!saslSession)
  {
    saslSession = new SASLSession(AXmppStream);
    connect(saslSession,SIGNAL(destroyed(QObject *)),SLOT(onSASLSessionDestroyed(QObject *)));
    FFeatures.append(saslSession);
    FCleanupHandler.add(saslSession);
    AXmppStream->addFeature(saslSession);
  }
  return saslSession;
}

IStreamFeature *SASLSessionPlugin::getFeature(const Jid &AStreamJid) const
{
  foreach(SASLSession *feature, FFeatures)
    if (feature->xmppStream()->jid() == AStreamJid)
      return feature;
  return NULL;
}

void SASLSessionPlugin::removeFeature(IXmppStream *AXmppStream)
{
  SASLSession *saslSession = (SASLSession *)getFeature(AXmppStream->jid());
  if (saslSession)
  {
    disconnect(saslSession,SIGNAL(destroyed(QObject *)),this,SLOT(onSASLSessionDestroyed(QObject *)));
    FFeatures.removeAt(FFeatures.indexOf(saslSession));
    AXmppStream->removeFeature(saslSession);
    delete saslSession;
  }
}


void SASLSessionPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = addFeature(AXmppStream); 
  emit featureAdded(feature);
}

void SASLSessionPlugin::onStreamRemoved( IXmppStream *AXmppStream )
{
  IStreamFeature *feature = getFeature(AXmppStream->jid());
  if (feature)
  {
    emit featureRemoved(feature);
    removeFeature(AXmppStream);
  }
}

void SASLSessionPlugin::onSASLSessionDestroyed( QObject *AObject )
{
  SASLSession *saslSession = qobject_cast<SASLSession *>(AObject);
  if (FFeatures.contains(saslSession))
    FFeatures.removeAt(FFeatures.indexOf(saslSession));
}

Q_EXPORT_PLUGIN2(SASLSessionPlugin, SASLSessionPlugin)
