#include "rosterchanger.h"

#include <QInputDialog>
#include <QMessageBox>

#define IN_EVENTS           "psi/events"
#define IN_ADDCONTACT       "psi/addContact"

#define NOTIFICATOR_ID      "RosterChanger"

RosterChanger::RosterChanger()
{
  FRosterPlugin = NULL;
  FRostersModel = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FRostersView = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL; 
  FNotifications = NULL;
  FAccountManager = NULL;
  FMultiUserChatPlugin = NULL;

  FSubsId = 1;
  FSubscriptionDialog = NULL;
  FAddContactMenu = NULL;
}

RosterChanger::~RosterChanger()
{

}

//IPlugin
void RosterChanger::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Manipulating roster items and groups");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster Changer"); 
  APluginInfo->uid = ROSTERCHANGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(ROSTER_UUID); 
}

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterSubscription(IRoster *, const Jid &, int, const QString &)),
        SLOT(onReceiveSubscription(IRoster *, const Jid &, int, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin)
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (FRostersViewPlugin)
    {
      FRostersView = FRostersViewPlugin->rostersView();
      connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)), SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    }
  }


  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin)
  {
    FNotifications = qobject_cast<INotifications *>(plugin->instance());
    if (FNotifications)
    {
      connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMultiUserChatPlugin").value(0,NULL);
  if (plugin)
  {
    FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
    if (FMultiUserChatPlugin)
    {
      connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
        SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
    }
  }

  return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
  if (FMainWindowPlugin)
  {
    FAddContactMenu = new Menu(FMainWindowPlugin->mainWindow()->mainMenu());
    FAddContactMenu->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
    FAddContactMenu->setTitle(tr("Add contact to"));
    FAddContactMenu->menuAction()->setEnabled(false);
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FAddContactMenu->menuAction(),AG_ROSTERCHANGER_MMENU,true);
  }

  if (FTrayManager && FAddContactMenu)
  {
    FTrayManager->addAction(FAddContactMenu->menuAction(),AG_ROSTERCHANGER_TRAY,true);
  }

  if (FNotifications)
  {
    uchar kindMask = INotification::RosterIcon|INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound;;
    FNotifications->insertNotificator(NOTIFICATOR_ID,tr("Subscription requests"),kindMask,kindMask);
  }

  return true;
}

//IRosterChanger
void RosterChanger::showAddContactDialog(const Jid &AStreamJid, const Jid &AJid, const QString &ANick, 
                                         const QString &AGroup, const QString &ARequest)
{
  IRoster *roster = FRosterPlugin->getRoster(AStreamJid);
  if (roster && roster->isOpen())
  {
    AddContactDialog *dialog = new AddContactDialog(roster->streamJid(),roster->groups());
    dialog->setContactJid(AJid);
    dialog->setNick(ANick);
    dialog->setGroup(AGroup);
    if (!ARequest.isEmpty())
      dialog->setRequestText(ARequest);
    connect(dialog,SIGNAL(addContact(AddContactDialog *)),SLOT(onAddContact(AddContactDialog *)));
    connect(dialog,SIGNAL(destroyed(QObject *)),SLOT(onAddContactDialogDestroyed(QObject *)));
    FAddContactDialogs[roster].append(dialog);
    dialog->show();
  }
}

