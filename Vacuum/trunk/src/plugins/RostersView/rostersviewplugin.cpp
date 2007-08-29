#include <QtDebug>
#include "rostersviewplugin.h"

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = NULL;
  FMainWindowPlugin = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FRostersView = NULL;
  FSortFilterProxyModel = NULL;
  FLastModel = NULL;
  actShowOffline = NULL;

  FSavedExpandStatusTypes 
    << IRosterIndex::IT_Group 
    << IRosterIndex::IT_AgentsGroup 
    << IRosterIndex::IT_BlankGroup 
    << IRosterIndex::IT_MyResourcesGroup
    << IRosterIndex::IT_NotInRosterGroup
    << IRosterIndex::IT_StreamRoot;
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
    }
  }

  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  if (FSettingsPlugin)
    FSettings = FSettingsPlugin->settingsForPlugin(ROSTERSVIEW_UUID);

  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));
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

  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);

    actShowOffline = new Action(this);
    actShowOffline->setIcon(STATUS_ICONSETFILE,"offline");
    actShowOffline->setToolTip(tr("Show offline contacts"));
    actShowOffline->setCheckable(true);
    actShowOffline->setChecked(true);
    connect(actShowOffline,SIGNAL(triggered(bool)),SLOT(setShowOfflineContacts(bool)));
    connect(this,SIGNAL(showOfflineContactsChanged(bool)),actShowOffline,SLOT(setChecked(bool)));
    FMainWindowPlugin->mainWindow()->topToolBar()->addAction(actShowOffline);
  }

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersView->setModel(FRostersModelPlugin->rostersModel());
    FSortFilterProxyModel = new SortFilterProxyModel(this);
    FSortFilterProxyModel->setDynamicSortFilter(true);
    FRostersView->addProxyModel(FSortFilterProxyModel);
  }

  return true;
}

IRostersView *RostersViewPlugin::rostersView()
{
  return FRostersView;
}

bool RostersViewPlugin::showOfflineContacts() const
{
  if (FSortFilterProxyModel)
    return FSortFilterProxyModel->showOffline();
  return false;
}

void RostersViewPlugin::setShowOfflineContacts(bool AShow)
{
  if (FSettings)
    FSettings->setValue("showOffline",AShow);
  if (FSortFilterProxyModel)
  {
    FSortFilterProxyModel->setShowOffline(AShow);
    emit showOfflineContactsChanged(AShow);
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
      int itemType = index.data(IRosterIndex::DR_Type).toInt();
      QString groupName = index.data(IRosterIndex::DR_Group).toString();
      if (!groupName.isEmpty() || itemType == IRosterIndex::IT_StreamRoot)
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
  int itemType = AIndex.data(IRosterIndex::DR_Type).toInt();
  if (FSavedExpandStatusTypes.contains(itemType))
  {
    Jid streamJid(AIndex.data(IRosterIndex::DR_StreamJid).toString());
    if (streamJid.isValid())
    {
      if (itemType != IRosterIndex::IT_StreamRoot)
      {
        QString groupName = AIndex.data(IRosterIndex::DR_Group).toString();
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
    Jid streamJid(AIndex.data(IRosterIndex::DR_StreamJid).toString());
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
    Jid streamJid(AIndex.data(IRosterIndex::DR_StreamJid).toString());
    if (FRostersView->isExpanded(AIndex))
      FSettings->deleteValueNS(settingsName,streamJid.pFull());
    else
      FSettings->setValueNS(settingsName,streamJid.pFull(),true);
  }
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
  }
}

void RostersViewPlugin::onIndexCollapsed(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onIndexExpanded(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onSettingsOpened()
{
  setShowOfflineContacts(FSettings->value("showOffline",true).toBool());  
}

void RostersViewPlugin::onSettingsClosed()
{

}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
