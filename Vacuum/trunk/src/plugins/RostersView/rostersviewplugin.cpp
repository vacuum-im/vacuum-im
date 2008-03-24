#include "rostersviewplugin.h"

#include <QtDebug>
#include <QTimer>
#include <QScrollBar>

#define SVN_SHOW_OFFLINE_CONTACTS         "showOfflineContacts"
#define SVN_SHOW_ONLINE_FIRST             "showOnlineFirst"
#define SVN_SHOW_FOOTER_TEXT              "showFooterText"
#define SVN_SHOW_RESOURCE                 "showResource"
#define SVN_COLLAPSE                      "collapse"
#define SVN_ACCOUNT                       "account"
#define SVN_GROUP                         "h%1" 
#define SVN_COLLAPSE_ACCOUNT              SVN_COLLAPSE ":" SVN_ACCOUNT
#define SVN_COLLAPSE_ACCOUNT_NS           SVN_COLLAPSE_ACCOUNT "[]"
#define SVN_COLLAPSE_ACCOUNT_NS_GROUP     SVN_COLLAPSE_ACCOUNT_NS ":" SVN_GROUP

#define IN_STATUS_OFFLINE                 "status/offline"

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModel = NULL;
  FMainWindowPlugin = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FRosterPlugin = NULL;
  FAccountManager = NULL;

  FIndexDataHolder = NULL;
  FSortFilterProxyModel = NULL;
  FLastModel = NULL;
  FShowOfflineAction = NULL;
  FOptions = 0;
  FStartRestoreExpandState = false;

  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(destroyed(QObject *)), SLOT(onRostersViewDestroyed(QObject *)));
}

RostersViewPlugin::~RostersViewPlugin()
{
  delete FRostersView;
}

void RostersViewPlugin::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Representing roster to user");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Rosters View"); 
  APluginInfo->uid = ROSTERSVIEW_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(ROSTERSMODEL_UUID);
  APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterJidAboutToBeChanged(IRoster *, const Jid &)),
        SLOT(onRosterJidAboutToBeChanged(IRoster *, const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(destroyed(const QString &)),SLOT(onAccountDestroyed(const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsRejected()));
   }
  }

  return FRostersModel!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  FIndexDataHolder = new IndexDataHolder(this);
  connect(FRostersView,SIGNAL(modelAboutToBeSeted(IRostersModel *)),
    SLOT(onModelAboutToBeSeted(IRostersModel *)));
  connect(FRostersView,SIGNAL(modelSeted(IRostersModel *)),
    SLOT(onModelSeted(IRostersModel *)));
  connect(FRostersView,SIGNAL(lastModelAboutToBeChanged(QAbstractItemModel *)),
    SLOT(onLastModelAboutToBeChanged(QAbstractItemModel *)));
  connect(FRostersView,SIGNAL(lastModelChanged(QAbstractItemModel *)),
    SLOT(onLastModelChanged(QAbstractItemModel *)));
  
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ROSTERSVIEW_UUID);
    FSettingsPlugin->openOptionsNode(ON_ROSTER,tr("Roster"),tr("Roster view options"),QIcon());
    FSettingsPlugin->insertOptionsHolder(this);

    connect(FRostersView,SIGNAL(proxyModelAdded(QAbstractProxyModel *)),
      SLOT(onProxyAdded(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(proxyModelRemoved(QAbstractProxyModel *)),
      SLOT(onProxyRemoved(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(indexInserted(const QModelIndex &, int, int)),
      SLOT(onIndexInserted(const QModelIndex &, int, int)));
    connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onIndexCollapsed(const QModelIndex &)));
    connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onIndexExpanded(const QModelIndex &)));
  }
 
  if (FMainWindowPlugin)
  {
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);

    FShowOfflineAction = new Action(this);
    FShowOfflineAction->setIcon(STATUS_ICONSETFILE,IN_STATUS_OFFLINE);
    FShowOfflineAction->setToolTip(tr("Show offline contacts"));
    FShowOfflineAction->setCheckable(true);
    connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflineContactsAction(bool)));
    FMainWindowPlugin->mainWindow()->topToolBarChanger()->addAction(FShowOfflineAction,AG_ROSTERSVIEW_MWTTB,false);
  }

  if (FRostersModel)
  {
    FRostersView->setModel(FRostersModel);
    FSortFilterProxyModel = new SortFilterProxyModel(this);
    FSortFilterProxyModel->setDynamicSortFilter(true);
    FRostersView->addProxyModel(FSortFilterProxyModel);
  }

  return true;
}

