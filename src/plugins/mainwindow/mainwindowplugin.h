#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <definations/actiongroups.h>
#include <definations/version.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/isettings.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/action.h>
#include "mainwindow.h"

class MainWindowPlugin :
  public QObject,
  public IPlugin,
  public IMainWindowPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMainWindowPlugin);
public:
  MainWindowPlugin();
  ~MainWindowPlugin();
  virtual QObject* instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return MAINWINDOW_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //IMainWindowPlugin
  virtual IMainWindow *mainWindow() const;
protected:
  void updateTitle();
  void showMainWindow();
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
  void onProfileRenamed(const QString &AProfileFrom, const QString &AProfileTo);
  void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
  void onShowMainWindowByAction(bool);
private:
  IPluginManager *FPluginManager;
  ISettingsPlugin *FSettingsPlugin;
  ITrayManager *FTrayManager;
private:
  MainWindow *FMainWindow;
};

#endif // MAINWINDOW_H
