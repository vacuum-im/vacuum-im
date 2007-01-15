#include "presenceplugin.h"
#include <QtDebug>

PresencePlugin::PresencePlugin()
{
  FStanzaProcessor = 0;
}

PresencePlugin::~PresencePlugin()
{
  qDebug() << "~PresencePlugin";
  FCleanupHandler.clear(); 
}

//IPlugin
void PresencePlugin::getPluginInfo(PluginInfo *APluginInfo)
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

bool PresencePlugin::initPlugin(IPluginManager *APluginManager)
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

bool PresencePlugin::startPlugin()
{
  return true;
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
  return 0;
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

void PresencePlugin::onStreamRemoved(IXmppStream *AStream)
{
  IPresence *presence = getPresence(AStream->jid());
  if (presence)
  {
    emit presenceRemoved(presence);
    removePresence(presence->streamJid());
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
