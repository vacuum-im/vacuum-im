#include "statuschanger.h"

#include <QTimer>
#include <QToolButton>

#define MAX_TEMP_STATUS_ID                  -10

#define ADR_STREAMJID                       Action::DR_StreamJid
#define ADR_STATUS_CODE                     Action::DR_Parametr1

#define SVN_LAST_ONLINE_MAIN_STATUS         "lastOnlineMainStatus"
#define SVN_MAIN_STATUS_ID                  "mainStatus"
#define SVN_MODIFY_STATUS                   "modifyStatus"
#define SVN_STATUS                          "status[]"
#define SVN_STATUS_CODE                     "status[]:code"
#define SVN_STATUS_NAME                     "status[]:name"
#define SVN_STATUS_SHOW                     "status[]:show"
#define SVN_STATUS_TEXT                     "status[]:text"
#define SVN_STATUS_PRIORITY                 "status[]:priority"

#define NOTIFICATOR_ID                      "StatusChanger"

StatusChanger::StatusChanger()
{
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FMainWindowPlugin = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FTrayManager = NULL;
  FSettingsPlugin = NULL;
  FAccountManager = NULL;
  FNotifications = NULL;

  FMainMenu = NULL;
  FStatusIcons = NULL;
  FConnectingLabel = RLID_NULL;
  FSettingStatusToPresence = NULL;
}

StatusChanger::~StatusChanger()
{
  if (!FEditStatusDialog.isNull())
    FEditStatusDialog->reject();
  FStatusItems.remove(STATUS_MAIN_ID);
  qDeleteAll(FStatusItems);
  if (FMainMenu)
    delete FMainMenu;
}

//IPlugin
void StatusChanger::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Status Changer"); 
  APluginInfo->description = tr("Managing and change status");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(PRESENCE_UUID);
}

bool StatusChanger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceAdded(IPresence *)),
        SLOT(onPresenceAdded(IPresence *)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceChanged(IPresence *, int, const QString &, int)),
        SLOT(onPresenceChanged(IPresence *, int, const QString &, int)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),
        SLOT(onPresenceRemoved(IPresence *)));
    }
  }

  plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
    }
  }

  plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  
  plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
    if (FRostersModel)
    {
      connect(FRostersModel->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),
        SLOT(onStreamJidChanged(const Jid &, const Jid &)));
    }
  }

  plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
  if (plugin)
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  
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

  plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(defaultIconsChanged()),SLOT(onDefaultStatusIconsChanged()));
    }
  }

  plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
  if (plugin)
  {
    FNotifications = qobject_cast<INotifications *>(plugin->instance());
    if (FNotifications)
    {
      connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
    }
  }

  return FPresencePlugin!=NULL;
}

bool StatusChanger::initObjects()
{
  FMainMenu = new Menu;

  FModifyStatus = new Action(FMainMenu);
  FModifyStatus->setCheckable(true);
  FModifyStatus->setText(tr("Modify status"));
  FModifyStatus->setIcon(RSR_STORAGE_MENUICONS, MNI_SCHANGER_MODIFY_STATUS);
  FMainMenu->addAction(FModifyStatus,AG_SCSM_STATUSCHANGER_ACTIONS,false);

  Action *editStatus = new Action(FMainMenu);
  editStatus->setText(tr("Edit statuses"));
  editStatus->setIcon(RSR_STORAGE_MENUICONS,MNI_SCHANGER_EDIT_STATUSES);
  connect(editStatus,SIGNAL(triggered(bool)), SLOT(onEditStatusAction(bool)));
  FMainMenu->addAction(editStatus,AG_SCSM_STATUSCHANGER_ACTIONS,false);
  
  createDefaultStatus();
  setMainStatusId(STATUS_OFFLINE);

  if (FSettingsPlugin)
    FSettingsPlugin->insertOptionsHolder(this);

  if (FMainWindowPlugin)
  {
    ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->bottomToolBarChanger();
    QToolButton *button = changer->addToolButton(FMainMenu->menuAction(),AG_DEFAULT,false);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  }

  if (FRostersViewPlugin)
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FConnectingLabel = FRostersView->createIndexLabel(RLO_CONNECTING,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_SCHANGER_CONNECTING),IRostersView::LabelBlink);
    connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)), SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
  }

  if (FTrayManager)
  {
    FTrayManager->addAction(FMainMenu->menuAction(),AG_TMTM_STATUSCHANGER,true);
  }

  if (FNotifications)
  {
    uchar kindMask = INotification::PopupWindow|INotification::PlaySound;
    FNotifications->insertNotificator(NOTIFICATOR_ID,tr("Connection errors"),kindMask,kindMask);
  }

  return true;
}

