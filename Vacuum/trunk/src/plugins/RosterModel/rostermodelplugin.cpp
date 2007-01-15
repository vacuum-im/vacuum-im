#include <QtDebug>
#include "rostermodelplugin.h"

#include <QTreeView>
RosterModelPlugin::RosterModelPlugin()
{

}

RosterModelPlugin::~RosterModelPlugin()
{
  qDebug() << "~RosterModelPlugin";
  FCleanupHandler.clear(); 
}

//IPlugin
void RosterModelPlugin::getPluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Creating and handling roster tree");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster Model"); 
  APluginInfo->uid = ROSTERMODEL_UUID;
  APluginInfo->version = "0.1";
}

bool RosterModelPlugin::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin;
  QList<IPlugin *> plugins = APluginManager->getPlugins("IRosterPlugin");
  foreach(plugin, plugins)
  {
    IRosterPlugin *rosters = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (rosters)
    {
      FRosterPlugin = rosters;
      connect(rosters->instance(), SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *))); 
      connect(rosters->instance(), SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *))); 
    }
  }

  plugins = APluginManager->getPlugins("IPresencePlugin");
  foreach(plugin, plugins)
  {
    IPresencePlugin *presences = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (presences)
    {
      FPresencePlugin = presences;
      connect(presences->instance(), SIGNAL(presenceAdded(IPresence *)),SLOT(onPresenceAdded(IPresence *))); 
      connect(presences->instance(), SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *))); 
    }
  }
  
  return true;
}

bool RosterModelPlugin::startPlugin()
{
  return true;
}

IRosterModel *RosterModelPlugin::newRosterModel(IRoster *ARoster, IPresence *APresence)
{
  RosterModel *model = new RosterModel(ARoster,APresence);
  connect(model,SIGNAL(destroyed(QObject *)),SLOT(onRosterModelDestroyed(QObject *)));
  FCleanupHandler.add(model); 
  FRosterModels.append(model); 
  return model; 
}

IRosterModel *RosterModelPlugin::getRosterModel(const Jid &AStreamJid) const
{
  RosterModel *model;
  foreach(model, FRosterModels)
    if (model->rootIndex()->data(IRosterIndex::DR_StreamJid) == AStreamJid.prep().full())
      return model;
  return 0;
}

void RosterModelPlugin::removeRosterModel(const Jid &AStreamJid)
{
  RosterModel *model = (RosterModel *)getRosterModel(AStreamJid);
  if (model)
  {
    FRosterModels.removeAt(FRosterModels.indexOf(model));
    delete model;
  }
}

void RosterModelPlugin::onRosterAdded(IRoster *ARoster)
{
  IPresence *presence =0;
  if (FPresencePlugin)
    presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (presence && !getRosterModel(ARoster->streamJid()))
  {
    IRosterModel *model = newRosterModel(ARoster,presence);
    connect(model->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterModelDestroyed(QObject *)));
    emit rosterModelAdded(model);
    //QTreeView *view = new QTreeView(0);
    //view->setModel(model);
    //view->show(); 
  }
}

void RosterModelPlugin::onRosterRemoved(IRoster *ARoster)
{
  IRosterModel *model = getRosterModel(ARoster->streamJid());
  if (model)
  {
    emit rosterModelRemoved(model);
    removeRosterModel(ARoster->streamJid()); 
  }
}

void RosterModelPlugin::onPresenceAdded(IPresence *APresence)
{
  IRoster *roster =0;
  if (FRosterPlugin)
    roster = FRosterPlugin->getRoster(APresence->streamJid());
  if (roster && !getRosterModel(APresence->streamJid()))
  {
    IRosterModel *model = newRosterModel(roster,APresence);
    connect(model->instance(),SIGNAL(destroyed(QObject *)),SLOT(onRosterModelDestroyed(QObject *)));
    emit rosterModelAdded(model);
  }
}

void RosterModelPlugin::onPresenceRemoved(IPresence *APresence)
{
  IRosterModel *model = getRosterModel(APresence->streamJid());
  if (model)
  {
    emit rosterModelRemoved(model);
    removeRosterModel(APresence->streamJid()); 
  }
}

void RosterModelPlugin::onRosterModelDestroyed(QObject *ARosterModel)
{
  RosterModel *roster = (RosterModel *)ARosterModel;
  if (FRosterModels.contains(roster))
    FRosterModels.removeAt(FRosterModels.indexOf(roster));  
}


Q_EXPORT_PLUGIN2(RosterModelPlugin, RosterModelPlugin)
