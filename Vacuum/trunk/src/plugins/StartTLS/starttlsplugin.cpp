#include "starttlsplugin.h"

StartTLSPlugin::StartTLSPlugin()
{
  FXmppStreams = NULL;
}

StartTLSPlugin::~StartTLSPlugin()
{

}

void StartTLSPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of StartTLS");
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
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FXmppStreams!=NULL;
}

bool StartTLSPlugin::initObjects()
{
  if (FXmppStreams)
  {
    FXmppStreams->registerFeature(NS_FEATURE_STARTTLS,this);
  }
  return true;
}

IStreamFeature *StartTLSPlugin::getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_STARTTLS)
  {
    IStreamFeature *feature = FFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new StartTLS(AXmppStream);
      FFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  return NULL;
}

void StartTLSPlugin::destroyStreamFeature(IStreamFeature *AFeature)
{
  if (AFeature && FFeatures.value(AFeature->xmppStream()) == AFeature)
  {
    FFeatures.remove(AFeature->xmppStream());
    AFeature->xmppStream()->removeFeature(AFeature);
    emit featureDestroyed(AFeature);
    AFeature->instance()->deleteLater();
  }
}

Q_EXPORT_PLUGIN2(StartTLSPlugin, StartTLSPlugin)
