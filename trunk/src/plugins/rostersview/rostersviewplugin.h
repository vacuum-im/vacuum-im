#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/actiongroups.h"
#include "../../definations/rosterindextyperole.h"
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
  virtual void restoreExpandState(const QModelIndex &AParent = QModelIndex());
signals:
  virtual void optionChanged(IRostersView::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void startRestoreExpandState();
  QString getExpandSettingsName(const QModelIndex &AIndex);
  void loadExpandedState(const QModelIndex &AIndex);
  void saveExpandedState(const QModelIndex &AIndex);
protected slots:
  void onRostersViewDestroyed(QObject *AObject);
  void onModelAboutToBeSeted(IRostersModel *AModel);
  void onModelSeted(IRostersModel *AModel);
  void onModelAboutToBeReset();
  void onModelReset();
  void onLastModelAboutToBeChanged(QAbstractItemModel *AModel);
  void onLastModelChanged(QAbstractItemModel *AModel);
  void onProxyAdded(QAbstractProxyModel *AProxyModel);
  void onProxyRemoved(QAbstractProxyModel *AProxyModel);
  void onIndexInserted(const QModelIndex &AParent, int AStart, int AEnd);
  void onIndexCollapsed(const QModelIndex &AIndex);
  void onIndexExpanded(const QModelIndex &AIndex);
  void onRosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  void onAccountShown(IAccount *AAccount);
  void onAccountHidden(IAccount *AAccount);
  void onAccountDestroyed(const QString &AAccountId);
  void onRestoreExpandState();
  void onSettingsOpened();
  void onSettingsClosed();
  void onShowOfflineContactsAction(bool AChecked);
  void onOptionsAccepted();
  void onOptionsRejected();
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
  struct  
  {
    int sliderPos;
    IRosterIndex *currentIndex;
  } FViewSavedState;
  bool FStartRestoreExpandState;
  int FOptions; 
  RostersView *FRostersView; 
  IndexDataHolder *FIndexDataHolder;
  SortFilterProxyModel *FSortFilterProxyModel;
  QAbstractItemModel *FLastModel;
  QPointer<RosterOptionsWidget> FRosterOptionsWidget;
  QHash<Jid,QString> FCollapseNS;
};

#endif // ROSTERSVIEWPLUGIN_H
