#include "presencemanager.h"

#include <utils/logger.h>

bool presenceItemLessThen(const IPresenceItem &AItem1, const IPresenceItem &AItem2)
{
	if (AItem1.show != AItem2.show)
	{
		static const int show2order[] = {6,2,1,3,5,4,7,8};
		static const int showCount = sizeof(show2order)/sizeof(show2order[0]);

		if (AItem1.show<showCount && AItem2.show<showCount)
			return show2order[AItem1.show] < show2order[AItem2.show];
	}
	
	if (AItem1.priority != AItem2.priority)
		return AItem1.priority > AItem2.priority;

	return AItem1.itemJid < AItem2.itemJid;
}

PresenceManager::PresenceManager()
{
	FXmppStreamManager = NULL;
	FStanzaProcessor = NULL;
}

PresenceManager::~PresenceManager()
{
	FCleanupHandler.clear();
}

//IPlugin
void PresenceManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Presence Manager");
	APluginInfo->description = tr("Allows other modules to obtain information about the status of contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PresenceManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(), SIGNAL(streamActiveChanged(IXmppStream *, bool)), SLOT(onXmppStreamActiveChanged(IXmppStream *, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL && FStanzaProcessor!=NULL;
}

QList<IPresence *> PresenceManager::presences() const
{
	return FPresences;
}

IPresence *PresenceManager::findPresence(const Jid &AStreamJid) const
{
	foreach(IPresence *presence, FPresences)
		if (presence->streamJid() == AStreamJid)
			return presence;
	return NULL;
}

IPresence *PresenceManager::createPresence(IXmppStream *AXmppStream)
{
	IPresence *presence = findPresence(AXmppStream->streamJid());
	if (!presence && FStanzaProcessor)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"Presence created");
		presence = new Presence(AXmppStream,FStanzaProcessor);
		connect(presence->instance(),SIGNAL(presenceDestroyed()),SLOT(onPresenceDestroyed()));
		FCleanupHandler.add(presence->instance());
		FPresences.append(presence);
		emit presenceCreated(presence);
	}
	return presence;
}

void PresenceManager::destroyPresence(IPresence *APresence)
{
	if (APresence)
	{
		LOG_STRM_INFO(APresence->streamJid(),"Destroying presence");
		delete APresence->instance();
	}
}

bool PresenceManager::isPresenceActive(IPresence *APresence) const
{
	return FXmppStreamManager!=NULL ? FXmppStreamManager->isXmppStreamActive(APresence->xmppStream()) : false;
}

QList<Jid> PresenceManager::onlineContacts() const
{
	return FContactPresences.keys();
}

bool PresenceManager::isOnlineContact(const Jid &AContactJid) const
{
	return FContactPresences.contains(AContactJid);
}

QList<IPresence *> PresenceManager::contactPresences(const Jid &AContactJid) const
{
	return FContactPresences.value(AContactJid).toList();
}

QList<IPresenceItem> PresenceManager::sortPresenceItems(const QList<IPresenceItem> &AItems) const
{
	if (AItems.count() > 1)
	{
		QList<IPresenceItem> items = AItems;
		qSort(items.begin(),items.end(),presenceItemLessThen);
		return items;
	}
	return AItems;
}

void PresenceManager::onPresenceOpened()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		LOG_STRM_INFO(presence->streamJid(),"Presence opened");
		emit presenceOpened(presence);
	}
}

void PresenceManager::onPresenceClosed()
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		LOG_STRM_INFO(presence->streamJid(),"Presence closed");
		emit presenceClosed(presence);
	}
}

void PresenceManager::onPresenceAboutToClose(int AShow, const QString &AStatus)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		LOG_STRM_INFO(presence->streamJid(),"Presence about to close");
		emit presenceAboutToClose(presence,AShow,AStatus);
	}
}

void PresenceManager::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
		emit presenceChanged(presence,AShow,AStatus,APriority);
}

void PresenceManager::onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
	{
		if (AItem.show != IPresence::Offline && AItem.show != IPresence::Error)
		{
			QSet<IPresence *> &presences = FContactPresences[AItem.itemJid];
			if (presences.isEmpty())
				emit contactStateChanged(presence->streamJid(),AItem.itemJid,true);
			presences += presence;
		}
		else if (FContactPresences.contains(AItem.itemJid))
		{
			QSet<IPresence *> &presences = FContactPresences[AItem.itemJid];
			presences -= presence;
			if (presences.isEmpty())
			{
				FContactPresences.remove(AItem.itemJid);
				emit contactStateChanged(presence->streamJid(),AItem.itemJid,false);
			}
		}
		emit presenceItemReceived(presence,AItem,ABefore);
	}
}

void PresenceManager::onPresenceDirectSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority)
{
	Presence *presence = qobject_cast<Presence *>(sender());
	if (presence)
		emit presenceDirectSent(presence,AContactJid,AShow,AStatus,APriority);
}

void PresenceManager::onPresenceDestroyed()
{
	IPresence *presence = qobject_cast<IPresence *>(sender());
	if (presence)
	{
		LOG_STRM_INFO(presence->streamJid(),QString("Presence destroyed"));
		FPresences.removeAll(presence);
		emit presenceDestroyed(presence);
	}
}

void PresenceManager::onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive)
{
	IPresence *presence = findPresence(AXmppStream->streamJid());
	if (AActive && presence==NULL)
	{
		IPresence *presence = createPresence(AXmppStream);
		connect(presence->instance(),SIGNAL(opened()),SLOT(onPresenceOpened()));
		connect(presence->instance(),SIGNAL(closed()),SLOT(onPresenceClosed()));
		connect(presence->instance(),SIGNAL(changed(int, const QString &, int)),
			SLOT(onPresenceChanged(int, const QString &, int)));
		connect(presence->instance(),SIGNAL(itemReceived(const IPresenceItem &, const IPresenceItem &)),
			SLOT(onPresenceItemReceived(const IPresenceItem &, const IPresenceItem &)));
		connect(presence->instance(),SIGNAL(directSent(const Jid &, int, const QString &, int)),
			SLOT(onPresenceDirectSent(const Jid &, int, const QString &, int)));
		connect(presence->instance(),SIGNAL(aboutToClose(int,const QString &)),
			SLOT(onPresenceAboutToClose(int,const QString &)));
		emit presenceActiveChanged(presence,AActive);
	}
	else if (!AActive && presence!=NULL)
	{
		emit presenceActiveChanged(presence,AActive);
		destroyPresence(presence);
	}
}

Q_EXPORT_PLUGIN2(plg_presence, PresenceManager)
