#include "rosterchanger.h"

#include <QInputDialog>
#include <QMessageBox>

RosterChanger::RosterChanger()
{
  FRosterPlugin = NULL;
  FRostersModelPlugin = NULL;
  FRostersModel = NULL;
  FRostersViewPlugin = NULL;
  FRostersView = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL; 

  FAddContactMenu = NULL;
  FSubsId = 0;
  FSubsLabelId = 0;

  FSystemIconset.openFile(SYSTEM_ICONSETFILE);
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
  AInitOrder = ROSTERCHANGER_INITORDER;

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

  return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{

  if (FRostersModelPlugin && FRostersModelPlugin->rostersModel())
  {
    FRostersModel = FRostersModelPlugin->rostersModel();
  }

  if (FRostersViewPlugin && FRostersViewPlugin->rostersView())
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FSubsLabelId = FRostersView->createIndexLabel(ROSTERCHANGER_LABEL_ORDER,
      FSystemIconset.iconByName("psi/events"),IRostersView::LabelBlink);
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
    FAddContactMenu->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
    FAddContactMenu->setTitle(tr("Add contact"));
    FAddContactMenu->menuAction()->setEnabled(false);
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FAddContactMenu->menuAction(),ROSTERCHANGER_ACTION_GROUP_CONTACT,true);
  }

  if (FTrayManager)
  {
    if (FAddContactMenu)
      FTrayManager->addAction(FAddContactMenu->menuAction(),ROSTERCHANGER_ACTION_GROUP_CONTACT,true);
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
            groupMenu->addAction(curGroupAction,ROSTERCHANGER_ACTION_GROUP+1);
          }

          if (ANewGroup)
          {
            Action *newGroupAction = new Action(groupMenu);
            newGroupAction->setText(tr("Create new..."));
            newGroupAction->setData(AData);
            newGroupAction->setData(Action::DR_Parametr4,groupName+groupDelim);
            connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(newGroupAction,ROSTERCHANGER_ACTION_GROUP+1);
          }

          menus.insert(groupName,groupMenu);
          parentMenu->addAction(groupMenu->menuAction(),ROSTERCHANGER_ACTION_GROUP,true);
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
      menu->addAction(curGroupAction,ROSTERCHANGER_ACTION_GROUP+1);
    }

    if (ANewGroup)
    {
      Action *newGroupAction = new Action(menu);
      newGroupAction->setText(tr("Create new..."));
      newGroupAction->setData(AData);
      newGroupAction->setData(Action::DR_Parametr4,groupDelim);
      connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(newGroupAction,ROSTERCHANGER_ACTION_GROUP+1);
    }
  }
  return menu;
}

IRosterIndexList RosterChanger::getContactIndexList(const Jid &AStreamJid, const Jid &AJid)
{
  IRosterIndexList indexList;
  if (FRostersModel)
  {
    IRosterIndex *streamRoot = FRostersModel->getStreamRoot(AStreamJid);
    if (streamRoot)
    {
      IRosterIndex::ItemType type = IRosterIndex::IT_Contact;
      if (AJid.node().isEmpty())
        type = IRosterIndex::IT_Agent;
      QHash<int,QVariant> data;
      data.insert(IRosterIndex::DR_Type, type);
      data.insert(IRosterIndex::DR_BareJid, AJid.pBare());
      indexList = streamRoot->findChild(data,true);
      if (indexList.isEmpty())
      {
        IRoster *roster = FRostersModel->getRoster(AStreamJid.pFull());
        IRosterIndex *notInRosterGroup = FRostersModel->createGroup(FRostersModel->notInRosterGroupName(),
          roster->groupDelimiter(),IRosterIndex::IT_NotInRosterGroup,streamRoot);
        IRosterIndex *index = FRostersModel->createRosterIndex(type,AJid.pBare(),notInRosterGroup);
        index->setData(IRosterIndex::DR_Jid,AJid.full());
        index->setData(IRosterIndex::DR_PJid,AJid.pFull());
        index->setData(IRosterIndex::DR_BareJid,AJid.pBare());
        index->setData(IRosterIndex::DR_Group,FRostersModel->notInRosterGroupName());
        FRostersModel->insertRosterIndex(index,notInRosterGroup);
        indexList.append(index);
      }
    }
  }
  return indexList;
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

    if (FRostersView)
    {
      bool removeLabel = true;
      foreach(SubsItem *sItem,FSubsItems)
        if (sItem->streamJid == subsItem->streamJid && sItem->contactJid.equals(subsItem->contactJid,false))
        {
          removeLabel = false;
          break;
        };
      if (removeLabel)
      {
        IRosterIndexList indexList = getContactIndexList(subsItem->streamJid, subsItem->contactJid);
        foreach(IRosterIndex *index, indexList)
          FRostersView->removeIndexLabel(FSubsLabelId,index);
      }
    }

    delete subsItem;
  }
}

