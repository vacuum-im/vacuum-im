#include "iqauthfeature.h"

#include <QCryptographicHash>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/logger.h>

IqAuthFeature::IqAuthFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FPasswordRequested = false;

	FXmppStream = AXmppStream;
	connect(FXmppStream->instance(),SIGNAL(passwordProvided(const QString &)),SLOT(onXmppStreamPasswordProvided(const QString &)));
}

IqAuthFeature::~IqAuthFeature()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool IqAuthFeature::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
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
					QByteArray shaData = FXmppStream->streamId().toUtf8()+FXmppStream->password().toUtf8();
					QByteArray shaDigest = QCryptographicHash::hash(shaData,QCryptographicHash::Sha1).toHex();
					query.appendChild(auth.createElement("digest")).appendChild(auth.createTextNode(shaDigest.toLower().trimmed()));
					FXmppStream->sendStanza(auth);
					LOG_STRM_INFO(AXmppStream->streamJid(),"Username and encrypted password sent");
				}
				else if (!reqElem.firstChildElement("password").isNull())
				{
					if (FXmppStream->connection()->isEncrypted())
					{
						query.appendChild(auth.createElement("password")).appendChild(auth.createTextNode(FXmppStream->password()));
						FXmppStream->sendStanza(auth);
						LOG_STRM_INFO(AXmppStream->streamJid(),"Username and plain text password sent");
					}
					else
					{
						LOG_STRM_ERROR(AXmppStream->streamJid(),"Failed to send username and plain text password: Connection not encrypted");
						emit error(XmppError(IERR_XMPPSTREAM_NOT_SECURE));
					}
				}
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_ERROR(AXmppStream->streamJid(),QString("Failed to receive authentication initialization: %1").arg(err.condition()));
				emit error(err);
			}
			return true;
		}
		else if (AStanza.id() == "setIqAuth")
		{
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			if (AStanza.type() == "result")
			{
				LOG_STRM_INFO(AXmppStream->streamJid(),"Username and password accepted");
				deleteLater();
				emit finished(false);
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_WARNING(AXmppStream->streamJid(),QString("Username and password rejected: %1").arg(err.condition()));
				emit error(XmppStanzaError(AStanza));
			}
			return true;
		}
	}
	return false;
}

bool IqAuthFeature::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString IqAuthFeature::featureNS() const
{
	return NS_FEATURE_IQAUTH;
}

IXmppStream *IqAuthFeature::xmppStream() const
{
	return FXmppStream;
}

bool IqAuthFeature::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "auth")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			if (!FXmppStream->requestPassword())
				sendAuthRequest();
			else
				FPasswordRequested = true;
			return true;
		}
		else
		{
			XmppError err(IERR_XMPPSTREAM_NOT_SECURE);
			LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to send authentication request: %1").arg(err.condition()));
			emit error(err);
		}
	}
	deleteLater();
	return false;
}

void IqAuthFeature::sendAuthRequest()
{
	Stanza request("iq");
	request.setType("get").setId("getIqAuth");
	request.addElement("query",NS_JABBER_IQ_AUTH).appendChild(request.createElement("username")).appendChild(request.createTextNode(FXmppStream->streamJid().pNode()));

	FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	FXmppStream->sendStanza(request);

	LOG_STRM_INFO(FXmppStream->streamJid(),"Authentication initialization request sent");
}

void IqAuthFeature::onXmppStreamPasswordProvided(const QString &APassword)
{
	Q_UNUSED(APassword);
	if (FPasswordRequested)
	{
		sendAuthRequest();
		FPasswordRequested = false;
	}
}