QString RosterChanger::subscriptionNotify(int ASubsType, const Jid &AContactJid) const
{
  switch(ASubsType) 
  {
  case IRoster::Subscribe:
    return tr("%1 wants to subscribe to your presence.").arg(AContactJid.hBare());
  case IRoster::Subscribed:
    return tr("You are now authorized for %1 presence.").arg(AContactJid.hBare());
  case IRoster::Unsubscribe:
    return tr("%1 unsubscribed from your presence.").arg(AContactJid.hBare());
  case IRoster::Unsubscribed:
    return tr("You are now unsubscribed from %1 presence.").arg(AContactJid.hBare());
  }
  return QString();
}

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
                                     bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent)
{
  Menu *menu = new Menu(AParent);
  IRoster *roster = FRosterPlugin->getRoster(AData.value(Action::DR_StreamJid).toString());
  if (roster)
  {
    QString groupDelim = roster->groupDelimiter();
    QString group;
    QHash<QString,Menu *> menus;
    QSet<QString> allGroups = roster->groups();
    foreach(group,allGroups)
    {
      Menu *parentMenu = menu;
      QList<QString> groupTree = group.split(groupDelim,QString::SkipEmptyParts);
      QString groupName;
      int index = 0;
      while (index < groupTree.count())
      {
        if (groupName.isEmpty())
          groupName = groupTree.at(index);
        else
          groupName += groupDelim + groupTree.at(index);

        if (!menus.contains(groupName))
        {
          Menu *groupMenu = new Menu(parentMenu);
          groupMenu->setTitle(groupTree.at(index));

          if (!AExceptGroups.contains(groupName))
          {
            Action *curGroupAction = new Action(groupMenu);
            curGroupAction->setText(tr("This group"));
            curGroupAction->setData(AData);
            curGroupAction->setData(Action::DR_Parametr4,groupName);
            connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(curGroupAction,AG_ROSTERCHANGER_ROSTER+1);
          }

          if (ANewGroup)
          {
            Action *newGroupAction = new Action(groupMenu);
            newGroupAction->setText(tr("Create new..."));
            newGroupAction->setData(AData);
            newGroupAction->setData(Action::DR_Parametr4,groupName+groupDelim);
            connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(newGroupAction,AG_ROSTERCHANGER_ROSTER+1);
          }

          menus.insert(groupName,groupMenu);
          parentMenu->addAction(groupMenu->menuAction(),AG_ROSTERCHANGER_ROSTER,true);
          parentMenu = groupMenu;
        }
        else
          parentMenu = menus.value(groupName); 

        index++;
      }
    }

    if (ARootGroup)
    {
      Action *curGroupAction = new Action(menu);
      curGroupAction->setText(tr("Root"));
      curGroupAction->setData(AData);
      curGroupAction->setData(Action::DR_Parametr4,"");
      connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(curGroupAction,AG_ROSTERCHANGER_ROSTER+1);
    }

    if (ANewGroup)
    {
      Action *newGroupAction = new Action(menu);
      newGroupAction->setText(tr("Create new..."));
      newGroupAction->setData(AData);
      newGroupAction->setData(Action::DR_Parametr4,groupDelim);
      connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(newGroupAction,AG_ROSTERCHANGER_ROSTER+1);
    }
  }
  return menu;
}

void RosterChanger::openSubscriptionDialog(int ASubsId)
{
  if (FSubsItems.contains(ASubsId))
  {
    SubscriptionItem &subsItem = FSubsItems[ASubsId];

    if (!FSubscriptionDialog)
    {
      FSubscriptionDialog = new SubscriptionDialog;
      connect(FSubscriptionDialog,SIGNAL(showNext()),SLOT(onSubscriptionDialogShowNext()));
      connect(FSubscriptionDialog->dialogAction(),SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      connect(FSubscriptionDialog,SIGNAL(destroyed(QObject *)),SLOT(onSubscriptionDialogDestroyed(QObject *)));
      emit subscriptionDialogCreated(FSubscriptionDialog);
    }
    
    QString subs = SUBSCRIPTION_NONE;
    IRoster *roster = FRosterPlugin->getRoster(subsItem.streamJid);
    if (roster)
      subs = roster->rosterItem(subsItem.contactJid).subscription;
    FSubscriptionDialog->setupDialog(subsItem.streamJid,subsItem.contactJid,subsItem.time,subsItem.type,subsItem.status,subs);
    FSubscriptionDialog->show();
    FSubscriptionDialog->activateWindow();

    removeSubscriptionMessage(ASubsId);
  }
}

void RosterChanger::removeSubscriptionMessage(int ASubsId)
{
  if (FSubsItems.contains(ASubsId))
  {
    SubscriptionItem subsItem = FSubsItems.take(ASubsId);
    if (FSubscriptionDialog)
      FSubscriptionDialog->setNextCount(FSubsItems.count());
    if (FNotifications)
      FNotifications->removeNotification(subsItem.notifyId);
  }
}

void RosterChanger::onShowAddContactDialog(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    QString contactJid = action->data(Action::DR_Parametr1).toString();
    QString nick = action->data(Action::DR_Parametr2).toString();
    QString group = action->data(Action::DR_Parametr3).toString();
    QString request = action->data(Action::DR_Parametr4).toString();
    showAddContactDialog(streamJid,contactJid,nick,group,request);
  }
}

