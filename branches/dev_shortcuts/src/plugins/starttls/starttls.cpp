#include "starttls.h"

StartTLS::StartTLS(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FConnection = NULL;
	FXmppStream = AXmppStream;
}

StartTLS::~StartTLS()
{
	FXmppStream->removeXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
	emit featureDestroyed();
}

bool StartTLS::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		FXmppStream->removeXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
		if (AStanza.tagName() == "proceed")
		{
			connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
			FConnection->startClientEncryption();
		}
		else if (AStanza.tagName() == "failure")
		{
			emit error(tr("StartTLS negotiation failed"));
		}
		else
		{
			emit error(tr("Wrong StartTLS negotiation response"));
		}
		return true;
	}
	return false;
}

bool StartTLS::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream);
	Q_UNUSED(AStanza);
	Q_UNUSED(AOrder);
	return false;
}

bool StartTLS::start(const QDomElement &AElem)
{
	FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
	if (FConnection && AElem.tagName()=="starttls")
	{
		Stanza request("starttls");
		request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
		FXmppStream->insertXmppStanzaHandler(this,XSHO_XMPP_FEATURE);
		FXmppStream->sendStanza(request);
		return true;
	}
	deleteLater();
	return false;
}

void StartTLS::onConnectionEncrypted()
{
	deleteLater();
	emit finished(true);
}
