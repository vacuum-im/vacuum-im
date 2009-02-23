#include "rosterchanger.h"

#include <QtDebug>
#include <QInputDialog>
#include <QMessageBox>

#define NOTIFICATOR_ID      "RosterChanger"

#define SVN_AUTOSUBSCRIBE   "autoSubscribe"
#define SVN_AUTOUNSUBSCRIBE "autoUnsubscribe"

#define ADR_STREAM_JID      Action::DR_StreamJid
#define ADR_CONTACT_JID     Action::DR_Parametr1
#define ADR_SUBSCRIPTION    Action::DR_Parametr2
#define ADR_NICK            Action::DR_Parametr2
#define ADR_GROUP           Action::DR_Parametr3
#define ADR_REQUEST         Action::DR_Parametr4
#define ADR_TO_GROUP        Action::DR_Parametr4

RosterChanger::RosterChanger()
{
  FPluginManager = NULL;
  FRosterPlugin = NULL;
  FRostersModel = NULL;
  FRostersModel = NULL;
  FRostersView = NULL;
  FMainWindowPlugin = NULL;
  FTrayManager = NULL; 
  FNotifications = NULL;
  FAccountManager = NULL;
  FMultiUserChatPlugin = NULL;
  FSettingsPlugin = NULL;

  FOptions = 0;
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
  FPluginManager = APluginManager;

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
    IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (rostersViewPlugin)
    {
      FRostersView = rostersViewPlugin->rostersView();
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

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }


  return FRosterPlugin!=NULL;
}

bool RosterChanger::initObjects()
{
  if (FMainWindowPlugin)
  {
    FAddContactMenu = new Menu(FMainWindowPlugin->mainWindow()->mainMenu());
    FAddContactMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
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

  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
  }

  return true;
}

//IoptionsHolder
QWidget *RosterChanger::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_ROSTER)
  {
    AOrder = OWO_ROSTER_CHENGER;
    SubscriptionOptions *widget = new SubscriptionOptions(this);
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(widget,SIGNAL(applied()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

//IRosterChanger
bool RosterChanger::isAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
    return (FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoOptions & IRosterChanger::AutoSubscribe) > 0;
  return checkOption(AutoSubscribe);
}

bool RosterChanger::isAutoUnsubscribe(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
    return (FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).autoOptions & IRosterChanger::AutoUnsubscribe) > 0;
  return checkOption(AutoUnsubscribe);
}

bool RosterChanger::isSilentSubsctiption(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (FAutoSubscriptions.value(AStreamJid).contains(AContactJid.bare()))
    return FAutoSubscriptions.value(AStreamJid).value(AContactJid.bare()).silent;
  return false;
}

void RosterChanger::insertAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid, int AAutoOptions, bool ASilently)
{
  AutoSubscription &asubscr = FAutoSubscriptions[AStreamJid][AContactJid.bare()];
  asubscr.silent = ASilently;
  asubscr.autoOptions = AAutoOptions;
}

void RosterChanger::removeAutoSubscribe(const Jid &AStreamJid, const Jid &AContactJid)
{
  FAutoSubscriptions[AStreamJid].remove(AContactJid.bare());
}

void RosterChanger::subscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster && roster->isOpen())
  {
    const IRosterItem &ritem = roster->rosterItem(AContactJid);
    roster->sendSubscription(AContactJid,IRoster::Subscribed,AMessage);
    if (ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
    {
      insertAutoSubscribe(AStreamJid,AContactJid,AutoSubscribe,ASilently);
      roster->sendSubscription(AContactJid,IRoster::Subscribe,AMessage);
    }
  }
}

void RosterChanger::unsubscribeContact(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage, bool ASilently)
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster && roster->isOpen())
  {
    const IRosterItem &ritem = roster->rosterItem(AContactJid);
    roster->sendSubscription(AContactJid,IRoster::Unsubscribed,AMessage);
    if (ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_NONE)
    {
      insertAutoSubscribe(AStreamJid,AContactJid,AutoUnsubscribe,ASilently);
      roster->sendSubscription(AContactJid,IRoster::Unsubscribe,AMessage);
    }
  }
}

bool RosterChanger::checkOption(IRosterChanger::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void RosterChanger::setOption(IRosterChanger::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    emit optionChanged(AOption,AValue);
  }
}