QWidget *RostersViewPlugin::optionsWidget(const QString &ANode, int &AOrder)
{
  AOrder = OO_ROSTER;
  if (ANode == ON_ROSTER)
  {
    FRosterOptionsWidget = new RosterOptionsWidget(this);
    return FRosterOptionsWidget;
  }
  return NULL;
}

IRostersView *RostersViewPlugin::rostersView()
{
  return FRostersView;
}

bool RostersViewPlugin::checkOption(IRostersView::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void RostersViewPlugin::setOption(IRostersView::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    if (FRostersView)
      FRostersView->setOption(AOption,AValue);
    if (FIndexDataHolder)
      FIndexDataHolder->setOption(AOption,AValue);
    if (FSortFilterProxyModel)
      FSortFilterProxyModel->setOption(AOption,AValue);
    if (AOption == IRostersView::ShowOfflineContacts)
    {
      FShowOfflineAction->setChecked(AValue);
      if (AValue)
        startRestoreExpandState();
    }
    emit optionChanged(AOption,AValue);
  }
}

void RostersViewPlugin::restoreExpandState(const QModelIndex &AParent)
{
  QAbstractItemModel *curModel = FRostersView->model();
  if (curModel)
  {
    if (AParent.isValid())
      loadExpandedState(AParent);
    int rows = curModel->rowCount(AParent);
    for (int i = 0; i<rows; ++i)
    {
      QModelIndex index = curModel->index(i,0,AParent);
      if (curModel->rowCount(index) > 0)
        restoreExpandState(index);
    }
  }
}

void RostersViewPlugin::startRestoreExpandState()
{
  if (!FStartRestoreExpandState)
  {
    FStartRestoreExpandState = true;
    QTimer::singleShot(0,this,SLOT(onRestoreExpandState()));
  }
}

QString RostersViewPlugin::getExpandSettingsName(const QModelIndex &AIndex)
{
  QString valueName;
  Jid streamJid(AIndex.data(RDR_StreamJid).toString());
  if (streamJid.isValid())
  {
    int itemType = AIndex.data(RDR_Type).toInt();
    if (itemType != RIT_StreamRoot)
    {
      QString groupName = AIndex.data(RDR_Group).toString();
      if (!groupName.isEmpty())
      {
        QString groupHash = QString::number(qHash(groupName),16);
        valueName = QString(SVN_COLLAPSE_ACCOUNT_NS_GROUP).arg(groupHash);
      }
    }
    else
      valueName = SVN_COLLAPSE_ACCOUNT_NS;
  }
  return valueName;
}

void RostersViewPlugin::loadExpandedState(const QModelIndex &AIndex)
{
  QString settingsName = getExpandSettingsName(AIndex);
  if (FSettings && !settingsName.isEmpty())
  {
    Jid streamJid(AIndex.data(RDR_StreamJid).toString());
    QString ns = FCollapseNS.value(streamJid);
    bool isCollapsed = FSettings->valueNS(settingsName,ns,false).toBool();
    if (isCollapsed && FRostersView->isExpanded(AIndex))
      FRostersView->collapse(AIndex);
    else if (!isCollapsed && !FRostersView->isExpanded(AIndex))
      FRostersView->expand(AIndex);
  }
}

void RostersViewPlugin::saveExpandedState(const QModelIndex &AIndex)
{
  QString settingsName = getExpandSettingsName(AIndex);
  if (FSettings && !settingsName.isEmpty())
  {
    Jid streamJid(AIndex.data(RDR_StreamJid).toString());
    QString ns = FCollapseNS.value(streamJid);
    if (FRostersView->isExpanded(AIndex))
    {
      if (AIndex.data(RDR_Type).toInt() == RIT_StreamRoot)
        FSettings->setValueNS(settingsName,ns,false);
      else
        FSettings->deleteValueNS(settingsName,ns);
    }
    else
      FSettings->setValueNS(settingsName,ns,true);
  }
}

void RostersViewPlugin::onRostersViewDestroyed(QObject * /*AObject*/)
{
  FRostersView = NULL;
}

void RostersViewPlugin::onModelAboutToBeSeted(IRostersModel * /*AModel*/)
{
  if (FRostersView->rostersModel())
    FRostersView->rostersModel()->removeDefaultDataHolder(FIndexDataHolder);
}

void RostersViewPlugin::onModelSeted(IRostersModel *AModel)
{
  AModel->insertDefaultDataHolder(FIndexDataHolder);
  startRestoreExpandState();
}

void RostersViewPlugin::onModelAboutToBeReset()
{
  FViewSavedState.sliderPos = FRostersView->verticalScrollBar()->sliderPosition();
  if (FRostersView->currentIndex().isValid())
    FViewSavedState.currentIndex = (IRosterIndex *)FRostersView->mapToModel(FRostersView->currentIndex()).internalPointer();
  else
    FViewSavedState.currentIndex = NULL;
}

