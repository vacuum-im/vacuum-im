#include "rosterplugin.h"
#include <QtDebug>

RosterPlugin::RosterPlugin()
{
  FStanzaProcessor = 0;
}

RosterPlugin::~RosterPlugin()
{
  qDebug() << "~RosterPlugin";
  FCleanupHandler.clear(); 
}

void RosterPlugin::getPluginInfo(PluginInfo *APluginInfo)
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

bool RosterPlugin::initPlugin(IPluginManager *APluginManager)
{
  QList<IPlugin *> plugins = APluginManager->getPlugins("IXmppStreams");
  foreach(IPlugin *plugin, plugins)
  {
    IXmppStreams *streams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (streams)
    {
      bool streamConnected = 
        connect(streams->instance(), SIGNAL(added(IXmppStream *)),
          SLOT(onStreamAdded(IXmppStream *))) && 
        connect(streams->instance(), SIGNAL(removed(IXmppStream *)),
          SLOT(onStreamRemoved(IXmppStream *))); 
      
      if (!streamConnected)
        disconnect(streams->instance(),0,this,0);
    }
  }

  plugins = APluginManager->getPlugins("IStanzaProcessor");
  if (plugins.count() > 0) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugins.at(0)->instance());
  
  return FStanzaProcessor!=0;
}

bool RosterPlugin::startPlugin()
{
  return true;
}

//RosterPlugin
IRoster *RosterPlugin::newRoster(IXmppStream *AStream)
{
  Roster *roster = (Roster *)getRoster(AStream->jid());
  if (!roster)
  {
    roster = new Roster(AStream, FStanzaProcessor, AStream->instance());
    connect(roster->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterDestroyed(QObject *)));
    FCleanupHandler.add(roster); 
    FRosters.append(roster); 
    return roster; 
  }

  return roster;
}

IRoster *RosterPlugin::getRoster(const Jid &AStreamJid)
{
  int i = 0;
  while (i < FRosters.count())
  {
    QPointer<Roster> roster = FRosters.at(i);
    if (roster.isNull())
      FRosters.removeAt(i);  
    else if (roster->streamJid() == AStreamJid)
      return roster;
    else
      i++;
  }
  return 0;    
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

void RosterPlugin::onRosterDestroyed(QObject *ARoster)
{
  Roster *roster = (Roster *)ARoster;
  if (FRosters.contains(roster))
    FRosters.removeAt(FRosters.indexOf(roster)); 
}

Q_EXPORT_PLUGIN2(RosterPlugin, RosterPlugin)
