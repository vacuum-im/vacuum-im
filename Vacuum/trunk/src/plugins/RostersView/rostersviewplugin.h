#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/irostersview.h"
#include "../../utils/action.h"
#include "../../utils/skin.h"
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
  virtual QUuid pluginUuid() const { return ROSTERSVIEW_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IRostersViewPlugin
  virtual IRostersView *rostersView();
  virtual Action *showOfflineAction() const { return FShowOffline; }
public slots:
  virtual void onShowOfflineContactsChanged(bool AShow);
private:
  IRostersModelPlugin *FRostersModelPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
private:
  Action *FShowOffline;
private:
  SkinIconset FStatusIconset;
  RostersView *FRostersView; 
};

#endif // ROSTERSVIEWPLUGIN_H
