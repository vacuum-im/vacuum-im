#include "saslsession.h"

SASLSession::SASLSession(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLSession::~SASLSession()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool SASLSession::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="session")
	{
		if (AStanza.type() == "result")
		{
			deleteLater();
			emit finished(false);
		}
		else
		{
			ErrorHandler err(AStanza.element());
			emit error(err.message());
		}
		return true;
	}
	return false;
}

bool SASLSession::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool SASLSession::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "session")
	{
		Stanza session("iq");
		session.setType("set").setId("session");
		session.addElement("session",NS_FEATURE_SESSION);
		FXmppStream->insertXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(session);
		return true;
	}
	deleteLater();
	return false;
}
