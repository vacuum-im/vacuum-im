#include "saslsession.h"
#include <QtDebug>
#include "utils/errorhandler.h"
#include "utils/stanza.h"

SASLSession::SASLSession(IXmppStream *AStream)
: QObject(AStream->instance())
{
  FNeedHook = false;
  FStream = AStream;
  connect(FStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
}

SASLSession::~SASLSession()
{
  qDebug() << "~SASLSession";
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

bool SASLSession::hookElement(QDomElement *AElem)
{
  if (AElem->attribute("id") == "session")
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
  qDebug() << "~SASLSessionPlugin";
  FCleanupHandler.clear(); 
}

void SASLSessionPlugin::getPluginInfo(PluginInfo *info)
{
  info->author = tr("Potapov S.A. aka Lion");
  info->description = tr("Implementation of Session Establishment (XMPP-IM)");
  info->homePage = "http://jrudevels.org";
  info->name = "Session Establishment";
  info->uid = SASLSESSION_UUID;
  info->version = "0.1";
  info->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool SASLSessionPlugin::initPlugin(IPluginManager *APluginManager)
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

bool SASLSessionPlugin::startPlugin()
{
  return true;
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
