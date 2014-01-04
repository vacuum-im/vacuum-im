#include "rosterplugin.h"

#include <QDir>
#include <definitions/optionvalues.h>
#include <definitions/internalerrors.h>
#include <utils/xmpperror.h>
#include <utils/options.h>
#include <utils/logger.h>

RosterPlugin::RosterPlugin()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
}

RosterPlugin::~RosterPlugin()
{
	FCleanupHandler.clear();
}

void RosterPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Roster Manager");
	APluginInfo->description = tr("Allows other modules to get information about contacts in the roster");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(), SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
			connect(FXmppStreams->instance(), SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool RosterPlugin::initObjects()
{
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_ROSTER_REQUEST_FAILED,tr("Roster request failed"));
	return true;
}

bool RosterPlugin::initSettings()
{
	Options::setDefaultValue(OPV_XMPPSTREAMS_TIMEOUT_ROSTERREQUEST,60000);
	return true;
}

//IRosterPlugin
IRoster *RosterPlugin::getRoster(IXmppStream *AXmppStream)
{
	IRoster *roster = findRoster(AXmppStream->streamJid());
	if (!roster && FStanzaProcessor)
	{
		LOG_STRM_INFO(AXmppStream->streamJid(),QString("Roster created"));
		roster = new Roster(AXmppStream, FStanzaProcessor);
		connect(roster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
		FCleanupHandler.add(roster->instance());
		FRosters.append(roster);
	}
	return roster;
}

IRoster *RosterPlugin::findRoster(const Jid &AStreamJid) const
{
	foreach(IRoster *roster, FRosters)
		if (roster->streamJid() == AStreamJid)
			return roster;
	return NULL;
}

QString RosterPlugin::rosterFileName(const Jid &AStreamJid) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists("rosters"))
		dir.mkdir("rosters");
	dir.cd("rosters");
	return dir.absoluteFilePath(Jid::encode(AStreamJid.pBare())+".xml");
}

void RosterPlugin::removeRoster(IXmppStream *AXmppStream)
{
	IRoster *roster = findRoster(AXmppStream->streamJid());
	if (roster)
	{
		LOG_STRM_INFO(roster->streamJid(),QString("Removing roster"));
		delete roster->instance();
	}
}

void RosterPlugin::onRosterOpened()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		LOG_STRM_INFO(roster->streamJid(),QString("Roster opened"));
		emit rosterOpened(roster);
	}
}

void RosterPlugin::onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterItemReceived(roster,AItem,ABefore);
}

void RosterPlugin::onRosterSubscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionSent(roster,AItemJid,ASubsType,AText);
}

void RosterPlugin::onRosterSubscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
		emit rosterSubscriptionReceived(roster,AItemJid,ASubsType,AText);
}

void RosterPlugin::onRosterClosed()
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		LOG_STRM_INFO(roster->streamJid(),QString("Roster closed"));
		emit rosterClosed(roster);
	}
}

void RosterPlugin::onRosterStreamJidAboutToBeChanged(const Jid &AAfter)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		if (!(roster->streamJid() && AAfter))
			roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterStreamJidAboutToBeChanged(roster,AAfter);
	}
}

void RosterPlugin::onRosterStreamJidChanged(const Jid &ABefore)
{
	Roster *roster = qobject_cast<Roster *>(sender());
	if (roster)
	{
		emit rosterStreamJidChanged(roster,ABefore);
		if (!(roster->streamJid() && ABefore))
			roster->loadRosterItems(rosterFileName(roster->streamJid()));
	}
}

void RosterPlugin::onRosterDestroyed(QObject *AObject)
{
	foreach(IRoster *roster, FRosters)
	{
		if (roster->instance() == AObject)
		{
			LOG_STRM_INFO(roster->streamJid(),QString("Roster destroyed"));
			FRosters.removeAll(roster);
			break;
		}
	}
}

void RosterPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
	IRoster *roster = getRoster(AXmppStream);
	connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
	connect(roster->instance(),SIGNAL(itemReceived(const IRosterItem &, const IRosterItem &)),SLOT(onRosterItemReceived(const IRosterItem &, const IRosterItem &)));
	connect(roster->instance(),SIGNAL(subscriptionSent(const Jid &, int, const QString &)),SLOT(onRosterSubscriptionSent(const Jid &, int, const QString &)));
	connect(roster->instance(),SIGNAL(subscriptionReceived(const Jid &, int, const QString &)),SLOT(onRosterSubscriptionReceived(const Jid &, int, const QString &)));
	connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
	connect(roster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),SLOT(onRosterStreamJidAboutToBeChanged(const Jid &)));
	connect(roster->instance(),SIGNAL(streamJidChanged(const Jid &)),SLOT(onRosterStreamJidChanged(const Jid &)));
	emit rosterAdded(roster);
	roster->loadRosterItems(rosterFileName(roster->streamJid()));
}

void RosterPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
	IRoster *roster = findRoster(AXmppStream->streamJid());
	if (roster)
	{
		roster->saveRosterItems(rosterFileName(roster->streamJid()));
		emit rosterRemoved(roster);
		removeRoster(AXmppStream);
	}
}

Q_EXPORT_PLUGIN2(plg_roster, RosterPlugin)
