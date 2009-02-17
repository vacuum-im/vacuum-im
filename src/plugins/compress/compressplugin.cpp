#include "compressplugin.h"

CompressPlugin::CompressPlugin()
{
  FXmppStreams = NULL;
}

CompressPlugin::~CompressPlugin()
{

}

void CompressPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Stream Compression");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Stream Compression");
  APluginInfo->uid = COMPRESS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool CompressPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

  return FXmppStreams!=NULL;
}

bool CompressPlugin::initObjects()
{
  ErrorHandler::addErrorItem("unsupported-method", ErrorHandler::CANCEL, 
    ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported compression method"),NS_FEATURE_COMPRESS);

  ErrorHandler::addErrorItem("setup-failed", ErrorHandler::CANCEL, 
    ErrorHandler::NOT_ACCEPTABLE, tr("Compression setup failed"), NS_FEATURE_COMPRESS);

  if (FXmppStreams)
  {
    FXmppStreams->registerFeature(NS_FEATURE_COMPRESS,this);
  }

  return true;
}

IStreamFeature *CompressPlugin::getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_COMPRESS)
  {
    IStreamFeature *feature = FFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new Compression(AXmppStream);
      FFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  return NULL;
}

void CompressPlugin::destroyStreamFeature(IStreamFeature *AFeature)
{
  if (AFeature && FFeatures.value(AFeature->xmppStream()) == AFeature)
  {
    FFeatures.remove(AFeature->xmppStream());
    AFeature->xmppStream()->removeFeature(AFeature);
    emit featureDestroyed(AFeature);
    AFeature->instance()->deleteLater();
  }
}

Q_EXPORT_PLUGIN2(CompressionPlugin, CompressPlugin)
