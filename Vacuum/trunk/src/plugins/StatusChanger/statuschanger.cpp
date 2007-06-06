#include "statuschanger.h"
#include <QTimer>
#include <QToolButton>

StatusChanger::StatusChanger()
{
  FBaseShow = IPresence::Error;
  FConnectingLabel = NULL_LABEL_ID;
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FMainWindowPlugin = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FRostersModelPlugin = NULL;
  FTrayManager = NULL;
  mnuBase = NULL;
}

StatusChanger::~StatusChanger()
{
  if (mnuBase)
    delete mnuBase;
}

//IPlugin
void StatusChanger::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing and change status");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Status Changer"); 
  APluginInfo->uid = STATUSCHANGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{511A07C4-FFA4-43ce-93B0-8C50409AFC0E}"); //IPresence  
}

bool StatusChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = STATUSCHANGER_INITORDER;

  IPlugin *plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(plugin->instance(),SIGNAL(presenceAdded(IPresence *)),
        SLOT(onPresenceAdded(IPresence *)));
      connect(plugin->instance(),SIGNAL(selfPresence(IPresence *, IPresence::Show, const QString &, qint8, const Jid &)),
        SLOT(onSelfPresence(IPresence *, IPresence::Show, const QString &, qint8, const Jid &)));
      connect(plugin->instance(),SIGNAL(presenceRemoved(IPresence *)),
        SLOT(onPresenceRemoved(IPresence *)));
    }
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
    }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }
  
  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
      connect(FAccountManager->instance(),SIGNAL(shown(IAccount *)),SLOT(onAccountShown(IAccount *)));
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(contextMenu(int, Menu *)),SLOT(onTrayContextMenu(int, Menu *)));
    }
  }
  
  return FPresencePlugin!=NULL && FMainWindowPlugin!=NULL;
}

bool StatusChanger::initObjects()
{
  FRosterIconset.openFile(ROSTER_ICONSETFILE);
  connect(&FRosterIconset,SIGNAL(skinChanged()),SLOT(onSkinChanged()));

  mnuBase = new Menu(NULL);
  createStatusActions(NULL);
 
  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    QToolButton *tbutton = new QToolButton;
    tbutton->setDefaultAction(mnuBase->menuAction());
    tbutton->setPopupMode(QToolButton::InstantPopup);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tbutton->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    FMainWindowPlugin->mainWindow()->bottomToolBar()->addWidget(tbutton);
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FConnectingLabel = FRostersView->createIndexLabel(STREAMCONNECTING_LABEL_ORDER,FRosterIconset.iconByName("connecting"));
    connect(FRostersView,SIGNAL(contextMenu(const QModelIndex &, Menu *)),
      SLOT(onRostersViewContextMenu(const QModelIndex &, Menu *)));
  }

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersModel = FRostersModelPlugin->rostersModel();
  }

  return true;
}

bool StatusChanger::initSettings()
{
  setBaseShow(IPresence::Offline);
  return true;
}


//IStatusChanger
Menu *StatusChanger::streamMenu(const Jid &AStreamJid) const
{
  QHash<IPresence *, Menu *>::const_iterator i = FStreamMenus.constBegin();
  while (i!=FStreamMenus.constEnd())
  {
    if (i.key()->streamJid() == AStreamJid)
      return i.value();
    i++;
  }
  return NULL;
}

void StatusChanger::setPresence(IPresence::Show AShow, const QString &AStatus, 
                                int APriority, const Jid &AStreamJid)
{
  IPresence *presence;
  QList<IPresence *> presences;

  if (!AStreamJid.isValid())
  {
    foreach(presence,FPresences)
      if (presence->show() == FBaseShow || AShow == IPresence::Offline)
        presences.append(presence);

    if (presences.isEmpty())
      presences = FPresences;
  } 
  else
  {
    presence = FPresencePlugin->getPresence(AStreamJid);
    if (presence)
      presences.append(presence);
  }

  foreach(presence,presences)
  {
    FWaitReconnect.remove(presence);
    if (AShow == IPresence::Offline)
    {
      presence->setPresence(AShow,AStatus,APriority,Jid());
      presence->xmppStream()->close();
      removeWaitOnline(presence);
    }
    else if (!FWaitOnline.contains(presence))
    {
      if (!presence->setPresence(AShow,AStatus,APriority,Jid()))
      {
        if (!presence->xmppStream()->isOpen())
        {
          PresenceItem item;
          item.show = AShow;
          item.status = AStatus;
          item.priority = APriority;
          insertWaitOnline(presence,item);
          presence->xmppStream()->open();
        }
      }
    }
  }
}

