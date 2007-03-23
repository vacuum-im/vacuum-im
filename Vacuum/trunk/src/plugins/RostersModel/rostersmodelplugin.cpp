#include <QtDebug>
#include "rostersmodelplugin.h"

#include <QTreeView>

RostersModelPlugin::RostersModelPlugin()
{
  FRostersModel = NULL;
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
}

RostersModelPlugin::~RostersModelPlugin()
{
  if (FRostersModel)
    delete FRostersModel;
}

//IPlugin
void RostersModelPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Creating and handling roster tree");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Rosters Model"); 
  APluginInfo->uid = ROSTERSMODEL_UUID;
  APluginInfo->version = "0.1";
}

bool RostersModelPlugin::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    IRosterPlugin *rosters = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (rosters)
    {
      FRosterPlugin = rosters;
      connect(rosters->instance(), SIGNAL(rosterAdded(IRoster *)),SLOT(onRosterAdded(IRoster *))); 
      connect(rosters->instance(), SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *))); 
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
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

bool RostersModelPlugin::startPlugin()
{
  return true;
}

//IRostersModelPlugin
IRostersModel *RostersModelPlugin::rostersModel()
{
  if (!FRostersModel)
  {
    FRostersModel = new RostersModel(this);
  }
  return FRostersModel;
}

bool RostersModelPlugin::addStreamRoster(IRoster *ARoster, IPresence *APresence)
{
  return rostersModel()->appendStream(ARoster,APresence) != NULL;
}

bool RostersModelPlugin::removeStreamRoster(const Jid &AStreamJid)
{
  return rostersModel()->removeStream(AStreamJid.pFull());
}

void RostersModelPlugin::onRosterAdded(IRoster *ARoster)
{
  IPresence *presence =0;
  if (FPresencePlugin)
    presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (presence)
    addStreamRoster(ARoster,presence);
}

void RostersModelPlugin::onRosterRemoved(IRoster *ARoster)
{
  if (rostersModel()->streams().contains(ARoster->streamJid().pFull()))
  {
    emit streamRosterRemoved(ARoster->streamJid());
    removeStreamRoster(ARoster->streamJid());
  }
}

void RostersModelPlugin::onPresenceAdded(IPresence *APresence)
{
  IRoster *roster =0;
  if (FRosterPlugin)
    roster = FRosterPlugin->getRoster(APresence->streamJid());
  if (roster)
    addStreamRoster(roster,APresence);
}

void RostersModelPlugin::onPresenceRemoved(IPresence *APresence)
{
  if (rostersModel()->streams().contains(APresence->streamJid().pFull()))
  {
    emit streamRosterRemoved(APresence->streamJid());
    removeStreamRoster(APresence->streamJid());
  }
}

Q_EXPORT_PLUGIN2(RostersModelPlugin, RostersModelPlugin)
