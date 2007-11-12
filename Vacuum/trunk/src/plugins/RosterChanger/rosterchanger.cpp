#include "rosterchanger.h"

#include <QInputDialog>
#include <QMessageBox>

#define IN_EVENTS           "psi/events"
#define IN_ADDCONTACT       "psi/addContact"

RosterChanger::RosterChanger()
{
  FRosterPlugin = NULL;
  FRostersModelPlugin = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FRostersView = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL; 
  FAccountManager = NULL;

  FAddContactMenu = NULL;
  FSubsId = 0;
  FSubsLabelId = 0;
}

RosterChanger::~RosterChanger()
{
  qDeleteAll(FSubsItems);
}

//IPlugin
void RosterChanger::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Manipulating roster items and groups");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster Changer"); 
  APluginInfo->uid = ROSTERCHANGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(ROSTER_UUID); 
}

bool RosterChanger::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  AInitOrder = IO_ROSTERCHANGER;

  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterOpened(IRoster *)),SLOT(onRosterOpened(IRoster *)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterClosed(IRoster *)),SLOT(onRosterClosed(IRoster *)));
      connect(FRosterPlugin->instance(),
        SIGNAL(rosterSubscription(IRoster *, const Jid &, IRoster::SubsType, const QString &)),
        SLOT(onReceiveSubscription(IRoster *, const Jid &, IRoster::SubsType, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(notifyActivated(int)),SLOT(onTrayNotifyActivated(int)));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
  }

  return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
  FSystemIconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  connect(FSystemIconset,SIGNAL(iconsetChanged()),SLOT(onSystemIconsetChanged()));

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersModel = FRostersModelPlugin->rostersModel();
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FSubsLabelId = FRostersView->createIndexLabel(RLO_SUBSCRIBTION,
      FSystemIconset->iconByName(IN_EVENTS),IRostersView::LabelBlink);
    connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    connect(FRostersView,SIGNAL(labelDoubleClicked(IRosterIndex *, int, bool &)),
      SLOT(onRosterLabelDClicked(IRosterIndex *, int, bool &)));
    connect(FRostersView,SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
  }

  if (FMainWindowPlugin && FMainWindowPlugin->mainWindow())
  {
    FAddContactMenu = new Menu(FMainWindowPlugin->mainWindow()->mainMenu());
    FAddContactMenu->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
    FAddContactMenu->setTitle(tr("Add contact to"));
    FAddContactMenu->menuAction()->setEnabled(false);
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FAddContactMenu->menuAction(),AG_ROSTERCHANGER_MMENU,true);
  }

  if (FTrayManager)
  {
    if (FAddContactMenu)
      FTrayManager->addAction(FAddContactMenu->menuAction(),AG_ROSTERCHANGER_TRAY,true);
  }

  return true;
}