void StatusChanger::onChangeStatusAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IPresence::Show show = (IPresence::Show)action->data(Action::DR_Parametr1).toInt();
    QString status = action->data(Action::DR_Parametr2).toString();
    int priority = action->data(Action::DR_Parametr3).toInt();
    setPresence(show,status,priority,streamJid);
  }
}

void StatusChanger::setBaseShow(IPresence::Show AShow)
{
  if (AShow != FBaseShow)
  {
    FBaseShow = AShow; 
    updateMenu();
  }
}

void StatusChanger::createStatusActions(IPresence *APresence)
{
  Menu *menu = mnuBase;
  QString streamJid;
  if (APresence)
  {
    streamJid = APresence->streamJid().pFull();
    menu = FStreamMenus.value(APresence);
  }

  Action *action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::Online));
  action->setText(getStatusName(IPresence::Online));
  action->setData(Action::DR_Parametr1,IPresence::Online);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Online));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Online));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::Chat));
  action->setText(getStatusName(IPresence::Chat));
  action->setData(Action::DR_Parametr1,IPresence::Chat);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Chat));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Chat));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::Away));
  action->setText(getStatusName(IPresence::Away));
  action->setData(Action::DR_Parametr1,IPresence::Away);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Away));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Away));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::ExtendedAway));
  action->setText(getStatusName(IPresence::ExtendedAway));
  action->setData(Action::DR_Parametr1,IPresence::ExtendedAway);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::ExtendedAway));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::ExtendedAway));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::DoNotDistrib));
  action->setText(getStatusName(IPresence::DoNotDistrib));
  action->setData(Action::DR_Parametr1,IPresence::DoNotDistrib);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::DoNotDistrib));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::DoNotDistrib));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::Invisible));
  action->setText(getStatusName(IPresence::Invisible));
  action->setData(Action::DR_Parametr1,IPresence::Invisible);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Invisible));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Invisible));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(STATUS_ICONSETFILE,getStatusIconName(IPresence::Offline));
  action->setText(getStatusName(IPresence::Offline));
  action->setData(Action::DR_Parametr1,IPresence::Offline);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Offline));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Offline));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatusAction(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);
}

void StatusChanger::updateMenu(IPresence *APresence)
{
  if (!APresence)
  {
    mnuBase->setIcon(STATUS_ICONSETFILE,getStatusIconName(FBaseShow));
    mnuBase->setTitle(getStatusName(FBaseShow));

    if (FTrayManager)
      FTrayManager->setBaseIcon(mnuBase->icon());
  }
  else if (FStreamMenus.contains(APresence))
  {
    Menu *menu = FStreamMenus.value(APresence);
    menu->setIcon(STATUS_ICONSETFILE,getStatusIconName(APresence->show()));
  }
}

void StatusChanger::addStreamMenu(IPresence *APresence)
{
  if (APresence && !FStreamMenus.contains(APresence))
  {
    Menu *menu = new Menu(mnuBase);
    menu->setTitle(APresence->streamJid().full());
    FStreamMenus.insert(APresence,menu);
    mnuBase->addAction(menu->menuAction(),STATUSMENU_ACTION_GROUP_STREAM,true);
    createStatusActions(APresence);
    updateMenu(APresence);
  }
}

void StatusChanger::removeStreamMenu(IPresence *APresence)
{
  if (APresence && FStreamMenus.contains(APresence))
  {
    Menu *menu = FStreamMenus.take(APresence);
    menu->clear();
    delete menu;
  }
}

void StatusChanger::updateAccount(IPresence *APresence)
{
  IAccount *account = FAccounts.value(APresence,NULL);
  if (account && APresence->show() != IPresence::Offline && APresence->show() != IPresence::Error)
  {
    account->setValue("presence::last::show",APresence->show());
    account->setValue("presence::last::status",APresence->status());
    account->setValue("presence::last::priority",APresence->priority());
  }
}

