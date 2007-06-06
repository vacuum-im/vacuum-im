#include <QtDebug>
#include "presenceplugin.h"

PresencePlugin::PresencePlugin()
{
  FStanzaProcessor = NULL;
}

PresencePlugin::~PresencePlugin()
{
  FCleanupHandler.clear(); 
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
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
  APluginInfo->dependences.append("{1175D470-5D4A-4c29-A69E-EDA46C2BC387}"); //IStanzaProcessor  
}

bool PresencePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(), SIGNAL(added(IXmppStream *)),
      SLOT(onStreamAdded(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
      SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &))); 
    connect(plugin->instance(), SIGNAL(jidChanged(IXmppStream *, const Jid &)),
      SLOT(onStreamJidChanged(IXmppStream *, const Jid &))); 
    connect(plugin->instance(), SIGNAL(removed(IXmppStream *)),
      SLOT(onStreamRemoved(IXmppStream *))); 
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  return FStanzaProcessor!=NULL;
}


//IPresencePlugin
IPresence *PresencePlugin::newPresence(IXmppStream *AStream)
{
  Presence *presence = (Presence *)getPresence(AStream->jid());
  if (!presence)
  {
    presence = new Presence(AStream,FStanzaProcessor,AStream->instance());
    connect(presence,SIGNAL(destroyed(QObject *)),SLOT(onPresenceDestroyed(QObject *)));
    FCleanupHandler.add(presence); 
    FPresences.append(presence); 
  }
  return presence;
}

IPresence *PresencePlugin::getPresence(const Jid &AStreamJid) const
{
  int i =0;
  while (i<FPresences.count())
  {
    if (FPresences.at(i)->streamJid() == AStreamJid)
      return FPresences.at(i);
    i++;
  }
  return NULL;
}

void PresencePlugin::removePresence(const Jid &AStreamJid)
{
  Presence *presence = (Presence *)getPresence(AStreamJid);
  if (presence)
  {
    FPresences.removeAt(FPresences.indexOf(presence));
    delete presence;
  }
}

void PresencePlugin::onStreamAdded(IXmppStream *AStream)
{
  IPresence *presence = newPresence(AStream);
  connect(presence->instance(),SIGNAL(opened()),SLOT(onPresenceOpened()));
  connect(presence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *)));
  connect(presence->instance(),SIGNAL(selfPresence(IPresence::Show, const QString &, qint8, const Jid &)),
    SLOT(onSelfPresence(IPresence::Show, const QString &, qint8, const Jid &)));
  connect(presence->instance(),SIGNAL(closed()),SLOT(onPresenceClosed()));
  emit presenceAdded(presence); 
}

void PresencePlugin::onStreamJidAboutToBeChanged(IXmppStream *AStream, const Jid &)
{
  FChangingStreams.append(AStream);
}

void PresencePlugin::onStreamJidChanged(IXmppStream *AStream, const Jid &)
{
  FChangingStreams.removeAt(FChangingStreams.indexOf(AStream));
}

void PresencePlugin::onStreamRemoved(IXmppStream *AStream)
{
  if (!FChangingStreams.contains(AStream))
  {
    IPresence *presence = getPresence(AStream->jid());
    if (presence)
    {
      emit presenceRemoved(presence);
      removePresence(presence->streamJid());
    }
  }
}

void PresencePlugin::onPresenceOpened()
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceOpened(presence);
}

void PresencePlugin::onSelfPresence(IPresence::Show AShow, const QString &AStatus, 
                                    qint8 APriority, const Jid &AToJid)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit selfPresence(presence,AShow,AStatus,APriority,AToJid); 
}

void PresencePlugin::onPresenceItem(IPresenceItem *APresenceItem)
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceItem(presence,APresenceItem); 
}

void PresencePlugin::onPresenceClosed()
{
  Presence *presence = qobject_cast<Presence *>(sender());
  if (presence)
    emit presenceClosed(presence);
}

void PresencePlugin::onPresenceDestroyed(QObject *APresence)
{
  Presence *presence = qobject_cast<Presence *>(APresence);
  if (FPresences.contains(presence))
    FPresences.removeAt(FPresences.indexOf(presence)); 
}

Q_EXPORT_PLUGIN2(PresencePlugin, PresencePlugin)