IAddContactDialog *RosterChanger::showAddContactDialog(const Jid &AStreamJid)
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  if (roster && roster->isOpen())
  {
    AddContactDialog *dialog = new AddContactDialog(this,FPluginManager,AStreamJid);
    connect(roster->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    emit addContactDialogCreated(dialog);
    dialog->show();
    return dialog;
  }
  return NULL;
}

QString RosterChanger::subscriptionNotify(int ASubsType, const Jid &AContactJid) const
{
  switch(ASubsType) 
  {
  case IRoster::Subscribe:
    return tr("%1 wants to subscribe to your presence.").arg(AContactJid.hBare());
  case IRoster::Subscribed:
    return tr("You are now subscribed for %1 presence.").arg(AContactJid.hBare());
  case IRoster::Unsubscribe:
    return tr("%1 unsubscribed from your presence.").arg(AContactJid.hBare());
  case IRoster::Unsubscribed:
    return tr("You are now unsubscribed from %1 presence.").arg(AContactJid.hBare());
  }
  return QString();
}

Menu *RosterChanger::createGroupMenu(const QHash<int,QVariant> &AData, const QSet<QString> &AExceptGroups, 
                                     bool ANewGroup, bool ARootGroup, const char *ASlot, Menu *AParent)
{
  Menu *menu = new Menu(AParent);
  IRoster *roster = FRosterPlugin->getRoster(AData.value(ADR_STREAM_JID).toString());
  if (roster)
  {
    QString group;
    QString groupDelim = roster->groupDelimiter();
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
          groupMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_GROUP);

          if (!AExceptGroups.contains(groupName))
          {
            Action *curGroupAction = new Action(groupMenu);
            curGroupAction->setText(tr("This group"));
            curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_THIS_GROUP);
            curGroupAction->setData(AData);
            curGroupAction->setData(ADR_TO_GROUP,groupName);
            connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
            groupMenu->addAction(curGroupAction,AG_ROSTERCHANGER_ROSTER+1);
          }

          if (ANewGroup)
          {
            Action *newGroupAction = new Action(groupMenu);
            newGroupAction->setText(tr("Create new..."));
            newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
            newGroupAction->setData(AData);
            newGroupAction->setData(ADR_TO_GROUP,groupName+groupDelim);
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
      curGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ROOT_GROUP);
      curGroupAction->setData(AData);
      curGroupAction->setData(ADR_TO_GROUP,"");
      connect(curGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(curGroupAction,AG_ROSTERCHANGER_ROSTER+1);
    }

    if (ANewGroup)
    {
      Action *newGroupAction = new Action(menu);
      newGroupAction->setText(tr("Create new..."));
      newGroupAction->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_CREATE_GROUP);
      newGroupAction->setData(AData);
      newGroupAction->setData(ADR_TO_GROUP,groupDelim);
      connect(newGroupAction,SIGNAL(triggered(bool)),ASlot);
      menu->addAction(newGroupAction,AG_ROSTERCHANGER_ROSTER+1);
    }
  }
  return menu;
}

SubscriptionDialog *RosterChanger::subscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid) const
{
  foreach (SubscriptionDialog *dialog, FSubscrDialogs.keys())
    if (dialog->streamJid()==AStreamJid && dialog->contactJid()==AContactJid)
      return dialog;
  return NULL;
}

SubscriptionDialog *RosterChanger::createSubscriptionDialog(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANotify, const QString &AMessage)
{
  SubscriptionDialog *dialog = subscriptionDialog(AStreamJid,AContactJid);
  if (dialog == NULL)
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
    if (roster && roster->isOpen())
    {
      dialog = new SubscriptionDialog(this,FPluginManager,AStreamJid,AContactJid,ANotify,AMessage);
      connect(roster->instance(),SIGNAL(closed()),dialog->instance(),SLOT(reject()));
      connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onSubscriptionDialogDestroyed()));
      emit subscriptionDialogCreated(dialog);
    }
  }
  return dialog;
}

void RosterChanger::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(ROSTERCHANGER_UUID);
  setOption(IRosterChanger::AutoSubscribe,settings->value(SVN_AUTOSUBSCRIBE,false).toBool());
  setOption(IRosterChanger::AutoUnsubscribe,settings->value(SVN_AUTOUNSUBSCRIBE,true).toBool());
}

