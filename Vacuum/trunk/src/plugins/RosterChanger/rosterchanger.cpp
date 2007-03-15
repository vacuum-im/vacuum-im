#include "rosterchanger.h"
#include <QInputDialog>

RosterChanger::RosterChanger()
{
  FSystemIconset.openFile("system/common.jisp");
}

RosterChanger::~RosterChanger()
{

}

//IPlugin
void RosterChanger::getPluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Manipulating roster items and groups");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Roster Changer"); 
  APluginInfo->uid = ROSTERCHANGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{5306971C-2488-40d9-BA8E-C83327B2EED5}"); //IRoster  
}

bool RosterChanger::initPlugin(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  return FRosterPlugin!=NULL;
}

bool RosterChanger::startPlugin()
{
  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(const QModelIndex &, Menu *)),
      SLOT(onRostersViewContextMenu(const QModelIndex &, Menu *)));
  }

  return true;
}

Menu * RosterChanger::createGroupMenu(const QHash<int,QVariant> AData, const QSet<QString> &AExceptGroups, 
                                      bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent)
{
  Menu *menu = new Menu(ROSTERCHANGER_MENU_ORDER,AParent);
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
          Menu *groupMenu = new Menu(ROSTERCHANGER_MENU_ORDER,parentMenu);
          groupMenu->setTitle(groupTree.at(index));
          //groupMenu->setIcon(FSystemIconset.iconByName("psi/groupChat"));

          if (!AExceptGroups.contains(groupName))
          {
            Action *curGroupAction = new Action(ROSTERCHANGER_MENU_ORDER+1,groupMenu);
            curGroupAction->setText(tr("This group"));
            //curGroupAction->setIcon(FSystemIconset.iconByName("psi/groupChat"));
            curGroupAction->setData(AData);
            curGroupAction->setData(Action::DR_Parametr4,groupName);
            connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(curGroupAction);
          }

          if (ANewGroup)
          {
            Action *newGroupAction = new Action(ROSTERCHANGER_MENU_ORDER+1,groupMenu);
            newGroupAction->setText(tr("Create new..."));
            //newGroupAction->setIcon(FSystemIconset.iconByName("psi/groupChat"));
            newGroupAction->setData(AData);
            newGroupAction->setData(Action::DR_Parametr4,groupName+groupDelim);
            connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(newGroupAction);
          }

          menus.insert(groupName,groupMenu);
          parentMenu->addAction(groupMenu->menuAction());
          parentMenu = groupMenu;
        }
        else
          parentMenu = menus.value(groupName); 

        index++;
      }
    }

    if (ARootGroup)
    {
      Action *curGroupAction = new Action(ROSTERCHANGER_MENU_ORDER+1,menu);
      curGroupAction->setText(tr("Root"));
      //curGroupAction->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      curGroupAction->setData(AData);
      curGroupAction->setData(Action::DR_Parametr4,"");
      connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(curGroupAction);
    }

    if (ANewGroup)
    {
      Action *newGroupAction = new Action(ROSTERCHANGER_MENU_ORDER+1,menu);
      newGroupAction->setText(tr("Create new..."));
      //newGroupAction->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      newGroupAction->setData(AData);
      newGroupAction->setData(Action::DR_Parametr4,groupDelim);
      connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(newGroupAction);
    }
  }
  return menu;
}

void RosterChanger::onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
  if (!AIndex.isValid())
    return;

  QString streamJid = AIndex.data(IRosterIndex::DR_StreamJid).toString();
  IRoster *roster = FRosterPlugin->getRoster(streamJid);
  if (roster && roster->isOpen())
  {
    int itemType = AIndex.data(IRosterIndex::DR_Type).toInt();
    if (itemType == IRosterIndex::IT_Contact || itemType == IRosterIndex::IT_Agent)
    {
      IRosterItem *rosterItem = roster->item(AIndex.data(IRosterIndex::DR_RosterJid).toString());
      QSet<QString> exceptGroups;
      if (rosterItem)
        exceptGroups = rosterItem->groups();

      QHash<int,QVariant> data;
      data.insert(Action::DR_Parametr1,AIndex.data(IRosterIndex::DR_RosterJid));
      data.insert(Action::DR_Parametr2,AIndex.data(IRosterIndex::DR_RosterName));
      data.insert(Action::DR_Parametr3,AIndex.data(IRosterIndex::DR_RosterGroup));
      data.insert(Action::DR_StreamJid,streamJid);

      Action *action;
      action = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
      action->setText(tr("Rename..."));
      //action->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
      AMenu->addAction(action);

      if (itemType == IRosterIndex::IT_Contact)
      {
        Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onCopyItemToGroup(bool)),AMenu);
        copyItem->setTitle(tr("Copy to group"));
        //copyItem->setIcon(FSystemIconset.iconByName("psi/groupChat"));
        AMenu->addAction(copyItem->menuAction());

        Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onMoveItemToGroup(bool)),AMenu);
        moveItem->setTitle(tr("Move to group"));
        //moveItem->setIcon(FSystemIconset.iconByName("psi/groupChat"));
        AMenu->addAction(moveItem->menuAction());
      }

      if (!AIndex.data(IRosterIndex::DR_RosterGroup).toString().isEmpty())
      {
        action = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
        action->setText(tr("Remove from group"));
        //action->setIcon(FSystemIconset.iconByName("psi/remove"));
        action->setData(data);
        connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromGroup(bool)));
        AMenu->addAction(action);
      }

      action = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
      action->setText(tr("Remove from roster"));
      //action->setIcon(FSystemIconset.iconByName("psi/remove"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
      AMenu->addAction(action);
    }
    else if (itemType == IRosterIndex::IT_Group)
    {
      QHash<int,QVariant> data;
      data.insert(Action::DR_Parametr1,AIndex.data(IRosterIndex::DR_GroupName));
      data.insert(Action::DR_StreamJid,streamJid);
      
      Action *action;
      action = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
      action->setText(tr("Rename..."));
      //action->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
      AMenu->addAction(action);

      QSet<QString> exceptGroups;
      exceptGroups << AIndex.data(IRosterIndex::DR_GroupName).toString();

      Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onCopyGroupToGroup(bool)),AMenu);
      copyGroup->setTitle(tr("Copy to group"));
      //copyGroup->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      AMenu->addAction(copyGroup->menuAction());

      Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onMoveGroupToGroup(bool)),AMenu);
      moveGroup->setTitle(tr("Move to group"));
      //moveGroup->setIcon(FSystemIconset.iconByName("psi/groupChat"));
      AMenu->addAction(moveGroup->menuAction());

      action = new Action(ROSTERCHANGER_MENU_ORDER,AMenu);
      action->setText(tr("Remove"));
      //action->setIcon(FSystemIconset.iconByName("psi/remove"));
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
      AMenu->addAction(action);
    }
  }
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
      roster->removeItem(action->data(Action::DR_Parametr1).toString());
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

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)