//IRosterChanger
void RosterChanger::showAddContactDialog(const Jid &AStreamJid, const Jid &AJid, const QString &ANick, 
                                         const QString &AGroup, const QString &ARequest) const
{
  IRoster *roster = FRosterPlugin->getRoster(AStreamJid);
  if (roster && roster->isOpen())
  {
    AddContactDialog *dialog = new AddContactDialog(NULL);
    dialog->setGroups(roster->groups());
    dialog->setStreamJid(AStreamJid);
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

void RosterChanger::showAddContactDialogByAction(bool)
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

Menu * RosterChanger::createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
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

void RosterChanger::openSubsDialog(int ASubsId)
{
  if (FSubsItems.contains(ASubsId))
  {
    SubsItem *subsItem = FSubsItems.value(ASubsId);

    if (FSubsDialog.isNull())
    {
      FSubsDialog = new SubscriptionDialog;
      connect(FSubsDialog,SIGNAL(setupNext()),SLOT(onSubsDialogSetupNext()));
      connect(FSubsDialog->dialogAction(),SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
    }
    
    QString subs = "none";
    IRoster *roster = FRosterPlugin->getRoster(subsItem->streamJid);
    if (roster && roster->isOpen())
    {
      IRosterItem *rosterItem = roster->item(subsItem->contactJid);
      if (rosterItem)
        subs = rosterItem->subscription();
    }

    FSubsDialog->setupDialog(subsItem->streamJid,subsItem->contactJid,subsItem->time,subsItem->type,subsItem->status,subs);
    FSubsDialog->show();
    FSubsDialog->activateWindow();

    removeSubsMessage(ASubsId);
  }
}

void RosterChanger::removeSubsMessage(int ASubsId)
{
  if (FSubsItems.contains(ASubsId))
  {
    SubsItem *subsItem = FSubsItems.take(ASubsId);

    if (!FSubsDialog.isNull())
      FSubsDialog->setNextCount(FSubsItems.count());

    if (FTrayManager)
      FTrayManager->removeNotify(subsItem->trayId);

    if (FRostersView && FRostersModel)
    {
      bool removeLabel = true;
      foreach(SubsItem *sItem,FSubsItems)
        if (sItem->streamJid == subsItem->streamJid && (sItem->contactJid && subsItem->contactJid))
        {
          removeLabel = false;
          break;
        };
      if (removeLabel)
      {
        IRosterIndexList indexList = FRostersModel->getContactIndexList(subsItem->streamJid, subsItem->contactJid, false);
        foreach(IRosterIndex *index, indexList)
          FRostersView->removeIndexLabel(FSubsLabelId,index);
      }
    }

    delete subsItem;
  }
}

void RosterChanger::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  QString streamJid = AIndex->data(RDR_StreamJid).toString();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    int itemType = AIndex->data(RDR_Type).toInt();
    IRosterItem *rosterItem = roster->item(AIndex->data(RDR_BareJid).toString());
    if (itemType == RIT_StreamRoot)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,AIndex->data(RDR_Jid));

      Action *action = new Action(AMenu);
      action->setText(tr("Add contact..."));
      action->setData(data);
      action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
      connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
    }
    else if (itemType == RIT_Contact || itemType == RIT_Agent)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,streamJid);
      data.insert(Action::DR_Parametr1,AIndex->data(RDR_BareJid).toString());
      
      Action *action;

      Menu *subsMenu = new Menu(AMenu);
      subsMenu->setTitle(tr("Subscription"));

      action = new Action(subsMenu);
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

      if (rosterItem)
      {
        QSet<QString> exceptGroups = rosterItem->groups();

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
        action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
        action->setText(tr("Add contact..."));
        action->setData(Action::DR_StreamJid,streamJid);
        action->setData(Action::DR_Parametr1,AIndex->data(RDR_Jid));
        connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
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
      
      Action *action;
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
      action->setText(tr("Remove"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
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
      IRoster::SubsType subsType = (IRoster::SubsType)action->data(Action::DR_Parametr2).toInt();
      roster->sendSubscription(rosterJid,subsType);
    }
  }
}

