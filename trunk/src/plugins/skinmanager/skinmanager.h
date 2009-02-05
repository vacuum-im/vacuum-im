#ifndef SKINMANAGER_H
#define SKINMANAGER_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iskinmanager.h"
#include "../../interfaces/isettings.h"

class SkinManager : 
  public QObject,
  public IPlugin,
  public ISkinManager
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ISkinManager);
public:
  SkinManager();
  ~SkinManager();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SKINMANAGER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //ISkinManager
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
private:
  ISettingsPlugin *FSettingsPlugin;  
};

#endif // SKINMANAGER_H
