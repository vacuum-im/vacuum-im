#include "saslbind.h"

#include <QProcess>

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
			Jid bindJid = AStanza.firstElement().firstChild().toElement().text();
			if (!bindJid.isValid() || bindJid.node().isEmpty())
			{
				emit error(XmppError(IERR_SASL_BIND_INVALID_STREAM_JID));
			}
			else if (bindJid.pBare() == FXmppStream->streamJid().pBare())
			{
				FXmppStream->setStreamJid(bindJid);

				deleteLater();
				emit finished(false);
			}
			else
			{
				Jid sessionJid = FXmppStream->streamJid();
				sessionJid.setResource(bindJid.resource());
				FXmppStream->setStreamJid(sessionJid);

				deleteLater();
				emit finished(false);
			}
		}
		else
		{
			emit error(XmppStanzaError(AStanza));
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

QString SASLBind::featureNS() const
{
	return NS_FEATURE_BIND;
}

IXmppStream *SASLBind::xmppStream() const
{
	return FXmppStream;
}

bool SASLBind::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "bind")
	{
		Stanza bind("iq");
		bind.setType("set").setId("bind");
		bind.addElement("bind",NS_FEATURE_BIND);
		if (!FXmppStream->streamJid().resource().isEmpty())
		{
			QString resource = FXmppStream->streamJid().resource();
			foreach(const QString &env, QProcess::systemEnvironment())
			{
				QList<QString> param_value = env.split("=");
				resource.replace("%"+param_value.value(0)+"%",param_value.value(1));
			}
			bind.firstElement("bind",NS_FEATURE_BIND).appendChild(bind.createElement("resource")).appendChild(bind.createTextNode(resource));
		}
		FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		FXmppStream->sendStanza(bind);
		return true;
	}
	deleteLater();
	return false;
}
