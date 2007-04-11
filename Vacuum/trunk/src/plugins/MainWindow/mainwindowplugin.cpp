#include "mainwindowplugin.h"

#define MAINWINDOW_ACTION_GROUP_QUIT 1000
#define SYSTEM_ICONSETFILE "system/common.jisp"


MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
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

bool MainWindowPlugin::initPlugin(IPluginManager *APluginManager)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = FPluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->openSettings(MAINWINDOW_UUID,this);
      connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
      connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
    }
  }
  return true;
}

bool MainWindowPlugin::startPlugin()
{
  if (!FSettings)
    createMainWindow();
  return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
  return FMainWindow;
}

void MainWindowPlugin::createMainWindow()
{
  if (!FMainWindow)
  {
    FMainWindow = new MainWindow();
    
    Action *action = new Action(this);
    action->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
    action->setText(tr("Quit"));
    connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
    FMainWindow->mainMenu()->addAction(action,MAINWINDOW_ACTION_GROUP_QUIT);
    
    emit mainWindowCreated(FMainWindow);
  }
}

void MainWindowPlugin::onSettingsOpened()
{
  if (!FMainWindow)
    createMainWindow();
  FMainWindow->setGeometry(FSettings->value("window:geometry",FMainWindow->geometry()).toRect());
  FMainWindow->show();
}

void MainWindowPlugin::onSettingsClosed()
{
  FSettings->setValue("window:geometry",FMainWindow->geometry());
  FMainWindow->hide();
}

Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
