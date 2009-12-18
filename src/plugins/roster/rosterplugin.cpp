#include "rosterplugin.h"

RosterPlugin::RosterPlugin()
{
  FPluginManager = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;
}

RosterPlugin::~RosterPlugin()
{

}

void RosterPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Roster Manager"); 
  APluginInfo->description = tr("Allows other modules to get information about contacts in the roster");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      connect(plugin->instance(), SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *))); 
      connect(plugin->instance(), SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin) 
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
  
  return FStanzaProcessor!=NULL;
}

//IRosterPlugin
IRoster *RosterPlugin::addRoster(IXmppStream *AXmppStream)
{
  IRoster *roster = getRoster(AXmppStream->streamJid());
  if (!roster)
  {
    roster = new Roster(AXmppStream, FStanzaProcessor);
    connect(roster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
    FCleanupHandler.add(roster->instance()); 
    FRosters.append(roster); 
  }
  return roster;
}

IRoster *RosterPlugin::getRoster(const Jid &AStreamJid) const
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

  return dir.absoluteFilePath(Jid::encode(AStreamJid.bare()).toLower()+".xml");
}

void RosterPlugin::removeRoster(IXmppStream *AXmppStream)
{
  IRoster *roster = getRoster(AXmppStream->streamJid());
  if (roster)
  {
    disconnect(roster->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onRosterDestroyed(QObject *)));
    FRosters.removeAt(FRosters.indexOf(roster));  
    delete roster->instance();
  }
}

void RosterPlugin::onRosterOpened()
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterOpened(roster); 
}

void RosterPlugin::onRosterItemReceived(const IRosterItem &ARosterItem)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterItemReceived(roster,ARosterItem); 
}

void RosterPlugin::onRosterItemRemoved(const IRosterItem &ARosterItem)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterItemRemoved(roster,ARosterItem); 
}

void RosterPlugin::onRosterSubscription(const Jid &AItemJid, int ASubsType, const QString &AText)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterSubscription(roster,AItemJid,ASubsType,AText);
}

void RosterPlugin::onRosterClosed()
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterClosed(roster); 
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

void RosterPlugin::onRosterStreamJidChanged(const Jid &ABefour)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
  {
    emit rosterStreamJidChanged(roster,ABefour);
    if (!(roster->streamJid() && ABefour))
      roster->loadRosterItems(rosterFileName(roster->streamJid()));
  }
}

void RosterPlugin::onRosterDestroyed(QObject *AObject)
{
  IRoster *roster = qobject_cast<IRoster *>(AObject);
  FRosters.removeAt(FRosters.indexOf(roster)); 
}

void RosterPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IRoster *roster = addRoster(AXmppStream);
  connect(roster->instance(),SIGNAL(streamJidAboutToBeChanged(const Jid &)),SLOT(onRosterStreamJidAboutToBeChanged(const Jid &)));
  connect(roster->instance(),SIGNAL(streamJidChanged(const Jid &)),SLOT(onRosterStreamJidChanged(const Jid &)));
  connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
  connect(roster->instance(),SIGNAL(received(const IRosterItem &)),SLOT(onRosterItemReceived(const IRosterItem &)));
  connect(roster->instance(),SIGNAL(removed(const IRosterItem &)),SLOT(onRosterItemRemoved(const IRosterItem &)));
  connect(roster->instance(),SIGNAL(subscription(const Jid &, int, const QString &)),
    SLOT(onRosterSubscription(const Jid &, int, const QString &)));
  connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
  emit rosterAdded(roster); 
  roster->loadRosterItems(rosterFileName(roster->streamJid()));
}

void RosterPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IRoster *roster = getRoster(AXmppStream->streamJid());
  if (roster)
  {
    roster->saveRosterItems(rosterFileName(roster->streamJid()));
    emit rosterRemoved(roster);
    removeRoster(AXmppStream);
  }
}

Q_EXPORT_PLUGIN2(plg_roster, RosterPlugin)
