#include "iqauth.h"

#include <QCryptographicHash>

IqAuth::IqAuth(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

IqAuth::~IqAuth()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool IqAuth::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.id() == "getIqAuth")
		{
			if (AStanza.type() == "result")
			{
				Stanza auth("iq");
				auth.setType("set").setTo(FXmppStream->streamJid().domain()).setId("setIqAuth");
				QDomElement query = auth.addElement("query",NS_JABBER_IQ_AUTH);
				query.appendChild(auth.createElement("username")).appendChild(auth.createTextNode(FXmppStream->streamJid().pNode()));
				query.appendChild(auth.createElement("resource")).appendChild(auth.createTextNode(FXmppStream->streamJid().resource()));

				QDomElement reqElem = AStanza.firstElement("query",NS_JABBER_IQ_AUTH);
				if (!reqElem.firstChildElement("digest").isNull())
				{
					QByteArray shaData = FXmppStream->streamId().toUtf8()+FXmppStream->getSessionPassword().toUtf8();
					QByteArray shaDigest = QCryptographicHash::hash(shaData,QCryptographicHash::Sha1).toHex();
					query.appendChild(auth.createElement("digest")).appendChild(auth.createTextNode(shaDigest.toLower().trimmed()));
					FXmppStream->sendStanza(auth);
				}
				else if (!reqElem.firstChildElement("password").isNull())
				{
					if (FXmppStream->connection()->isEncrypted())
					{
						query.appendChild(auth.createElement("password")).appendChild(auth.createTextNode(FXmppStream->getSessionPassword()));
						FXmppStream->sendStanza(auth);
					}
					else
					{
						emit error(XmppError(IERR_XMPPSTREAM_NOT_SECURE));
					}
				}
			}
			else if (AStanza.type() == "error")
			{
				emit error(XmppStanzaError(AStanza));
			}
			return true;
		}
		else if (AStanza.id() == "setIqAuth")
		{
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			if (AStanza.type() == "result")
			{
				deleteLater();
				emit finished(false);
			}
			else if (AStanza.type() == "error")
			{
				emit error(XmppStanzaError(AStanza));
			}
			return true;
		}
	}
	return false;
}

bool IqAuth::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

QString IqAuth::featureNS() const
{
	return NS_FEATURE_IQAUTH;
}

IXmppStream *IqAuth::xmppStream() const
{
	return FXmppStream;
}

bool IqAuth::start(const QDomElement &AElem)
{
	if (AElem.tagName()=="auth")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			Stanza request("iq");
			request.setType("get").setId("getIqAuth");
			request.addElement("query",NS_JABBER_IQ_AUTH).appendChild(request.createElement("username")).appendChild(request.createTextNode(FXmppStream->streamJid().node()));
			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(request);
			return true;
		}
		else
		{
			emit error(XmppError(IERR_XMPPSTREAM_NOT_SECURE));
		}
	}
	deleteLater();
	return false;
}

//IqAuthPlugin
IqAuthPlugin::IqAuthPlugin()
{
	FXmppStreams = NULL;
}

IqAuthPlugin::~IqAuthPlugin()
{

}

void IqAuthPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Query Authentication");
	APluginInfo->description = tr("Allow you to log on the Jabber server without support SASL authentication");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
}

bool IqAuthPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());

	return FXmppStreams!=NULL;
}

bool IqAuthPlugin::initObjects()
{
	if (FXmppStreams)
	{
		FXmppStreams->registerXmppFeature(XFO_IQAUTH,NS_FEATURE_IQAUTH);
		FXmppStreams->registerXmppFeaturePlugin(XFPO_DEFAULT,NS_FEATURE_IQAUTH,this);
	}
	return true;
}

QList<QString> IqAuthPlugin::xmppFeatures() const
{
	return QList<QString>() << NS_FEATURE_IQAUTH;
}

IXmppFeature *IqAuthPlugin::newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
	if (AFeatureNS == NS_FEATURE_IQAUTH)
	{
		IXmppFeature *feature = new IqAuth(AXmppStream);
		connect(feature->instance(),SIGNAL(featureDestroyed()),SLOT(onFeatureDestroyed()));
		emit featureCreated(feature);
		return feature;
	}
	return NULL;
}

void IqAuthPlugin::onFeatureDestroyed()
{
	IXmppFeature *feature = qobject_cast<IXmppFeature *>(sender());
	if (feature)
		emit featureDestroyed(feature);
}

Q_EXPORT_PLUGIN2(plg_iqauth, IqAuthPlugin)
