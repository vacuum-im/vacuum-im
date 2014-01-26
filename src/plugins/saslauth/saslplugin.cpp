#include "saslplugin.h"

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

bool SASLPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(created(IXmppStream *)),SLOT(onXmppStreamCreated(IXmppStream *)));
		}
	}

	return FXmppStreams!=NULL;
}

bool SASLPlugin::initObjects()
{
	XmppError::registerErrorString(NS_FEATURE_SASL,"aborted",tr("Authorization aborted"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"account-disabled",tr("Account disabled"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"credentials-expired",tr("Credentials expired"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"encryption-required",tr("Encryption required"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"incorrect-encoding",tr("Incorrect encoding"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"invalid-authzid",tr("Invalid authorization id"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"invalid-mechanism",tr("Invalid mechanism"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"malformed-request",tr("Malformed request"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"mechanism-too-weak",tr("Mechanism is too weak"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"not-authorized",tr("Not authorized"));
	XmppError::registerErrorString(NS_FEATURE_SASL,"temporary-auth-failure",tr("Temporary authentication failure"));

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
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
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
		IXmppFeature *feature = new SASLAuth(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_BIND)
	{
		IXmppFeature *feature = new SASLBind(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	else if (AFeatureNS == NS_FEATURE_SESSION)
	{
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
		emit featureDestroyed(feature);
}

Q_EXPORT_PLUGIN2(plg_sasl, SASLPlugin)
