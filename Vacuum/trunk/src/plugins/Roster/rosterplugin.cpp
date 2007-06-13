#include <QtDebug>
#include "rosterplugin.h"

RosterPlugin::RosterPlugin()
{
  FStanzaProcessor = NULL;
}

RosterPlugin::~RosterPlugin()
{
  FCleanupHandler.clear(); 
}

void RosterPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing roster and subscriptions");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster and Subscriptions Manager"); 
  APluginInfo->uid = ROSTER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
  APluginInfo->dependences.append("{1175D470-5D4A-4c29-A69E-EDA46C2BC387}"); //IStanzaProcessor  
}

bool RosterPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(), SIGNAL(added(IXmppStream *)),
      SLOT(onStreamAdded(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(removed(IXmppStream *)),
      SLOT(onStreamRemoved(IXmppStream *))); 
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
  
  return FStanzaProcessor!=NULL;
}

//RosterPlugin
IRoster *RosterPlugin::newRoster(IXmppStream *AStream)
{
  Roster *roster = (Roster *)getRoster(AStream->jid());
  if (!roster)
  {
    roster = new Roster(AStream, FStanzaProcessor);
    connect(roster,SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
    FCleanupHandler.add(roster); 
    FRosters.append(roster); 
  }
  return roster;
}

IRoster *RosterPlugin::getRoster(const Jid &AStreamJid) const
{
  int i = 0;
  while (i < FRosters.count())
  {
    if (FRosters.at(i)->streamJid() == AStreamJid)
      return FRosters.at(i);
    i++;
  }
  return NULL;    
}

void RosterPlugin::removeRoster(const Jid &AStreamJid)
{
  Roster *roster = (Roster *)getRoster(AStreamJid);
  if (roster)
  {
    FRosters.removeAt(FRosters.indexOf(roster));  
    delete roster;
  }
}

void RosterPlugin::onStreamAdded(IXmppStream *AStream)
{
  IRoster *roster = newRoster(AStream);
  connect(roster->instance(),SIGNAL(opened()),SLOT(onRosterOpened()));
  connect(roster->instance(),SIGNAL(closed()),SLOT(onRosterClosed()));
  connect(roster->instance(),SIGNAL(itemPush(IRosterItem *)),SLOT(onRosterItemPush(IRosterItem *)));
  connect(roster->instance(),SIGNAL(itemRemoved(IRosterItem *)),SLOT(onRosterItemRemoved(IRosterItem *)));
  connect(roster->instance(),SIGNAL(subscription(const Jid &, IRoster::SubsType, const QString &)),
    SLOT(onRosterSubscription(const Jid &, IRoster::SubsType, const QString &)));
  emit rosterAdded(roster); 
}

void RosterPlugin::onStreamRemoved(IXmppStream *AStream)
{
  IRoster *roster = getRoster(AStream->jid());
  if (roster)
  {
    emit rosterRemoved(roster);
    removeRoster(roster->streamJid());
  }
}

void RosterPlugin::onRosterOpened()
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterOpened(roster); 
}

void RosterPlugin::onRosterClosed()
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterClosed(roster); 
}

void RosterPlugin::onRosterItemPush(IRosterItem *ARosterItem)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterItemPush(roster,ARosterItem); 
}

void RosterPlugin::onRosterItemRemoved(IRosterItem *ARosterItem)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterItemRemoved(roster,ARosterItem); 
}

void RosterPlugin::onRosterSubscription(const Jid &AJid, IRoster::SubsType ASType, const QString &AStatus)
{
  Roster *roster = qobject_cast<Roster *>(sender());
  if (roster)
    emit rosterSubscription(roster,AJid,ASType,AStatus);
}

void RosterPlugin::onRosterDestroyed(QObject *ARoster)
{
  Roster *roster = qobject_cast<Roster *>(ARoster);
  if (FRosters.contains(roster))
    FRosters.removeAt(FRosters.indexOf(roster)); 
}

Q_EXPORT_PLUGIN2(RosterPlugin, RosterPlugin)
