#include "presenceplugin.h"

PresencePlugin::PresencePlugin()
{
  FStanzaProcessor = NULL;
}

PresencePlugin::~PresencePlugin()
{

}

//IPlugin
void PresencePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Presence Manager"); 
  APluginInfo->description = tr("Managing presences");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PresencePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(), SIGNAL(added(IXmppStream *)), SLOT(onStreamAdded(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(removed(IXmppStream *)), SLOT(onStreamRemoved(IXmppStream *))); 
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}


//IPresencePlugin
IPresence *PresencePlugin::addPresence(IXmppStream *AXmppStream)
{
  IPresence *presence = getPresence(AXmppStream->streamJid());
  if (!presence)
  {
    presence = new Presence(AXmppStream,FStanzaProcessor);
    connect(presence->instance(),SIGNAL(destroyed(QObject *)),SLOT(onPresenceDestroyed(QObject *)));
    FCleanupHandler.add(presence->instance()); 
    FPresences.append(presence); 
  }
  return presence;
}

IPresence *PresencePlugin::getPresence(const Jid &AStreamJid) const
{
  foreach(IPresence *presence, FPresences)
    if (presence->streamJid() == AStreamJid)
      return presence;
  return NULL;
}

void PresencePlugin::removePresence(IXmppStream *AXmppStream)
{
  IPresence *presence = getPresence(AXmppStream->streamJid());
  if (presence)
  {
    disconnect(presence->instance(),SIGNAL(destroyed(QObject *)),this,SLOT(onPresenceDestroyed(QObject *)));
    FPresences.removeAt(FPresences.indexOf(presence));
    delete presence->instance();
  }
}

void PresencePlugin::onPresenceOpened()
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
  {
    emit streamStateChanged(presence->streamJid(),true);
    emit presenceOpened(presence);
  }
}

void PresencePlugin::onPresenceChanged(int AShow, const QString &AStatus, int APriority)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceChanged(presence,AShow,AStatus,APriority); 
}

void PresencePlugin::onPresenceReceived(const IPresenceItem &APresenceItem)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
  {
    if (APresenceItem.show != IPresence::Offline && APresenceItem.show != IPresence::Error)
    {
      QSet<IPresence *> &presences = FContactPresences[APresenceItem.itemJid];
      if (presences.isEmpty())
        emit contactStateChanged(presence->streamJid(),APresenceItem.itemJid,true);
      presences += presence;
    }
    else if (FContactPresences.contains(APresenceItem.itemJid))
    {
      QSet<IPresence *> &presences = FContactPresences[APresenceItem.itemJid];
      presences -= presence;
      if (presences.isEmpty())
      {
        FContactPresences.remove(APresenceItem.itemJid);
        emit contactStateChanged(presence->streamJid(),APresenceItem.itemJid,false);
      }
    }
    emit presenceReceived(presence,APresenceItem);
  }
}

void PresencePlugin::onPresenceSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceSent(presence,AContactJid,AShow,AStatus,APriority);
}

void PresencePlugin::onPresenceAboutToClose(int AShow, const QString &AStatus)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceAboutToClose(presence,AShow,AStatus);
}

void PresencePlugin::onPresenceClosed()
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
  {
    emit streamStateChanged(presence->streamJid(),false);
    emit presenceClosed(presence);
  }
}

void PresencePlugin::onPresenceDestroyed(QObject *AObject)
{
  IPresence *presence = qobject_cast<IPresence *>(AObject);
  FPresences.removeAt(FPresences.indexOf(presence)); 
}

void PresencePlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IPresence *presence = addPresence(AXmppStream);
  connect(presence->instance(),SIGNAL(opened()),SLOT(onPresenceOpened()));
  connect(presence->instance(),SIGNAL(changed(int, const QString &, int)),
    SLOT(onPresenceChanged(int, const QString &, int)));
  connect(presence->instance(),SIGNAL(received(const IPresenceItem &)),
    SLOT(onPresenceReceived(const IPresenceItem &)));
  connect(presence->instance(),SIGNAL(sent(const Jid &, int, const QString &, int)),
    SLOT(onPresenceSent(const Jid &, int, const QString &, int)));
  connect(presence->instance(),SIGNAL(aboutToClose(int,const QString &)),
    SLOT(onPresenceAboutToClose(int,const QString &)));
  connect(presence->instance(),SIGNAL(closed()),SLOT(onPresenceClosed()));
  emit presenceAdded(presence); 
}

void PresencePlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IPresence *presence = getPresence(AXmppStream->streamJid());
  if (presence)
  {
    emit presenceRemoved(presence);
    removePresence(AXmppStream);
  }
}

Q_EXPORT_PLUGIN2(PresencePlugin, PresencePlugin)
