#include "saslsession.h"

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
			deleteLater();
			emit finished(false);
		}
		else
		{
			emit error(XmppStanzaError(AStanza));
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
		return true;
	}
	deleteLater();
	return false;
}
