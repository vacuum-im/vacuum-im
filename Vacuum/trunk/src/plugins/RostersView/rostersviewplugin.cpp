#include "rostersviewplugin.h"
//#include "sortfilterproxymodel.h"

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = 0;
  FMainWindowPlugin = 0;
  FRostersView = 0;
}

RostersViewPlugin::~RostersViewPlugin()
{

}

void RostersViewPlugin::getPluginInfo(PluginInfo *APluginInfo)
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
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,rostersView());
  return true;
}

IRostersView *RostersViewPlugin::rostersView()
{
  if (!FRostersView && FRostersModelPlugin)
    FRostersView = new RostersView(FRostersModelPlugin->rostersModel(),0);

  return FRostersView;
}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