void RosterChanger::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  QString streamJid = AIndex->data(RDR_StreamJid).toString();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    int itemType = AIndex->data(RDR_Type).toInt();
    IRosterItem ritem = roster->rosterItem(AIndex->data(RDR_BareJid).toString());
    if (itemType == RIT_StreamRoot)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,AIndex->data(RDR_Jid));

      Action *action = new Action(AMenu);
      action->setText(tr("Add contact..."));
      action->setData(data);
      action->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
    }
    else if (itemType == RIT_Contact || itemType == RIT_Agent)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,streamJid);
      data.insert(Action::DR_Parametr1,AIndex->data(RDR_BareJid).toString());
      
      Menu *subsMenu = new Menu(AMenu);
      subsMenu->setTitle(tr("Subscription"));

      Action *action = new Action(subsMenu);
      action->setText(tr("Send"));
      action->setData(data);
      action->setData(Action::DR_Parametr2,IRoster::Subscribed);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Remove"));
      action->setData(data);
      action->setData(Action::DR_Parametr2,IRoster::Unsubscribed);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Request"));
      action->setData(data);
      action->setData(Action::DR_Parametr2,IRoster::Subscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Refuse"));
      action->setData(data);
      action->setData(Action::DR_Parametr2,IRoster::Unsubscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      AMenu->addAction(subsMenu->menuAction(),AG_ROSTERCHANGER_ROSTER_SUBSCRIPTION);

      if (ritem.isValid)
      {
        QSet<QString> exceptGroups = ritem.groups;

        data.insert(Action::DR_Parametr2,AIndex->data(RDR_Name));
        data.insert(Action::DR_Parametr3,AIndex->data(RDR_Group));

        action = new Action(AMenu);
        action->setText(tr("Rename..."));
        action->setData(data);
        connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
        AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

        if (itemType == RIT_Contact)
        {
          Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onCopyItemToGroup(bool)),AMenu);
          copyItem->setTitle(tr("Copy to group"));
          AMenu->addAction(copyItem->menuAction(),AG_ROSTERCHANGER_ROSTER);

          Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onMoveItemToGroup(bool)),AMenu);
          moveItem->setTitle(tr("Move to group"));
          AMenu->addAction(moveItem->menuAction(),AG_ROSTERCHANGER_ROSTER);
        }

        if (!AIndex->data(RDR_Group).toString().isEmpty())
        {
          action = new Action(AMenu);
          action->setText(tr("Remove from group"));
          action->setData(data);
          connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromGroup(bool)));
          AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
        }
      }
      else
      {
        action = new Action(AMenu);
        action->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
        action->setText(tr("Add contact..."));
        action->setData(Action::DR_StreamJid,streamJid);
        action->setData(Action::DR_Parametr1,AIndex->data(RDR_Jid));
        connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
        AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
      }

      action = new Action(AMenu);
      action->setText(tr("Remove from roster"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
    }
    else if (itemType == RIT_Group)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_Parametr1,AIndex->data(RDR_Group));
      data.insert(Action::DR_StreamJid,streamJid);
      
      Action *action = new Action(AMenu);
      action->setText(tr("Add contact..."));
      action->setData(Action::DR_StreamJid,AIndex->data(RDR_StreamJid));
      action->setData(Action::DR_Parametr3,AIndex->data(RDR_Group));
      action->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);

      action = new Action(AMenu);
      action->setText(tr("Rename..."));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

      QSet<QString> exceptGroups;
      exceptGroups << AIndex->data(RDR_Group).toString();

      Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onCopyGroupToGroup(bool)),AMenu);
      copyGroup->setTitle(tr("Copy to group"));
      AMenu->addAction(copyGroup->menuAction(),AG_ROSTERCHANGER_ROSTER);

      Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onMoveGroupToGroup(bool)),AMenu);
      moveGroup->setTitle(tr("Move to group"));
      AMenu->addAction(moveGroup->menuAction(),AG_ROSTERCHANGER_ROSTER);

      action = new Action(AMenu);
      action->setText(tr("Remove group"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

      action = new Action(AMenu);
      action->setText(tr("Remove contacts"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupItems(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
    }
  }
}

void RosterChanger::onSendSubscription(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      int subsType = action->data(Action::DR_Parametr2).toInt();
      roster->sendSubscription(rosterJid,subsType);
    }
  }
}

