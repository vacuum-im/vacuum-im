#include "saslplugin.h"

#include <definitions/namespaces.h>
#include <definitions/xmpperrors.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppfeatureorders.h>
#include <definitions/xmppfeaturepluginorders.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/logger.h>

SASLPlugin::SASLPlugin()
{
	FXmppStreams = NULL;
}

SASLPlugin::~SASLPlugin()
{

}

void SASLPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("SASL Authentication");
	APluginInfo->description = tr("Allows to log in to Jabber server using SASL authentication");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool SASLPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
			connect(FXmppStreams->instance(),SIGNAL(created(IXmppStream *)),SLOT(onXmppStreamCreated(IXmppStream *)));
		else
			LOG_WARNING("Failed to load required interface: IXmppStreams");
	}

	return FXmppStreams!=NULL;
}

bool SASLPlugin::initObjects()
{
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_ABORTED,tr("Authorization aborted"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_ACCOUNT_DISABLED,tr("Account disabled"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_CREDENTIALS_EXPIRED,tr("Credentials expired"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_ENCRYPTION_REQUIRED,tr("Encryption required"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_INCORRECT_ENCODING,tr("Incorrect encoding"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_INVALID_AUTHZID,tr("Invalid authorization id"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_INVALID_MECHANISM,tr("Invalid mechanism"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_MAILFORMED_REQUEST,tr("Malformed request"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_MECHANISM_TOO_WEAK,tr("Mechanism is too weak"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_NOT_AUTHORIZED,tr("Not authorized"));
	XmppError::registerError(NS_FEATURE_SASL,XERR_SASL_TEMPORARY_AUTH_FAILURE,tr("Temporary authentication failure"));

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SASL_AUTH_INVALID_RESPONCE,tr("Wrong SASL authentication response"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_SASL_BIND_INVALID_STREAM_JID,tr("Invalid XMPP stream JID in SASL bind response"));

	if (FXmppStreams)
	{
		FXmppStreams->registerXmppFeature(XFO_SASL,NS_FEATURE_SASL);
		FXmppStreams->registerXmppFeature(XFO_BIND,NS_FEATURE_BIND);
		FXmppStreams->registerXmppFeature(XFO_SESSION,NS_FEATURE_SESSION);

		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_SASL,this);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_BIND,this);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_SESSION,this);
	}
	return true;
}

bool SASLPlugin::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

bool SASLPlugin::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
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

QList<QString> SASLPlugin::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_SASL << NS_FEATURE_BIND << NS_FEATURE_SESSION;
}

IXmppFeature *SASLPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_SASL)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLAuth XMPP stream feature created");
		IXmppFeature *feature = new SASLAuth(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_BIND)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLBind XMPP stream feature created");
		IXmppFeature *feature = new SASLBind(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_SESSION)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"SASLSession XMPP stream feature created");
		IXmppFeature *feature = new SASLSession(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void SASLPlugin::onXmppStreamCreated(IXmppStream *AXmppStream)
{
	AXmppStream->insertXmppStanzaHandler(XSHO_SASL_VERSION,this);
}

void SASLPlugin::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
	{
		if (qobject_cast<SASLAuth *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLAuth XMPP stream feature destroyed");
		else if (qobject_cast<SASLBind *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLBind XMPP stream feature destroyed");
		else if (qobject_cast<SASLSession *>(feature->instance()))
			LOG_STRM_INFO(feature->xmppStream()->streamJid(),"SASLSession XMPP stream feature destroyed");
		emit featureDestroyed(feature);
	}
}

Q_EXPORT_PLUGIN2(plg_sasl, SASLPlugin)
