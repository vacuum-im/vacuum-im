#ifndef ROSTERCHANGER_H
#define ROSTERCHANGER_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iroster.h"
#include "../../utils/menu.h"

#define ROSTERCHANGER_UUID "{018E7891-2743-4155-8A70-EAB430573500}"

#define ROSTERCHANGER_MENU_ORDER 400

class RosterChanger : 
  public QObject,
  public IPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin);

public:
  RosterChanger();
  ~RosterChanger();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid getPluginUuid() const { return ROSTERCHANGER_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IRosterChanger
protected slots:
  void onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu);
  void onRenameItem(bool);
  void onCopyItemToGroup(bool);
  void onMoveItemToGroup(bool);
  void onRemoveItemFromGroup(bool);
  void onRenameGroup(bool);
  void onRemoveGroup(bool);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersViewPlugin *FRostersViewPlugin;
private:
    
};

#endif // ROSTERCHANGER_H
