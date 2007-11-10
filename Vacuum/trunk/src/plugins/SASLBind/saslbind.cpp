#include <QtDebug>
#include "saslbind.h"

#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"

SASLBind::SASLBind(IXmppStream *AXmppStream)
  : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
}

SASLBind::~SASLBind()
{

}

bool SASLBind::start(const QDomElement &/*AElem*/)
{
  FNeedHook = true;
  Stanza bind("iq");
  bind.setType("set").setId("bind"); 
  if (FXmppStream->jid().resource().isEmpty())
    bind.addElement("bind",NS_FEATURE_BIND);
  else
    bind.addElement("bind",NS_FEATURE_BIND)
      .appendChild(bind.createElement("resource"))
      .appendChild(bind.createTextNode(FXmppStream->jid().resource()));
  FXmppStream->sendStanza(bind); 
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
        FXmppStream->sendStanza(errStanza);
        ErrorHandler err(errStanza.element()); 
        emit error(err.meaning());
        return true;
      };
      FXmppStream->setJid(jid); 
      emit finished(false);
      return true;
    }
    else if (AElem->attribute("type") == "error")
    {
      ErrorHandler err(*AElem);
      emit error(err.meaning());
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

}

void SASLBindPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Resource Binding (XMPP-Core)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Resource Binding";
  APluginInfo->uid = SASLBIND_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool SASLBindPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
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
IStreamFeature *SASLBindPlugin::addFeature(IXmppStream *AXmppStream)
{
  SASLBind *saslBind = (SASLBind *)getFeature(AXmppStream->jid());
  if (!saslBind)
  {
    saslBind = new SASLBind(AXmppStream);
    connect(saslBind,SIGNAL(destroyed(QObject *)),SLOT(onSASLBindDestroyed(QObject *)));
    FFeatures.append(saslBind);
    FCleanupHandler.add(saslBind);
    AXmppStream->addFeature(saslBind);
  }
  return saslBind;
}

IStreamFeature *SASLBindPlugin::getFeature(const Jid &AStreamJid) const
{
  foreach(SASLBind *feature, FFeatures)
    if (feature->xmppStream()->jid() == AStreamJid)
      return feature;
  return NULL;
}

void SASLBindPlugin::removeFeature( IXmppStream *AXmppStream )
{
  SASLBind *saslBind = (SASLBind *)getFeature(AXmppStream->jid());
  if (saslBind)
  {
    disconnect(saslBind,SIGNAL(destroyed(QObject *)),this,SLOT(onSASLBindDestroyed(QObject *)));
    FFeatures.removeAt(FFeatures.indexOf(saslBind));
    AXmppStream->removeFeature(saslBind);
    delete saslBind;
  }
}

void SASLBindPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = addFeature(AXmppStream); 
  emit featureAdded(feature);
}

void SASLBindPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = getFeature(AXmppStream->jid());
  if (feature)
  {
    emit featureRemoved(feature);
    removeFeature(AXmppStream);
  }
}

void SASLBindPlugin::onSASLBindDestroyed(QObject *AObject)
{
  SASLBind *saslBind = qobject_cast<SASLBind *>(AObject);
  if (FFeatures.contains(saslBind))
    FFeatures.removeAt(FFeatures.indexOf(saslBind));
}

Q_EXPORT_PLUGIN2(SASLBindPlugin, SASLBindPlugin)