void RosterChanger::onReceiveSubscription(IRoster *ARoster, const Jid &AFromJid, int ASubsType, const QString &AText)
{
  SubscriptionItem sitem;
  sitem.subsId = FSubsId++;
  sitem.streamJid = ARoster->streamJid();
  sitem.contactJid = AFromJid;
  sitem.type = ASubsType;
  sitem.status = AText;
  sitem.time = QDateTime::currentDateTime();

  if (FNotifications)
  {
    INotification notify;
    notify.kinds = FNotifications->notificatorKinds(NOTIFICATOR_ID);
    notify.data.insert(NDR_ICON,Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_EVENTS));
    notify.data.insert(NDR_TOOLTIP,tr("Subscription message from %1").arg(FNotifications->contactName(ARoster->streamJid(),AFromJid)));
    notify.data.insert(NDR_ROSTER_STREAM_JID,ARoster->streamJid().full());
    notify.data.insert(NDR_ROSTER_CONTACT_JID,AFromJid.full());
    notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_SUBSCRIBTION);
    notify.data.insert(NDR_WINDOW_CAPTION, tr("Subscription message"));
    notify.data.insert(NDR_WINDOW_TITLE,FNotifications->contactName(ARoster->streamJid(),AFromJid));
    notify.data.insert(NDR_WINDOW_IMAGE, FNotifications->contactAvatar(AFromJid));
    notify.data.insert(NDR_WINDOW_TEXT,subscriptionNotify(ASubsType,AFromJid));
    sitem.notifyId = FNotifications->appendNotification(notify);
  }

  FSubsItems.insert(sitem.subsId,sitem);

  if (FSubscriptionDialog)
    FSubscriptionDialog->setNextCount(FSubsItems.count());
  else if (!FNotifications)
    openSubscriptionDialog(sitem.subsId);
}

void RosterChanger::onRenameItem(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      Jid rosterJid = action->data(Action::DR_Parametr1).toString();
      QString oldName = action->data(Action::DR_Parametr2).toString();
      bool ok = false;
      QString newName = QInputDialog::getText(NULL,tr("Rename contact"),tr("Enter name for: <b>%1</b>").arg(rosterJid.hBare()),
        QLineEdit::Normal,oldName,&ok);
      if (ok && !newName.isEmpty() && newName != oldName)
        roster->renameItem(rosterJid,newName);
    }
  }
}

void RosterChanger::onCopyItemToGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      QString groupName = action->data(Action::DR_Parametr4).toString();
      if (groupName.endsWith(groupDelim))
      {
        bool ok = false;
        QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
          QLineEdit::Normal,QString(),&ok);
        if (ok && !newGroupName.isEmpty())
        {
          if (groupName == groupDelim)
            groupName = newGroupName;
          else
            groupName+=newGroupName;
          roster->copyItemToGroup(rosterJid,groupName);
        }
      }
      else
        roster->copyItemToGroup(rosterJid,groupName);
    }
  }
}

void RosterChanger::onMoveItemToGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      QString groupName = action->data(Action::DR_Parametr3).toString();
      QString moveToGroup = action->data(Action::DR_Parametr4).toString();
      if (moveToGroup.endsWith(groupDelim))
      {
        bool ok = false;
        QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
          QLineEdit::Normal,QString(),&ok);
        if (ok && !newGroupName.isEmpty())
        {
          if (moveToGroup == groupDelim)
            moveToGroup = newGroupName;
          else
            moveToGroup+=newGroupName;
          roster->moveItemToGroup(rosterJid,groupName,moveToGroup);
        }
      }
      else
        roster->moveItemToGroup(rosterJid,groupName,moveToGroup);
    }
  }
}

void RosterChanger::onRemoveItemFromGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      QString groupName = action->data(Action::DR_Parametr3).toString();
      roster->removeItemFromGroup(rosterJid,groupName);
    }
  }
}