void RostersViewPlugin::onModelReset()
{
  restoreExpandState();
  if (FViewSavedState.currentIndex != NULL)
    FRostersView->setCurrentIndex(FRostersView->mapFromModel(FRostersView->rostersModel()->modelIndexByRosterIndex(FViewSavedState.currentIndex)));
  FRostersView->verticalScrollBar()->setSliderPosition(FViewSavedState.sliderPos);
}

void RostersViewPlugin::onLastModelAboutToBeChanged(QAbstractItemModel * /*AModel*/)
{
  if (FRostersView->rostersModel())
  {
    disconnect(FRostersView->rostersModel(),SIGNAL(modelAboutToBeReset()),this,SLOT(onModelAboutToBeReset()));
    disconnect(FRostersView->rostersModel(),SIGNAL(modelReset()),this,SLOT(onModelReset()));
  }
}

void RostersViewPlugin::onLastModelChanged(QAbstractItemModel *AModel)
{
  if (AModel)
  {
    connect(AModel,SIGNAL(modelAboutToBeReset()),SLOT(onModelAboutToBeReset()));
    connect(AModel,SIGNAL(modelReset()),this,SLOT(onModelReset()));
  }
}

void RostersViewPlugin::onProxyAdded(QAbstractProxyModel * AProxyModel)
{
  connect(AProxyModel,SIGNAL(modelReset()),SLOT(onModelReset()));
  startRestoreExpandState();
}

void RostersViewPlugin::onProxyRemoved(QAbstractProxyModel * /*AProxyModel*/)
{
  startRestoreExpandState();
}

void RostersViewPlugin::onIndexInserted(const QModelIndex &/*AParent*/, int /*AStart*/, int /*AEnd*/)
{
  startRestoreExpandState();
}

void RostersViewPlugin::onIndexCollapsed(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onIndexExpanded(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onRosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter)
{
  Jid befour = ARoster->streamJid();
  if (FSettings && FCollapseNS.contains(befour))
  {
    QString collapseNS = FCollapseNS.take(befour);
    if ( !(befour && AAfter) )
      FSettings->deleteValueNS(SVN_COLLAPSE_ACCOUNT_NS,collapseNS);
    FCollapseNS.insert(AAfter,collapseNS);
  }
}

void RostersViewPlugin::onAccountShown(IAccount *AAccount)
{
  FCollapseNS.insert(AAccount->streamJid(),AAccount->accountId());
}

void RostersViewPlugin::onAccountHidden(IAccount *AAccount)
{
  FCollapseNS.remove(AAccount->streamJid());
}

void RostersViewPlugin::onAccountDestroyed(const QString &AAccountId)
{
  if (FSettings)
    FSettings->deleteValueNS(SVN_COLLAPSE_ACCOUNT_NS,AAccountId);
}

void RostersViewPlugin::onRestoreExpandState()
{
  restoreExpandState();
  FStartRestoreExpandState = false;
}

void RostersViewPlugin::onSettingsOpened()
{
  setOption(IRostersView::ShowOfflineContacts,FSettings->value(SVN_SHOW_OFFLINE_CONTACTS,true).toBool()); 
  setOption(IRostersView::ShowOnlineFirst,FSettings->value(SVN_SHOW_ONLINE_FIRST,true).toBool()); 
  setOption(IRostersView::ShowFooterText,FSettings->value(SVN_SHOW_FOOTER_TEXT,true).toBool()); 
  setOption(IRostersView::ShowResource,FSettings->value(SVN_SHOW_RESOURCE,true).toBool()); 
}

void RostersViewPlugin::onSettingsClosed()
{
  FSettings->setValue(SVN_SHOW_OFFLINE_CONTACTS,checkOption(IRostersView::ShowOfflineContacts));
  FSettings->setValue(SVN_SHOW_ONLINE_FIRST,checkOption(IRostersView::ShowOnlineFirst));
  FSettings->setValue(SVN_SHOW_FOOTER_TEXT,checkOption(IRostersView::ShowFooterText));
  FSettings->setValue(SVN_SHOW_RESOURCE,checkOption(IRostersView::ShowResource));
}

void RostersViewPlugin::onShowOfflineContactsAction(bool AChecked)
{
  setOption(IRostersView::ShowOfflineContacts, AChecked);
}

void RostersViewPlugin::onOptionsAccepted()
{
  if (!FRosterOptionsWidget.isNull())
    FRosterOptionsWidget->apply();
  emit optionsAccepted();
}

void RostersViewPlugin::onOptionsRejected()
{
  emit optionsRejected();
}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
