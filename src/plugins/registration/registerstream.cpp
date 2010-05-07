#include "registerstream.h"

RegisterStream::RegisterStream(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

RegisterStream::~RegisterStream()
{
	FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool RegisterStream::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.id() == "getReg")
		{
			if (AStanza.type() == "result")
			{
				Stanza submit("iq");
				submit.setType("set").setId("setReg");
				QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
				query.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(FXmppStream->streamJid().eNode()));
				query.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(FXmppStream->password()));
				query.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(AStanza.firstElement("query").attribute("key")));
				FXmppStream->sendStanza(submit);
			}
			else if (AStanza.type() == "error")
			{
				ErrorHandler err(AStanza.element());
				emit error(err.message());
			}
			return true;
		}
		else if (AStanza.id() == "setReg")
		{
			FXmppStream->removeXmppStanzaHandler(this, XSHO_XMPP_FEATURE);
			if (AStanza.type() == "result")
			{
				deleteLater();
				emit finished(false);
			}
			else if (AStanza.type() == "error")
			{
				ErrorHandler err(AStanza.element());
				emit error(err.message());
			}
			return true;
		}
	}
	return false;
}

bool RegisterStream::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool RegisterStream::start(const QDomElement &AElem)
{
	if (AElem.tagName()=="register" && !FXmppStream->password().isEmpty())
	{
		Stanza reg("iq");
		reg.setType("get").setId("getReg");
		reg.addElement("query",NS_JABBER_REGISTER);
		FXmppStream->insertXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(reg);
		return true;
	}
	deleteLater();
	return false;
}
