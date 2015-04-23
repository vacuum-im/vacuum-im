#include "saslfeaturefactory.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <definitions/xmppfeaturefactoryorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

SASLFeatureFactory::SASLFeatureFactory()
{
	FXmppStreamManager = NULL;
}

SASLFeatureFactory::~SASLFeatureFactory()
{

}

void SASLFeatureFactory::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SASL Authentication");
	APluginInfo->description = tr("Allows to log in to Jabber server using SASL authentication");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool SASLFeatureFactory::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
			connect(FXmppStreamManager->instance(),SIGNAL(streamCreated(IXmppStream *)),SLOT(onXmppStreamCreated(IXmppStream *)));
	}

	return FXmppStreamManager!=NULL;
}

bool SASLFeatureFactory::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SASL_AUTH_INVALID_RESPONSE,tr("Wrong SASL authentication response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SASL_BIND_INVALID_STREAM_JID,tr("Invalid XMPP stream JID in SASL bind response"));

	if (FXmppStreamManager)
	{
		FXmppStreamManager->registerXmppFeature(XFO_SASL,NS_FEATURE_SASL);
		FXmppStreamManager->registerXmppFeature(XFO_BIND,NS_FEATURE_BIND);
		FXmppStreamManager->registerXmppFeature(XFO_SESSION,NS_FEATURE_SESSION);

		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_SASL,this);
		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_BIND,this);
		FXmppStreamManager->registerXmppFeatureFactory(XFFO_DEFAULT,NS_FEATURE_SESSION,this);
	}
	return true;
}

bool SASLFeatureFactory::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

bool SASLFeatureFactory::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	if (AOrder==XSHO_SASL_VERSION && AStanza.element().nodeName()=="stream:stream")
	{
		if (!AStanza.element().hasAttribute("version"))
		{
			// GOOGLE HACK - sending xmpp stream version 0.0 for IQ authorization
			QString domain = AXmppStream->streamJid().domain().toLower();
			if (AXmppStream->connection()->isEncrypted() && (domain=="googlemail.com" || domain=="gmail.com"))
				AStanza.element().setAttribute("version","0.0");
			else
				AStanza.element().setAttribute("version","1.0");
		}
	}
	return false;
}

QList<QString> SASLFeatureFactory::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_SASL << NS_FEATURE_BIND << NS_FEATURE_SESSION;
}

IXmppFeature *SASLFeatureFactory::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_SASL)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLAuth XMPP stream feature created");
		IXmppFeature *feature = new SASLAuthFeature(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_BIND)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLBind XMPP stream feature created");
		IXmppFeature *feature = new SASLBindFeature(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_SESSION)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLSession XMPP stream feature created");
		IXmppFeature *feature = new SASLSessionFeature(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void SASLFeatureFactory::onXmppStreamCreated(IXmppStream *AXmppStream)
{
	AXmppStream->insertXmppStanzaHandler(XSHO_SASL_VERSION,this);
}

void SASLFeatureFactory::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		if (qobject_cast<SASLAuthFeature *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLAuth XMPP stream feature destroyed");
		else if (qobject_cast<SASLBindFeature *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLBind XMPP stream feature destroyed");
		else if (qobject_cast<SASLSessionFeature *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLSession XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}
