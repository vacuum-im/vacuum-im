#include <QtDebug>
#include "rosterplugin.h"

RosterPlugin::RosterPlugin()
{
  FStanzaProcessor = NULL;
}

RosterPlugin::~RosterPlugin()
{

}

void RosterPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing roster and subscriptions");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster and Subscriptions Manager"); 
  APluginInfo->uid = ROSTER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID); 
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool RosterPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      connect(plugin->instance(), SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *))); 
      connect(plugin->instance(), SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin) 
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
  
  return FStanzaProcessor!=NULL;
}

//IRosterPlugin
IRoster *RosterPlugin::addRoster(IXmppStream *AXmppStream)
{
  Roster *roster = (Roster *)getRoster(AXmppStream->jid());
  if (!roster)
  {
    roster = new Roster(AXmppStream, FStanzaProcessor);
    connect(roster,SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
    FCleanupHandler.add(roster); 
    FRosters.append(roster); 
  }
  return roster;
}

IRoster *RosterPlugin::getRoster(const Jid &AStreamJid) const
{
  foreach(Roster *roster, FRosters)
    if (roster->streamJid() == AStreamJid)
      return roster;
  return NULL;    
}

void RosterPlugin::saveRosterItems(const Jid &AStreamJid)
{
  IRoster *roster = getRoster(AStreamJid);
  if (roster)
    roster->saveRosterItems(rosterFileName(AStreamJid));
}

void RosterPlugin::loadRosterItems(const Jid &AStreamJid)
{
  IRoster *roster = getRoster(AStreamJid);
  if (roster)
    roster->loadRosterItems(rosterFileName(AStreamJid));
}

void RosterPlugin::removeRoster(IXmppStream *AXmppStream)
{
  Roster *roster = (Roster *)getRoster(AXmppStream->jid());
  if (roster)
  {
    disconnect(roster,SIGNAL(destroyed(QObject *)),this,SLOT(onRosterDestroyed(QObject *)));
    FRosters.removeAt(FRosters.indexOf(roster));  
    delete roster;
  }
}

QString RosterPlugin::rosterFileName(const Jid &AStreamJid) const
{
  QString fileName = Jid::encode(AStreamJid.bare()).toLower()+".xml";
  QDir dir;
  if (FSettingsPlugin)
    dir.setPath(FSettingsPlugin->homeDir().path());
  if (!dir.exists("rosters"))
    dir.mkdir("rosters");
  fileName = dir.path()+"/rosters/"+fileName;
  return fileName;
}

void RosterPlugin::onRosterJidAboutToBeChanged(const Jid &AAfter)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterJidAboutToBeChanged(roster,AAfter);
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

void RosterPlugin::onRosterDestroyed(QObject *AObject)
{
  Roster *roster = qobject_cast<Roster *>(AObject);
  FRosters.removeAt(FRosters.indexOf(roster)); 
}

void RosterPlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IRoster *roster = addRoster(AXmppStream);
  connect(roster->instance(),SIGNAL(jidAboutToBeChanged(const Jid &)),SLOT(onRosterJidAboutToBeChanged(const Jid &)));
  connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
  connect(roster->instance(),SIGNAL(received(const IRosterItem &)),SLOT(onRosterItemReceived(const IRosterItem &)));
  connect(roster->instance(),SIGNAL(removed(const IRosterItem &)),SLOT(onRosterItemRemoved(const IRosterItem &)));
  connect(roster->instance(),SIGNAL(subscription(const Jid &, int, const QString &)),
    SLOT(onRosterSubscription(const Jid &, int, const QString &)));
  connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
  emit rosterAdded(roster); 
}

void RosterPlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IRoster *roster = getRoster(AXmppStream->jid());
  if (roster)
  {
    saveRosterItems(roster->streamJid());
    emit rosterRemoved(roster);
    removeRoster(AXmppStream);
  }
}

Q_EXPORT_PLUGIN2(RosterPlugin, RosterPlugin)