void RosterChanger::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(ROSTERCHANGER_UUID);
  settings->setValue(SVN_AUTOSUBSCRIBE,checkOption(IRosterChanger::AutoSubscribe));
  settings->setValue(SVN_AUTOUNSUBSCRIBE,checkOption(IRosterChanger::AutoUnsubscribe));
}

void RosterChanger::onShowAddContactDialog(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    IAddContactDialog *dialog = showAddContactDialog(action->data(ADR_STREAM_JID).toString());
    if (dialog)
    {
      dialog->setContactJid(action->data(ADR_CONTACT_JID).toString());
      dialog->setNickName(action->data(ADR_NICK).toString());
      dialog->setGroup(action->data(ADR_GROUP).toString());
      dialog->setSubscriptionMessage(action->data(Action::DR_Parametr4).toString());
    }
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
      Action *action = new Action(AMenu);
      action->setText(tr("Add contact"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
      action->setData(ADR_STREAM_JID,AIndex->data(RDR_Jid));
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
    }
    else if (itemType == RIT_Contact || itemType == RIT_Agent)
    {
      QHash<int,QVariant> data;
      data.insert(ADR_STREAM_JID,streamJid);
      data.insert(ADR_CONTACT_JID,AIndex->data(RDR_BareJid).toString());
      
      Menu *subsMenu = new Menu(AMenu);
      subsMenu->setTitle(tr("Subscription"));
      subsMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR);

      Action *action = new Action(subsMenu);
      action->setText(tr("Subscribe contact"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCRIBE);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
      subsMenu->addAction(action,AG_DEFAULT-1);

      action = new Action(subsMenu);
      action->setText(tr("Unsubscribe contact"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_UNSUBSCRIBE);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onContactSubscription(bool)));
      subsMenu->addAction(action,AG_DEFAULT-1);

      action = new Action(subsMenu);
      action->setText(tr("Send"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_SEND);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Subscribed);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Request"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REQUEST);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Subscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Remove"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REMOVE);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribed);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      action = new Action(subsMenu);
      action->setText(tr("Refuse"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_SUBSCR_REFUSE);
      action->setData(data);
      action->setData(ADR_SUBSCRIPTION,IRoster::Unsubscribe);
      connect(action,SIGNAL(triggered(bool)),SLOT(onSendSubscription(bool)));
      subsMenu->addAction(action);

      AMenu->addAction(subsMenu->menuAction(),AG_ROSTERCHANGER_ROSTER_SUBSCRIPTION);

      if (ritem.isValid)
      {
        QSet<QString> exceptGroups = ritem.groups;

        data.insert(ADR_NICK,AIndex->data(RDR_Name));
        data.insert(ADR_GROUP,AIndex->data(RDR_Group));

        action = new Action(AMenu);
        action->setText(tr("Rename..."));
        action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
        action->setData(data);
        connect(action,SIGNAL(triggered(bool)),SLOT(onRenameItem(bool)));
        AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

        if (itemType == RIT_Contact)
        {
          Menu *copyItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onCopyItemToGroup(bool)),AMenu);
          copyItem->setTitle(tr("Copy to group"));
          copyItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
          AMenu->addAction(copyItem->menuAction(),AG_ROSTERCHANGER_ROSTER);

          Menu *moveItem = createGroupMenu(data,exceptGroups,true,false,SLOT(onMoveItemToGroup(bool)),AMenu);
          moveItem->setTitle(tr("Move to group"));
          moveItem->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
          AMenu->addAction(moveItem->menuAction(),AG_ROSTERCHANGER_ROSTER);
        }

        if (!AIndex->data(RDR_Group).toString().isEmpty())
        {
          action = new Action(AMenu);
          action->setText(tr("Remove from group"));
          action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_FROM_GROUP);
          action->setData(data);
          connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromGroup(bool)));
          AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
        }
      }
      else
      {
        action = new Action(AMenu);
        action->setText(tr("Add contact..."));
        action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
        action->setData(ADR_STREAM_JID,streamJid);
        action->setData(ADR_CONTACT_JID,AIndex->data(RDR_Jid));
        connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
        AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);
      }

      action = new Action(AMenu);
      action->setText(tr("Remove from roster"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACT);
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveItemFromRoster(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
    }
    else if (itemType == RIT_Group)
    {
      QHash<int,QVariant> data;
      data.insert(ADR_STREAM_JID,streamJid);
      data.insert(ADR_GROUP,AIndex->data(RDR_Group));
      
      Action *action = new Action(AMenu);
      action->setText(tr("Add contact"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER_ADD_CONTACT,true);

      action = new Action(AMenu);
      action->setText(tr("Rename..."));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_RENAME);
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRenameGroup(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

      QSet<QString> exceptGroups;
      exceptGroups << AIndex->data(RDR_Group).toString();

      Menu *copyGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onCopyGroupToGroup(bool)),AMenu);
      copyGroup->setTitle(tr("Copy to group"));
      copyGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_COPY_GROUP);
      AMenu->addAction(copyGroup->menuAction(),AG_ROSTERCHANGER_ROSTER);

      Menu *moveGroup = createGroupMenu(data,exceptGroups,true,true,SLOT(onMoveGroupToGroup(bool)),AMenu);
      moveGroup->setTitle(tr("Move to group"));
      moveGroup->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_MOVE_GROUP);
      AMenu->addAction(moveGroup->menuAction(),AG_ROSTERCHANGER_ROSTER);

      action = new Action(AMenu);
      action->setText(tr("Remove group"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_GROUP);
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroup(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);

      action = new Action(AMenu);
      action->setText(tr("Remove contacts"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_REMOVE_CONTACTS);
      action->setData(data);
      connect(action,SIGNAL(triggered(bool)),SLOT(onRemoveGroupItems(bool)));
      AMenu->addAction(action,AG_ROSTERCHANGER_ROSTER);
    }
  }
}