bool StatusChanger::startPlugin()
{
  foreach(IPresence *presence, FStreamStatus.keys())
  {
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(presence->streamJid()) : NULL;
    if (account!=NULL && account->value(AVN_AUTO_CONNECT,false).toBool())
    {
      int statusId = FStreamMainStatus.contains(presence) ? STATUS_MAIN_ID : account->value(AVN_LAST_ONLINE_STATUS, STATUS_ONLINE).toInt();
      if (!FStatusItems.contains(statusId))
        statusId = STATUS_ONLINE;
      setStreamStatus(presence->streamJid(), statusId);
    }
  }
  updateMainMenu();
  return true;
}

//IOptionsHolder
QWidget *StatusChanger::optionsWidget(const QString &ANode, int &AOrder)
{
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (FAccountManager && nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    AOrder = OWO_ACCOUNT_STATUS;
    AccountOptionsWidget *widget = new AccountOptionsWidget(FAccountManager,nodeTree.at(1));
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FAccountManager->instance(),SIGNAL(optionsAccepted()),widget,SLOT(apply()));
    connect(FAccountManager->instance(),SIGNAL(optionsRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

//IStatusChanger
Menu *StatusChanger::statusMenu() const
{
  return FMainMenu;
}

Menu *StatusChanger::streamMenu(const Jid &AStreamJid) const
{
  QMap<IPresence *, Menu *>::const_iterator it = FStreamMenu.constBegin();
  while (it!=FStreamMenu.constEnd())
  {
    if (it.key()->streamJid() == AStreamJid)
      return it.value();
    it++;
  }
  return NULL;
}

int StatusChanger::mainStatus() const
{
  return FStatusItems.value(STATUS_MAIN_ID)->code;
}

void StatusChanger::setMainStatus(int AStatusId)
{
  setStreamStatus(Jid(), AStatusId);
}

int StatusChanger::streamStatus(const Jid &AStreamJid) const
{
  QMap<IPresence *, int>::const_iterator it = FStreamStatus.constBegin();
  while (it!=FStreamStatus.constEnd())
  {
    if (it.key()->streamJid() == AStreamJid)
      return it.value();
    it++;
  }
  return !AStreamJid.isValid() ? mainStatus() : STATUS_NULL_ID;
}

void StatusChanger::setStreamStatus(const Jid &AStreamJid, int AStatusId)
{
  if (FStatusItems.contains(AStatusId)) 
  {
    bool changeMainStatus = !AStreamJid.isValid() && AStatusId != STATUS_MAIN_ID;

    if (changeMainStatus)
      setMainStatusId(AStatusId);

    StatusItem *status = FStatusItems.value(AStatusId);
    QMap<IPresence *, int>::const_iterator it = FStreamStatus.constBegin();
    while (it != FStreamStatus.constEnd())
    {
      IPresence *presence = it.key();
      
      bool setStatusToPresence = presence->streamJid() == AStreamJid;
      setStatusToPresence |= FStreamMenu.count() == 1;
      setStatusToPresence |= changeMainStatus && FStreamMainStatus.contains(presence);
      setStatusToPresence |= !AStreamJid.isValid() && AStatusId == STATUS_MAIN_ID;
      setStatusToPresence |= changeMainStatus && status->show == IPresence::Offline;
      if (setStatusToPresence)
      {
        if (AStatusId == STATUS_MAIN_ID)
          FStreamMainStatus += presence;
        else if (!changeMainStatus)
          FStreamMainStatus -= presence;

        emit statusAboutToBeChanged(presence->streamJid(), status->code);
        
        FSettingStatusToPresence = presence;
        if (!presence->setPresence(status->show,status->text,status->priority))
        {
          FSettingStatusToPresence = NULL;
          if (status->show != IPresence::Offline && !presence->xmppStream()->isOpen() && presence->xmppStream()->open())
          {
            insertConnectingLabel(presence);
            FStreamWaitStatus.insert(presence,FStreamMainStatus.contains(presence) ? STATUS_MAIN_ID : status->code);
          }
        }
        else
        {
          FSettingStatusToPresence = NULL;
          setStreamStatusId(presence,changeMainStatus ? STATUS_MAIN_ID : AStatusId);

          if (status->show == IPresence::Offline)
            presence->xmppStream()->close();
          
          if (!FStreamMainStatus.contains(presence))
            FStreamLastStatus.insert(presence->streamJid(),AStatusId);
          
          updateStreamMenu(presence);
          updateMainMenu();
          emit statusChanged(presence->streamJid(), status->code);
        }
      }
      it++;
    }
  }
}

QString StatusChanger::statusItemName(int AStatusId) const
{
  if (FStatusItems.contains(AStatusId))
    return FStatusItems.value(AStatusId)->name;
  return QString();
}

int StatusChanger::statusItemShow(int AStatusId) const
{
  if (FStatusItems.contains(AStatusId))
    return FStatusItems.value(AStatusId)->show;
  return -1;
}

QString StatusChanger::statusItemText(int AStatusId) const
{
  if (FStatusItems.contains(AStatusId))
    return FStatusItems.value(AStatusId)->text;
  return QString();
}

int StatusChanger::statusItemPriority(int AStatusId) const
{
  if (FStatusItems.contains(AStatusId))
    return FStatusItems.value(AStatusId)->priority;
  return 0;
}

QList<int> StatusChanger::statusItems() const
{
  return FStatusItems.keys();
}

QList<int> StatusChanger::activeStatusItems() const
{
  QList<int> active;
  foreach (int statusId, FStreamStatus)
    active.append(statusId == STATUS_MAIN_ID ? FStatusItems.value(STATUS_MAIN_ID)->code : statusId);
  return active;
}

QList<int> StatusChanger::statusByShow(int AShow) const
{
  QList<int> statuses;
  foreach(StatusItem *status, FStatusItems)
    if (status->show == AShow)
      statuses.append(status->code);
  return statuses;
}

int StatusChanger::statusByName(const QString &AName) const
{
  foreach(StatusItem *status, FStatusItems)
    if (status->name.toLower() == AName.toLower())
      return status->code;
  return STATUS_NULL_ID;
}

int StatusChanger::addStatusItem(const QString &AName, int AShow, const QString &AText, int APriority)
{
  int statusId = statusByName(AName);
  if (statusId == 0 && !AName.isEmpty())
  {
    while(statusId<=STATUS_MAX_STANDART_ID || FStatusItems.contains(statusId)) 
      statusId = qrand() + (qrand() << 16);
    StatusItem *status = new StatusItem;
    status->code = statusId;
    status->name = AName;
    status->show = AShow;
    status->text = AText;
    status->priority = APriority;
    FStatusItems.insert(statusId,status);
    createStatusActions(statusId);
    emit statusItemAdded(statusId);
  }
  else if (statusId > 0)
  {
    updateStatusItem(statusId,AName,AShow,AText,APriority);
  }
  return statusId;
}

void StatusChanger::updateStatusItem(int AStatusId, const QString &AName, int AShow, const QString &AText, int APriority)
{
  if (FStatusItems.contains(AStatusId) && !AName.isEmpty())
  {
    StatusItem *status = FStatusItems.value(AStatusId);
    if (status->name == AName || statusByName(AName) == STATUS_NULL_ID)
    {
      status->name = AName;
      status->show = AShow;
      status->text = AText;
      status->priority = APriority;
      updateStatusActions(AStatusId);
      emit statusItemChanged(AStatusId);
      resendUpdatedStatus(AStatusId);
    }
  }
}

void StatusChanger::removeStatusItem(int AStatusId)
{
  if (AStatusId>STATUS_MAX_STANDART_ID && FStatusItems.contains(AStatusId) && !activeStatusItems().contains(AStatusId))
  {
    emit statusItemRemoved(AStatusId);
    removeStatusActions(AStatusId);
    delete FStatusItems.take(AStatusId);
  }
}

QIcon StatusChanger::iconByShow(int AShow) const
{
  return FStatusIcons != NULL ? FStatusIcons->iconByStatus(AShow,"",false) : QIcon();
}

QString StatusChanger::nameByShow(int AShow) const 
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return tr("Offline");
  case IPresence::Online: 
    return tr("Online");
  case IPresence::Chat: 
    return tr("Chat");
  case IPresence::Away: 
    return tr("Away");
  case IPresence::ExtendedAway: 
    return tr("Extended Away");
  case IPresence::DoNotDisturb: 
    return tr("Do not disturb");
  case IPresence::Invisible: 
    return tr("Invisible");
  case IPresence::Error: 
    return tr("Error");
  default:
    return tr("Unknown Status");
  }
}

void StatusChanger::createDefaultStatus()
{
  StatusItem *status = new StatusItem;
  status->code = STATUS_ONLINE;
  status->name = nameByShow(IPresence::Online);
  status->show = IPresence::Online;
  status->text = tr("Online");
  status->priority = 30;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_CHAT;
  status->name = nameByShow(IPresence::Chat);
  status->show = IPresence::Chat;
  status->text = tr("Free for chat");
  status->priority = 25;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_AWAY;
  status->name = nameByShow(IPresence::Away);
  status->show = IPresence::Away;
  status->text = tr("I`am away from my desk");
  status->priority = 20;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_DND;
  status->name = nameByShow(IPresence::DoNotDisturb);
  status->show = IPresence::DoNotDisturb;
  status->text = tr("Do not disturb");
  status->priority = 15;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_EXAWAY;
  status->name = nameByShow(IPresence::ExtendedAway);
  status->show = IPresence::ExtendedAway;
  status->text = tr("Not available");
  status->priority = 10;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_INVISIBLE;
  status->name = nameByShow(IPresence::Invisible);
  status->show = IPresence::Invisible;
  status->text = tr("Disconnected");
  status->priority = 5;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_OFFLINE;
  status->name = nameByShow(IPresence::Offline);
  status->show = IPresence::Offline;
  status->text = tr("Disconnected");
  status->priority = 0;
  FStatusItems.insert(status->code,status);
  createStatusActions(status->code);

  status = new StatusItem;
  status->code = STATUS_ERROR_ID;
  status->name = nameByShow(IPresence::Error);
  status->show = IPresence::Error;
  status->text = tr("Error");
  status->priority = 0;
  FStatusItems.insert(status->code,status);
}

void StatusChanger::setMainStatusId(int AStatusId)
{
  if (FStatusItems.contains(AStatusId))
  {
    FStatusItems[STATUS_MAIN_ID] = FStatusItems.value(AStatusId);
    updateMainStatusActions();
  }
}

void StatusChanger::setStreamStatusId(IPresence *APresence, int AStatusId)
{
  if (FStatusItems.contains(AStatusId))
  {
    FStreamStatus[APresence] = AStatusId;
    if (AStatusId > MAX_TEMP_STATUS_ID)
      removeTempStatus(APresence);

    IRosterIndex *index = FRostersView && FRostersModel ? FRostersModel->streamRoot(APresence->streamJid()) : NULL;
    if (APresence->show() == IPresence::Error)
    {
      if (index && !FRostersViewPlugin->checkOption(IRostersView::ShowStatusText))
        FRostersView->insertFooterText(FTO_ROSTERSVIEW_STATUS,APresence->status(),index);
      if (!FStreamNotify.contains(APresence))
        insertStatusNotification(APresence);
    }
    else
    {
      if (index && !FRostersViewPlugin->checkOption(IRostersView::ShowStatusText))
        FRostersView->removeFooterText(FTO_ROSTERSVIEW_STATUS,index);
      removeStatusNotification(APresence);
    }
  }
}

Action *StatusChanger::createStatusAction(int AStatusId, const Jid &AStreamJid, QObject *AParent) const
{
  StatusItem *status = FStatusItems.value(AStatusId);

  Action *action = new Action(AParent);
  if (AStreamJid.isValid())
    action->setData(ADR_STREAMJID,AStreamJid.full());
  action->setData(ADR_STATUS_CODE,AStatusId);
  connect(action,SIGNAL(triggered(bool)),SLOT(onSetStatusByAction(bool)));
  updateStatusAction(status,action);

  return action;
}

void StatusChanger::updateStatusAction(StatusItem *AStatus, Action *AAction) const
{
  AAction->setText(AStatus->name);
  AAction->setIcon(iconByShow(AStatus->show));

  int sortShow = AStatus->show != IPresence::Offline ? AStatus->show : 100;
  AAction->setData(Action::DR_SortString,QString("%1-%2").arg(sortShow,5,10,QChar('0')).arg(AStatus->name));
}

void StatusChanger::createStatusActions(int AStatusId)
{
  int group = AStatusId > STATUS_MAX_STANDART_ID ? AG_SCSM_STATUSCHANGER_CUSTOM_STATUS : AG_SCSM_STATUSCHANGER_DEFAULT_STATUS;

  FMainMenu->addAction(createStatusAction(AStatusId,Jid(),FMainMenu),group,true);
  for (QMap<IPresence *, Menu *>::const_iterator it = FStreamMenu.constBegin(); it!=FStreamMenu.constEnd(); it++)
    it.value()->addAction(createStatusAction(AStatusId,it.key()->streamJid(),it.value()),group,true);
}

void StatusChanger::updateStatusActions(int AStatusId)
{
  StatusItem *status = FStatusItems.value(AStatusId);

  QMultiHash<int, QVariant> data;
  data.insert(ADR_STATUS_CODE,AStatusId);
  QList<Action *> actionList = FMainMenu->findActions(data,true);
  foreach (Action *action, actionList)
    updateStatusAction(status,action);
}

void StatusChanger::removeStatusActions(int AStatusId)
{
  QMultiHash<int, QVariant> data;
  data.insert(ADR_STATUS_CODE,AStatusId);
  qDeleteAll(FMainMenu->findActions(data,true));
}

void StatusChanger::createStreamMenu(IPresence *APresence)
{
  if (!FStreamMenu.contains(APresence))
  {
    Jid streamJid = APresence->streamJid();
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(streamJid) : NULL;

    Menu *sMenu = new Menu(FMainMenu);
    if (account)
    {
      sMenu->setTitle(account->name());
      connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),SLOT(onAccountChanged(const QString &, const QVariant &)));
    }
    else
      sMenu->setTitle(APresence->streamJid().hFull());
    FStreamMenu.insert(APresence,sMenu);

    QMap<int, StatusItem *>::const_iterator it = FStatusItems.constBegin();
    while (it != FStatusItems.constEnd())
    {
      if (it.key() > STATUS_MAX_STANDART_ID)
        sMenu->addAction(createStatusAction(it.key(),streamJid,sMenu),AG_SCSM_STATUSCHANGER_CUSTOM_STATUS,true);
      else if (it.key() > STATUS_NULL_ID)
        sMenu->addAction(createStatusAction(it.key(),streamJid,sMenu),AG_SCSM_STATUSCHANGER_DEFAULT_STATUS,true);
      it++;
    }

    Action *action = createStatusAction(STATUS_MAIN_ID, streamJid, sMenu);
    action->setData(ADR_STATUS_CODE, STATUS_MAIN_ID);
    sMenu->addAction(action,AG_SCSM_STATUSCHANGER_STREAMS,true);
    FStreamMainStatusAction.insert(APresence,action);

    FMainMenu->addAction(sMenu->menuAction(),AG_SCSM_STATUSCHANGER_STREAMS,true);
  }
}

