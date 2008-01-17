#ifndef SKINMANAGER_H
#define SKINMANAGER_H

#include "../../definations/actiongroups.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iskinmanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/isettings.h"
#include "../../utils/menu.h"
#include "../../utils/skin.h"

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

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return SKINMANAGER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();

  //ISkinManager
  virtual void setSkin(const QString &ASkin);
  virtual void setSkinsDirectory(const QString &ASkinsDirectory);
public slots:
  virtual void setSkinByAction(bool);
signals:
  virtual void skinDirectoryChanged();
  virtual void skinAboutToBeChanged();
  virtual void skinChanged();
protected:
  void updateSkinMenu();
protected slots:
  void onSettingsOpened();
  void onSettingsClosed();
private:
  ISettingsPlugin *FSettingsPlugin;  
  IMainWindowPlugin *FMainWindowPlugin;
  ITrayManager *FTrayManager;
private:
  Menu *FSkinMenu;
};

#endif // SKINMANAGER_H
