#include "rostersviewplugin.h"
//#include "sortfilterproxymodel.h"

#define STATUS_ICONSETFILE "status/common.jisp" 

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = NULL;
  FMainWindowPlugin = NULL;
  FRostersView = NULL;
  actShowOffline = NULL;
}

RostersViewPlugin::~RostersViewPlugin()
{
  if (FRostersView)
    delete FRostersView;
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
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
    if (FRostersModelPlugin)
    {
      connect(FRostersModelPlugin->instance(),SIGNAL(modelCreated(IRostersModel *)),
        SLOT(onRostersModelCreated(IRostersModel *)));
    }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
    if (FMainWindowPlugin)
    {
      connect(FMainWindowPlugin->instance(),SIGNAL(mainWindowCreated(IMainWindow *)),
        SLOT(onMainWindowCreated(IMainWindow *)));
    }
  }

  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::startPlugin()
{
  return true;
}

IRostersView *RostersViewPlugin::rostersView()
{
  return FRostersView;
}

void RostersViewPlugin::createRostersView()
{
  if (!FRostersView && FRostersModelPlugin->rostersModel() && FMainWindowPlugin->mainWindow())
  {
    FRostersView = new RostersView(FRostersModelPlugin->rostersModel(),NULL);
    connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));
    connect(FRostersView,SIGNAL(showOfflineContactsChanged(bool)),SLOT(onShowOfflineContactsChanged(bool)));
    emit viewCreated(FRostersView);
    createActions();
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);
  }
}

void RostersViewPlugin::createActions()
{
  actShowOffline = new Action(this);
  actShowOffline->setIcon(STATUS_ICONSETFILE,"offline");
  actShowOffline->setToolTip(tr("Show offline contacts"));
  actShowOffline->setCheckable(true);
  actShowOffline->setChecked(true);
  connect(actShowOffline,SIGNAL(triggered(bool)),FRostersView,SLOT(setShowOfflineContacts(bool)));
  FMainWindowPlugin->mainWindow()->topToolBar()->addAction(actShowOffline);
}

void RostersViewPlugin::onRostersModelCreated(IRostersModel *)
{
  createRostersView();
}

void RostersViewPlugin::onMainWindowCreated(IMainWindow *)
{
  createRostersView();
}

void RostersViewPlugin::onShowOfflineContactsChanged(bool AShow)
{
  actShowOffline->setChecked(AShow);
}

void RostersViewPlugin::onRostersViewDestroyed(QObject *)
{
  emit viewDestroyed(FRostersView);
  FRostersView = NULL;
}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
