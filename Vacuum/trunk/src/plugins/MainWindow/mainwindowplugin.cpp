#include "mainwindowplugin.h"


MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
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
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(), SIGNAL(profileRenamed(const QString &, const QString &)),
        SLOT(onProfileRenamed(const QString &, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
        SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
    }
  }
  return true;
}

bool MainWindowPlugin::initObjects()
{
  FMainWindow = new MainWindow(Qt::Tool);
  emit mainWindowCreated(FMainWindow);
  updateTitle();

  actQuit = new Action(this);
  actQuit->setIcon(SYSTEM_ICONSETFILE,"psi/quit");
  actQuit->setText(tr("Quit"));
  connect(actQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
  FMainWindow->mainMenu()->addAction(actQuit,AG_MAINWINDOW_MMENU_QUIT);

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

void MainWindowPlugin::updateTitle()
{
  if (FMainWindow)
  {
    if (FSettingsPlugin && FSettingsPlugin->isProfileOpened())
      FMainWindow->setWindowTitle("Vacuum - "+FSettingsPlugin->profile());
    else
      FMainWindow->setWindowTitle("Vacuum");
  }

}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
  if (FMainWindow && ANotifyId == 0 && AReason == QSystemTrayIcon::DoubleClick)
  {
    if (!FMainWindow->isVisible())
    {
      FMainWindow->show();
      FMainWindow->activateWindow();
    }
    else
      FMainWindow->close();
  }
}

void MainWindowPlugin::onSettingsOpened()
{
  ISettings *FSettings = FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID);
  FMainWindow->setGeometry(FSettings->value("window:geometry",FMainWindow->geometry()).toRect());
  updateTitle();
}

void MainWindowPlugin::onSettingsClosed()
{
  ISettings *FSettings = FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID);
  FSettings->setValue("window:geometry",FMainWindow->geometry());
  updateTitle();
}

void MainWindowPlugin::onProfileRenamed(const QString &, const QString &)
{
  updateTitle();
}

Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
