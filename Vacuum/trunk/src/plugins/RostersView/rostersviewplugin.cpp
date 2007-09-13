#include <QtDebug>
#include "rostersviewplugin.h"

#define SVN_SHOW_OFFLINE_CONTACTS         "showOfflineContacts"
#define SVN_SHOW_ONLINE_FIRST             "showOnlineFirst"
#define SVN_SHOW_FOOTER_TEXT              "showFooterText"
#define SVN_SHOW_RESOURCE                 "showResource"

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = NULL;
  FMainWindowPlugin = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FRostersView = NULL;
  FSortFilterProxyModel = NULL;
  FLastModel = NULL;
  FShowOfflineAction = NULL;
  FOptions = 0;

  FSaveExpandStatusTypes 
    << RIT_Group 
    << RIT_AgentsGroup 
    << RIT_BlankGroup 
    << RIT_MyResourcesGroup
    << RIT_NotInRosterGroup
    << RIT_StreamRoot;
  
  FShowExpandStatusTypes
    << RIT_Group 
    << RIT_AgentsGroup 
    << RIT_BlankGroup 
    << RIT_MyResourcesGroup
    << RIT_NotInRosterGroup;

  FRosterIconset.openFile(ROSTER_ICONSETFILE);
}

RostersViewPlugin::~RostersViewPlugin()
{
  if (FRostersView)
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

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_ROSTERSVIEW;

  IPlugin *plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
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

  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));

  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ROSTERSVIEW_UUID);
    FSettingsPlugin->openOptionsNode(ON_ROSTER,tr("Roster"),tr("Roster view options"),QIcon());
    FSettingsPlugin->appendOptionsHolder(this);
  }

  if (FSettings)
  {
    connect(FRostersView,SIGNAL(modelSeted(IRostersModel *)),
      SLOT(onModelSeted(IRostersModel *)));
    connect(FRostersView,SIGNAL(modelAboutToBeSeted(IRostersModel *)),
      SLOT(onModelAboutToBeSeted(IRostersModel *)));
    connect(FRostersView,SIGNAL(proxyModelAdded(QAbstractProxyModel *)),
      SLOT(onProxyAdded(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(proxyModelRemoved(QAbstractProxyModel *)),
      SLOT(onProxyRemoved(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onIndexCollapsed(const QModelIndex &)));
    connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onIndexExpanded(const QModelIndex &)));
  }
  emit viewCreated(FRostersView);
 
  //FExpandedLabelId = FRostersView->createIndexLabel(RLO_GROUPEXPANDED,FRosterIconset.iconByName("groupOpen"));
  //FCollapsedLabelId = FRostersView->createIndexLabel(RLO_GROUPCOLLAPSED,FRosterIconset.iconByName("groupClosed"));

  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);

    FShowOfflineAction = new Action(this);
    FShowOfflineAction->setIcon(STATUS_ICONSETFILE,"status/offline");
    FShowOfflineAction->setToolTip(tr("Show offline contacts"));
    FShowOfflineAction->setCheckable(true);
    connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflineContactsAction(bool)));
    FMainWindowPlugin->mainWindow()->topToolBar()->addAction(FShowOfflineAction);
  }

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersView->setModel(FRostersModelPlugin->rostersModel());
    FSortFilterProxyModel = new SortFilterProxyModel(this);
    FSortFilterProxyModel->setDynamicSortFilter(true);
    FRostersView->addProxyModel(FSortFilterProxyModel);
  }

  connect(this,SIGNAL(optionChanged(IRostersView::Option,bool)),SLOT(onOptionChanged(IRostersView::Option,bool)));

  return true;
}

QWidget *RostersViewPlugin::optionsWidget(const QString &ANode, int &AOrder)
{
  AOrder = OO_ROSTER;
  if (ANode == ON_ROSTER )
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
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
  emit optionChanged(AOption,AValue);
}

void RostersViewPlugin::setOptionByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    int option = action->data(Action::DR_Parametr1).toInt();
    bool value = action->data(Action::DR_Parametr2).toBool();
    setOption((IRostersView::Option)option,value);
  }
}

void RostersViewPlugin::reExpandItems(const QModelIndex &AParent)
{
  QAbstractItemModel *curModel = FRostersView->model();
  if (curModel)
  {
    int rows = curModel->rowCount(AParent);
    for (int i = 0; i<rows; ++i)
    {
      QModelIndex index = curModel->index(i,0,AParent);
      int itemType = index.data(RDR_Type).toInt();
      QString groupName = index.data(RDR_Group).toString();
      if (!groupName.isEmpty() || itemType == RIT_StreamRoot)
      {
        loadExpandedState(index);
        reExpandItems(index);
      }
    }
  }
}

