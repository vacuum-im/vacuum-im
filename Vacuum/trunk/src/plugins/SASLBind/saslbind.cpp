#include <QtDebug>
#include "saslbind.h"

#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

SASLBind::SASLBind(IXmppStream *AStream)
  : QObject(AStream->instance())
{
  FNeedHook = false;
  FStream = AStream;
  connect(FStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
}

SASLBind::~SASLBind()
{

}

bool SASLBind::start(const QDomElement &/*AElem*/)
{
  FNeedHook = true;
  Stanza bind("iq");
  bind.setType("set").setId("bind"); 
  if (FStream->jid().resource().isEmpty())
    bind.addElement("bind",NS_FEATURE_BIND);
  else
    bind.addElement("bind",NS_FEATURE_BIND)
      .appendChild(bind.createElement("resource"))
      .appendChild(bind.createTextNode(FStream->jid().resource()));
  FStream->sendStanza(bind); 
  return true;
}

bool SASLBind::needHook(Direction ADirection) const
{
  if (ADirection == DirectionIn) 
    return FNeedHook; 
  
  return false;
}

bool SASLBind::hookElement(QDomElement *AElem, Direction ADirection)
{
  if (ADirection == DirectionIn && AElem->attribute("id") == "bind")
  {
    FNeedHook = false;
    if (AElem->attribute("type") == "result")
    {
      QString jid = AElem->firstChild().firstChild().toElement().text();  
      if (jid.isEmpty())
      {
        Stanza errStanza = Stanza(*AElem).replyError("jid-malformed");
        FStream->sendStanza(errStanza);
        ErrorHandler err(ErrorHandler::DEFAULTNS,errStanza.element()); 
        emit error(err.message());
        return true;
      };
      FStream->setJid(jid); 
      emit finished(false);
      return true;
    }
    else if (AElem->attribute("type") == "error")
    {
      ErrorHandler err(ErrorHandler::DEFAULTNS, *AElem);
      err.setContext(tr("During resource bind there was an error:")); 
      emit error(err.message());
      return true;
    }
  }
  return false;
}

void SASLBind::onStreamClosed(IXmppStream *)
{
  FNeedHook = false;
}


//SASLBindPlugin
SASLBindPlugin::SASLBindPlugin()
{

}

SASLBindPlugin::~SASLBindPlugin()
{
  FCleanupHandler.clear(); 
}

void SASLBindPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Resource Binding (XMPP-Core)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Resource Binding";
  APluginInfo->uid = SASLBIND_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool SASLBindPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    connect(plugin->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
  return plugin!=NULL;
}

IStreamFeature *SASLBindPlugin::newInstance(IXmppStream *AStream)
{
  SASLBind *saslBind = new SASLBind(AStream);
  FCleanupHandler.add(saslBind);
  return saslBind;
}

void SASLBindPlugin::onStreamAdded(IXmppStream *AStream)
{
  AStream->addFeature(newInstance(AStream)); 
}

Q_EXPORT_PLUGIN2(SASLBindPlugin, SASLBindPlugin)
