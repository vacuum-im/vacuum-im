#include "mainwindowplugin.h"

#define IN_QUIT       "psi/quit"
#define SVN_GEOMETRY  "window:geometry"

MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FTrayManager = NULL;

  FMainWindow = new MainWindow(NULL,Qt::Tool);
}

MainWindowPlugin::~MainWindowPlugin()
{
  delete FMainWindow;
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
  FActionQuit = new Action(this);
  FActionQuit->setIcon(SYSTEM_ICONSETFILE,IN_QUIT);
  FActionQuit->setText(tr("Quit"));
  connect(FActionQuit,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
  FMainWindow->mainMenu()->addAction(FActionQuit,AG_MAINWINDOW_MMENU_QUIT);
  return true;
}

bool MainWindowPlugin::startPlugin()
{
  updateTitle();
  FMainWindow->show();
  return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
  return FMainWindow;
}

void MainWindowPlugin::updateTitle()
{
  if (FSettingsPlugin && FSettingsPlugin->isProfileOpened())
    FMainWindow->setWindowTitle("Vacuum - "+FSettingsPlugin->profile());
  else
    FMainWindow->setWindowTitle("Vacuum");
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
  if (ANotifyId == 0 && AReason == QSystemTrayIcon::DoubleClick)
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
  FMainWindow->restoreGeometry(FSettings->value(SVN_GEOMETRY).toByteArray());
  updateTitle();
}

void MainWindowPlugin::onSettingsClosed()
{
  ISettings *FSettings = FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID);
  FSettings->setValue(SVN_GEOMETRY,FMainWindow->saveGeometry());
  updateTitle();
}

void MainWindowPlugin::onProfileRenamed(const QString &/*AProfileFrom*/, const QString &/*AProfileTo*/)
{
  updateTitle();
}

Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
