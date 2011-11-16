#include "saslbind.h"

SASLBind::SASLBind(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLBind::~SASLBind()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool SASLBind::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="bind")
	{
		FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		if (AStanza.type() == "result")
		{
			Jid streamJid = AStanza.firstElement().firstChild().toElement().text();
			if (streamJid.isValid())
			{
				deleteLater();
				FXmppStream->setStreamJid(streamJid);
				emit finished(false);
			}
			else
			{
				emit error(tr("Invalid XMPP stream JID in SASL bind response"));
			}
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

bool SASLBind::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool SASLBind::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "bind")
	{
		Stanza bind("iq");
		bind.setType("set").setId("bind");
		bind.addElement("bind",NS_FEATURE_BIND);
		if (!FXmppStream->streamJid().resource().isEmpty())
			bind.firstElement("bind",NS_FEATURE_BIND).appendChild(bind.createElement("resource")).appendChild(bind.createTextNode(FXmppStream->streamJid().resource()));
		FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		FXmppStream->sendStanza(bind);
		return true;
	}
	deleteLater();
	return false;
}