void StatusChanger::updateStreamMenu(IPresence *APresence)
{
  int statusId = FStreamStatus.value(APresence,STATUS_MAIN_ID);

  Menu *sMenu = FStreamMenu.value(APresence);
  if (sMenu)
    sMenu->setIcon(iconByShow(statusItemShow(statusId)));

  Action *mAction = FStreamMainStatusAction.value(APresence);
  if (mAction)
    mAction->setVisible(FStreamStatus.value(APresence) != STATUS_MAIN_ID);
}

void StatusChanger::removeStreamMenu(IPresence *APresence)
{
  if (FStreamMenu.contains(APresence))
  {
    FStreamMainStatusAction.remove(APresence);
    FStreamStatus.remove(APresence);
    FStreamWaitStatus.remove(APresence);
    FStreamLastStatus.remove(APresence->streamJid());
    FStreamWaitReconnect.remove(APresence);
    removeTempStatus(APresence);
    delete FStreamMenu.take(APresence);
  }
}

void StatusChanger::updateMainMenu()
{
  int mainStatusToShow = STATUS_MAIN_ID;
  
  if (FStreamStatus.isEmpty())
  {
    mainStatusToShow = STATUS_OFFLINE;
    FMainMenu->menuAction()->setEnabled(false);
  }
  else
  {
    QList<int> statusOnline, statusOffline;
    QMap<IPresence *, int>::const_iterator it = FStreamStatus.constBegin();
    while (it != FStreamStatus.constEnd())
    {
      if (it.key()->xmppStream()->isOpen())
        statusOnline.append(it.value());
      else
        statusOffline.append(it.value());
      it++;
    }
    if (!statusOnline.isEmpty() && !statusOnline.contains(STATUS_MAIN_ID))
      mainStatusToShow = statusOnline.first();
    else if (statusOnline.isEmpty() && !statusOffline.contains(STATUS_MAIN_ID))
      mainStatusToShow = statusOffline.first();

    FMainMenu->menuAction()->setEnabled(true);
  }
  
  QIcon mStatusIcon = iconByShow(statusItemShow(mainStatusToShow));
  QString mStatusName = statusItemName(mainStatusToShow);
  FMainMenu->setIcon(mStatusIcon);
  FMainMenu->setTitle(mStatusName);

  if (FTrayManager)
    FTrayManager->setMainIcon(mStatusIcon);
}

