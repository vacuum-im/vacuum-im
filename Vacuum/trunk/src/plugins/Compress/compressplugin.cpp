#include "compressplugin.h"

CompressPlugin::CompressPlugin()
{

}

CompressPlugin::~CompressPlugin()
{

}

void CompressPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Implementation of Stream Compression");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Stream Compression";
  APluginInfo->uid = COMPRESS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool CompressPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *xmppStreams = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (xmppStreams)
  {
    connect(xmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
    connect(xmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
  }
  return xmppStreams!=NULL;
}

bool CompressPlugin::initObjects()
{
  ErrorHandler::addErrorItem(NS_FEATURE_COMPRESS, "unsupported-method", 
    ErrorHandler::CANCEL, ErrorHandler::FEATURE_NOT_IMPLEMENTED, tr("Unsupported compression method"));

  ErrorHandler::addErrorItem(NS_FEATURE_COMPRESS, "setup-failed", 
    ErrorHandler::CANCEL, ErrorHandler::NOT_ACCEPTABLE, tr("Compression setup faild"));

  return true;
}

IStreamFeature *CompressPlugin::addFeature(IXmppStream *AXmppStream)
{
  Compression *compression = (Compression *)getFeature(AXmppStream->jid());
  if (!compression)
  {
    Compression *compression = new Compression(AXmppStream);
    connect(compression,SIGNAL(destroyed(QObject *)),SLOT(onFeatureDestroyed(QObject *)));
    FFeatures.append(compression);
    FCleanupHandler.add(compression);
    AXmppStream->addFeature(compression);
  }
  return compression;
}

IStreamFeature *CompressPlugin::getFeature(const Jid &AStreamJid) const
{
  foreach(Compression *feature, FFeatures)
    if (feature->xmppStream()->jid() == AStreamJid)
      return feature;
  return NULL;
}

void CompressPlugin::removeFeature(IXmppStream *AXmppStream)
{
  Compression *compression = (Compression *)getFeature(AXmppStream->jid());
  if (compression)
  {
    disconnect(compression,SIGNAL(destroyed(QObject *)),this,SLOT(onFeatureDestroyed(QObject *)));
    FFeatures.removeAt(FFeatures.indexOf(compression));
    AXmppStream->removeFeature(compression);
    delete compression;
  }
}

void CompressPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = addFeature(AXmppStream); 
  emit featureAdded(feature);
}

void CompressPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IStreamFeature *feature = getFeature(AXmppStream->jid());
  if (feature)
  {
    emit featureRemoved(feature);
    removeFeature(AXmppStream);
  }
}

void CompressPlugin::onFeatureDestroyed(QObject *AObject)
{
  Compression *compression = qobject_cast<Compression *>(AObject);
  if (FFeatures.contains(compression))
    FFeatures.removeAt(FFeatures.indexOf(compression));
}

Q_EXPORT_PLUGIN2(CompressionPlugin, CompressPlugin)
