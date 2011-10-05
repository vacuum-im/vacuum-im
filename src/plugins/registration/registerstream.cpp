#include "registerstream.h"

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
							form.fields[userFiled].value = FXmppStream->streamJid().eNode();
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
					}
					else
					{
						emit error(tr("Invalid registration form"));
					}
				}
				else
				{
					Stanza submit("iq");
					submit.setType("set").setId("setReg");
					QDomElement querySubmit = submit.addElement("query",NS_JABBER_REGISTER);
					if (!queryElem.firstChildElement("username").isNull())
						querySubmit.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(FXmppStream->streamJid().eNode()));
					if (!queryElem.firstChildElement("password").isNull())
						querySubmit.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(FXmppStream->getSessionPassword()));
					if (!queryElem.firstChildElement("key").isNull())
						querySubmit.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(AStanza.firstElement("query").attribute("key")));
					FXmppStream->sendStanza(submit);
				}
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
			FXmppStream->removeXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
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
	if (AElem.tagName()=="register")
	{
		if (!xmppStream()->isEncryptionRequired() || xmppStream()->connection()->isEncrypted())
		{
			Stanza reg("iq");
			reg.setType("get").setId("getReg");
			reg.addElement("query",NS_JABBER_REGISTER);
			FXmppStream->insertXmppStanzaHandler(XSHO_XMPP_FEATURE,this);
			FXmppStream->sendStanza(reg);
			return true;
		}
		else
		{
			emit error(tr("Secure connection is not established"));
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
	}
	else
	{
		emit error(tr("Invalid registration dialog"));
	}
	FDialog = NULL;
}

void RegisterStream::onRegisterDialogRejected()
{
	FXmppStream->setKeepAliveTimerActive(true);
	emit error(tr("Registration rejected by user"));
	FDialog = NULL;
}
