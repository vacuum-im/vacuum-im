#include "mainwindowplugin.h"

#include "../../definations/actiongroups.h"

MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FTrayManager = NULL;
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

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(contextMenu(int, Menu *)),SLOT(onTrayContextMenu(int, Menu *)));
      connect(FTrayManager->instance(),SIGNAL(notifyActivated(int)),SLOT(onTrayNotifyActivated(int)));
    }
  }
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

  actQuit = new Action(this);
  actQuit->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
  actQuit->setText(tr("Quit"));
  connect(actQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
  FMainWindow->mainMenu()->addAction(actQuit,MAINWINDOW_ACTION_GROUP_QUIT);

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

void MainWindowPlugin::onTrayContextMenu(int /*ANotifyId*/, Menu *AMenu)
{
  AMenu->addAction(actQuit,MAINWINDOW_ACTION_GROUP_QUIT,true);
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId)
{
  if (FMainWindow && ANotifyId == 0)
  {
    if (FMainWindow->isVisible())
      FMainWindow->hide();
    else
      FMainWindow->show();
  }
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
