#include "starttls.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/stanza.h>
#include <utils/logger.h>

StartTLS::StartTLS(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FConnection = NULL;
	FXmppStream = AXmppStream;
}

StartTLS::~StartTLS()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool StartTLS::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		if (AStanza.tagName() == "proceed")
		{
			LOG_STRM_INFO(FXmppStream->streamJid(),"Starting StartTLS encryption");
			connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
			FConnection->startClientEncryption();
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

bool StartTLS::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString StartTLS::featureNS() const
{
	return NS_FEATURE_STARTTLS;
}

IXmppStream *StartTLS::xmppStream() const
{
	return FXmppStream;
}

bool StartTLS::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "starttls")
	{
		FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
		if (FConnection)
		{
			Stanza request("starttls");
			request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(request);
			LOG_STRM_INFO(FXmppStream->streamJid(),"StartTLS negotiation request sent");
			return true;
		}
		else
		{
			LOG_STRM_ERROR(FXmppStream->streamJid(),"Failed to send StartTLS negotiation request: Unsupported connection instance");
		}
	}
	else
	{
		LOG_STRM_ERROR(FXmppStream->streamJid(),QString("Failed to send StartTLS negotiation request: Invalid element=%1").arg(AElem.tagName()));
	}
	deleteLater();
	return false;
}

void StartTLS::onConnectionEncrypted()
{
	LOG_STRM_INFO(FXmppStream->streamJid(),"StartTLS encryption established");
	deleteLater();
	emit finished(true);
}