void StatusChanger::updateTrayToolTip()
{
  QString trayToolTip;
  QMap<IPresence *, int>::const_iterator it = FStreamStatus.constBegin();
  while (it != FStreamStatus.constEnd())
  {
    IAccount *account = FAccountManager->accountByStream(it.key()->streamJid());
    if (!trayToolTip.isEmpty())
      trayToolTip+="\n";
    trayToolTip += tr("%1 - %2").arg(account->name()).arg(it.key()->status());
    it++;
  }
  if (FTrayManager)
    FTrayManager->setMainToolTip(trayToolTip);
}

void StatusChanger::updateMainStatusActions()
{
  QIcon mStatusIcon = iconByShow(statusItemShow(STATUS_MAIN_ID));
  QString mStatusName = statusItemName(STATUS_MAIN_ID);
  foreach (Action *action, FStreamMainStatusAction)
  {
    action->setIcon(mStatusIcon);
    action->setText(mStatusName);
  }
}

void StatusChanger::insertConnectingLabel(IPresence *APresence)
{
  if (FRostersModel && FRostersView)
  {
    IRosterIndex *index = FRostersModel->streamRoot(APresence->xmppStream()->streamJid());
    if (index)
      FRostersView->insertIndexLabel(FConnectingLabel,index);
  }
}

