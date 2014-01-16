#include "starttlsplugin.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

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
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool StartTLSPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
	}

	return FXmppStreams!=NULL;
}

bool StartTLSPlugin::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_NOT_STARTED,tr("Failed to begin StartTLS handshake"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_INVALID_RESPONCE,tr("Wrong StartTLS negotiation response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_NEGOTIATION_FAILED,tr("StartTLS negotiation failed"));

	if (FXmppStreams)
	{
		FXmppStreams->registerXmppFeature(XFO_STARTTLS,NS_FEATURE_STARTTLS);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_STARTTLS,this);
	}
	return true;
}

QList<QString> StartTLSPlugin::xmppFeatures() const
{
	static const QList<QString> features = QList<QString>() << NS_FEATURE_STARTTLS;
	return features;
}

IXmppFeature *StartTLSPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_STARTTLS)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"StartTLS XMPP stream feature created");
		IXmppFeature *feature = new StartTLS(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void StartTLSPlugin::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"StartTLS XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}
