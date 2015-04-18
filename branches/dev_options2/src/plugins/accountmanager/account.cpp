#include "account.h"

#include <QCheckBox>
#include <QTextDocument>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/logger.h>

Account::Account(IXmppStreamManager *AXmppStreamManager, const OptionsNode &AOptionsNode, QObject *AParent) : QObject(AParent)
{
	FXmppStream = NULL;
	FXmppStreamManager = AXmppStreamManager;

	FPasswordDialog = NULL;
	FInvalidPassword = false;
	FOptionsNode = AOptionsNode;

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

QUuid Account::accountId() const
{
	return FOptionsNode.nspace();
}

Jid Account::accountJid() const
{
	Jid jid = FOptionsNode.value("streamJid").toString();
	jid.setResource(resource());
	return jid;
}

Jid Account::streamJid() const
{
	return FXmppStream!=NULL ? FXmppStream->streamJid() : accountJid();
}

bool Account::isActive() const
{
	return FXmppStream!=NULL;
}

void Account::setActive(bool AActive)
{
	if (AActive && FXmppStream==NULL)
	{
		LOG_STRM_INFO(accountJid(),QString("Activating account=%1, id=%2").arg(name(), accountId().toString()));
		
		FXmppStream = FXmppStreamManager->createXmppStream(accountJid());
		onOptionsChanged(FOptionsNode.node("password"));
		onOptionsChanged(FOptionsNode.node("require-encryption"));
		connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()),Qt::QueuedConnection);
		connect(FXmppStream->instance(),SIGNAL(error(const XmppError &)),SLOT(onXmppStreamError(const XmppError &)));
		connect(FXmppStream->instance(),SIGNAL(passwordRequested(bool &)),SLOT(onXmppStreamPasswordRequested(bool &)));

		FXmppStreamManager->setXmppStreamActive(FXmppStream,true);
		emit activeChanged(true);
	}
	else if (!AActive && FXmppStream!=NULL)
	{
		LOG_STRM_INFO(accountJid(),QString("Deactivating account=%1, id=%2").arg(name(), accountId().toString()));
		emit activeChanged(false);

		FXmppStream->abort(XmppError(IERR_XMPPSTREAM_DESTROYED));
		FXmppStreamManager->setXmppStreamActive(FXmppStream,false);
		FXmppStreamManager->destroyXmppStream(FXmppStream);
		FXmppStream = NULL;
	}
}

QString Account::name() const
{
	return FOptionsNode.value("name").toString();
}

void Account::setName(const QString &AName)
{
	FOptionsNode.setValue(AName,"name");
}

QString Account::resource() const
{
	return FOptionsNode.value("resource").toString();
}

void Account::setResource(const QString &AResource)
{
	FOptionsNode.setValue(AResource,"resource");
}

QString Account::password() const
{
	return Options::decrypt(FOptionsNode.value("password").toByteArray()).toString();
}

void Account::setPassword(const QString &APassword)
{
	FOptionsNode.setValue(Options::encrypt(APassword),"password");
}

OptionsNode Account::optionsNode() const
{
	return FOptionsNode;
}

IXmppStream *Account::xmppStream() const
{
	return FXmppStream;
}

void Account::onXmppStreamClosed()
{
	if (FPasswordDialog)
		FPasswordDialog->reject();
	if (FXmppStream)
		FXmppStream->setStreamJid(accountJid());
}

void Account::onXmppStreamError(const XmppError &AError)
{
	if (AError.isSaslError() && AError.toSaslError().conditionCode()==XmppSaslError::EC_NOT_AUTHORIZED)
		FInvalidPassword = true;
	else if (AError.isStanzaError() && AError.toStanzaError().conditionCode()==XmppStanzaError::EC_NOT_AUTHORIZED)
		FInvalidPassword = true;
	else
		FInvalidPassword = false;
}

void Account::onXmppStreamPasswordRequested(bool &AWait)
{
	if (FPasswordDialog==NULL && FXmppStream!=NULL && FXmppStream->isConnected())
	{
		if (FInvalidPassword || FXmppStream->password().isEmpty())
		{
			FPasswordDialog = new PasswordDialog();
			FPasswordDialog->setAttribute(Qt::WA_DeleteOnClose);
			FPasswordDialog->setWindowTitle(tr("Password Request"));
			FPasswordDialog->setLabelText(tr("Enter password for account <b>%1</b>").arg(Qt::escape(name())));
			FPasswordDialog->setPassword(FXmppStream->password());
			FPasswordDialog->setSavePassword(!password().isEmpty());
			connect(FPasswordDialog,SIGNAL(accepted()),SLOT(onPasswordDialogAccepted()));
			connect(FPasswordDialog,SIGNAL(rejected()),SLOT(onPasswordDialogRejected()));
			
			FXmppStream->setKeepAliveTimerActive(false);
			FPasswordDialog->show();

			LOG_STRM_INFO(streamJid(),"Account password dialog shown");
		}
	}
	AWait = FPasswordDialog!=NULL;
}

void Account::onPasswordDialogAccepted()
{
	if (FXmppStream)
	{
		LOG_STRM_INFO(streamJid(),"Account password dialog accepted");
		FXmppStream->setKeepAliveTimerActive(true);
		if (FPasswordDialog->savePassword())
			setPassword(FPasswordDialog->password());
		else
			setPassword(QString::null);
		FXmppStream->setPassword(FPasswordDialog->password());
	}
	FInvalidPassword = false;
	FPasswordDialog = NULL;
}

void Account::onPasswordDialogRejected()
{
	if (FXmppStream)
	{
		LOG_STRM_INFO(streamJid(),"Account password dialog rejected");
		FXmppStream->abort(XmppSaslError(XmppSaslError::EC_NOT_AUTHORIZED));
	}
	FPasswordDialog = NULL;
}

void Account::onOptionsChanged(const OptionsNode &ANode)
{
	if (FOptionsNode.isChildNode(ANode))
	{
		if (FXmppStream)
		{
			if (FOptionsNode.node("password") == ANode)
				FXmppStream->setPassword(password());
			else if (FOptionsNode.node("require-encryption") == ANode)
				FXmppStream->setEncryptionRequired(ANode.value().toBool());
			else if (FXmppStream->isConnected())
				; // Next options can not be applied on connected stream
			else if (FOptionsNode.node("streamJid") == ANode)
				FXmppStream->setStreamJid(accountJid());
			else if (FOptionsNode.node("resource") == ANode)
				FXmppStream->setStreamJid(accountJid());
		}
		emit optionsChanged(ANode);
	}
	else if (ANode.path() == OPV_ACCOUNT_DEFAULTRESOURCE)
	{
		if (FXmppStream && !FXmppStream->isConnected())
		{
			FXmppStream->setStreamJid(accountJid());
		}
	}
}