void StatusChanger::autoReconnect(IPresence *APresence)
{
  IAccount *account = FAccounts.value(APresence,NULL);
  if (account && APresence->show() == IPresence::Error)
  {
    if (account->autoReconnect())
    {
      IPresence::Show show = (IPresence::Show)account->value("presence::last::show",IPresence::Online).toInt();
      QString status = account->value("presence::last::status",getStatusText(IPresence::Online)).toString();
      int priority = account->value("presence::last::priority",getStatusPriority(IPresence::Online)).toInt();
      int reconnectTime = account->value("autoReconnectTime",30000).toInt();
      QPair<QDateTime,PresenceItem> pair;
      pair.first = QDateTime::currentDateTime().addMSecs(reconnectTime);
      pair.second.show = show;
      pair.second.status = status;
      pair.second.priority = priority;
      FWaitReconnect.insert(APresence,pair);
      QTimer::singleShot(reconnectTime+100,this,SLOT(onReconnectTimer()));
    }
  }
  else
    FWaitReconnect.remove(APresence);
}

QString StatusChanger::getStatusIconName(IPresence::Show AShow) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return "offline";
  case IPresence::Online: 
    return "online";
  case IPresence::Chat: 
    return "chat";
  case IPresence::Away: 
    return "away";
  case IPresence::ExtendedAway: 
    return "xa";
  case IPresence::DoNotDistrib: 
    return "dnd";
  case IPresence::Invisible: 
    return "invisible";
  case IPresence::Error: 
    return "error";
  }
  return QString();
}

QString StatusChanger::getStatusName(IPresence::Show AShow) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return tr("Offline");
  case IPresence::Online: 
    return tr("Online");
  case IPresence::Chat: 
    return tr("Free for Chat");
  case IPresence::Away: 
    return tr("Away");
  case IPresence::ExtendedAway: 
    return tr("Not Available");
  case IPresence::DoNotDistrib: 
    return tr("Do Not Distrib");
  case IPresence::Invisible: 
    return tr("Invisible");
  case IPresence::Error: 
    return tr("Error");
  }
  return tr("Unknown");
}

QString StatusChanger::getStatusText(IPresence::Show AShow) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return tr("Offline");
  case IPresence::Online: 
    return tr("Online");
  case IPresence::Chat: 
    return tr("I`am free for chat");
  case IPresence::Away: 
    return tr("I`am away from my desk");
  case IPresence::ExtendedAway: 
    return tr("Not available");
  case IPresence::DoNotDistrib: 
    return tr("Do Not Distrib");
  case IPresence::Invisible: 
    return tr("Invisible");
  case IPresence::Error: 
    return tr("Error");
  }
  return tr("Unknown");
}

int StatusChanger::getStatusPriority(IPresence::Show AShow) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return 0;
  case IPresence::Online: 
    return 4;
  case IPresence::Chat: 
    return 8;
  case IPresence::Away: 
    return 16;
  case IPresence::ExtendedAway: 
    return 32;
  case IPresence::DoNotDistrib: 
    return 64;
  case IPresence::Invisible: 
    return 128;
  case IPresence::Error: 
    return 0;
  }
  return -1;
}

void StatusChanger::insertWaitOnline(IPresence *APresence, PresenceItem &AItem)
{
  FWaitOnline.insert(APresence,AItem);
  if (FRostersModel && FRostersView)
  {
    IRosterIndex *index = FRostersModel->getStreamRoot(APresence->xmppStream()->jid());
    if (index)
      FRostersView->insertIndexLabel(FConnectingLabel,index);
  }
}

void StatusChanger::removeWaitOnline(IPresence *APresence)
{
  FWaitOnline.remove(APresence);
  if (FRostersModel && FRostersView)
  {
    IRosterIndex *index = FRostersModel->getStreamRoot(APresence->xmppStream()->jid());
    if (index)
      FRostersView->removeIndexLabel(FConnectingLabel,index);
  }
}

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
  FPresences.append(APresence);
  addStreamMenu(APresence);
  if (FAccountManager)
    FAccounts.insert(APresence,FAccountManager->accountByStream(APresence->xmppStream()->jid()));
}

