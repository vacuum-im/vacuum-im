#include "rostersviewplugin.h"
//#include "sortfilterproxymodel.h"

#define STATUS_ICONSETFILE "status/common.jisp" 

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = 0;
  FMainWindowPlugin = 0;
  FRostersView = 0;

  FShowOffline = new Action(this);
  FShowOffline->setIcon(STATUS_ICONSETFILE,"offline");
  FShowOffline->setToolTip(tr("Show offline contacts"));
  FShowOffline->setCheckable(true);
  FShowOffline->setChecked(true);
}

RostersViewPlugin::~RostersViewPlugin()
{

}

void RostersViewPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Representing roster to user");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Rosters View"); 
  APluginInfo->uid = ROSTERSVIEW_UUID;
  APluginInfo->version = "0.1";
}

bool RostersViewPlugin::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  return true;
}

bool RostersViewPlugin::startPlugin()
{
  if (FRostersModelPlugin && FMainWindowPlugin)
  {
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,rostersView());
    FMainWindowPlugin->mainWindow()->topToolBar()->addAction(FShowOffline);
    connect(FShowOffline,SIGNAL(triggered(bool)),FRostersView,SLOT(setShowOfflineContacts(bool)));
    connect(FRostersView,SIGNAL(showOfflineContactsChanged(bool)),SLOT(onShowOfflineContactsChanged(bool)));
  }
  return true;
}

IRostersView *RostersViewPlugin::rostersView()
{
  if (!FRostersView && FRostersModelPlugin)
    FRostersView = new RostersView(FRostersModelPlugin->rostersModel(),0);

  return FRostersView;
}

void RostersViewPlugin::onShowOfflineContactsChanged(bool AShow)
{
  FShowOffline->setChecked(AShow);
}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
