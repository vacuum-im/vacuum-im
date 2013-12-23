#include "registerstream.h"

#include <definitions/namespaces.h>
#include <definitions/internalerrors.h>
#include <definitions/statisticsparams.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/stanza.h>
#include <utils/logger.h>

RegisterStream::RegisterStream(IDataForms *ADataForms, IXmppStream *AXmppStream) : QObject(AXmppStream->instance())
{
	FDialog = NULL;
	FDataForms = ADataForms;
	FXmppStream = AXmppStream;
	connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()));
}

RegisterStream::~RegisterStream()
{
	FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
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
				LOG_STRM_INFO(AXmppStream->streamJid(),"Account registration fileds loaded");

				QDomElement queryElem = AStanza.firstElement("query",NS_JABBER_REGISTER);
				QDomElement formElem = Stanza::findElement(queryElem,"x",NS_JABBER_DATA);
				if (FDataForms && !formElem.isNull())
				{
					IDataForm form = FDataForms->dataForm(formElem);
					if (FDataForms->isFormValid(form))
					{
						int userFiled = FDataForms->fieldIndex("username",form.fields);
						if (userFiled >= 0)
						{
							form.fields[userFiled].value = FXmppStream->streamJid().node();
							form.fields[userFiled].type = DATAFIELD_TYPE_HIDDEN;
						}

						int passFiled = FDataForms->fieldIndex("password",form.fields);
						if (passFiled >= 0)
						{
							form.fields[passFiled].value = FXmppStream->getSessionPassword();
							form.fields[passFiled].type = DATAFIELD_TYPE_HIDDEN;
						}

						FDialog = FDataForms->dialogWidget(form,NULL);
						FDialog->setAllowInvalid(false);
						FDialog->instance()->setWindowTitle(tr("Registration on %1").arg(FXmppStream->streamJid().domain()));
						connect(FDialog->instance(),SIGNAL(accepted()),SLOT(onRegisterDialogAccepred()));
						connect(FDialog->instance(),SIGNAL(rejected()),SLOT(onRegisterDialogRejected()));
						WidgetManager::showActivateRaiseWindow(FDialog->instance());
						FXmppStream->setKeepAliveTimerActive(false);

						LOG_STRM_INFO(AXmppStream->streamJid(),"Account registration form dialog shown");
					}
					else
					{
						LOG_STRM_WARNING(AXmppStream->streamJid(),"Failed to register new account on server: Invalid registration form received");
						emit error(XmppError(IERR_REGISTER_INVALID_FORM));
					}
				}
				else
				{
					Stanza submit("iq");
					submit.setType("set").setId("setReg");
					QDomElement querySubmit = submit.addElement("query",NS_JABBER_REGISTER);
					if (!queryElem.firstChildElement("username").isNull())
						querySubmit.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(FXmppStream->streamJid().node()));
					if (!queryElem.firstChildElement("password").isNull())
						querySubmit.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(FXmppStream->getSessionPassword()));
					if (!queryElem.firstChildElement("key").isNull())
						querySubmit.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(AStanza.firstElement("query").attribute("key")));
					FXmppStream->sendStanza(submit);

					LOG_STRM_INFO(AXmppStream->streamJid(),"Account registration submit request sent");
				}
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_WARNING(AXmppStream->streamJid(),QString("Failed to load account registration fields: %1").arg(err.condition()));
				emit error(err);
			}
			return true;
		}
		else if (AStanza.id() == "setReg")
		{
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			if (AStanza.type() == "result")
			{
				REPORT_EVENT(SEVP_REGISTRATION_STREAM_SUCCESS,1);
				LOG_STRM_INFO(AXmppStream->streamJid(),"Account registration submit accepted");
				deleteLater();
				emit finished(false);
			}
			else
			{
				XmppStanzaError err(AStanza);
				LOG_STRM_WARNING(AXmppStream->streamJid(),QString("Account registration submit rejected: %1").arg(err.condition()));
				emit error(err);
			}
			return true;
		}
	}
	return false;
}

bool RegisterStream::xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder)
{
	Q_UNUSED(AXmppStream); Q_UNUSED(AStanza); Q_UNUSED(AOrder);
	return false;
}

QString RegisterStream::featureNS() const
{
	return NS_FEATURE_REGISTER;
}

IXmppStream *RegisterStream::xmppStream() const
{
	return FXmppStream;
}

bool RegisterStream::start(const QDomElement &AElem)
{
	if (AElem.tagName()=="register")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			Stanza request("iq");
			request.setType("get").setId("getReg");
			request.addElement("query",NS_JABBER_REGISTER);
			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(request);

			REPORT_EVENT(SEVP_REGISTRATION_STREAM_BEGIN,1);
			LOG_STRM_INFO(FXmppStream->streamJid(),"Account registration fields request sent");

			return true;
		}
		else
		{
			LOG_STRM_WARNING(FXmppStream->streamJid(),QString("Failed to register new account on server: Connection not secure"));
			emit error(XmppError(IERR_XMPPSTREAM_NOT_SECURE));
		}
	}
	deleteLater();
	return false;
}

void RegisterStream::onXmppStreamClosed()
{
	if (FDialog)
	{
		FDialog->instance()->close();
		FDialog = NULL;
	}
}

void RegisterStream::onRegisterDialogAccepred()
{
	FXmppStream->setKeepAliveTimerActive(true);
	if (FDialog)
	{
		Stanza submit("iq");
		submit.setType("set").setId("setReg");
		QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
		FDataForms->xmlForm(FDataForms->dataSubmit(FDialog->formWidget()->userDataForm()),query);
		FXmppStream->sendStanza(submit);

		LOG_STRM_INFO(FXmppStream->streamJid(),"Account registration submit request sent");
	}
	else
	{
		LOG_STRM_WARNING(FXmppStream->streamJid(),"Account registration form dialog destroyed");
		emit error(XmppError(IERR_REGISTER_INVALID_DIALOG));
	}
	FDialog = NULL;
}

void RegisterStream::onRegisterDialogRejected()
{
	LOG_STRM_INFO(FXmppStream->streamJid(),"Account registration terminated by user");
	FXmppStream->setKeepAliveTimerActive(true);
	emit error(XmppError(IERR_REGISTER_REJECTED_BY_USER));
	FDialog = NULL;
}
