#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "rostersview.h"

#define ROSTERSVIEW_UUID "{BDD12B32-9C88-4e3c-9B36-2DCB5075288F}"

class RostersViewPlugin : 
  public QObject,
  public IPlugin,
  public IRostersViewPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRostersViewPlugin);

public:
  RostersViewPlugin();
  ~RostersViewPlugin();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid getPluginUuid() const { return ROSTERSVIEW_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IRostersViewPlugin
  virtual IRostersView *rostersView();

private:
  IRostersModelPlugin *FRostersModelPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
private:
  RostersView *FRostersView;   
};

#endif // ROSTERSVIEWPLUGIN_H
