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
	APluginInfo->name = tr("Stream Compression");
	APluginInfo->description = tr("Allows to compress a stream of messages sent and received from the server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool CompressPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
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
		FXmppStreams->registerXmppFeature(this,NS_FEATURE_COMPRESS,XFO_COMPRESS);
	}

	return true;
}

IXmppFeature *CompressPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_COMPRESS)
	{
		IXmppFeature *feature = new Compression(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void CompressPlugin::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
		emit featureDestroyed(feature);
}

Q_EXPORT_PLUGIN2(plg_compress, CompressPlugin)
