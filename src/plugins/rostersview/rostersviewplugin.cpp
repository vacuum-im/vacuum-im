#include "rostersviewplugin.h"

#include <QTimer>
#include <QScrollBar>

#define SVN_SHOW_OFFLINE_CONTACTS         "showOfflineContacts"
#define SVN_SHOW_ONLINE_FIRST             "showOnlineFirst"
#define SVN_SHOW_RESOURCE                 "showResource"
#define SVN_SHOW_STATUS                   "showStatusText"
#define SVN_COLLAPSE                      "collapse"
#define SVN_ACCOUNT                       "account"
#define SVN_GROUP                         "h%1" 
#define SVN_COLLAPSE_ACCOUNT              SVN_COLLAPSE ":" SVN_ACCOUNT
#define SVN_COLLAPSE_ACCOUNT_NS           SVN_COLLAPSE_ACCOUNT "[]"
#define SVN_COLLAPSE_ACCOUNT_NS_GROUP     SVN_COLLAPSE_ACCOUNT_NS ":" SVN_GROUP

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModel = NULL;
  FMainWindowPlugin = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FRosterPlugin = NULL;
  FAccountManager = NULL;

  FSortFilterProxyModel = NULL;
  FLastModel = NULL;
  FShowOfflineAction = NULL;
  FOptions = 0;
  FStartRestoreExpandState = false;
  
  FViewSavedState.sliderPos = 0;
  FViewSavedState.currentIndex = NULL;

  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(viewModelAboutToBeChanged(QAbstractItemModel *)),
    SLOT(onViewModelAboutToBeChanged(QAbstractItemModel *)));
  connect(FRostersView,SIGNAL(viewModelChanged(QAbstractItemModel *)),
    SLOT(onViewModelChanged(QAbstractItemModel *)));
  connect(FRostersView,SIGNAL(destroyed(QObject *)), SLOT(onRostersViewDestroyed(QObject *)));
}

RostersViewPlugin::~RostersViewPlugin()
{
  delete FRostersView;
}

void RostersViewPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Roster View"); 
  APluginInfo->description = tr("Displays a hierarchical roster's model");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(ROSTERSMODEL_UUID);
}

bool RostersViewPlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterStreamJidAboutToBeChanged(IRoster *, const Jid &)),
        SLOT(onRosterStreamJidAboutToBeChanged(IRoster *, const Jid &)));
    }
  }

  plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(hidden(IAccount *)),SLOT(onAccountHidden(IAccount *)));
      connect(FAccountManager->instance(),SIGNAL(destroyed(const QUuid &)),SLOT(onAccountDestroyed(const QUuid &)));
    }
  }

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
   }
  }

  return FRostersModel!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  FSortFilterProxyModel = new SortFilterProxyModel(this, this);
  FSortFilterProxyModel->setSortLocaleAware(true);
  FSortFilterProxyModel->setDynamicSortFilter(true);
  FSortFilterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
  FSortFilterProxyModel->sort(0,Qt::AscendingOrder);
  FRostersView->insertProxyModel(FSortFilterProxyModel,RPO_ROSTERSVIEW_SORTFILTER);

  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ROSTERSVIEW_UUID);
    FSettingsPlugin->openOptionsNode(ON_ROSTER,tr("Roster"),tr("Roster view options"),MNI_ROSTERVIEW_OPTIONS,ONO_ROSTER);
    FSettingsPlugin->insertOptionsHolder(this);

    connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onViewIndexCollapsed(const QModelIndex &)));
    connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onViewIndexExpanded(const QModelIndex &)));
  }
 
  if (FMainWindowPlugin)
  {
    FShowOfflineAction = new Action(this);
    FShowOfflineAction->setIcon(RSR_STORAGE_MENUICONS, MNI_ROSTERVIEW_HIDE_OFFLINE);
    FShowOfflineAction->setToolTip(tr("Show/Hide offline contacts"));
    connect(FShowOfflineAction,SIGNAL(triggered(bool)),SLOT(onShowOfflineContactsAction(bool)));
    FMainWindowPlugin->mainWindow()->topToolBarChanger()->addAction(FShowOfflineAction,TBG_MWTTB_ROSTERSVIEW,false);
    
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);
  }

  if (FRostersModel)
  {
    FRostersModel->insertDefaultDataHolder(this);
    FRostersView->setRostersModel(FRostersModel);
  }

  return true;
}

