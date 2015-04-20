#include "registerfeature.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/statisticsparams.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/pluginhelper.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>
#include "registration.h"

#define FIELDS_STANZA_ID   "__GetReg__"
#define SUBMIT_STANZA_ID   "__SetReg__"

RegisterFeature::RegisterFeature(IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FFinished = false;
	FXmppStream = AXmppStream;
	FRegistration = qobject_cast<Registration *>(PluginHelper::pluginInstance<IRegistration>()->instance());
}

RegisterFeature::~RegisterFeature()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
	emit featureDestroyed();
}

bool RegisterFeature::xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	if (AXmppStream==FXmppStream && AOrder==XSHO_XMPP_FEATURE)
	{
		if (AStanza.id() == FIELDS_STANZA_ID)
		{
			if (AStanza.type() == "result")
			{
				QDomElement queryElem = AStanza.firstElement("query",NS_JABBER_REGISTER);
				IRegisterFields fields = FRegistration!=NULL ? FRegistration->readFields(FXmppStream->streamJid().domain(),queryElem) : IRegisterFields();
				if (fields.fieldMask > 0)
				{
					LOG_INFO(QString("XMPP account registration fields loaded, server=%1").arg(FXmppStream->streamJid().pDomain()));
					FXmppStream->setKeepAliveTimerActive(false);
					emit registerFields(fields);
				}
				else
				{
					XmppError err(IERR_REGISTER_INVALID_FIELDS);
					LOG_WARNING(QString("Failed to load XMPP account registration fields, server=%1: %2").arg(FXmppStream->streamJid().pDomain(),err.condition()));
					emit error(XmppError(IERR_REGISTER_INVALID_FIELDS));
				}
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_WARNING(QString("Failed to load XMPP account registration fields, server=%1: %2").arg(FXmppStream->streamJid().pDomain(),err.condition()));
				emit error(err);
			}
			return true;
		}
		else if (AStanza.id() == SUBMIT_STANZA_ID)
		{
			if (AStanza.type() == "result")
			{
				REPORT_EVENT(SEVP_REGISTRATION_STREAM_SUCCESS,1);
				LOG_INFO(QString("XMPP account registration submit accepted, server=%1").arg(FXmppStream->streamJid().pDomain()));

				FFinished = true;
				FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);

				emit finished(false);
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_WARNING(QString("XMPP account registration submit rejected, server=%1: %2").arg(FXmppStream->streamJid().pDomain(),err.condition()));
				emit error(err);
			}
			return true;
		}
	}
	return false;
}

bool RegisterFeature::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString RegisterFeature::featureNS() const
{
	return NS_FEATURE_REGISTER;
}

IXmppStream *RegisterFeature::xmppStream() const
{
	return FXmppStream;
}

bool RegisterFeature::start(const QDomElement &AElem)
{
	if (AElem.tagName()=="register" && AElem.namespaceURI()==NS_FEATURE_REGISTER)
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			Stanza request("iq");
			request.setType("get").setId(FIELDS_STANZA_ID);
			request.addElement("query",NS_JABBER_REGISTER);

			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(request);

			REPORT_EVENT(SEVP_REGISTRATION_STREAM_BEGIN,1);
			LOG_INFO(QString("XMPP account registration feature started, server=%1").arg(FXmppStream->streamJid().pDomain()));

			return true;
		}
		else
		{
			XmppError err(IERR_XMPPSTREAM_NOT_SECURE);
			LOG_WARNING(QString("Failed to start XMPP account registration feature, server=%1: %2").arg(FXmppStream->streamJid().pDomain(),err.condition()));
			emit error(err);
		}
	}
	return false;
}

bool RegisterFeature::isFinished() const
{
	return FFinished;
}

IRegisterSubmit RegisterFeature::sentSubmit() const
{
	return FSubmit;
}

bool RegisterFeature::sendSubmit(const IRegisterSubmit &ASubmit)
{
	if (FXmppStream->isConnected())
	{
		Stanza request("iq");
		request.setTo(ASubmit.serviceJid.full()).setType("set").setId(SUBMIT_STANZA_ID);

		QDomElement queryElem = request.addElement("query",NS_JABBER_REGISTER);
		FRegistration->writeSubmit(queryElem,ASubmit);

		FSubmit = ASubmit;
		FXmppStream->sendStanza(request);
		FXmppStream->setKeepAliveTimerActive(true);

		LOG_INFO(QString("XMPP account registration submit sent, server=%1").arg(FXmppStream->streamJid().pDomain()));
		return true;
	}
	else
	{
		LOG_ERROR(QString("Failed to send XMPP account registration submit, server=%1: Stream is not connected").arg(FXmppStream->streamJid().pDomain()));
	}
	return true;
}
