#include <QtDebug>
#include "saslsession.h"

#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

SASLSession::SASLSession(IXmppStream *AStream) 
 : QObject(AStream->instance())
{
  FNeedHook = false;
  FStream = AStream;
  connect(FStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
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
  FStream->sendStanza(session); 
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
      err.setContext(tr("During session establishment there was an error:")); 
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
  FCleanupHandler.clear(); 
}

void SASLSessionPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Session Establishment (XMPP-IM)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Session Establishment";
  APluginInfo->uid = SASLSESSION_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool SASLSessionPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    connect(plugin->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
  return plugin!=NULL;
}

IStreamFeature *SASLSessionPlugin::newInstance(IXmppStream *AStream)
{
  SASLSession *saslSession = new SASLSession(AStream);
  FCleanupHandler.add(saslSession);
  return saslSession;
}

void SASLSessionPlugin::onStreamAdded(IXmppStream *AStream)
{
  AStream->addFeature(newInstance(AStream)); 
}

Q_EXPORT_PLUGIN2(SASLSessionPlugin, SASLSessionPlugin)
