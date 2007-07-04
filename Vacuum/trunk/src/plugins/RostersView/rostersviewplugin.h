#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../definations/initorders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "rostersview.h"
#include "sortfilterproxymodel.h"

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
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

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
  void reExpandItems(const QModelIndex &AParent = QModelIndex());
  QString getExpandSettingsName(const QModelIndex &AIndex);
  void loadExpandedState(const QModelIndex &AIndex);
  void saveExpandedState(const QModelIndex &AIndex);
protected slots:
  void onRostersViewDestroyed(QObject *);
  void onModelAboutToBeSeted(IRostersModel *AModel);
  void onModelSeted(IRostersModel *AModel);
  void onProxyAdded(QAbstractProxyModel *AProxyModel);
  void onProxyRemoved(QAbstractProxyModel *AProxyModel);
  void onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onIndexCollapsed(const QModelIndex &AIndex);
  void onIndexExpanded(const QModelIndex &AIndex);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IRostersModelPlugin *FRostersModelPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  ISettingsPlugin *FSettingsPlugin;
  ISettings *FSettings;
private:
  Action *actShowOffline;
private:
  RostersView *FRostersView; 
  SortFilterProxyModel *FSortFilterProxyModel;
  QAbstractItemModel *FLastModel;
  QList<int> FSavedExpandStatusTypes;
};

#endif // ROSTERSVIEWPLUGIN_H
