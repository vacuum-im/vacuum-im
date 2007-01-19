#include "mainwindowplugin.h"

MainWindowPlugin::MainWindowPlugin()
{
  FMainWindow = new MainWindow();
  FCleanupHandler.add(FMainWindow); 
}

MainWindowPlugin::~MainWindowPlugin()
{
  delete FMainWindow;
}

void MainWindowPlugin::getPluginInfo(PluginInfo *APluginInfo)
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
  if (FMainWindow)
    return FMainWindow->init(APluginManager); 

  return false;
}

bool MainWindowPlugin::startPlugin()
{
  if (FMainWindow)
    return FMainWindow->start();

  return false;
}

IMainWindow * MainWindowPlugin::mainWindow() const
{
  return FMainWindow;
}
Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