void RosterChanger::onRemoveItemFromRoster(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      Jid rosterJid = action->data(Action::DR_Parametr1).toString();
      if (roster->rosterItem(rosterJid).isValid)
      {
        if (QMessageBox::question(NULL,tr("Remove contact"),
          tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(rosterJid.hBare()),
          QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        {
          roster->removeItem(rosterJid);
        }
      }
      else if (FRostersModel)
      {
        QMultiHash<int, QVariant> data;
        data.insert(RDR_Type,RIT_Contact);
        data.insert(RDR_Type,RIT_Agent);
        data.insert(RDR_BareJid,rosterJid.pBare());
        IRosterIndex *streamIndex = FRostersModel->streamRoot(streamJid);
        IRosterIndexList indexList = streamIndex->findChild(data,true);
        foreach(IRosterIndex *index, indexList)
          FRostersModel->removeRosterIndex(index);
      }
    }
  }
}

void RosterChanger::onRenameGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      bool ok = false;
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(Action::DR_Parametr1).toString();
      QList<QString> groupTree = groupName.split(groupDelim,QString::SkipEmptyParts);
      
      QString newGroupPart = QInputDialog::getText(NULL,tr("Rename group"),tr("Enter new group name:"),
        QLineEdit::Normal,groupTree.last(),&ok);
      
      if (ok && !newGroupPart.isEmpty())
      {
        QString newGroupName = groupName;
        newGroupName.chop(groupTree.last().size());
        newGroupName += newGroupPart;
        roster->renameGroup(groupName,newGroupName);
      }
    }
  }
}

void RosterChanger::onCopyGroupToGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(Action::DR_Parametr1).toString();
      QString copyToGroup = action->data(Action::DR_Parametr4).toString();
      if (copyToGroup.endsWith(groupDelim))
      {
        bool ok = false;
        QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
          QLineEdit::Normal,QString(),&ok);
        if (ok && !newGroupName.isEmpty())
        {
          if (copyToGroup == groupDelim)
            copyToGroup = newGroupName;
          else
            copyToGroup+=newGroupName;
          roster->copyGroupToGroup(groupName,copyToGroup);
        }
      }
      else
        roster->copyGroupToGroup(groupName,copyToGroup);
    }
  }
}

void RosterChanger::onMoveGroupToGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(Action::DR_Parametr1).toString();
      QString moveToGroup = action->data(Action::DR_Parametr4).toString();
      if (moveToGroup.endsWith(roster->groupDelimiter()))
      {
        bool ok = false;
        QString newGroupName = QInputDialog::getText(NULL,tr("Create new group"),tr("Enter group name:"),
          QLineEdit::Normal,QString(),&ok);
        if (ok && !newGroupName.isEmpty())
        {
          if (moveToGroup == groupDelim)
            moveToGroup = newGroupName;
          else
            moveToGroup+=newGroupName;
          roster->moveGroupToGroup(groupName,moveToGroup);
        }
      }
      else
        roster->moveGroupToGroup(groupName,moveToGroup);
    }
  }
}

void RosterChanger::onRemoveGroup(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupName = action->data(Action::DR_Parametr1).toString();
      roster->removeGroup(groupName);
    }
  }
}

void RosterChanger::onRemoveGroupItems(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(Action::DR_StreamJid).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupName = action->data(Action::DR_Parametr1).toString();
      QList<IRosterItem> ritems = roster->groupItems(groupName);
      if (ritems.count()>0 && 
        QMessageBox::question(NULL,tr("Remove contacts"),
        tr("You are assured that wish to remove %1 contact(s) from roster?").arg(ritems.count()),
        QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        roster->removeItems(ritems);
      }
    }
  }
}

void RosterChanger::onAddContact(AddContactDialog *ADialog)
{
  Jid streamJid = ADialog->streamJid();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    Jid contactJid = ADialog->contactJid();
    if (!roster->rosterItem(contactJid).isValid)
    {
      QSet<QString> grps = ADialog->groups();
      if (contactJid.node().isEmpty())
        grps.clear();

      roster->setItem(contactJid,ADialog->nick(),grps);
      
      if (ADialog->requestSubscription())
        roster->sendSubscription(contactJid,IRoster::Subscribe,ADialog->requestText());
      if (ADialog->sendSubscription())
        roster->sendSubscription(contactJid,IRoster::Subscribed);
    }
    else
      QMessageBox::information(NULL,streamJid.full(),tr("Contact <b>%1</b> already exists.").arg(contactJid.hBare()));
  }
}