void RosterChanger::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  QString streamJid = AIndex->data(IRosterIndex::DR_StreamJid).toString();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    int itemType = AIndex->data(IRosterIndex::DR_Type).toInt();
    IRosterItem *rosterItem = roster->item(AIndex->data(IRosterIndex::DR_BareJid).toString());
    if (itemType == IRosterIndex::IT_StreamRoot)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,AIndex->data(IRosterIndex::DR_Jid));

      Action *action = new Action(AMenu);
      action->setText(tr("Add contact..."));
      action->setData(data);
      action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
      connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
      AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP_CONTACT,true);
    }
    else if (itemType == IRosterIndex::IT_Contact || itemType == IRosterIndex::IT_Agent)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_StreamJid,streamJid);
      data.insert(Action::DR_Parametr1,AIndex->data(IRosterIndex::DR_BareJid));
      
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

      AMenu->addAction(subsMenu->menuAction(),ROSTERCHANGER_ACTION_GROUP_SUBSCRIPTION);

      if (rosterItem)
      {
        QSet<QString> exceptGroups = rosterItem->groups();

        data.insert(Action::DR_Parametr2,AIndex->data(IRosterIndex::DR_Name));
        data.insert(Action::DR_Parametr3,AIndex->data(IRosterIndex::DR_Group));

        action = new Action(AMenu);
        action->setText(tr("Rename..."));
        action->setData(data);
        connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
        AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP);

        if (itemType == IRosterIndex::IT_Contact)
        {
          Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onCopyItemToGroup(bool)),AMenu);
          copyItem->setTitle(tr("Copy to group"));
          AMenu->addAction(copyItem->menuAction(),ROSTERCHANGER_ACTION_GROUP);

          Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onMoveItemToGroup(bool)),AMenu);
          moveItem->setTitle(tr("Move to group"));
          AMenu->addAction(moveItem->menuAction(),ROSTERCHANGER_ACTION_GROUP);
        }

        if (!AIndex->data(IRosterIndex::DR_Group).toString().isEmpty())
        {
          action = new Action(AMenu);
          action->setText(tr("Remove from group"));
          action->setData(data);
          connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromGroup(bool)));
          AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP);
        }
      }
      else
      {
        action = new Action(AMenu);
        action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
        action->setText(tr("Add contact..."));
        action->setData(Action::DR_StreamJid,streamJid);
        action->setData(Action::DR_Parametr1,AIndex->data(IRosterIndex::DR_Jid));
        connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
        AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP_CONTACT,true);
      }

      action = new Action(AMenu);
      action->setText(tr("Remove from roster"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
      AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP);
    }
    else if (itemType == IRosterIndex::IT_Group)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_Parametr1,AIndex->data(IRosterIndex::DR_Group));
      data.insert(Action::DR_StreamJid,streamJid);
      
      Action *action;
      action = new Action(AMenu);
      action->setText(tr("Rename..."));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
      AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP);

      QSet<QString> exceptGroups;
      exceptGroups << AIndex->data(IRosterIndex::DR_Group).toString();

      Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onCopyGroupToGroup(bool)),AMenu);
      copyGroup->setTitle(tr("Copy to group"));
      AMenu->addAction(copyGroup->menuAction(),ROSTERCHANGER_ACTION_GROUP);

      Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onMoveGroupToGroup(bool)),AMenu);
      moveGroup->setTitle(tr("Move to group"));
      AMenu->addAction(moveGroup->menuAction(),ROSTERCHANGER_ACTION_GROUP);

      action = new Action(AMenu);
      action->setText(tr("Remove"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
      AMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP);
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
    subsItem->trayId = FTrayManager->appendNotify(FSystemIconset.iconByName("psi/events"),
      tr("Subscription message from %1").arg(AFromJid.full()),true);
  }

  if (FRostersView)
  {
    IRosterIndexList indexList = getContactIndexList(ARoster->streamJid(),AFromJid);
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
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      QString oldName = action->data(Action::DR_Parametr2).toString();
      bool ok = false;
      QString newName = QInputDialog::getText(NULL,tr("Rename contact"),tr("Enter name for: ")+rosterJid,
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
      QString rosterJid = action->data(Action::DR_Parametr1).toString();
      IRosterItem *rosterItem = roster->item(rosterJid);
      if (rosterItem)
      {
        int button = QMessageBox::question(NULL,tr("Remove contact"),
          tr("You are assured that wish to remove a contact <b>%1</b> from roster?").arg(rosterJid),
          QMessageBox::Yes | QMessageBox::No);
        if (button == QMessageBox::Yes)
          roster->removeItem(rosterJid);
      }
      else if (FRostersModel)
      {
        QMultiHash<int, QVariant> data;
        data.insert(IRosterIndex::DR_Type,IRosterIndex::IT_Contact);
        data.insert(IRosterIndex::DR_Type,IRosterIndex::IT_Agent);
        data.insert(IRosterIndex::DR_BareJid,rosterJid);
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
      QMessageBox::information(NULL,streamJid.full(),tr("Contact <b>%1</b> already exists.").arg(contactJid.bare()));
  }
}

void RosterChanger::onRosterOpened(IRoster *ARoster)
{
  if (FAddContactMenu && !FActions.contains(ARoster))
  {
    Action *action = new Action(FAddContactMenu);
    action->setIcon(SYSTEM_ICONSETFILE,"psi/addContact");
    action->setText(ARoster->streamJid().full());
    action->setData(Action::DR_StreamJid,ARoster->streamJid().full());
    connect(action,SIGNAL(triggered(bool)),SLOT(showAddContactDialogByAction(bool)));
    FActions.insert(ARoster,action);
    FAddContactMenu->addAction(action,ROSTERCHANGER_ACTION_GROUP_CONTACT,true);
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
    Jid sJid = AIndex->data(IRosterIndex::DR_StreamJid).toString();
    Jid cJid = AIndex->data(IRosterIndex::DR_BareJid).toString();
    QHash<int,SubsItem *>::iterator it = FSubsItems.begin();
    while (it != FSubsItems.end())
    {
      if (it.value()->streamJid == sJid && it.value()->contactJid.equals(cJid,false))
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

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)

