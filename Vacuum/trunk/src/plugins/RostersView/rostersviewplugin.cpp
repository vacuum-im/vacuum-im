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
  APluginInfo->dependences.append("{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"); //IRostersModelPlugin
  APluginInfo->dependences.append("{A6F3D775-8464-4599-AB79-97BA1BAA6E96}"); //IMainWindowPlugin
}

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = ROSTERSVIEW_INITORDER;

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
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());

  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->openSettings(pluginUuid(),this);
    connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
    connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
  }

  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));
  if (FSettings)
  {
    connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onIndexCollapsed(const QModelIndex &)));
    connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onIndexExpanded(const QModelIndex &)));
    connect(FRostersView,SIGNAL(proxyModelAdded(QAbstractProxyModel *)),
      SLOT(onProxyAdded(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(proxyModelRemoved(QAbstractProxyModel *)),
      SLOT(onProxyRemoved(QAbstractProxyModel *)));
  }
  emit viewCreated(FRostersView);

  if (FMainWindowPlugin->mainWindow())
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

  if (FRostersModelPlugin->rostersModel())
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

void RostersViewPlugin::onRostersViewDestroyed(QObject *)
{
  emit viewDestroyed(FRostersView);
  FRostersView = NULL;
}

void RostersViewPlugin::onProxyAdded(QAbstractProxyModel *AProxyModel)
{
  if (FLastModel)
    disconnect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    this,SLOT(onRowsInserted(const QModelIndex &,int,int)));

  FLastModel = AProxyModel;

  connect(FLastModel,SIGNAL(rowsInserted(const QModelIndex &,int,int)),
    SLOT(onRowsInserted(const QModelIndex &,int,int)));
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
}

void RostersViewPlugin::onRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
  for (int i = AStart; i <= AEnd; ++i)
  {
    QModelIndex index = FRostersView->model()->index(i,0,AParent);
    IRosterIndex::ItemType itemType = (IRosterIndex::ItemType)index.data(IRosterIndex::DR_Type).toInt();
    QString groupName = index.data(IRosterIndex::DR_Group).toString();
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
        
        bool itemCollapsed = FSettings->valueNS(valueName,streamJid.pBare(),false).toBool();
        if (!itemCollapsed)
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
  QString groupName = AIndex.data(IRosterIndex::DR_Group).toString();
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
  QString groupName = AIndex.data(IRosterIndex::DR_Group).toString();
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
