#include <QtDebug>
#include "presenceplugin.h"

PresencePlugin::PresencePlugin()
{
  FStanzaProcessor = NULL;
}

PresencePlugin::~PresencePlugin()
{

}

//IPlugin
void PresencePlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing presences");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Presence Manager"); 
  APluginInfo->uid = PRESENCE_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool PresencePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
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


//IPresencePlugin
IPresence *PresencePlugin::addPresence(IXmppStream *AXmppStream)
{
  Presence *presence = (Presence *)getPresence(AXmppStream->jid());
  if (!presence)
  {
    presence = new Presence(AXmppStream,FStanzaProcessor);
    connect(presence,SIGNAL(destroyed(QObject *)),SLOT(onPresenceDestroyed(QObject *)));
    FCleanupHandler.add(presence); 
    FPresences.append(presence); 
  }
  return presence;
}

IPresence *PresencePlugin::getPresence(const Jid &AStreamJid) const
{
  foreach(Presence *presence, FPresences)
    if (presence->streamJid() == AStreamJid)
      return presence;
  return NULL;
}

void PresencePlugin::removePresence(IXmppStream *AXmppStream)
{
  Presence *presence = (Presence *)getPresence(AXmppStream->jid());
  if (presence)
  {
    disconnect(presence,SIGNAL(destroyed(QObject *)),this,SLOT(onPresenceDestroyed(QObject *)));
    FPresences.removeAt(FPresences.indexOf(presence));
    delete presence;
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

void PresencePlugin::onSelfPresence(int AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit selfPresence(presence,AShow,AStatus,APriority,AToJid); 
}

void PresencePlugin::onPresenceItem(IPresenceItem *APresenceItem)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
  {
    if (APresenceItem->show() != IPresence::Offline && APresenceItem->show() != IPresence::Error)
    {
      QSet<IPresence *> &presences = FContactPresences[APresenceItem->jid()];
      if (presences.isEmpty())
        emit contactStateChanged(presence->streamJid(),APresenceItem->jid(),true);
      presences += presence;
    }
    else if (FContactPresences.contains(APresenceItem->jid()))
    {
      QSet<IPresence *> &presences = FContactPresences[APresenceItem->jid()];
      presences -= presence;
      if (presences.isEmpty())
      {
        FContactPresences.remove(APresenceItem->jid());
        emit contactStateChanged(presence->streamJid(),APresenceItem->jid(),false);
      }
    }
    emit presenceItem(presence,APresenceItem);
  }
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

void PresencePlugin::onStreamAdded(IXmppStream *AXmppStream)
{
  IPresence *presence = addPresence(AXmppStream);
  connect(presence->instance(),SIGNAL(opened()),SLOT(onPresenceOpened()));
  connect(presence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *)));
  connect(presence->instance(),SIGNAL(selfPresence(int, const QString &, qint8, const Jid &)),
    SLOT(onSelfPresence(int, const QString &, qint8, const Jid &)));
  connect(presence->instance(),SIGNAL(closed()),SLOT(onPresenceClosed()));
  emit presenceAdded(presence); 
}

void PresencePlugin::onStreamRemoved(IXmppStream *AXmppStream)
{
  IPresence *presence = getPresence(AXmppStream->jid());
  if (presence)
  {
    emit presenceRemoved(presence);
    removePresence(AXmppStream);
  }
}

void PresencePlugin::onPresenceDestroyed(QObject *AObject)
{
  Presence *presence = qobject_cast<Presence *>(AObject);
  if (FPresences.contains(presence))
    FPresences.removeAt(FPresences.indexOf(presence)); 
}

Q_EXPORT_PLUGIN2(PresencePlugin, PresencePlugin)