void RosterChanger::onContactSubscription(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString contactJid = action->data(ADR_CONTACT_JID).toString();
      int subsType = action->data(ADR_SUBSCRIPTION).toInt();
      if (subsType == IRoster::Subscribe)
        subscribeContact(streamJid,contactJid);
      else if (subsType == IRoster::Unsubscribe)
        unsubscribeContact(streamJid,contactJid);
    }
  }
}

void RosterChanger::onSendSubscription(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString rosterJid = action->data(ADR_CONTACT_JID).toString();
      int subsType = action->data(ADR_SUBSCRIPTION).toInt();
      roster->sendSubscription(rosterJid,subsType);
    }
  }
}

void RosterChanger::onReceiveSubscription(IRoster *ARoster, const Jid &AContactJid, int ASubsType, const QString &AMessage)
{
  INotification notify;
  if (FNotifications)
  {
    notify.kinds = INotifications::EnablePopupWindows;
    notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_RCHANGER_NOTIFY));
    notify.data.insert(NDR_TOOLTIP,tr("Subscription message from %1").arg(FNotifications->contactName(ARoster->streamJid(),AContactJid)));
    notify.data.insert(NDR_ROSTER_STREAM_JID,ARoster->streamJid().full());
    notify.data.insert(NDR_ROSTER_CONTACT_JID,AContactJid.full());
    notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_SUBSCRIBTION);
    notify.data.insert(NDR_WINDOW_CAPTION, tr("Subscription message"));
    notify.data.insert(NDR_WINDOW_TITLE,FNotifications->contactName(ARoster->streamJid(),AContactJid));
    notify.data.insert(NDR_WINDOW_IMAGE, FNotifications->contactAvatar(AContactJid));
    notify.data.insert(NDR_WINDOW_TEXT,subscriptionNotify(ASubsType,AContactJid));
    notify.data.insert(NDR_SOUND_FILE,SDF_RCHANGER_SUBSCRIPTION);
  }

  const IRosterItem &ritem = ARoster->rosterItem(AContactJid);
  if (ASubsType == IRoster::Subscribe)
  {
    bool autoSubscribe = isAutoSubscribe(ARoster->streamJid(),AContactJid);
    if (!autoSubscribe && ritem.subscription!=SUBSCRIPTION_FROM && ritem.subscription!=SUBSCRIPTION_BOTH)
    {
      SubscriptionDialog *dialog = createSubscriptionDialog(ARoster->streamJid(),AContactJid,subscriptionNotify(ASubsType,AContactJid),AMessage);
      if (dialog!=NULL && !FSubscrDialogs.contains(dialog))
      {
        if (FNotifications)
        {
          notify.kinds = FNotifications->notificatorKinds(NOTIFICATOR_ID);
          int notifyId = FNotifications->appendNotification(notify);
          FSubscrDialogs.insert(dialog,notifyId);
        }
        else
        {
          dialog->instance()->show();
          FSubscrDialogs.insert(dialog,-1);
        }
      }
    }
    else
    {
      ARoster->sendSubscription(AContactJid,IRoster::Subscribed);
      if (autoSubscribe && ritem.subscription!=SUBSCRIPTION_TO && ritem.subscription!=SUBSCRIPTION_BOTH)
        ARoster->sendSubscription(AContactJid,IRoster::Subscribe);
    }
  }
  else if (ASubsType == IRoster::Unsubscribed)
  {
    if (FNotifications && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
      FNotifications->appendNotification(notify);
    
    if (isAutoUnsubscribe(ARoster->streamJid(),AContactJid))
      ARoster->sendSubscription(AContactJid,IRoster::Unsubscribed);
  }
  else  if (ASubsType == IRoster::Subscribed)
  {
    if (FNotifications && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
      FNotifications->appendNotification(notify);
  }
  else if (ASubsType == IRoster::Unsubscribe)
  {
    if (FNotifications && !isSilentSubsctiption(ARoster->streamJid(),AContactJid))
      FNotifications->appendNotification(notify);
  }
}

void RosterChanger::onRenameItem(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      Jid rosterJid = action->data(ADR_CONTACT_JID).toString();
      QString oldName = action->data(ADR_NICK).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString rosterJid = action->data(ADR_CONTACT_JID).toString();
      QString groupName = action->data(ADR_TO_GROUP).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString rosterJid = action->data(ADR_CONTACT_JID).toString();
      QString groupName = action->data(ADR_GROUP).toString();
      QString moveToGroup = action->data(ADR_TO_GROUP).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString rosterJid = action->data(ADR_CONTACT_JID).toString();
      QString groupName = action->data(ADR_GROUP).toString();
      roster->removeItemFromGroup(rosterJid,groupName);
    }
  }
}