void RosterChanger::onRosterOpened(IRoster *ARoster)
{
  if (FAddContactMenu && !FActions.contains(ARoster))
  {
    IAccount *account = FAccountManager!= NULL ? FAccountManager->accountByStream(ARoster->streamJid()) : NULL;

    Action *action = new Action(FAddContactMenu);
    action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
    if (account)
    {
      action->setText(account->name());
      connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),SLOT(onAccountChanged(const QString &, const QVariant &)));
    }
    else
      action->setText(ARoster->streamJid().hFull());
    action->setData(Action::DR_StreamJid,ARoster->streamJid().full());
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
    FActions.insert(ARoster,action);
    FAddContactMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
    FAddContactMenu->menuAction()->setEnabled(true);
  }
}

void RosterChanger::onRosterClosed(IRoster *ARoster)
{
  if (FAddContactMenu && FActions.contains(ARoster))
  {
    Action *action = FActions.value(ARoster);
    FAddContactMenu->removeAction(action);
    FActions.remove(ARoster);
    if (FActions.count() == 0)
      FAddContactMenu->menuAction()->setEnabled(false);
  }

  if (FSubscriptionDialog && FSubscriptionDialog->streamJid() == ARoster->streamJid())
    FSubscriptionDialog->reject();

  QMap<int,SubscriptionItem> subsItemsCopy = FSubsItems;
  foreach(SubscriptionItem item, subsItemsCopy)
    if (item.streamJid == ARoster->streamJid())
      removeSubscriptionMessage(item.subsId);

  if (FAddContactDialogs.contains(ARoster))
  {
    QList<AddContactDialog *> dialogs = FAddContactDialogs.take(ARoster);
    foreach(AddContactDialog *dialog, dialogs)
      delete dialog;
  }
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
  foreach(SubscriptionItem sitem, FSubsItems)
    if (sitem.notifyId == ANotifyId)
    {
      openSubscriptionDialog(sitem.subsId);
      break;
    }
}

void RosterChanger::onSubscriptionDialogShowNext()
{
  if (FSubsItems.count()>0)
    openSubscriptionDialog(FSubsItems.keys().first());
}

void RosterChanger::onSubscriptionDialogDestroyed(QObject * /*AObject*/)
{
  FSubscriptionDialog = NULL;
  emit subscriptionDialogDestroyed(FSubscriptionDialog);
}

void RosterChanger::onAddContactDialogDestroyed(QObject *AObject)
{
  AddContactDialog *dialog = static_cast<AddContactDialog *>(AObject);
  if (dialog)
  {
    QHash<IRoster *, QList<AddContactDialog *> >::iterator it = FAddContactDialogs.begin();
    while (it != FAddContactDialogs.end())
    {
      if (it.value().contains(dialog))
        it.value().removeAt(it.value().indexOf(dialog));
      it++;
    }
  }
}

void RosterChanger::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == AVN_NAME)
  {
    IAccount *account = qobject_cast<IAccount *>(sender());
    if (account && account->isActive())
    {
      Action *action = FActions.value(FRosterPlugin->getRoster(account->xmppStream()->jid()),NULL);
      if (action)
        action->setText(AValue.toString());
    }
  }
}

void RosterChanger::onMultiUserContextMenu(IMultiUserChatWindow * /*AWindow*/, IMultiUser *AUser, Menu *AMenu)
{
  if (!AUser->data(MUDR_REALJID).toString().isEmpty())
  {
    IRoster *roster = FRosterPlugin->getRoster(AUser->data(MUDR_STREAMJID).toString());
    if (roster && !roster->rosterItem(AUser->data(MUDR_REALJID).toString()).isValid)
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Add contact..."));
      action->setData(Action::DR_StreamJid,AUser->data(MUDR_STREAMJID));
      action->setData(Action::DR_Parametr1,AUser->data(MUDR_REALJID));
      action->setData(Action::DR_Parametr2,AUser->data(MUDR_NICK_NAME));
      action->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_MUCM_ROSTERCHANGER,true);
    }
  }
}

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)

