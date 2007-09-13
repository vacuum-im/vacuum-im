#ifndef ROSTERSVIEWPLUGIN_H
#define ROSTERSVIEWPLUGIN_H

#include <QPointer>
#include "../../definations/initorders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "../../utils/skin.h"
#include "../../utils/action.h"
#include "rostersview.h"
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
  virtual void pluginInfo(PluginInfo *APluginInfo);
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
public slots:
  virtual void setOptionByAction(bool);
signals:
  virtual void viewCreated(IRostersView *);
  virtual void viewDestroyed(IRostersView *);
  virtual void optionChanged(IRostersView::Option AOption, bool AValue);
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  void reExpandItems(const QModelIndex &AParent = QModelIndex());
  QString getExpandSettingsName(const QModelIndex &AIndex);
  void loadExpandedState(const QModelIndex &AIndex);
  void saveExpandedState(const QModelIndex &AIndex);
  void setExpandedLabel(const QModelIndex &AIndex);
protected slots:
  void onOptionChanged(IRostersView::Option AOption, bool AValue);
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
  void onShowOfflineContactsAction(bool AChecked);
  void onOptionsAccepted();
  void onOptionsRejected();
private:
  IRostersModelPlugin *FRostersModelPlugin;
  IMainWindowPlugin *FMainWindowPlugin;
  ISettingsPlugin *FSettingsPlugin;
  ISettings *FSettings;
private:
  Action *FShowOfflineAction;
private:
  int FOptions; 
  int FExpandedLabelId;
  int FCollapsedLabelId;
  RostersView *FRostersView; 
  SortFilterProxyModel *FSortFilterProxyModel;
  QAbstractItemModel *FLastModel;
  QList<int> FSaveExpandStatusTypes;
  QList<int> FShowExpandStatusTypes;
  SkinIconset FRosterIconset;
  QPointer<RosterOptionsWidget> FRosterOptionsWidget;
};


#endif // ROSTERSVIEWPLUGIN_H