void StatusChanger::removeConnectingLabel(IPresence *APresence)
{
  if (FRostersModel && FRostersView)
  {
    IRosterIndex *index = FRostersModel->streamRoot(APresence->xmppStream()->streamJid());
    if (index)
      FRostersView->removeIndexLabel(FConnectingLabel,index);
  }
}

void StatusChanger::autoReconnect(IPresence *APresence)
{
  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
  if (account && account->value(AVN_AUTO_RECONNECT,true).toBool())
  {
    int statusId = FStreamWaitStatus.value(APresence, FStreamStatus.value(APresence));
    int statusShow = statusItemShow(statusId);
    if (statusShow != IPresence::Offline && statusShow != IPresence::Error)
    {
      int waitTime = account->value(AVN_RECONNECT_TIME,30).toInt();
      FStreamWaitReconnect.insert(APresence,QPair<QDateTime,int>(QDateTime::currentDateTime().addSecs(waitTime),statusId));
      QTimer::singleShot(waitTime*1000+100,this,SLOT(onReconnectTimer()));
    }
  }
}

int StatusChanger::createTempStatus(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
  removeTempStatus(APresence);
  StatusItem *status = new StatusItem;
  status->name = nameByShow(AShow).append('*');
  status->show = AShow;
  status->text = AText;
  status->priority = APriority;
  status->code = MAX_TEMP_STATUS_ID;
  while (FStatusItems.contains(status->code))
    status->code--;
  FStatusItems.insert(status->code,status);
  FStreamTempStatus.insert(APresence,status->code);
  return status->code;
}