void RosterChanger::onReceiveSubscription(IRoster *ARoster, const Jid &AFromJid, 
                                          IRoster::SubsType AType, const QString &AStatus)
{
  SubsItem *subsItem = new SubsItem;
  subsItem->streamJid = ARoster->streamJid();
  subsItem->contactJid = AFromJid;
  subsItem->type = AType;
  subsItem->status = AStatus;
  subsItem->time = QDateTime::currentDateTime();

  if (FTrayManager)
  {
    subsItem->trayId = FTrayManager->appendNotify(FSystemIconset->iconByName(IN_EVENTS),
      tr("Subscription message from %1").arg(AFromJid.full()),true);
  }

  if (FRostersView && FRostersModel)
  {
    IRosterIndexList indexList = FRostersModel->getContactIndexList(ARoster->streamJid(),AFromJid,true);
    foreach(IRosterIndex *index, indexList)
      FRostersView->insertIndexLabel(FSubsLabelId,index);
  }

  FSubsItems.insert(FSubsId++,subsItem);

  if (!FSubsDialog.isNull())
    FSubsDialog->setNextCount(FSubsItems.count());
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
      IRosterItem *rosterItem = roster->item(rosterJid);
      if (rosterItem)
      {
        int button = QMessageBox::question(NULL,tr("Remove contact"),
          tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(rosterJid.hBare()),
          QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes)
          roster->removeItem(rosterJid);
      }
      else if (FRostersModel)
      {
        QMultiHash<int, QVariant> data;
        data.insert(RDR_Type,RIT_Contact);
        data.insert(RDR_Type,RIT_Agent);
        data.insert(RDR_BareJid,rosterJid.pBare());
        IRosterIndex *streamRoot = FRostersModel->getStreamRoot(streamJid);
        IRosterIndexList indexList = streamRoot->findChild(data,true);
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

void RosterChanger::onAddContact(AddContactDialog *ADialog)
{
  Jid streamJid = ADialog->stramJid();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    Jid contactJid = ADialog->contactJid();
    if (!roster->item(contactJid))
    {
      QSet<QString> grps = ADialog->groups();
      if (contactJid.node().isEmpty())
        grps.clear();

      roster->setItem(contactJid,ADialog->nick(),grps);
      
      if (ADialog->requestSubscr())
        roster->sendSubscription(contactJid,IRoster::Subscribe,ADialog->requestText());
      if (ADialog->sendSubscr())
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
    connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
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
    delete action;
  }

  Jid streamJid = ARoster->streamJid();
  if (!FSubsDialog.isNull() && FSubsDialog->streamJid() == streamJid)
    FSubsDialog->reject();

  QHash<int,SubsItem *>::iterator it = FSubsItems.begin();
  while (it != FSubsItems.end())
  {
    if (it.value()->streamJid == streamJid)
    {
      removeSubsMessage(it.key());
      it = FSubsItems.begin(); 
    }
    else
      it++;
  }

  if (FAddContactDialogs.contains(ARoster))
  {
    QList<AddContactDialog *> dialogs = FAddContactDialogs.take(ARoster);
    foreach(AddContactDialog *dialog, dialogs)
      delete dialog;
    FAddContactDialogs.remove(ARoster);
  }
}

void RosterChanger::onRosterLabelDClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted)
{
  if (ALabelId == FSubsLabelId)
  {
    AAccepted = true;
    Jid sJid = AIndex->data(RDR_StreamJid).toString();
    Jid cJid = AIndex->data(RDR_BareJid).toString();
    QHash<int,SubsItem *>::iterator it = FSubsItems.begin();
    while (it != FSubsItems.end())
    {
      if ((it.value()->streamJid == sJid) && (it.value()->contactJid && cJid))
      {
        openSubsDialog(it.key());
        break;
      }
      it++;
    }
  }
}

void RosterChanger::onRosterLabelToolTips(IRosterIndex * /*AIndex*/, int /*ALabelId*/, 
                                          QMultiMap<int,QString> &/*AToolTips*/)
{

}

void RosterChanger::onTrayNotifyActivated(int ANotifyId)
{
  QHash<int,SubsItem *>::iterator it = FSubsItems.begin();
  while (it != FSubsItems.end())
  {
    if (it.value()->trayId == ANotifyId)
    {
      openSubsDialog(it.key());
      break;
    }
    it++;
  }
}

void RosterChanger::onSubsDialogSetupNext()
{
  if (FSubsItems.count()>0)
    openSubsDialog(FSubsItems.keys().first());
}

void RosterChanger::onAddContactDialogDestroyed(QObject *AObject)
{
  AddContactDialog *dialog = static_cast<AddContactDialog *>(AObject);
  if (dialog)
  {
    QHash<IRoster *, QList<AddContactDialog *>>::iterator it = FAddContactDialogs.begin();
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
  if (AName == "name")
  {
    IAccount *account = qobject_cast<IAccount *>(sender());
    if (account)
    {
      Action *action = FActions.value(FRosterPlugin->getRoster(account->streamJid()),NULL);
      if (action)
        action->setText(AValue.toString());
    }
  }
}

void RosterChanger::onSystemIconsetChanged()
{
  if (FRostersView)
  {
    FRostersView->updateIndexLabel(RLO_SUBSCRIBTION,FSystemIconset->iconByName(IN_EVENTS),IRostersView::LabelBlink);
  }
}

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)

