#include "starttlsplugin.h"

StartTLSPlugin::StartTLSPlugin()
  : QObject()
{
  FXmppStreams = NULL;
}

StartTLSPlugin::~StartTLSPlugin()
{

}

void StartTLSPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of StartTLS (XMPP-Core)");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "StartTLS";
  APluginInfo->uid = STARTTLS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(DEFAULTCONNECTION_UUID);
}

bool StartTLSPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }
  return FXmppStreams != NULL;
}

//StartTLS
IStreamFeature *StartTLSPlugin::addFeature(IXmppStream *AXmppStream)
{
  StartTLS *feature = (StartTLS *)getFeature(AXmppStream->jid());
  if (!feature)
  {
    feature = new StartTLS(AXmppStream);
    connect(feature,SIGNAL(destroyed(QObject *)),SLOT(onFeatureDestroyed(QObject *)));
    FFeatures.append(feature);
    FCleanupHandler.add(feature);
    AXmppStream->addFeature(feature);
  }
  return feature;
}

IStreamFeature *StartTLSPlugin::getFeature(const Jid &AStreamJid) const
{
  foreach(StartTLS *feature, FFeatures)
    if (feature->xmppStream()->jid() == AStreamJid)
      return feature;
  return NULL;
}

void StartTLSPlugin::removeFeature(IXmppStream *AXmppStream)
{
  StartTLS *feature = (StartTLS *)getFeature(AXmppStream->jid());
  if (feature)
  {
    disconnect(feature,SIGNAL(destroyed(QObject *)),this,SLOT(onFeatureDestroyed(QObject *)));
    FFeatures.removeAt(FFeatures.indexOf(feature));
    AXmppStream->removeFeature(feature);
    delete feature;
  }
}

void StartTLSPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  emit featureAdded(addFeature(AXmppStream));
}

void StartTLSPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = getFeature(AXmppStream->jid());
  if (feature)
  {
    emit featureRemoved(feature);
    removeFeature(AXmppStream);
  }
}

void StartTLSPlugin::onFeatureDestroyed(QObject *AObject)
{
  StartTLS *feature = qobject_cast<StartTLS *>(AObject);
  if (FFeatures.contains(feature))
    FFeatures.removeAt(FFeatures.indexOf(feature));
}

Q_EXPORT_PLUGIN2(StartTLSPlugin, StartTLSPlugin)
