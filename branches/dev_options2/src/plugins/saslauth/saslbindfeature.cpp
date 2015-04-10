#include "saslbindfeature.h"

#include <QProcess>
#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

SASLBindFeature::SASLBindFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FXmppStream = AXmppStream;
}

SASLBindFeature::~SASLBindFeature()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool SASLBindFeature::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE && AStanza.id()=="bind")
	{
		FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
		if (AStanza.type() == "result")
		{
			Jid bindJid = AStanza.firstElement().firstChild().toElement().text();
			if (!bindJid.isValid() || bindJid.node().isEmpty())
			{
				LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to bind resource, jid=%1: Invalid JID").arg(bindJid.full()));
				emit error(XmppError(IERR_SASL_BIND_INVALID_STREAM_JID));
			}
			else
			{
				LOG_STRM_INFO(FXmppStream->streamJid(),QString("Resource binding finished, jid=%1").arg(bindJid.full()));
				FXmppStream->setStreamJid(bindJid);
				deleteLater();
				emit finished(false);
			}
		}
		else
		{
			XmppStanzaError err(AStanza);
			LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to bind resource: %1").arg(err.condition()));
			emit error(err);
		}
		return true;
	}
	return false;
}

bool SASLBindFeature::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString SASLBindFeature::featureNS() const
{
	return NS_FEATURE_BIND;
}

IXmppStream *SASLBindFeature::xmppStream() const
{
	return FXmppStream;
}

bool SASLBindFeature::start(const QDomElement &AElem)
{
	if (AElem.tagName() == "bind")
	{
		Stanza bind("iq");
		bind.setType("set").setId("bind");
		bind.addElement("bind",NS_FEATURE_BIND);

		QString resource = FXmppStream->streamJid().resource();
		if (!resource.isEmpty())
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
		LOG_STRM_INFO(FXmppStream->streamJid(),QString("Resource binding request sent, resource='%1'").arg(resource));

		return true;
	}
	else
	{
		LOG_STRM_ERROR(FXmppStream->streamJid(),QString("Failed to send resource binding request: Invalid element=%1").arg(AElem.tagName()));
	}
	deleteLater();
	return false;
}