void StatusChanger::removeTempStatus(IPresence *APresence)
{
  if (FStreamTempStatus.contains(APresence))
    if (!activeStatusItems().contains(FStreamTempStatus.value(APresence)))
      delete FStatusItems.take(FStreamTempStatus.take(APresence));
}

void StatusChanger::resendUpdatedStatus(int AStatusId)
{
  if (FStatusItems[STATUS_MAIN_ID]->code == AStatusId)
    setMainStatus(AStatusId);

  QMap<IPresence *,int>::const_iterator it;
  for (it = FStreamStatus.constBegin(); it != FStreamStatus.constEnd(); it++)
    if (it.value() == AStatusId)
      setStreamStatus(it.key()->streamJid(), AStatusId);
}

void StatusChanger::removeAllCustomStatuses()
{
  QList<int> statusList = FStatusItems.keys();
  foreach (int statusId, statusList)
    if (statusId > STATUS_MAX_STANDART_ID)
      removeStatusItem(statusId);
}

void StatusChanger::insertStatusNotification(IPresence *APresence)
{
  removeStatusNotification(APresence);
  if (FNotifications)
  {
    INotification notify;
    notify.kinds = FNotifications->notificatorKinds(NOTIFICATOR_ID);
    notify.data.insert(NDR_ICON,FStatusIcons!=NULL ? FStatusIcons->iconByStatus(IPresence::Error,"","") : QIcon());
    notify.data.insert(NDR_WINDOW_CAPTION, tr("Connection error"));
    notify.data.insert(NDR_WINDOW_TITLE,FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid())->name() : APresence->streamJid().full());
    notify.data.insert(NDR_WINDOW_IMAGE, FNotifications->contactAvatar(APresence->streamJid()));
    notify.data.insert(NDR_WINDOW_TEXT,APresence->status());
    notify.data.insert(NDR_WINDOW_TEXT,APresence->status());
    notify.data.insert(NDR_SOUND_FILE,SDF_SCHANGER_CONNECTION_ERROR);
    FStreamNotify.insert(APresence,FNotifications->appendNotification(notify));
  }
}

void StatusChanger::removeStatusNotification(IPresence *APresence)
{
  if (FNotifications && FStreamNotify.contains(APresence))
  {
    FNotifications->removeNotification(FStreamNotify.take(APresence));
  }
}

