#ifndef CONSOLEPLUGIN_H
#define CONSOLEPLUGIN_H

#include <definations/actiongroups.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imainwindow.h>
#include <utils/iconstorage.h>
#include <utils/action.h>
#include "consolewidget.h"

#define CONSOLE_UUID  "{2572D474-5F3E-8d24-B10A-BAA57C2BC693}"

class ConsolePlugin : 
  public QObject,
  public IPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin);
public:
  ConsolePlugin();
  ~ConsolePlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return CONSOLE_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //ConsolePlugin
protected slots:
  void onShowXMLConsole(bool);
private:
  IPluginManager *FPluginManager;
  IMainWindowPlugin *FMainWindowPlugin;
};

#endif // CONSOLEPLUGIN_H
