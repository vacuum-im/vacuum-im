#include "mainwindowplugin.h"

#define MAINWINDOW_ACTION_GROUP_QUIT 1000
#define SYSTEM_ICONSETFILE "system/common.jisp"


MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FMainWindow = NULL;
}

MainWindowPlugin::~MainWindowPlugin()
{
  if (FMainWindow)
  {
    emit mainWindowDestroyed(FMainWindow);
    delete FMainWindow;
  }
}

void MainWindowPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Main window holder");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Main Window"); 
  APluginInfo->uid = MAINWINDOW_UUID;
  APluginInfo->version = "0.1";
}

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = FPluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  return true;
}

bool MainWindowPlugin::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->openSettings(MAINWINDOW_UUID,this);
    connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
    connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
  }

  FMainWindow = new MainWindow();
  emit mainWindowCreated(FMainWindow);

  Action *action = new Action(this);
  action->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
  action->setText(tr("Quit"));
  connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
  FMainWindow->mainMenu()->addAction(action,MAINWINDOW_ACTION_GROUP_QUIT);

  return true;
}

bool MainWindowPlugin::startPlugin()
{
  FMainWindow->show();
  return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
  return FMainWindow;
}

void MainWindowPlugin::onSettingsOpened()
{
  FMainWindow->setGeometry(FSettings->value("window:geometry",FMainWindow->geometry()).toRect());
}

void MainWindowPlugin::onSettingsClosed()
{
  FSettings->setValue("window:geometry",FMainWindow->geometry());
}

Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