void StatusChanger::onSetStatusByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAMJID).toString();
    int statusId = action->data(ADR_STATUS_CODE).toInt();
    if (FModifyStatus->isChecked())
    {
      delete FModifyStatusDialog;
      FModifyStatusDialog = new ModifyStatusDialog(this,statusId,streamJid,NULL);
      FModifyStatusDialog->show();
    }
    else
      setStreamStatus(streamJid, statusId);
  }
}

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
  if (FStreamMenu.count() == 1)
    FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(true);

  createStreamMenu(APresence);
  FStreamStatus.insert(APresence,STATUS_OFFLINE);

  if (FStreamMenu.count() == 1)
    FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(false);

  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
  if (account && account->value(AVN_IS_MAIN_STATUS, true).toBool())
    FStreamMainStatus += APresence;
  
  updateStreamMenu(APresence);
  updateMainMenu();
  updateTrayToolTip();
}

void StatusChanger::onPresenceChanged(IPresence *APresence, int AShow, const QString &AText, int APriority)
{
  if (FStreamStatus.contains(APresence))
  {
    if (AShow == IPresence::Error)
    {
      autoReconnect(APresence);
      setStreamStatusId(APresence, STATUS_ERROR_ID);
      updateStreamMenu(APresence);
      updateMainMenu();
    }
    else if (FSettingStatusToPresence != APresence)
    {
      StatusItem *item = FStatusItems.value(FStreamStatus.value(APresence),NULL);
      if (!item || item->show!=AShow || item->priority!=APriority || item->text!=AText)
      {
        setStreamStatusId(APresence, createTempStatus(APresence,AShow,AText,APriority));
        updateStreamMenu(APresence);
        updateMainMenu();
      }
    }
    if (FStreamWaitStatus.contains(APresence))
    {
      FStreamWaitStatus.remove(APresence);
      removeConnectingLabel(APresence);
    }
    updateTrayToolTip();
  }
}

void StatusChanger::onPresenceRemoved(IPresence *APresence)
{
  IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(APresence->streamJid()) : NULL;
  if (account)
  {
    bool isMainStatus = FStreamMainStatus.contains(APresence);
    account->setValue(AVN_IS_MAIN_STATUS,isMainStatus);
    if (!isMainStatus && account->value(AVN_AUTO_CONNECT,false).toBool() && FStreamLastStatus.contains(APresence->streamJid()))  
      account->setValue(AVN_LAST_ONLINE_STATUS,FStreamLastStatus.take(APresence->streamJid()));
    else
      account->delValue(AVN_LAST_ONLINE_STATUS);
  }

  removeStatusNotification(APresence);
  removeStreamMenu(APresence);

  if (FStreamMenu.count() == 1)
    FStreamMenu.value(FStreamMenu.keys().first())->menuAction()->setVisible(false);
  
  updateMainMenu();
  updateTrayToolTip();
}

void StatusChanger::onRosterOpened(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (FStreamWaitStatus.contains(presence))
    setStreamStatus(presence->streamJid(), FStreamWaitStatus.value(presence));
}

void StatusChanger::onRosterClosed(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (FStreamWaitStatus.contains(presence))
    setStreamStatus(presence->streamJid(), FStreamWaitStatus.value(presence));
}

void StatusChanger::onStreamJidChanged(const Jid &ABefour, const Jid &AAfter)
{
  QMultiHash<int,QVariant> data;
  data.insert(ADR_STREAMJID,ABefour.full());
  QList<Action *> actionList = FMainMenu->findActions(data,true);
  foreach (Action *action, actionList)
    action->setData(ADR_STREAMJID,AAfter.full());
}

void StatusChanger::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->data(RDR_TYPE).toInt() == RIT_STREAM_ROOT)
  {
    QString streamJid = AIndex->data(RDR_STREAM_JID).toString();
    Menu *menu = FStreamMenu.count() > 1 ? streamMenu(streamJid) : FMainMenu;
    if (menu)
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Status"));
      action->setMenu(menu);
      action->setIcon(menu->menuAction()->icon());
      AMenu->addAction(action,AG_RVCM_STATUSCHANGER,true);
    }
  }
}

void StatusChanger::onDefaultStatusIconsChanged()
{
  foreach (StatusItem *statusItem, FStatusItems)
    updateStatusActions(statusItem->code);
  foreach (IPresence *presence, FStreamMenu.keys())
    updateStreamMenu(presence);
  updateMainStatusActions();
  updateMainMenu();
}

