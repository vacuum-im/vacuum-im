#include "account.h"

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
	Jid sJid = streamJid();
	bool valid = sJid.isValid();
	valid = valid && !sJid.node().isEmpty();
	valid = valid && !sJid.domain().isEmpty();
	valid = valid && (FXmppStream==FXmppStreams->xmppStream(sJid) || FXmppStreams->xmppStream(sJid)==NULL);
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
		FXmppStream = FXmppStreams->newXmppStream(streamJid());
		connect(FXmppStream->instance(),SIGNAL(closed()),SLOT(onXmppStreamClosed()),Qt::QueuedConnection);
		onXmppStreamClosed();
		FXmppStreams->addXmppStream(FXmppStream);
		emit activeChanged(true);
	}
	else if (!AActive && FXmppStream!=NULL)
	{
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
	return FOptionsNode.value("streamJid").toString();
}

void Account::setStreamJid(const Jid &AJid)
{
	FOptionsNode.setValue(AJid.full(),"streamJid");
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
			{
				FXmppStream->setStreamJid(ANode.value().toString());
			}
			else if (FOptionsNode.node("password") == ANode)
			{
				FXmppStream->setPassword(Options::decrypt(ANode.value().toByteArray()).toString());
			}
			else if (FOptionsNode.node("require-encryption") == ANode)
			{
				FXmppStream->setEncryptionRequired(ANode.value().toBool());
			}
		}
		emit optionsChanged(ANode);
	}
}
