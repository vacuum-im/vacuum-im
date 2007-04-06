#include "statuschanger.h"
#include <QToolButton>

StatusChanger::StatusChanger()
{
  FBaseShow = IPresence::Error;
  FStatusIconset.openFile("status/common.jisp");
  FPresencePlugin = NULL;
  FRosterPlugin = NULL;
  FMainWindowPlugin = NULL;
  FRostersViewPlugin = NULL;
  connect(&FStatusIconset,SIGNAL(reseted(const QString &)),SLOT(onSkinChanged(const QString &)));
  FMenu = new Menu(NULL);
  createStatusActions();
  setBaseShow(IPresence::Offline);
}

StatusChanger::~StatusChanger()
{
  delete FMenu;
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

bool StatusChanger::initPlugin(IPluginManager *APluginManager)
{
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
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  
  return FPresencePlugin!=NULL;
}

bool StatusChanger::startPlugin()
{
  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(const QModelIndex &, Menu *)),
      SLOT(onRostersViewContextMenu(const QModelIndex &, Menu *)));
  }

  if (FMainWindowPlugin)
  {
    QToolButton *tbutton = new QToolButton;
    tbutton->setDefaultAction(FMenu->menuAction());
    tbutton->setPopupMode(QToolButton::InstantPopup);
    tbutton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    FMainWindowPlugin->mainWindow()->bottomToolBar()->addWidget(tbutton);
  }

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
    if (AShow == IPresence::Offline)
    {
      presence->setPresence(AShow,AStatus,APriority,Jid());
      presence->xmppStream()->close();
      FWaitOnline.remove(presence);
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
          FWaitOnline.insert(presence,item);
          presence->xmppStream()->open();
        }
      }
    }
  }
}

void StatusChanger::onChangeStatus(bool)
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
  Menu *menu = FMenu;
  QString streamJid;
  if (APresence)
  {
    streamJid = APresence->streamJid().pFull();
    menu = FStreamMenus.value(APresence);
  }

  Action *action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::Online));
  action->setText(getStatusName(IPresence::Online));
  action->setData(Action::DR_Parametr1,IPresence::Online);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Online));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Online));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::Chat));
  action->setText(getStatusName(IPresence::Chat));
  action->setData(Action::DR_Parametr1,IPresence::Chat);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Chat));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Chat));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::Away));
  action->setText(getStatusName(IPresence::Away));
  action->setData(Action::DR_Parametr1,IPresence::Away);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Away));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Away));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::ExtendedAway));
  action->setText(getStatusName(IPresence::ExtendedAway));
  action->setData(Action::DR_Parametr1,IPresence::ExtendedAway);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::ExtendedAway));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::ExtendedAway));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::DoNotDistrib));
  action->setText(getStatusName(IPresence::DoNotDistrib));
  action->setData(Action::DR_Parametr1,IPresence::DoNotDistrib);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::DoNotDistrib));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::DoNotDistrib));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::Invisible));
  action->setText(getStatusName(IPresence::Invisible));
  action->setData(Action::DR_Parametr1,IPresence::Invisible);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Invisible));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Invisible));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);

  action = new Action(menu);
  action->setIcon(getStatusIcon(IPresence::Offline));
  action->setText(getStatusName(IPresence::Offline));
  action->setData(Action::DR_Parametr1,IPresence::Offline);
  action->setData(Action::DR_Parametr2,getStatusText(IPresence::Offline));
  action->setData(Action::DR_Parametr3,getStatusPriority(IPresence::Offline));
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onChangeStatus(bool)));
  menu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);
}

void StatusChanger::updateMenu(IPresence *APresence)
{
  if (!APresence)
  {
    FMenu->setIcon(getStatusIcon(FBaseShow));
    FMenu->setTitle(getStatusName(FBaseShow));
  }
  else if (FStreamMenus.contains(APresence))
  {
    Menu *menu = FStreamMenus.value(APresence);
    menu->setIcon(getStatusIcon(APresence->show()));
  }
}

void StatusChanger::addStreamMenu(IPresence *APresence)
{
  if (APresence && !FStreamMenus.contains(APresence))
  {
    Menu *menu = new Menu(FMenu);
    menu->setTitle(APresence->streamJid().full());
    FStreamMenus.insert(APresence,menu);
    FMenu->addAction(menu->menuAction(),STATUSMENU_ACTION_GROUP_STREAM,true);
    updateMenu(APresence);
    createStatusActions(APresence);
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

QIcon StatusChanger::getStatusIcon(IPresence::Show AShow) const
{
  switch (AShow)
  {
  case IPresence::Offline: 
    return FStatusIconset.iconByName("offline");
  case IPresence::Online: 
    return FStatusIconset.iconByName("online");
  case IPresence::Chat: 
    return FStatusIconset.iconByName("chat");
  case IPresence::Away: 
    return FStatusIconset.iconByName("away");
  case IPresence::ExtendedAway: 
    return FStatusIconset.iconByName("xa");
  case IPresence::DoNotDistrib: 
    return FStatusIconset.iconByName("dnd");
  case IPresence::Invisible: 
    return FStatusIconset.iconByName("invisible");
  case IPresence::Error: 
    return FStatusIconset.iconByName("error");
  }
  return QIcon();
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

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
  FPresences.append(APresence);
  addStreamMenu(APresence);
}

void StatusChanger::onSelfPresence(IPresence *APresence, IPresence::Show AShow,
                                   const QString &, qint8 , const Jid &)
{
  FWaitOnline.remove(APresence);
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
}

void StatusChanger::onPresenceRemoved(IPresence *APresence)
{
  FWaitOnline.remove(APresence);
  removeStreamMenu(APresence);
  FPresences.removeAt(FPresences.indexOf(APresence));
}

void StatusChanger::onSkinChanged(const QString &)
{
  updateMenu();
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
      action->setText(tr("Status"));
      AMenu->addAction(action,STATUSMENU_ACTION_GROUP_MAIN);
    }
  }
}

void StatusChanger::onRosterOpened(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  if (FWaitOnline.contains(presence))
  {
    PresenceItem item = FWaitOnline.value(presence);
    presence->setPresence(item.show,item.status,item.priority,Jid());
    FWaitOnline.remove(presence);
  }
}

void StatusChanger::onRosterClosed(IRoster *ARoster)
{
  IPresence *presence = FPresencePlugin->getPresence(ARoster->streamJid());
  FWaitOnline.remove(presence);
}

Q_EXPORT_PLUGIN2(StatusChangerPlugin, StatusChanger)
