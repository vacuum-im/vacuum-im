#include "address.h"

#include <QSet>
#include <utils/pluginhelper.h>

Address::Address(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid, QObject *AParent) : QObject(AParent)
{
	FAutoAddresses = false;

	FMessageWidgets = AMessageWidgets;

	FXmppStreamManager = PluginHelper::pluginInstance<IXmppStreamManager>();
	if (FXmppStreamManager)
	{
		connect(FXmppStreamManager->instance(),SIGNAL(streamJidChanged(IXmppStream *, const Jid &)),SLOT(onXmppStreamJidChanged(IXmppStream *, const Jid &)));
	}

	FPresenceManager = PluginHelper::pluginInstance<IPresenceManager>();
	if (FPresenceManager)
	{
		connect(FPresenceManager->instance(),SIGNAL(presenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)),
			SLOT(onPresenceItemReceived(IPresence *, const IPresenceItem &, const IPresenceItem &)));
	}

	appendAddress(AStreamJid,AContactJid);
	setAddress(AStreamJid,AContactJid);
}

Address::~Address()
{

}

Jid Address::streamJid() const
{
	return FStreamJid;
}

Jid Address::contactJid() const
{
	return FContactJid;
}

bool Address::isAutoAddresses() const
{
	return FAutoAddresses;
}

void Address::setAutoAddresses(bool AEnabled)
{
	if (FAutoAddresses != AEnabled)
	{
		FAutoAddresses = AEnabled;
		emit autoAddressesChanged(AEnabled);
		updateAutoAddresses(true);
	}
}

QMultiMap<Jid, Jid> Address::availAddresses(bool AUnique) const
{
	QMap<Jid,Jid> addresses;
	for (QMap<Jid, QMultiMap<Jid,Jid> >::const_iterator streamIt=FAddresses.constBegin(); streamIt!=FAddresses.constEnd(); ++streamIt)
	{
		QList<Jid> contacts = AUnique ? streamIt->uniqueKeys() : streamIt->values();
		foreach(const Jid &contact, contacts)
			addresses.insertMulti(streamIt.key(),contact);
	}
	return addresses;
}

void Address::setAddress(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (FAddresses.value(AStreamJid).contains(AContactJid.bare(),AContactJid))
	{
		if (AStreamJid!=FStreamJid || AContactJid!=FContactJid)
		{
			Jid streamBefore = FStreamJid;
			Jid contactBefore = FContactJid;
			FStreamJid = AStreamJid;
			FContactJid = AContactJid;
			emit addressChanged(streamBefore,contactBefore);
		}
	}
}

void Address::appendAddress(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (!FAddresses.value(AStreamJid).contains(AContactJid.bare(),AContactJid))
	{
		if (!AContactJid.resource().isEmpty() || !FAddresses.value(AStreamJid).contains(AContactJid))
		{
			FAddresses[AStreamJid].insertMulti(AContactJid.bare(),AContactJid);
			updateAutoAddresses(false);
			emit availAddressesChanged();
		}
	}
}

void Address::removeAddress(const Jid &AStreamJid, const Jid &AContactJid)
{
	if (AContactJid.isEmpty())
	{
		if (FAddresses.contains(AStreamJid))
		{
			FAddresses.remove(AStreamJid);
			emit availAddressesChanged();
		}
	}
	else if (AContactJid.resource().isEmpty())
	{
		if (FAddresses.value(AStreamJid).contains(AContactJid))
		{
			FAddresses[AStreamJid].remove(AContactJid);
			emit availAddressesChanged();
		}
	}
	else if (FAddresses.value(AStreamJid).contains(AContactJid.bare(),AContactJid))
	{
		FAddresses[AStreamJid].remove(AContactJid.bare(),AContactJid);
		emit availAddressesChanged();
	}
}

bool Address::updateAutoAddresses(bool AEmit)
{
	bool changed = false;
	if (FAutoAddresses && FPresenceManager)
	{
		for (QMap<Jid, QMultiMap<Jid,Jid> >::iterator streamIt=FAddresses.begin(); streamIt!=FAddresses.end(); ++streamIt)
		{
			IPresence *presence = FPresenceManager->findPresence(streamIt.key());
			foreach(const Jid &contact, streamIt->keys())
			{
				QList<IPresenceItem> pitemList = presence!=NULL ? presence->findItems(contact) : QList<IPresenceItem>();
				for (QList<IPresenceItem>::iterator pit=pitemList.begin(); pit!=pitemList.end(); )
				{
					if (pit->show==IPresence::Offline || pit->show==IPresence::Error)
						pit = pitemList.erase(pit);
					else
						++pit;
				}

				QSet<Jid> oldAddresses = streamIt->values(contact).toSet();
				if (pitemList.isEmpty())
				{
					if (!oldAddresses.contains(contact))
					{
						changed = true;
						streamIt->insertMulti(contact,contact);
					}
					else
					{
						oldAddresses -= contact;
					}
				}
				else foreach(const IPresenceItem &pitem, pitemList)
				{
					if (!oldAddresses.contains(pitem.itemJid))
					{
						changed = true;
						streamIt->insertMulti(contact,pitem.itemJid);
					}
					else
					{
						oldAddresses -= pitem.itemJid;
					}
				}

				foreach(const Jid &oldAddress, oldAddresses)
				{
					changed = true;
					streamIt->remove(contact,oldAddress);
				}
			}
		}

		if (AEmit && changed)
			emit availAddressesChanged();
	}
	return changed;
}

void Address::onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore)
{
	if (FAddresses.contains(ABefore))
	{
		QMultiMap<Jid,Jid> contacts = FAddresses.take(ABefore);
		FAddresses.insert(AXmppStream->streamJid(),contacts);
		emit streamJidChanged(ABefore,AXmppStream->streamJid());

		if (streamJid() == ABefore)
			setAddress(AXmppStream->streamJid(),contactJid());
	}
}

void Address::onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	if (FAutoAddresses && AItem.show!=ABefore.show)
	{
		QList<Jid> contacts = FAddresses.value(APresence->streamJid()).values(AItem.itemJid.bare());
		if (!contacts.isEmpty())
		{
			Jid bareJid = AItem.itemJid.bare();
			if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
			{
				if (!AItem.itemJid.resource().isEmpty() && contacts.contains(AItem.itemJid))
				{
					if (contacts.count() == 1)
						FAddresses[APresence->streamJid()].insertMulti(bareJid,bareJid);
					FAddresses[APresence->streamJid()].remove(bareJid,AItem.itemJid);
					emit availAddressesChanged();
				}
			}
			else if (!contacts.contains(AItem.itemJid))
			{
				if (contacts.contains(bareJid))
					FAddresses[APresence->streamJid()].remove(bareJid,bareJid);
				FAddresses[APresence->streamJid()].insertMulti(bareJid,AItem.itemJid);
				emit availAddressesChanged();
			}
		}
	}
}
