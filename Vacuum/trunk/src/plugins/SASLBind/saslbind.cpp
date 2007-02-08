#include "saslbind.h"
#include <QtDebug>
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
  qDebug() << "~SASLBind";
}

bool SASLBind::start(const QDomElement &AElem)
{
  Q_UNUSED(AElem);
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
  qDebug() << "~SASLBindPlugin";
  FCleanupHandler.clear(); 
}

void SASLBindPlugin::getPluginInfo(PluginInfo *info)
{
  info->author = tr("Potapov S.A. aka Lion");
  info->description = tr("Implementation of Resource Binding (XMPP-Core)");
  info->homePage = "http://jrudevels.org";
  info->name = "Resource Binding";
  info->uid = SASLBIND_UUID;
  info->version = "0.1";
  info->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
}

bool SASLBindPlugin::initPlugin(IPluginManager *APluginManager)
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

bool SASLBindPlugin::startPlugin()
{
  return true;
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