void StatusChanger::onSelfPresence(IPresence *APresence, IPresence::Show AShow,
                                   const QString &, qint8 , const Jid &AJid)
{
  if (AJid.isValid())
    return;

  removeWaitOnline(APresence);
  if (AShow != FBaseShow)
  {
    if (AShow == IPresence::Offline || AShow == IPresence::Error)
    {
      IPresence::Show baseShow = AShow;
      QList<IPresence *>::const_iterator i = FPresences.constBegin();
      while (baseShow != FBaseShow && i != FPresences.constEnd())
      {
        if ((*i)->show() == FBaseShow)
          baseShow = FBaseShow;
        else if (baseShow == AShow && (*i)->show() != IPresence::Offline && (*i)->show() != IPresence::Error)
          baseShow = (*i)->show();
        i++;
      }
      setBaseShow(baseShow);
    }
    else
      setBaseShow(AShow);
  }
  updateMenu(APresence);
  updateAccount(APresence);
  autoReconnect(APresence);
}

void StatusChanger::onPresenceRemoved(IPresence *APresence)
{
  removeWaitOnline(APresence);
  removeStreamMenu(APresence);
  FPresences.removeAt(FPresences.indexOf(APresence));
  FAccounts.remove(APresence);
  FWaitReconnect.remove(APresence);
}

void StatusChanger::onRosterOpened(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (FWaitOnline.contains(presence))
  {
    PresenceItem item = FWaitOnline.value(presence);
    presence->setPresence(item.show,item.status,item.priority,Jid());
    removeWaitOnline(presence);
  }
}

void StatusChanger::onRosterClosed(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  removeWaitOnline(presence);
}

void StatusChanger::onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
  if (AIndex.isValid() && AIndex.data(IRosterIndex::DR_Type).toInt() == IRosterIndex::IT_StreamRoot)
  {
    QString streamJid = AIndex.data(IRosterIndex::DR_StreamJid).toString();
    Menu *menu = streamMenu(streamJid);
    if (menu)
    {
      Action *action = new Action(AMenu);
      action->setMenu(menu);
      action->setIcon(menu->icon());
      action->setText(tr("Status"));
      AMenu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);
    }
  }
}

void StatusChanger::onTrayContextMenu(int /*ANotifyId*/, Menu *AMenu)
{
  Action *action = new Action(AMenu);
  action->setIcon(mnuBase->icon());
  action->setText(tr("Status"));
  action->setMenu(mnuBase);
  AMenu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN,true);
}

void StatusChanger::onReconnectTimer()
{
  QHash<IPresence *,QPair<QDateTime,PresenceItem> >::iterator it = FWaitReconnect.begin();
  while (it != FWaitReconnect.end())
  {
    if (it.value().first <= QDateTime::currentDateTime()) 
    {
      IPresence *presence = it.key();
      PresenceItem item = it.value().second;
      it = FWaitReconnect.erase(it);
      
      IAccount *account = FAccounts.value(presence);
      if (account)
        if (presence->show() == IPresence::Error && account->autoReconnect())
          setPresence(item.show,item.status,item.priority,presence->streamJid());
    }
    else
      it++;
  }
}

void StatusChanger::onAccountShown(IAccount *AAccount)
{
  if (AAccount->autoConnect())
  {
    IPresence *presence = FPresencePlugin->getPresence(AAccount->streamJid());
    if (presence && presence->show() == IPresence::Offline)
    {
      IPresence::Show show = (IPresence::Show)AAccount->value("presence::last::show",IPresence::Online).toInt();
      QString status = AAccount->value("presence::last::status",getStatusText(IPresence::Online)).toString();
      int priority = AAccount->value("presence::last::priority",getStatusPriority(IPresence::Online)).toInt();
      setPresence(show,status,priority,presence->xmppStream()->jid());
    }
  }
}

void StatusChanger::onSkinChanged()
{
  if (FRostersView)
    FRostersView->updateIndexLabel(FConnectingLabel,FRosterIconset.iconByName("connecting"));
}

Q_EXPORT_PLUGIN2(StatusChangerPlugin, StatusChanger)