QWidget *RostersViewPlugin::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_ROSTER)
  {
    AOrder = OWO_ROSTER;
    RosterOptionsWidget *widget = new RosterOptionsWidget(this);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

int RostersViewPlugin::rosterDataOrder() const
{
  return RDHO_DEFAULT;
}

QList<int> RostersViewPlugin::rosterDataRoles() const
{
  static QList<int> dataRoles  = QList<int>() 
    << Qt::DisplayRole 
    << Qt::BackgroundColorRole 
    << Qt::ForegroundRole
    << RDR_FONT_WEIGHT;
  return dataRoles;
}

QList<int> RostersViewPlugin::rosterDataTypes() const
{
  static QList<int> indexTypes  = QList<int>()  
    << RIT_STREAM_ROOT 
    << RIT_GROUP
    << RIT_GROUP_BLANK
    << RIT_GROUP_AGENTS
    << RIT_GROUP_MY_RESOURCES
    << RIT_GROUP_NOT_IN_ROSTER
    << RIT_CONTACT
    << RIT_AGENT
    << RIT_MY_RESOURCE;
  return indexTypes;
}

QVariant RostersViewPlugin::rosterData(const IRosterIndex *AIndex, int ARole) const
{
  switch (AIndex->type())
  {
  case RIT_STREAM_ROOT:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      {
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
          display = AIndex->data(RDR_JID).toString();
        return display;
      }
    case Qt::ForegroundRole:
      return Qt::white;
    case Qt::BackgroundColorRole:
      return Qt::gray;
    case RDR_FONT_WEIGHT:
      return QFont::Bold;
    } 
    break;

  case RIT_GROUP:
  case RIT_GROUP_BLANK:
  case RIT_GROUP_AGENTS:
  case RIT_GROUP_MY_RESOURCES:
  case RIT_GROUP_NOT_IN_ROSTER:
    switch (ARole)
    {
    case Qt::DisplayRole:  
      return AIndex->data(RDR_NAME);
    case Qt::ForegroundRole:
      return Qt::blue;
    case RDR_FONT_WEIGHT:
      return QFont::DemiBold;
    } 
    break;

  case RIT_CONTACT:
    switch (ARole)
    {
    case Qt::DisplayRole: 
      {
        Jid indexJid = AIndex->data(RDR_JID).toString();
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
          display = indexJid.bare();
        if (checkOption(IRostersView::ShowResource) && !indexJid.resource().isEmpty())
          display += "/" + indexJid.resource();
        return display;
      }
    } 
    break;

  case RIT_AGENT:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        QString display = AIndex->data(RDR_NAME).toString();
        if (display.isEmpty())
        {
          Jid indexJid = AIndex->data(RDR_JID).toString();
          display = indexJid.bare();
        }
        return display;
      }
    } 
    break;

  case RIT_MY_RESOURCE:
    switch (ARole)
    {
    case Qt::DisplayRole:
      {
        Jid indexJid = AIndex->data(RDR_JID).toString();
        return indexJid.resource();
      }
    } 
    break;
  }
  return QVariant();
}

