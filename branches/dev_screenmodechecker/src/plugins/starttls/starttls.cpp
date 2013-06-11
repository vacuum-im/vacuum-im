#include "starttls.h"

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
			connect(FConnection->instance(),SIGNAL(encrypted()),SLOT(onConnectionEncrypted()));
			FConnection->startClientEncryption();
		}
		else if (AStanza.tagName() == "failure")
		{
			emit error(XmppError(IERR_STARTTLS_NEGOTIATION_FAILED));
		}
		else
		{
			emit error(XmppError(IERR_STARTTLS_INVALID_RESPONCE));
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
	FConnection = qobject_cast<IDefaultConnection *>(FXmppStream->connection()->instance());
	if (FConnection && AElem.tagName()=="starttls")
	{
		Stanza request("starttls");
		request.setAttribute("xmlns",NS_FEATURE_STARTTLS);
		FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
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
