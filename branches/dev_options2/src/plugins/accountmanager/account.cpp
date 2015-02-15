#include "account.h"

#include <definitions/optionvalues.h>
#include <utils/logger.h>

Account::Account(IXmppStreams *AXmppStreams, const OptionsNode &AOptionsNode, QObject *AParent) : QObject(AParent)
{
	FXmppStreams = AXmppStreams;
	FOptionsNode = AOptionsNode;
	FXmppStream = NULL;

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

Account::~Account()
{

}

bool Account::isValid() const
{
	Jid jid = streamJid();
	bool valid = jid.isValid();
	valid = valid && !jid.node().isEmpty();
	valid = valid && !jid.domain().isEmpty();
	valid = valid && (FXmppStream==FXmppStreams->xmppStream(jid) || FXmppStreams->xmppStream(jid)==NULL);
	return valid;
}

QUuid Account::accountId() const
{
	return FOptionsNode.nspace();
}

bool Account::isActive() const
{
	return FXmppStream!=NULL;
}

void Account::setActive(bool AActive)
{
	if (AActive && FXmppStream==NULL && isValid())
	{
		LOG_STRM_INFO(streamJid(),QString("Activating account=%1").arg(name()));
		FXmppStream = FXmppStreams->newXmppStream(streamJid());
		connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()),Qt::QueuedConnection);
		onXmppStreamClosed();
		FXmppStreams->addXmppStream(FXmppStream);
		emit activeChanged(true);
	}
	else if (!AActive && FXmppStream!=NULL)
	{
		LOG_STRM_INFO(streamJid(),QString("Deactivating account=%1").arg(name()));
		emit activeChanged(false);
		FXmppStreams->removeXmppStream(FXmppStream);
		FXmppStreams->destroyXmppStream(FXmppStream->streamJid());
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

Jid Account::streamJid() const
{
	Jid jid = FOptionsNode.value("streamJid").toString();
	jid.setResource(resource());
	return jid;
}

void Account::setStreamJid(const Jid &AStreamJid)
{
	FOptionsNode.setValue(AStreamJid.bare(),"streamJid");
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
		FXmppStream->setStreamJid(streamJid());
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
				FXmppStream->setStreamJid(streamJid());
			else if (FOptionsNode.node("resource") == ANode)
				FXmppStream->setStreamJid(streamJid());
			else if (FOptionsNode.node("password") == ANode)
				FXmppStream->setPassword(Options::decrypt(ANode.value().toByteArray()).toString());
			else if (FOptionsNode.node("require-encryption") == ANode)
				FXmppStream->setEncryptionRequired(ANode.value().toBool());
		}
		emit optionsChanged(ANode);
	}
	else if (ANode.path() == OPV_ACCOUNT_DEFAULTRESOURCE)
	{
		if (FXmppStream && !FXmppStream->isConnected())
			FXmppStream->setStreamJid(streamJid());
	}
}
