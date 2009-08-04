#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include "../../definations/optionnodes.h"
#include "../../definations/optionnodeorders.h"
#include "../../definations/optionwidgetorders.h"
#include "../../definations/actiongroups.h"
#include "../../definations/toolbargroups.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterproxyorders.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/isettings.h"
#include "../../utils/action.h"
#include "rostersview.h"
#include "indexdataholder.h"
#include "sortfilterproxymodel.h"
#include "rosteroptionswidget.h"

class RostersViewPlugin : 
  public QObject,
  public IPlugin,
  public IRostersViewPlugin,
  public IOptionsHolder
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRostersViewPlugin IOptionsHolder);
public:
  RostersViewPlugin();
  ~RostersViewPlugin();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERSVIEW_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IoptionHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IRostersViewPlugin
  virtual IRostersView *rostersView();
  virtual bool checkOption(IRostersView::Option AOption) const;
  virtual void setOption(IRostersView::Option AOption, bool AValue);
  virtual void startRestoreExpandState();
  virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex());
signals:
  virtual void optionChanged(IRostersView::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  QString getExpandSettingsName(const QModelIndex &AIndex);
  void loadExpandedState(const QModelIndex &AIndex);
  void saveExpandedState(const QModelIndex &AIndex);
protected slots:
  void onRostersViewDestroyed(QObject *AObject);
  void onViewModelAboutToBeReset();
  void onViewModelReset();
  void onViewModelAboutToBeChanged(QAbstractItemModel *AModel);
  void onViewModelChanged(QAbstractItemModel *AModel);
  void onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onViewIndexCollapsed(const QModelIndex &AIndex);
  void onViewIndexExpanded(const QModelIndex &AIndex);
  void onRosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  void onAccountShown(IAccount *AAccount);
  void onAccountHidden(IAccount *AAccount);
  void onAccountDestroyed(const QUuid &AAccountId);
  void onRestoreExpandState();
  void onSettingsOpened();
  void onSettingsClosed();
  void onShowOfflineContactsAction(bool AChecked);
private:
  IRosterPlugin *FRosterPlugin;
  IRostersModel *FRostersModel;
  IMainWindowPlugin *FMainWindowPlugin;
  ISettings *FSettings;
  ISettingsPlugin *FSettingsPlugin;
  IAccountManager *FAccountManager;
private:
  Action *FShowOfflineAction;
private:
  int FOptions; 
  bool FStartRestoreExpandState;
  RostersView *FRostersView; 
  IndexDataHolder *FIndexDataHolder;
  SortFilterProxyModel *FSortFilterProxyModel;
  QAbstractItemModel *FLastModel;
  QHash<Jid,QString> FCollapseNS;
  struct { int sliderPos; IRosterIndex *currentIndex; } FViewSavedState;
};

#endif // ROSTERSVIEWPLUGIN_H
