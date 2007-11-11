#include <QtDebug>
#include "rostersviewplugin.h"

#include <QTimer>

#define SVN_SHOW_OFFLINE_CONTACTS         "showOfflineContacts"
#define SVN_SHOW_ONLINE_FIRST             "showOnlineFirst"
#define SVN_SHOW_FOOTER_TEXT              "showFooterText"
#define SVN_SHOW_RESOURCE                 "showResource"

#define IN_STATUS_OFFLINE                 "status/offline"

#define SVN_COLLAPSE                      "collapse"
#define SVN_ACCOUNT                       "account"
#define SVN_GROUP                         "h%1" 
#define SVN_COLLAPSE_ACCOUNT              SVN_COLLAPSE ":" SVN_ACCOUNT
#define SVN_COLLAPSE_ACCOUNT_NS           SVN_COLLAPSE_ACCOUNT "[]"
#define SVN_COLLAPSE_ACCOUNT_NS_GROUP     SVN_COLLAPSE_ACCOUNT_NS ":" SVN_GROUP

RostersViewPlugin::RostersViewPlugin()
{
  FRostersModelPlugin = NULL;
  FMainWindowPlugin = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FRostersView = NULL;
  FRosterPlugin = NULL;
  FAccountManager = NULL;

  FIndexDataHolder = NULL;
  FSortFilterProxyModel = NULL;
  FLastModel = NULL;
  FShowOfflineAction = NULL;
  FOptions = 0;
  FStartRestoreExpandState = false;

  FSaveExpandStatusTypes 
    << RIT_Group 
    << RIT_AgentsGroup 
    << RIT_BlankGroup 
    << RIT_MyResourcesGroup
    << RIT_NotInRosterGroup
    << RIT_StreamRoot;
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
      connect(FAccountManager->instance(),SIGNAL(destroyed(IAccount *)),SLOT(onAccountDestroyed(IAccount *)));
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

  return FRostersModelPlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool RostersViewPlugin::initObjects()
{
  FIndexDataHolder = new IndexDataHolder(this);
  FRostersView = new RostersView;
  connect(FRostersView,SIGNAL(modelAboutToBeSeted(IRostersModel *)),
    SLOT(onModelAboutToBeSeted(IRostersModel *)));
  connect(FRostersView,SIGNAL(modelSeted(IRostersModel *)),
    SLOT(onModelSeted(IRostersModel *)));
  connect(FRostersView,SIGNAL(destroyed(QObject *)),SLOT(onRostersViewDestroyed(QObject *)));
  
  if (FSettingsPlugin)
  {
    FSettings = FSettingsPlugin->settingsForPlugin(ROSTERSVIEW_UUID);
    FSettingsPlugin->openOptionsNode(ON_ROSTER,tr("Roster"),tr("Roster view options"),QIcon());
    FSettingsPlugin->appendOptionsHolder(this);

    connect(FRostersView,SIGNAL(proxyModelAdded(QAbstractProxyModel *)),
      SLOT(onProxyAdded(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(proxyModelRemoved(QAbstractProxyModel *)),
      SLOT(onProxyRemoved(QAbstractProxyModel *)));
    connect(FRostersView,SIGNAL(indexInserted(const QModelIndex &, int, int)),
      SLOT(onIndexInserted(const QModelIndex &, int, int)));
    connect(FRostersView,SIGNAL(collapsed(const QModelIndex &)),SLOT(onIndexCollapsed(const QModelIndex &)));
    connect(FRostersView,SIGNAL(expanded(const QModelIndex &)),SLOT(onIndexExpanded(const QModelIndex &)));
  }
  emit viewCreated(FRostersView);
 
  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FMainWindowPlugin->mainWindow()->rostersWidget()->insertWidget(0,FRostersView);

    FShowOfflineAction = new Action(this);
    FShowOfflineAction->setIcon(STATUS_ICONSETFILE,IN_STATUS_OFFLINE);
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
  bool changed = checkOption(AOption) != AValue;
  AValue ? FOptions |= AOption : FOptions &= ~AOption;
  if (changed)
  {
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
    bool isCollapsed = FSettings->valueNS(settingsName,streamJid.pFull(),false).toBool();
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

void RostersViewPlugin::onRostersViewDestroyed(QObject *)
{
  emit viewDestroyed(FRostersView);
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

void RostersViewPlugin::onProxyAdded(QAbstractProxyModel * /*AProxyModel*/)
{
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
  if (FSettings)
  {
    Jid befour = ARoster->streamJid();
    if (befour && AAfter)
    {
      QDomElement elem = FSettingsPlugin->pluginNode(ROSTERSVIEW_UUID).firstChildElement(SVN_COLLAPSE).firstChildElement(SVN_ACCOUNT);
      while(!elem.isNull() && elem.attribute("ns")!=befour.pFull())
        elem = elem.nextSiblingElement(SVN_ACCOUNT);
      if (!elem.isNull())
        elem.setAttribute("ns",AAfter.pFull());
    }
    else
      FSettings->deleteValueNS(SVN_COLLAPSE_ACCOUNT_NS,befour.pFull());
  }
}

void RostersViewPlugin::onAccountDestroyed(IAccount *AAccount)
{
  if (FSettings)
  {
    FSettings->deleteValueNS(SVN_COLLAPSE_ACCOUNT_NS,AAccount->streamJid().pFull());
  }
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
