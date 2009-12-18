#include "starttlsplugin.h"

StartTLSPlugin::StartTLSPlugin()
{
  FXmppStreams = NULL;
}

StartTLSPlugin::~StartTLSPlugin()
{

}

void StartTLSPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("StartTLS");
  APluginInfo->description = tr("Allows to establish a secure connection to the server after connecting");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(DEFAULTCONNECTION_UUID);
}

bool StartTLSPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
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

IStreamFeature *StartTLSPlugin::newStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
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

Q_EXPORT_PLUGIN2(plg_starttls, StartTLSPlugin)