bool RostersViewPlugin::setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue)
{
  Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
  return false;
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
  if (checkOption(AOption) != AValue)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    if (FRostersView)
      FRostersView->setOption(AOption,AValue);
    if (FSortFilterProxyModel)
      FSortFilterProxyModel->setOption(AOption,AValue);
    if (AOption == IRostersView::ShowOfflineContacts)
      FShowOfflineAction->setIcon(RSR_STORAGE_MENUICONS, AValue ? MNI_ROSTERVIEW_SHOW_OFFLINE : MNI_ROSTERVIEW_HIDE_OFFLINE);
    if (AOption==IRostersView::ShowResource || AOption==IRostersView::ShowStatusText)
      emit rosterDataChanged(NULL, Qt::DisplayRole);
    emit optionChanged(AOption,AValue);
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

void RostersViewPlugin::restoreExpandState(const QModelIndex &AParent)
{
  QAbstractItemModel *curModel = FRostersView->model();
  if (curModel)
  {
    if (AParent.isValid())
      loadExpandedState(AParent);
    int rows = curModel->rowCount(AParent);
    for (int row = 0; row<rows; row++)
    {
      QModelIndex index = curModel->index(row,0,AParent);
      if (curModel->rowCount(index) > 0)
        restoreExpandState(index);
    }
  }
}

QString RostersViewPlugin::getExpandSettingsName(const QModelIndex &AIndex)
{
  if (AIndex.isValid())
  {
    QString valueName;
    int itemType = AIndex.data(RDR_TYPE).toInt();
    if (itemType != RIT_STREAM_ROOT)
    {
      QString groupName = AIndex.data(RDR_GROUP).toString();
      if (!groupName.isEmpty())
      {
        QString groupHash = QString::number(qHash(groupName),16);
        valueName = QString(SVN_COLLAPSE_ACCOUNT_NS_GROUP).arg(groupHash);
      }
    }
    else
      valueName = SVN_COLLAPSE_ACCOUNT_NS;
    return valueName;
  }
  return QString::null;
}

void RostersViewPlugin::loadExpandedState(const QModelIndex &AIndex)
{
  QString settingsName = getExpandSettingsName(AIndex);
  if (FSettings && !settingsName.isEmpty())
  {
    Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
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
    Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
    QString ns = FCollapseNS.value(streamJid);
    if (FRostersView->isExpanded(AIndex))
    {
      if (AIndex.data(RDR_TYPE).toInt() == RIT_STREAM_ROOT)
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

void RostersViewPlugin::onViewModelAboutToBeReset()
{
  if (FRostersView->currentIndex().isValid())
  {
    FViewSavedState.currentIndex = (IRosterIndex *)FRostersView->mapToModel(FRostersView->currentIndex()).internalPointer();
    FViewSavedState.sliderPos = FRostersView->verticalScrollBar()->sliderPosition();
  }
  else
    FViewSavedState.currentIndex = NULL;
}

void RostersViewPlugin::onViewModelReset()
{
  restoreExpandState();
  if (FViewSavedState.currentIndex != NULL)
  {
    FRostersView->setCurrentIndex(FRostersView->mapFromModel(FRostersView->rostersModel()->modelIndexByRosterIndex(FViewSavedState.currentIndex)));
    FRostersView->verticalScrollBar()->setSliderPosition(FViewSavedState.sliderPos);
  }
}

void RostersViewPlugin::onViewModelAboutToBeChanged(QAbstractItemModel * /*AModel*/)
{
  if (FRostersView->model())
  {
    disconnect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),this,SLOT(onViewModelAboutToBeReset()));
    disconnect(FRostersView->model(),SIGNAL(modelReset()),this,SLOT(onViewModelReset()));
    disconnect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),this,SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
  }
}

void RostersViewPlugin::onViewModelChanged(QAbstractItemModel * /*AModel*/)
{
  if (FRostersView->model())
  {
    connect(FRostersView->model(),SIGNAL(modelAboutToBeReset()),SLOT(onViewModelAboutToBeReset()));
    connect(FRostersView->model(),SIGNAL(modelReset()),SLOT(onViewModelReset()));
    connect(FRostersView->model(),SIGNAL(rowsInserted(const QModelIndex &, int , int )),SLOT(onViewRowsInserted(const QModelIndex &, int , int )));
    startRestoreExpandState();
  }
}

void RostersViewPlugin::onViewRowsInserted(const QModelIndex &AParent, int AStart, int AEnd)
{
  for (int row=AStart; row<=AEnd; row++)
    loadExpandedState(AParent.child(row,0));
  loadExpandedState(AParent);
}

void RostersViewPlugin::onViewIndexCollapsed(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onViewIndexExpanded(const QModelIndex &AIndex)
{
  saveExpandedState(AIndex);
}

void RostersViewPlugin::onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter)
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
  if (AAccount->isActive())
    FCollapseNS.insert(AAccount->xmppStream()->streamJid(),AAccount->accountId().toString());
}

void RostersViewPlugin::onAccountHidden(IAccount *AAccount)
{
  if (AAccount->isActive())
    FCollapseNS.remove(AAccount->xmppStream()->streamJid());
}

void RostersViewPlugin::onAccountDestroyed(const QUuid &AAccountId)
{
  if (FSettings)
    FSettings->deleteValueNS(SVN_COLLAPSE_ACCOUNT_NS,AAccountId.toString());
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
  setOption(IRostersView::ShowResource,FSettings->value(SVN_SHOW_RESOURCE,true).toBool()); 
  setOption(IRostersView::ShowStatusText,FSettings->value(SVN_SHOW_STATUS,true).toBool()); 
}

void RostersViewPlugin::onSettingsClosed()
{
  FSettings->setValue(SVN_SHOW_OFFLINE_CONTACTS,checkOption(IRostersView::ShowOfflineContacts));
  FSettings->setValue(SVN_SHOW_ONLINE_FIRST,checkOption(IRostersView::ShowOnlineFirst));
  FSettings->setValue(SVN_SHOW_RESOURCE,checkOption(IRostersView::ShowResource));
  FSettings->setValue(SVN_SHOW_STATUS,checkOption(IRostersView::ShowStatusText));
}

void RostersViewPlugin::onShowOfflineContactsAction(bool)
{
  setOption(IRostersView::ShowOfflineContacts, !checkOption(IRostersView::ShowOfflineContacts));
}

Q_EXPORT_PLUGIN2(plg_rostersview, RostersViewPlugin)
