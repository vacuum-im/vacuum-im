#include "statuschanger.h"

StatusChanger::StatusChanger()
{
  FBaseShow = IPresence::Error;
  FStatusIconset.openFile("status/default.jisp");
  connect(&FStatusIconset,SIGNAL(reseted(const QString &)),SLOT(onSkinChanged(const QString &)));
  FMenu = new Menu(STATUSMENU_MENU_MAIN_ORDER,NULL);
  createStatusActions();
  setBaseShow(IPresence::Offline);
}

StatusChanger::~StatusChanger()
{
  delete FMenu;
}

//IPlugin
void StatusChanger::getPluginInfo(PluginInfo *APluginInfo)
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
      connect(plugin->instance(),SIGNAL(presenceOpened(IPresence *)),
        SLOT(onPresenceOpened(IPresence *)));
      connect(plugin->instance(),SIGNAL(selfPresence(IPresence *, IPresence::Show, const QString &, qint8, const Jid &)),
        SLOT(onSelfPresence(IPresence *, IPresence::Show, const QString &, qint8, const Jid &)));
      connect(plugin->instance(),SIGNAL(presenceClosed(IPresence *)),
        SLOT(onPresenceClosed(IPresence *)));
      connect(plugin->instance(),SIGNAL(presenceRemoved(IPresence *)),
        SLOT(onPresenceRemoved(IPresence *)));
    }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  
  return FPresencePlugin!=NULL;
}

bool StatusChanger::startPlugin()
{
  if (FMainWindowPlugin)
    FMainWindowPlugin->mainWindow()->bottomToolBar()->addAction(FMenu->menuAction());
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
    if (!presence->setPresence(AShow,AStatus,APriority,Jid()))
    {
      if (!presence->xmppStream()->isOpen() && AShow != IPresence::Offline)
      {
        PresenceItem item;
        item.show = AShow;
        item.status = AStatus;
        item.priority = APriority;
        FWaitOnline.insert(presence,item);
        presence->xmppStream()->open();
      }
    };

    if (AShow == IPresence::Offline && presence->xmppStream()->isOpen())
      presence->xmppStream()->close();
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

  Action *action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::Offline));
  action->setText(getStatusText(IPresence::Offline));
  action->setData(Action::DR_Parametr1,IPresence::Offline);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::Invisible));
  action->setText(getStatusText(IPresence::Invisible));
  action->setData(Action::DR_Parametr1,IPresence::Invisible);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::DoNotDistrib));
  action->setText(getStatusText(IPresence::DoNotDistrib));
  action->setData(Action::DR_Parametr1,IPresence::DoNotDistrib);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::ExtendedAway));
  action->setText(getStatusText(IPresence::ExtendedAway));
  action->setData(Action::DR_Parametr1,IPresence::ExtendedAway);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::Away));
  action->setText(getStatusText(IPresence::Away));
  action->setData(Action::DR_Parametr1,IPresence::Away);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::Chat));
  action->setText(getStatusText(IPresence::Chat));
  action->setData(Action::DR_Parametr1,IPresence::Chat);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);

  action = new Action(STATUSMENU_MENU_MAIN_ORDER,menu);
  action->setIcon(getStatusIcon(IPresence::Online));
  action->setText(getStatusText(IPresence::Online));
  action->setData(Action::DR_Parametr1,IPresence::Online);
  action->setData(Action::DR_StreamJid,streamJid);
  connect(action,SIGNAL(triggered(bool)),SLOT(onStatusTriggered(bool)));
  menu->addAction(action);
}

void StatusChanger::updateMenu(IPresence *APresence)
{
  if (!APresence)
  {
    QIcon icon = getStatusIcon(FBaseShow);
    QString text = getStatusText(FBaseShow);
    FMenu->setIcon(icon);
    FMenu->setTitle(text);
    FMenu->menuAction()->setIcon(icon);
  }
  else if (FStreamMenus.contains(APresence))
  {
    Menu *menu = FStreamMenus.value(APresence);
    menu->menuAction()->setIcon(getStatusIcon(APresence->show()));
  }
}

void StatusChanger::addStreamMenu(IPresence *APresence)
{
  if (APresence && !FStreamMenus.contains(APresence))
  {
    Menu *menu = new Menu(STATUSMENU_MENU_STREAM_ORDER,FMenu);
    menu->setTitle(APresence->streamJid().full());
    FStreamMenus.insert(APresence,menu);
    FMenu->addAction(menu->menuAction());
    updateMenu(APresence);
    createStatusActions(APresence);
  }
}

void StatusChanger::removeStreamMenu(IPresence *APresence)
{
  if (APresence && FStreamMenus.contains(APresence))
  {
    Menu *menu = FStreamMenus.value(APresence,NULL);
    FStreamMenus.remove(APresence);
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

QString StatusChanger::getStatusText(IPresence::Show AShow) const
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

void StatusChanger::onStatusTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IPresence::Show show = (IPresence::Show)action->data(Action::DR_Parametr1).toInt();
    setPresence(show,getStatusText(show),getStatusPriority(show),streamJid);
  }
}

void StatusChanger::onPresenceAdded(IPresence *APresence)
{
  FPresences.append(APresence);
  addStreamMenu(APresence);
}

void StatusChanger::onPresenceOpened(IPresence *APresence)
{
  if (FWaitOnline.contains(APresence))
  {
    PresenceItem item = FWaitOnline.value(APresence);
    APresence->setPresence(item.show,item.status,item.priority,Jid());
    FWaitOnline.remove(APresence);
  }
}

void StatusChanger::onSelfPresence(IPresence *APresence, IPresence::Show AShow,
                                   const QString &, qint8 , const Jid &)
{
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

void StatusChanger::onPresenceClosed(IPresence *APresence)
{
  FWaitOnline.remove(APresence);
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

Q_EXPORT_PLUGIN2(StatusChangerPlugin, StatusChanger)
