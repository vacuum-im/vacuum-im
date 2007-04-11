#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "mainwindow.h"

#define MAINWINDOW_UUID "{A6F3D775-8464-4599-AB79-97BA1BAA6E96}"

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
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IMainWindowPlugin
  virtual IMainWindow *mainWindow() const;
signals:
  virtual void mainWindowCreated(IMainWindow *);
  virtual void mainWindowDestroyed(IMainWindow *);
protected:
  void createMainWindow();
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IPluginManager *FPluginManager;
  ISettings *FSettings;
private:
  MainWindow *FMainWindow;
};

#endif // MAINWINDOW_H
