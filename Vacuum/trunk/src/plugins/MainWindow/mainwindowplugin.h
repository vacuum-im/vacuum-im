#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imainwindow.h"
#include "mainwindow.h"

#define MAINWINDOW_UUID "{A6F3D775-8464-4599-AB79-97BA1BAA6E96}";

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
  virtual QUuid getPluginUuid() const { return MAINWINDOW_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IMainWindowPlugin
  virtual IMainWindow *mainWindow() const;
private:
  IPluginManager *FPluginManager;
private:
  MainWindow *FMainWindow;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // MAINWINDOW_H
