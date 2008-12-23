#include "mainwindowplugin.h"

#define IN_QUIT               "psi/quit"
#define SVN_SHOW_ON_START     "showOnStart"
#define BDI_WINDOW_GEOMETRY   "MainWindowGeometry"

MainWindowPlugin::MainWindowPlugin()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FTrayManager = NULL;

  FMainWindow = new MainWindow(NULL,Qt::Tool);
  FMainWindow->resize(200,500);
  FMainWindow->move(0,0);
}

MainWindowPlugin::~MainWindowPlugin()
{
  delete FMainWindow;
}

void MainWindowPlugin::pluginInfo(IPluginInfo *APluginInfo)
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
  Action *action = new Action(this);
  action->setIcon(SYSTEM_ICONSETFILE,IN_QUIT);
  action->setText(tr("Quit"));
  connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit())); 
  FMainWindow->mainMenu()->addAction(action,AG_MAINWINDOW_MMENU_QUIT,true);

  if (FTrayManager)
  {
    action = new Action(this);
    action->setText(tr("Show roster"));
    connect(action,SIGNAL(triggered()),SLOT(onShowMainWindow())); 
    FTrayManager->addAction(action,AG_MAINWONDOW_SHOW_TRAY,true);
  }

  return true;
}

bool MainWindowPlugin::startPlugin()
{
  updateTitle();
  ISettings *settings = FSettingsPlugin!=NULL ? FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID) : NULL;
  FMainWindow->setVisible(settings!=NULL ? settings->value(SVN_SHOW_ON_START,true).toBool() : true);
  return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
  return FMainWindow;
}

void MainWindowPlugin::updateTitle()
{
  if (FSettingsPlugin && FSettingsPlugin->isProfileOpened())
    FMainWindow->setWindowTitle(CLIENT_NAME" - "+FSettingsPlugin->profile());
  else
    FMainWindow->setWindowTitle(CLIENT_NAME);
}

void MainWindowPlugin::showMainWindow()
{
  FMainWindow->show();
  FMainWindow->activateWindow();
}

void MainWindowPlugin::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID);
  FMainWindow->restoreGeometry(settings->loadBinaryData(BDI_WINDOW_GEOMETRY));
  updateTitle();
}

void MainWindowPlugin::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MAINWINDOW_UUID);
  settings->setValue(SVN_SHOW_ON_START,FMainWindow->isVisible());
  settings->saveBinaryData(BDI_WINDOW_GEOMETRY,FMainWindow->saveGeometry());
  updateTitle();
}

void MainWindowPlugin::onProfileRenamed(const QString &/*AProfileFrom*/, const QString &/*AProfileTo*/)
{
  updateTitle();
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
  if (ANotifyId == 0 && AReason == QSystemTrayIcon::DoubleClick)
    showMainWindow();
}

void MainWindowPlugin::onShowMainWindow()
{
  showMainWindow();
}

Q_EXPORT_PLUGIN2(MainWindowPlugin, MainWindowPlugin)
