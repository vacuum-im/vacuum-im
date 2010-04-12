#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterproxyorders.h>
#include <definations/rosterdataholderorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersview.h>
#include <interfaces/iroster.h>
#include <interfaces/imainwindow.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/action.h>
#include <utils/options.h>
#include "rostersview.h"
#include "sortfilterproxymodel.h"

class RostersViewPlugin : 
  public QObject,
  public IPlugin,
  public IRostersViewPlugin,
  public IOptionsHolder,
  public IRosterDataHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRostersViewPlugin IOptionsHolder IRosterDataHolder);
public:
  RostersViewPlugin();
  ~RostersViewPlugin();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERSVIEW_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IRosterDataHolder
  virtual int rosterDataOrder() const;
  virtual QList<int> rosterDataRoles() const;
  virtual QList<int> rosterDataTypes() const;
  virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
  //IRostersViewPlugin
  virtual IRostersView *rostersView();
  virtual void startRestoreExpandState();
  virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex());
signals:
  //IRosterDataHolder
  void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = RDR_ANY_ROLE);
protected:
  void loadExpandState(const QModelIndex &AIndex);
  void saveExpandState(const QModelIndex &AIndex);
protected slots:
  void onRostersViewDestroyed(QObject *AObject);
  void onViewModelAboutToBeReset();
  void onViewModelReset();
  void onViewModelAboutToBeChanged(QAbstractItemModel *AModel);
  void onViewModelChanged(QAbstractItemModel *AModel);
  void onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onViewIndexCollapsed(const QModelIndex &AIndex);
  void onViewIndexExpanded(const QModelIndex &AIndex);
  void onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  void onAccountShown(IAccount *AAccount);
  void onAccountHidden(IAccount *AAccount);
  void onAccountDestroyed(const QUuid &AAccountId);
  void onRestoreExpandState();
  void onOptionsOpened();
  void onOptionsChanged(const OptionsNode &ANode);
  void onShowOfflineContactsAction(bool AChecked);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersModel *FRostersModel;
  IMainWindowPlugin *FMainWindowPlugin;
  IAccountManager *FAccountManager;
  IOptionsManager *FOptionsManager;
private:
  bool FShowResource;
  bool FStartRestoreExpandState;
  Action *FShowOfflineAction;
  RostersView *FRostersView; 
  QAbstractItemModel *FLastModel;
  SortFilterProxyModel *FSortFilterProxyModel;
  QMap<Jid, QHash<QString,bool> > FExpandState;
  struct { int sliderPos; IRosterIndex *currentIndex; } FViewSavedState;
};

#endif // ROSTERSVIEWPLUGIN_H
