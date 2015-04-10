#include "account.h"

#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/logger.h>

Account::Account(IXmppStreamManager *AXmppStreams, const OptionsNode &AOptionsNode, QObject *AParent) : QObject(AParent)
{
	FXmppStream = NULL;
	FXmppStreamManager = AXmppStreams;
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
		
		FXmppStream = FXmppStreamManager->createXmppStream(streamJid());
		connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()),Qt::QueuedConnection);
		onXmppStreamClosed();

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
	if (FXmppStream)
	{
		FXmppStream->setStreamJid(accountJid());
		FXmppStream->setPassword(password());
		FXmppStream->setEncryptionRequired(FOptionsNode.node("require-encryption").value().toBool());
	}
}

void Account::onOptionsChanged(const OptionsNode &ANode)
{
	if (FOptionsNode.isChildNode(ANode))
	{
		if (FXmppStream && !FXmppStream->isConnected())
		{
			if (FOptionsNode.node("streamJid") == ANode)
				FXmppStream->setStreamJid(accountJid());
			else if (FOptionsNode.node("resource") == ANode)
				FXmppStream->setStreamJid(accountJid());
			else if (FOptionsNode.node("password") == ANode)
				FXmppStream->setPassword(password());
			else if (FOptionsNode.node("require-encryption") == ANode)
				FXmppStream->setEncryptionRequired(ANode.value().toBool());
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