QString RostersViewPlugin::getExpandSettingsName(const QModelIndex &AIndex)
{
  QString valueName;
  int itemType = AIndex.data(RDR_Type).toInt();
  if (FSaveExpandStatusTypes.contains(itemType))
  {
    Jid streamJid(AIndex.data(RDR_StreamJid).toString());
    if (streamJid.isValid())
    {
      if (itemType != RIT_StreamRoot)
      {
        QString groupName = AIndex.data(RDR_Group).toString();
        QString groupHash = QString::number(qHash(groupName),16);
        valueName = QString("collapsed[]:h%1").arg(groupHash);
      }
      else
        valueName = "collapsed[]";
    }
  }
  return valueName;
}

void RostersViewPlugin::loadExpandedState(const QModelIndex &AIndex)
{
  QString settingsName = getExpandSettingsName(AIndex);
  if (!settingsName.isEmpty())
  {
    Jid streamJid(AIndex.data(RDR_StreamJid).toString());
    if (FSettings->valueNS(settingsName,streamJid.pFull(),false).toBool())
      FRostersView->collapse(AIndex);
    else
      FRostersView->expand(AIndex);
  }
}

void RostersViewPlugin::saveExpandedState(const QModelIndex &AIndex)
{
  QString settingsName = getExpandSettingsName(AIndex);
  if (!settingsName.isEmpty())
  {
    Jid streamJid(AIndex.data(RDR_StreamJid).toString());
    if (FRostersView->isExpanded(AIndex))
    {
      if (AIndex.data(RDR_Type).toInt() == RIT_StreamRoot)
        FSettings->setValueNS(settingsName,streamJid.pFull(),false);
      else
        FSettings->deleteValueNS(settingsName,streamJid.pFull());
    }
    else
      FSettings->setValueNS(settingsName,streamJid.pFull(),true);
  }
}

void RostersViewPlugin::setExpandedLabel(const QModelIndex &AIndex)
{
  int itemType = AIndex.data(RDR_Type).toInt();
  if (FShowExpandStatusTypes.contains(itemType) && !AIndex.data(RDR_HideGroupExpander).toBool())
  {
    QModelIndex modelIndex = FRostersView->mapToModel(AIndex);
    IRosterIndex *rosterIndex = static_cast<IRosterIndex *>(modelIndex.internalPointer());
    if (rosterIndex)
    {
      if (FRostersView->isExpanded(AIndex))
      {
        FRostersView->removeIndexLabel(FCollapsedLabelId,rosterIndex);
        FRostersView->insertIndexLabel(FExpandedLabelId,rosterIndex);
      }
      else
      {
        FRostersView->removeIndexLabel(FExpandedLabelId,rosterIndex);
        FRostersView->insertIndexLabel(FCollapsedLabelId,rosterIndex);
      }
    }
  }
}

void RostersViewPlugin::onOptionChanged(IRostersView::Option AOption, bool AValue)
{
  if (FRostersView)
    FRostersView->setOption(AOption,AValue);
  if (FSortFilterProxyModel)
    FSortFilterProxyModel->setOption(AOption,AValue);
  if (AOption == IRostersView::ShowOfflineContacts)
    FShowOfflineAction->setChecked(AValue);
}

void RostersViewPlugin::onRostersViewDestroyed(QObject *)
{
  emit viewDestroyed(FRostersView);
  FRostersView = NULL;
}

void RostersViewPlugin::onModelAboutToBeSeted(IRostersModel *AModel)
{
  if (FLastModel == FRostersView->model())
  {
    if (FLastModel)
      disconnect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      this,SLOT(onRowsInserted(const QModelIndex &,int,int)));

    FLastModel = AModel;

    connect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      SLOT(onRowsInserted(const QModelIndex &,int,int)));
  }
}

void RostersViewPlugin::onModelSeted(IRostersModel *)
{
  reExpandItems();
}

void RostersViewPlugin::onProxyAdded(QAbstractProxyModel *AProxyModel)
{
  if (FLastModel)
    disconnect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    this,SLOT(onRowsInserted(const QModelIndex &,int,int)));

  FLastModel = AProxyModel;

  connect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    SLOT(onRowsInserted(const QModelIndex &,int,int)));
  
  reExpandItems();
}

void RostersViewPlugin::onProxyRemoved(QAbstractProxyModel *AProxyModel)
{
  if (FLastModel == AProxyModel)
  {
    disconnect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      this,SLOT(onRowsInserted(const QModelIndex &,int,int)));

    FLastModel = FRostersView->lastProxyModel();
    if (!FLastModel)
      FLastModel = FRostersView->model();

    if (FLastModel)
      connect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      SLOT(onRowsInserted(const QModelIndex &,int,int)));
  }
  reExpandItems();
}

void RostersViewPlugin::onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
  for (int i = AStart; i <= AEnd; ++i)
  {
    QModelIndex index = FRostersView->model()->index(i,0,AParent);
    loadExpandedState(index);
    //setExpandedLabel(index);
  }
}

void RostersViewPlugin::onIndexCollapsed(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
  //setExpandedLabel(AIndex);
}

void RostersViewPlugin::onIndexExpanded(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
  //setExpandedLabel(AIndex);
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
