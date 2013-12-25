#include "saslsession.h"

#include <definitions/namespaces.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

SASLSession::SASLSession(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLSession::~SASLSession()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool SASLSession::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="session")
	{
		if (AStanza.type() == "result")
		{
			LOG_STRM_INFO(FXmppStream->streamJid(),"Session started");
			deleteLater();
			emit finished(false);
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_INFO(FXmppStream->streamJid(),QString("Failed to start session: %1").arg(err.condition()));
			emit error(err);
		}
		return true;
	}
	return false;
}

bool SASLSession::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString SASLSession::featureNS() const
{
	return NS_FEATURE_SESSION;
}

IXmppStream *SASLSession::xmppStream() const
{
	return FXmppStream;
}

bool SASLSession::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "session")
	{
		Stanza session("iq");
		session.setType("set").setId("session");
		session.addElement("session",NS_FEATURE_SESSION);
		FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		FXmppStream->sendStanza(session);
		LOG_STRM_INFO(FXmppStream->streamJid(),"Session start request sent");
		return true;
	}
	else
	{
		LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to start session: Invalid element=%1").arg(AElem.tagName()));
	}
	deleteLater();
	return false;
}
