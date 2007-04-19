#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "rostersview.h"
#include "sortfilterproxymodel.h"

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
  virtual bool showOfflineContacts() const;
public slots:
  virtual void setShowOfflineContacts(bool AShow);
signals:
  virtual void viewCreated(IRostersView *);
  virtual void viewDestroyed(IRostersView *);
  virtual void showOfflineContactsChanged(bool AShow);
protected:
  void createRostersView();
  void createActions();
protected slots:
  void onRostersModelCreated(IRostersModel *AModel);
  void onMainWindowCreated(IMainWindow *AMainWindow);
  void onRostersViewDestroyed(QObject *);
  void onProxyAboutToBeAdded(QAbstractProxyModel *AProxyModel);
  void onProxyRemoved(QAbstractProxyModel *AProxyModel);
  void onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onIndexCollapsed(const QModelIndex &AIndex);
  void onIndexExpanded(const QModelIndex &AIndex);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IRostersModelPlugin *FRostersModelPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  ISettings *FSettings;
private:
  Action *actShowOffline;
private:
  RostersView *FRostersView; 
  SortFilterProxyModel *FSortFilterProxyModel;
};

#endif // ROSTERSVIEWPLUGIN_H