void RosterChanger::onRemoveItemFromRoster(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      Jid rosterJid = action->data(ADR_CONTACT_JID).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      bool ok = false;
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(ADR_GROUP).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(ADR_GROUP).toString();
      QString copyToGroup = action->data(ADR_TO_GROUP).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupDelim = roster->groupDelimiter();
      QString groupName = action->data(ADR_GROUP).toString();
      QString moveToGroup = action->data(ADR_TO_GROUP).toString();
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
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupName = action->data(ADR_GROUP).toString();
      roster->removeGroup(groupName);
    }
  }
}

void RosterChanger::onRemoveGroupItems(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString streamJid = action->data(ADR_STREAM_JID).toString();
    IRoster *roster = FRosterPlugin->getRoster(streamJid);
    if (roster && roster->isOpen())
    {
      QString groupName = action->data(ADR_GROUP).toString();
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

void RosterChanger::onRosterOpened(IRoster *ARoster)
{
  if (FAddContactMenu && !FActions.contains(ARoster))
  {
    IAccount *account = FAccountManager!= NULL ? FAccountManager->accountByStream(ARoster->streamJid()) : NULL;

    Action *action = new Action(FAddContactMenu);
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
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
  FAutoSubscriptions.remove(ARoster->streamJid());
}

void RosterChanger::onNotificationActivated(int ANotifyId)
{
  SubscriptionDialog *dialog = FSubscrDialogs.key(ANotifyId);
  if (dialog)
    dialog->show();
  FNotifications->removeNotification(ANotifyId);
}

void RosterChanger::onSubscriptionDialogDestroyed()
{
  SubscriptionDialog *dialog = static_cast<SubscriptionDialog *>(sender());
  if (dialog)
  {
    int notifyId = FSubscrDialogs.take(dialog);
    FNotifications->removeNotification(notifyId);
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
      action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAMJID));
      action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REALJID));
      action->setData(ADR_NICK,AUser->data(MUDR_NICK_NAME));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_RCHANGER_ADD_CONTACT);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowAddContactDialog(bool)));
      AMenu->addAction(action,AG_MUCM_ROSTERCHANGER,true);
    }
  }
}

Q_EXPORT_PLUGIN2(RosterChangerPlugin, RosterChanger)