void StatusChanger::onSettingsOpened()
{
  removeAllCustomStatuses();

  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSCHANGER_UUID);
  QList<QString> nsList = settings->values(SVN_STATUS_CODE).keys();
  foreach (QString ns, nsList)
  {
    int statusId = settings->valueNS(SVN_STATUS_CODE,ns).toInt();
    QString statusName = settings->valueNS(SVN_STATUS_NAME,ns).toString();
    if (statusId > STATUS_MAX_STANDART_ID)
    {
      if (!statusName.isEmpty() && statusByName(statusName)==STATUS_NULL_ID)
      {
        StatusItem *status = new StatusItem;
        status->code = statusId;
        status->name = statusName;
        status->show = (IPresence::Show)settings->valueNS(SVN_STATUS_SHOW,ns).toInt();
        status->text = settings->valueNS(SVN_STATUS_TEXT,ns).toString();
        status->priority = settings->valueNS(SVN_STATUS_PRIORITY,ns).toInt();
        FStatusItems.insert(status->code,status);
        createStatusActions(status->code);
      }
    }
    else if (statusId > STATUS_NULL_ID)
    {
      StatusItem *status = FStatusItems.value(statusId);
      if (status)
      {
        if (!statusName.isEmpty())
          status->name = statusName;
        status->text = settings->valueNS(SVN_STATUS_TEXT,ns,status->text).toString();
        status->priority = settings->valueNS(SVN_STATUS_PRIORITY,ns,status->priority).toInt();
        updateStatusActions(statusId);
      }
    }
  }

  int mainStatusId = settings->value(SVN_MAIN_STATUS_ID,STATUS_OFFLINE).toInt();
  FModifyStatus->setChecked(settings->value(SVN_MODIFY_STATUS,false).toBool());
  setMainStatusId(mainStatusId);
}

void StatusChanger::onSettingsClosed()
{
  delete FEditStatusDialog;
  delete FModifyStatusDialog;

  ISettings *settings = FSettingsPlugin->settingsForPlugin(STATUSCHANGER_UUID);
  QSet<QString> oldNS = settings->values(SVN_STATUS_CODE).keys().toSet();
  foreach (StatusItem *status, FStatusItems)
  {
    QString ns = QString::number(status->code);
    if (status->code > STATUS_MAX_STANDART_ID)
    {
      settings->setValueNS(SVN_STATUS_CODE,ns,status->code);
      settings->setValueNS(SVN_STATUS_NAME,ns,status->name);
      settings->setValueNS(SVN_STATUS_SHOW,ns,status->show);
      settings->setValueNS(SVN_STATUS_TEXT,ns,status->text);
      settings->setValueNS(SVN_STATUS_PRIORITY,ns,status->priority);
    }
    else if (status->code > STATUS_NULL_ID)
    {
      settings->setValueNS(SVN_STATUS_CODE,ns,status->code);
      settings->setValueNS(SVN_STATUS_NAME,ns,status->name);
      settings->setValueNS(SVN_STATUS_TEXT,ns,status->text);
      settings->setValueNS(SVN_STATUS_PRIORITY,ns,status->priority);
    }
    oldNS -= ns;
  }

  foreach(QString ns, oldNS)
    settings->deleteValueNS(SVN_STATUS,ns);

  settings->setValue(SVN_MAIN_STATUS_ID,FStatusItems.value(STATUS_MAIN_ID)->code);
  settings->setValue(SVN_MODIFY_STATUS, FModifyStatus->isChecked());

  setMainStatusId(STATUS_OFFLINE);
  removeAllCustomStatuses();
}

void StatusChanger::onReconnectTimer()
{
  QMap<IPresence *,QPair<QDateTime,int> >::iterator it = FStreamWaitReconnect.begin();
  while (it != FStreamWaitReconnect.end())
  {
    if (it.value().first <= QDateTime::currentDateTime()) 
    {
      IPresence *presence = it.key();
      int statusId = FStatusItems.contains(it.value().second) ? it.value().second : STATUS_ONLINE;
      it = FStreamWaitReconnect.erase(it);
      if (presence->show() == IPresence::Error)
        setStreamStatus(presence->streamJid(), statusId);
    }
    else
    {
      it++;
    }
  }
}

void StatusChanger::onEditStatusAction(bool)
{
  if (FEditStatusDialog.isNull())
  {
    FEditStatusDialog = new EditStatusDialog(this);
    FEditStatusDialog->show();
  }
  else
    FEditStatusDialog->show();
}

void StatusChanger::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == AVN_NAME)
  {
    IAccount *account = qobject_cast<IAccount *>(sender());
    if (account)
    {
      Menu *sMenu = streamMenu(account->streamJid());
      if (sMenu)
        sMenu->setTitle(AValue.toString());
    }
  }
}

void StatusChanger::onNotificationActivated(int ANotifyId)
{
  if (FStreamNotify.values().contains(ANotifyId))
     FNotifications->removeNotification(ANotifyId);
}

Q_EXPORT_PLUGIN2(StatusChangerPlugin, StatusChanger)
