#include "rostermanager.h"

#include <QDir>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/logger.h>

RosterManager::RosterManager()
{
	FPluginManager = NULL;
	FXmppStreamManager = NULL;
	FStanzaProcessor = NULL;
}

RosterManager::~RosterManager()
{
	FCleanupHandler.clear();
}

void RosterManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Manager");
	APluginInfo->description = tr("Allows other modules to get information about contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

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

bool RosterManager::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_ROSTER_REQUEST_FAILED,tr("Roster request failed"));
	return true;
}

bool RosterManager::initSettings()
{
	Options::setDefaultValue(OPV_XMPPSTREAMS_TIMEOUT_ROSTERREQUEST,60000);
	return true;
}

QList<IRoster *> RosterManager::rosters() const
{
	return FRosters;
}

IRoster *RosterManager::findRoster(const Jid &AStreamJid) const
{
	foreach(IRoster *roster, FRosters)
		if (roster->streamJid() == AStreamJid)
			return roster;
	return NULL;
}

IRoster *RosterManager::createRoster(IXmppStream *AXmppStream)
{
	IRoster *roster = findRoster(AXmppStream->streamJid());
	if (!roster && FStanzaProcessor)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),"Roster created");
		roster = new Roster(AXmppStream, FStanzaProcessor);
		connect(roster->instance(),SIGNAL(rosterDestroyed()),SLOT(onRosterDestroyed()));
		FCleanupHandler.add(roster->instance());
		FRosters.append(roster);
		emit rosterCreated(roster);
	}
	return roster;
}

void RosterManager::destroyRoster(IRoster *ARoster)
{
	if (ARoster)
	{
		LOG_STRM_INFO(ARoster->streamJid(),"Destroying roster");
		delete ARoster->instance();
	}
}

bool RosterManager::isRosterActive(IRoster *ARoster) const
{
	return FXmppStreamManager!=NULL ? FXmppStreamManager->isXmppStreamActive(ARoster->xmppStream()) : false;
}

QString RosterManager::rosterFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists("rosters"))
		dir.mkdir("rosters");
	dir.cd("rosters");
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

void RosterManager::onRosterOpened()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		LOG_STRM_INFO(roster->streamJid(),"Roster opened");
		emit rosterOpened(roster);
	}
}

void RosterManager::onRosterClosed()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		LOG_STRM_INFO(roster->streamJid(),"Roster closed");
		emit rosterClosed(roster);
	}
}

void RosterManager::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterItemReceived(roster,AItem,ABefore);
}

void RosterManager::onRosterSubscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionSent(roster,AItemJid,ASubsType,AText);
}

void RosterManager::onRosterSubscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionReceived(roster,AItemJid,ASubsType,AText);
}

void RosterManager::onRosterStreamJidAboutToBeChanged(const Jid &AAfter)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		if (!(roster->streamJid() && AAfter))
			roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterStreamJidAboutToBeChanged(roster,AAfter);
	}
}

void RosterManager::onRosterStreamJidChanged(const Jid &ABefore)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		emit rosterStreamJidChanged(roster,ABefore);
		if (!(roster->streamJid() && ABefore))
			roster->loadRosterItems(rosterFileName(roster->streamJid()));
	}
}

void RosterManager::onRosterDestroyed()
{
	IRoster *roster = qobject_cast<IRoster *>(sender());
	if (roster)
	{
		FRosters.removeAll(roster);
		emit rosterDestroyed(roster);
		LOG_STRM_INFO(roster->streamJid(),"Roster destroyed");
	}
}

void RosterManager::onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive)
{
	IRoster *roster = findRoster(AXmppStream->streamJid());
	if (AActive && roster==NULL)
	{
		IRoster *roster = createRoster(AXmppStream);
		connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
		connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
		connect(roster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)),
			SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
		connect(roster->instance(),SIGNAL(subscriptionSent(const Jid &, int, const QString &)),
			SLOT(onRosterSubscriptionSent(const Jid &, int, const QString &)));
		connect(roster->instance(),SIGNAL(subscriptionReceived(const Jid &, int, const QString &)),
			SLOT(onRosterSubscriptionReceived(const Jid &, int, const QString &)));
		connect(roster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),
			SLOT(onRosterStreamJidAboutToBeChanged(const Jid &)));
		connect(roster->instance(),SIGNAL(streamJidChanged(const Jid &)),
			SLOT(onRosterStreamJidChanged(const Jid &)));
		emit rosterActiveChanged(roster,AActive);
		roster->loadRosterItems(rosterFileName(roster->streamJid()));
	}
	else if (!AActive && roster!=NULL)
	{
		roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterActiveChanged(roster,AActive);
		destroyRoster(roster);
	}
}

Q_EXPORT_PLUGIN2(plg_roster, RosterManager)
