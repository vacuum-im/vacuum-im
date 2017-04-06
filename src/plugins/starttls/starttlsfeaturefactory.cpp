#include "starttlsfeaturefactory.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturefactoryorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

StartTLSFeatureFactory::StartTLSFeatureFactory()
{
	FXmppStreamManager = NULL;
}

StartTLSFeatureFactory::~StartTLSFeatureFactory()
{

}

void StartTLSFeatureFactory::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("StartTLS");
	APluginInfo->description = tr("Allows to establish a secure connection to the server after connecting");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool StartTLSFeatureFactory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL;
}

bool StartTLSFeatureFactory::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_NOT_STARTED,tr("Failed to begin StartTLS handshake"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_INVALID_RESPONCE,tr("Wrong StartTLS negotiation response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_STARTTLS_NEGOTIATION_FAILED,tr("StartTLS negotiation failed"));

	if (FXmppStreamManager)
	{
		FXmppStreamManager->registerXmppFeature(XFO_STARTTLS,NS_FEATURE_STARTTLS);
		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_STARTTLS,this);
	}
	return true;
}

QList<QString> StartTLSFeatureFactory::xmppFeatures() const
{
	static const QList<QString> features = QList<QString>() << NS_FEATURE_STARTTLS;
	return features;
}

IXmppFeature *StartTLSFeatureFactory::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_STARTTLS)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"StartTLS XMPP stream feature created");
		IXmppFeature *feature = new StartTLSFeature(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void StartTLSFeatureFactory::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		LOG_STRM_INFO(feature->xmppStream()->streamJid(),"StartTLS XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}
