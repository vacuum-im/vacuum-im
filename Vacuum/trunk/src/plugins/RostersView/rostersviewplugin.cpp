#include <QtDebug>
#include "rostersviewplugin.h"

#define STATUS_ICONSETFILE "status/common.jisp" 

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = NULL;
  FMainWindowPlugin = NULL;
  FRostersView = NULL;
  FSortFilterProxyModel = NULL;
  actShowOffline = NULL;
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
}

bool RostersViewPlugin::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
    if (FRostersModelPlugin)
    {
      connect(FRostersModelPlugin->instance(),SIGNAL(modelCreated(IRostersModel *)),
        SLOT(onRostersModelCreated(IRostersModel *)));
    }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
    if (FMainWindowPlugin)
    {
      connect(FMainWindowPlugin->instance(),SIGNAL(mainWindowCreated(IMainWindow *)),
        SLOT(onMainWindowCreated(IMainWindow *)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->openSettings(pluginUuid(),this);
      connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
      connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
    }
  }
  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::startPlugin()
{
  createRostersView();
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

void RostersViewPlugin::createRostersView()
{
  if (!FRostersView)
  {
    FRostersView = new RostersView;
    connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));
    if (FSettings)
    {
      connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onIndexCollapsed(const QModelIndex &)));
      connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onIndexExpanded(const QModelIndex &)));
      connect(FRostersView,SIGNAL(proxyModelAboutToBeAdded(QAbstractProxyModel *)),
        SLOT(onProxyAboutToBeAdded(QAbstractProxyModel *)));
      connect(FRostersView,SIGNAL(proxyModelRemoved(QAbstractProxyModel *)),
        SLOT(onProxyRemoved(QAbstractProxyModel *)));
    }
    emit viewCreated(FRostersView);
  }
}

void RostersViewPlugin::createActions()
{
  if (!actShowOffline)
  {
    actShowOffline = new Action(this);
    actShowOffline->setIcon(STATUS_ICONSETFILE,"offline");
    actShowOffline->setToolTip(tr("Show offline contacts"));
    actShowOffline->setCheckable(true);
    actShowOffline->setChecked(true);
    connect(actShowOffline,SIGNAL(triggered(bool)),SLOT(setShowOfflineContacts(bool)));
    connect(this,SIGNAL(showOfflineContactsChanged(bool)),actShowOffline,SLOT(setChecked(bool)));
  }
}

void RostersViewPlugin::onRostersModelCreated(IRostersModel *AModel)
{
  createRostersView();
  createActions();
  FRostersView->setModel(AModel);
  FSortFilterProxyModel = new SortFilterProxyModel(this);
  FSortFilterProxyModel->setDynamicSortFilter(true);
  FRostersView->addProxyModel(FSortFilterProxyModel);
}

void RostersViewPlugin::onMainWindowCreated(IMainWindow *AMainWindow)
{
  createRostersView();
  AMainWindow->rostersWidget()->insertWidget(0,FRostersView);
  createActions();
  AMainWindow->topToolBar()->addAction(actShowOffline);
}

void RostersViewPlugin::onRostersViewDestroyed(QObject *)
{
  emit viewDestroyed(FRostersView);
  FRostersView = NULL;
}

void RostersViewPlugin::onProxyAboutToBeAdded(QAbstractProxyModel *AProxyModel)
{
  QAbstractProxyModel *last = FRostersView->lastProxyModel();
  if (last)
    disconnect(last,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      this,SLOT(onRowsInserted(const QModelIndex &,int,int)));
  else
    disconnect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      this,SLOT(onRowsInserted(const QModelIndex &,int,int)));

  connect(AProxyModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    SLOT(onRowsInserted(const QModelIndex &,int,int)));
}

void RostersViewPlugin::onProxyRemoved(QAbstractProxyModel *AProxyModel)
{
  disconnect(AProxyModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    this,SLOT(onRowsInserted(const QModelIndex &,int,int)));
  
  QAbstractProxyModel *last = FRostersView->lastProxyModel();
  if (last)
    connect(last,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      SLOT(onRowsInserted(const QModelIndex &,int,int)));
  else
    connect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &,int,int)),
      SLOT(onRowsInserted(const QModelIndex &,int,int)));
}

void RostersViewPlugin::onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
  for (int i = AStart; i <= AEnd; ++i)
  {
    QModelIndex index = FRostersView->model()->index(i,0,AParent);
    IRosterIndex::ItemType itemType = (IRosterIndex::ItemType)index.data(IRosterIndex::DR_Type).toInt();
    QString groupName = index.data(IRosterIndex::DR_GroupName).toString();
    if (!groupName.isEmpty() || itemType == IRosterIndex::IT_StreamRoot)
    {
      Jid streamJid(index.data(IRosterIndex::DR_StreamJid).toString());
      if (streamJid.isValid())
      {
        QString valueName;
        if (itemType != IRosterIndex::IT_StreamRoot)
        {
          QString groupHash = QString::number(qHash(groupName),16);
          valueName = QString("collapsed[]:h%1").arg(groupHash);
        }
        else
          valueName = "collapsed[]";
        
        bool collapsed = FSettings->valueNS(valueName,streamJid.pBare(),false).toBool();
        if (!collapsed)
        {
          FRostersView->expand(index);
        }
      }
    }
  }
}

void RostersViewPlugin::onIndexCollapsed(const QModelIndex &AIndex)
{
  IRosterIndex::ItemType itemType = (IRosterIndex::ItemType)AIndex.data(IRosterIndex::DR_Type).toInt();
  QString groupName = AIndex.data(IRosterIndex::DR_GroupName).toString();
  if (!groupName.isEmpty() || itemType == IRosterIndex::IT_StreamRoot)
  {
    Jid streamJid(AIndex.data(IRosterIndex::DR_StreamJid).toString());
    if (streamJid.isValid())
    {
      QString valueName;
      if (itemType != IRosterIndex::IT_StreamRoot)
      {
        QString groupHash = QString::number(qHash(groupName),16);
        valueName = QString("collapsed[]:h%1").arg(groupHash);
      }
      else
        valueName = "collapsed[]";
      
      FSettings->setValueNS(valueName,streamJid.pBare(),true);
    }
  }
}

void RostersViewPlugin::onIndexExpanded(const QModelIndex &AIndex)
{
  IRosterIndex::ItemType itemType = (IRosterIndex::ItemType)AIndex.data(IRosterIndex::DR_Type).toInt();
  QString groupName = AIndex.data(IRosterIndex::DR_GroupName).toString();
  if (!groupName.isEmpty() || itemType == IRosterIndex::IT_StreamRoot)
  {
    Jid streamJid(AIndex.data(IRosterIndex::DR_StreamJid).toString());
    if (streamJid.isValid())
    {
      if (itemType != IRosterIndex::IT_StreamRoot)
      {
        QString groupHash = QString::number(qHash(groupName),16);
        QString valueName = QString("collapsed[]:h%1").arg(groupHash);
        FSettings->delValueNS(valueName,streamJid.pBare());
      }
      else
      {
        QString valueName = "collapsed[]";
        FSettings->setValueNS(valueName,streamJid.pBare(),false);
      }
    }
  }
}

void RostersViewPlugin::onSettingsOpened()
{
  setShowOfflineContacts(FSettings->value("showOffline",true).toBool());  
}

void RostersViewPlugin::onSettingsClosed()
{

}

Q_EXPORT_PLUGIN2(RostersViewPlugin, RostersViewPlugin)
