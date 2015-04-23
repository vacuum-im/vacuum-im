#include "iqauthfeaturefactory.h"

#include <definitions/namespaces.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturefactoryorders.h>
#include <utils/logger.h>

IqAuthFeatureFactory::IqAuthFeatureFactory()
{
	FXmppStreamManager = NULL;
}

IqAuthFeatureFactory::~IqAuthFeatureFactory()
{

}

void IqAuthFeatureFactory::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Query Authentication");
	APluginInfo->description = tr("Allow you to log on the Jabber server without support SASL authentication");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool IqAuthFeatureFactory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL;
}

bool IqAuthFeatureFactory::initObjects()
{
	if (FXmppStreamManager)
	{
		FXmppStreamManager->registerXmppFeature(XFO_IQAUTH,NS_FEATURE_IQAUTH);
		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_IQAUTH,this);
	}
	return true;
}

QList<QString> IqAuthFeatureFactory::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_IQAUTH;
}

IXmppFeature *IqAuthFeatureFactory::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_IQAUTH)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"Iq-Auth XMPP stream feature created");
		IXmppFeature *feature = new IqAuthFeature(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void IqAuthFeatureFactory::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"Iq-Auth XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}
