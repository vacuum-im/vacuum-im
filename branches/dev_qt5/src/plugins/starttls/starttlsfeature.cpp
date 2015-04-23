#include "starttlsfeature.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/stanza.h>
#include <utils/logger.h>

StartTLSFeature::StartTLSFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

StartTLSFeature::~StartTLSFeature()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool StartTLSFeature::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		if (AStanza.tagName() == "proceed")
		{
			if (FXmppStream->connection()->startEncryption())
			{
				LOG_STRM_INFO(FXmppStream->streamJid(),"Starting StartTLS encryption");
				connect(FXmppStream->connection()->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
			}
			else
			{
				LOG_STRM_ERROR(FXmppStream->streamJid(),"Failed to negotiate StartTLS encryption: Handshake not started");
				emit error(XmppError(IERR_STARTTLS_NOT_STARTED));
			}
		}
		else if (AStanza.tagName() == "failure")
		{
			LOG_STRM_WARNING(FXmppStream->streamJid(),"Failed to negotiate StartTLS encryption: Negotiation failed");
			emit error(XmppError(IERR_STARTTLS_NEGOTIATION_FAILED));
		}
		else
		{
			LOG_STRM_WARNING(FXmppStream->streamJid(),"Failed to negotiate StartTLS encryption: Invalid responce");
			emit error(XmppError(IERR_STARTTLS_INVALID_RESPONCE));
		}
		return true;
	}
	return false;
}

bool StartTLSFeature::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString StartTLSFeature::featureNS() const
{
	return NS_FEATURE_STARTTLS;
}

IXmppStream *StartTLSFeature::xmppStream() const
{
	return FXmppStream;
}

bool StartTLSFeature::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "starttls")
	{
		if (FXmppStream->connection()->isEncryptionSupported() && !FXmppStream->connection()->isEncrypted())
		{
			Stanza request("starttls");
			request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(request);
			LOG_STRM_INFO(FXmppStream->streamJid(),"StartTLS negotiation request sent");
			return true;
		}
		else if (!FXmppStream->connection()->isEncryptionSupported())
		{
			LOG_STRM_WARNING(FXmppStream->streamJid(),"Failed to send StartTLS negotiation request: Encryption is not supported");
		}
	}
	else
	{
		LOG_STRM_ERROR(FXmppStream->streamJid(),QString("Failed to send StartTLS negotiation request: Invalid element=%1").arg(AElem.tagName()));
	}
	deleteLater();
	return false;
}

void StartTLSFeature::onConnectionEncrypted()
{
	LOG_STRM_INFO(FXmppStream->streamJid(),"StartTLS encryption established");
	deleteLater();
	emit finished(true);
}
