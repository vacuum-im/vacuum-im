#include "multiuserchatwindow.h"

#include <QTimer>
#include <QInputDialog>
#include <QContextMenuEvent>

#define BDI_WINDOW_GEOMETRY         "MultiChatWindowGeometry"
#define BDI_WINDOW_HSPLITTER        "MultiChatWindowHSplitterState"
#define BDI_WINDOW_VSPLITTER        "MultiChatWindowVSplitterState"

#define ADR_STREAM_JID              Action::DR_StreamJid
#define ADR_ROOM_JID                Action::DR_Parametr1
#define ADR_USER_JID                Action::DR_Parametr2
#define ADR_USER_REAL_JID           Action::DR_Parametr3
#define ADR_USER_NICK               Action::DR_Parametr4

#define NICK_MENU_KEY               Qt::ControlModifier+Qt::Key_Space

MultiUserChatWindow::MultiUserChatWindow(IMultiUserChatPlugin *AChatPlugin, IMultiUserChat *AMultiChat)
{
  ui.setupUi(this);
  ui.lblRoom->setText(AMultiChat->roomJid().hFull());
  ui.lblNick->setText(AMultiChat->nickName());
  ui.ltwUsers->installEventFilter(this);

  FSettings = NULL;
  FStatusIcons = NULL;
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;

  FMultiChat = AMultiChat;
  FChatPlugin = AChatPlugin;
  FMultiChat->instance()->setParent(this);

  FViewWidget = NULL;
  FEditWidget = NULL;
  FToolBarWidget = NULL;
  FSplitterLoaded = false;
  FDestroyOnChatClosed = false;

  initialize();
  connectMultiChat();
  createMenuBarActions();
  createRoomUtilsActions();

  connect(ui.ltwUsers,SIGNAL(itemActivated(QListWidgetItem *)),SLOT(onListItemActivated(QListWidgetItem *)));
  connect(this,SIGNAL(windowActivated()),SLOT(onWindowActivated()));
}

MultiUserChatWindow::~MultiUserChatWindow()
{
  if (FMultiChat->isOpen())
    FMultiChat->setPresence(IPresence::Offline,"");

  QList<IChatWindow *> chatWindows = FChatWindows;
  foreach(IChatWindow *window,chatWindows)
    delete window->instance();

  if (FMessageProcessor)
    FMessageProcessor->removeMessageHandler(this,MHO_MULTIUSERCHAT);

  emit windowDestroyed();
}

void MultiUserChatWindow::showWindow()
{
  if (isWindow() && !isVisible() && FMessageWidgets && FMessageWidgets->checkOption(IMessageWidgets::UseTabWindow))
  {
    ITabWindow *tabWindow = FMessageWidgets->openTabWindow();
    tabWindow->addWidget(this);
  }
  if (isWindow())
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
  else
    emit windowShow();
}

void MultiUserChatWindow::closeWindow()
{
  if (isWindow())
    close();
  else
    emit windowClose();
}

bool MultiUserChatWindow::checkMessage(const Message &AMessage)
{
  return (streamJid() == AMessage.to()) && (roomJid() && AMessage.from());
}

void MultiUserChatWindow::receiveMessage(int AMessageId)
{
  Message message = FMessageProcessor->messageById(AMessageId);
  Jid contactJid = message.from();

  if (message.type() == Message::Error)
  {
    FMessageProcessor->removeMessage(AMessageId);
  }
  else if (contactJid.resource().isEmpty() && !message.stanza().firstElement("x",NS_JABBER_DATA).isNull())
  {
    IDataForm form = FDataForms->dataForm(message.stanza().firstElement("x",NS_JABBER_DATA));
    IDataDialogWidget *dialog = FDataForms->dialogWidget(form,this);
    connect(dialog->instance(),SIGNAL(accepted()),SLOT(onDataFormMessageDialogAccepted()));
    showServiceMessage(tr("Data form received: %1").arg(form.title));
    FDataFormMessages.insert(AMessageId,dialog);
  }
  else if (message.type() == Message::GroupChat)
  {
    if (!isActive())
    {
      FActiveMessages.append(AMessageId);
      updateWindow();
    }
    else
      FMessageProcessor->removeMessage(AMessageId);
  }
  else
  {
    IChatWindow *window = getChatWindow(contactJid);
    if (window)
    {
      if (!window->isActive())
      {
        FActiveChatMessages.insertMulti(window, AMessageId);
        updateChatWindow(window);
        updateListItem(contactJid);
      }
      else
        FMessageProcessor->removeMessage(AMessageId);
    }
  }
}

void MultiUserChatWindow::showMessage(int AMessageId)
{
  if (FDataFormMessages.contains(AMessageId))
  {
    IDataDialogWidget *dialog = FDataFormMessages.take(AMessageId);
    if(dialog)
    {
      dialog->instance()->show();
      FMessageProcessor->removeMessage(AMessageId);
    }
  }
  else
  {
    Message message = FMessageProcessor->messageById(AMessageId);
    openWindow(message.to(),message.from(),message.type());
  }
}

bool MultiUserChatWindow::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType)
{
  if ((streamJid() == AStreamJid) && (roomJid() && AContactJid))
  {
    if (AType == Message::GroupChat)
    {
      showWindow();
    }
    else
    {
      openChatWindow(AContactJid);
    }
    return true;
  }
  return false;
}

INotification MultiUserChatWindow::notification(INotifications *ANotifications, const Message &AMessage)
{
  INotification notify;
  Jid contactJid = AMessage.from();
  if (AMessage.type() != Message::Error)
  {
    IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
    if (!contactJid.resource().isEmpty())
    {
      if (AMessage.type() == Message::GroupChat)
      {
        if (!isActive())
        {
          notify.kinds = ANotifications->notificatorKinds(GROUP_NOTIFICATOR_ID);
          notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_MESSAGE));
          notify.data.insert(NDR_TOOLTIP,tr("New message in conference: %1").arg(contactJid.node()));
          notify.data.insert(NDR_WINDOW_CAPTION,tr("Conference message"));
          notify.data.insert(NDR_WINDOW_TITLE,tr("%1 from %2").arg(contactJid.resource()).arg(contactJid.node()));
          notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
          notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);
        }
      }
      else
      {
        IChatWindow *window = findChatWindow(AMessage.from());
        if (window == NULL || !window->isActive())
        {
          notify.kinds = ANotifications->notificatorKinds(PRIVATE_NOTIFICATOR_ID);
          notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_PRIVATE_MESSAGE));
          notify.data.insert(NDR_TOOLTIP,tr("Private message from: [%1]").arg(contactJid.resource()));
          notify.data.insert(NDR_WINDOW_CAPTION,tr("Private message"));
          notify.data.insert(NDR_WINDOW_TITLE,contactJid.resource());
          notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
          notify.data.insert(NDR_SOUND_FILE,SDF_MUC_PRIVATE_MESSAGE);
        }
      }
    }
    else
    {
      if (!AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
      {
        notify.kinds = ANotifications->notificatorKinds(PRIVATE_NOTIFICATOR_ID);;
        notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_DATA_MESSAGE));
        notify.data.insert(NDR_TOOLTIP,tr("Data form received from: %1").arg(contactJid.node()));
        notify.data.insert(NDR_WINDOW_CAPTION,tr("Data form received"));
        notify.data.insert(NDR_WINDOW_TITLE,contactJid.full());
        notify.data.insert(NDR_WINDOW_TEXT,AMessage.stanza().firstElement("x",NS_JABBER_DATA).firstChildElement("instructions").text());
        notify.data.insert(NDR_SOUND_FILE,SDF_MUC_DATA_MESSAGE);
      }
    }
  }
  return notify;
}

IChatWindow *MultiUserChatWindow::openChatWindow(const Jid &AContactJid)
{
  IChatWindow *window = getChatWindow(AContactJid);
  if (window)
    showChatWindow(window);
  return window;
}

IChatWindow *MultiUserChatWindow::findChatWindow(const Jid &AContactJid) const
{
  foreach(IChatWindow *window,FChatWindows)
    if (window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void MultiUserChatWindow::exitAndDestroy(const QString &AStatus, int AWaitClose)
{
  closeWindow();
  FDestroyOnChatClosed = true;

  if (FMultiChat->isOpen())
    FMultiChat->setPresence(IPresence::Offline,AStatus);

  if (FMultiChat->isOpen() && AWaitClose>0)
    QTimer::singleShot(AWaitClose,this,SLOT(deleteLater()));
  else if (!FMultiChat->isOpen() || AWaitClose==0)
    delete this;
}

void MultiUserChatWindow::initialize()
{
  IPlugin *plugin = FChatPlugin->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
    }
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    IAccountManager *accountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (accountManager)
    {
      IAccount *account = accountManager->accountByStream(streamJid());
      if (account)
      {
        ui.lblAccount->setText(account->name());
        connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          SLOT(onAccountChanged(const QString &, const QVariant &)));
      }
    }
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
  {
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("IMessageWidgets").value(0,NULL);
  if (plugin)
  {
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
    if (FMessageWidgets)
    {
      createMessageWidgets();
      connect(FMessageWidgets->instance(),SIGNAL(defaultChatFontChanged(const QFont &)), SLOT(onDefaultChatFontChanged(const QFont &)));
    }
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("IMessageProcessor").value(0,NULL);
  if (plugin)
  {
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
    if (FMessageProcessor)
    {
      FMessageProcessor->insertMessageHandler(this,MHO_MULTIUSERCHAT);
    }
  }
}

void MultiUserChatWindow::connectMultiChat()
{
  connect(FMultiChat->instance(),SIGNAL(chatOpened()),SLOT(onChatOpened()));
  connect(FMultiChat->instance(),SIGNAL(chatNotify(const QString &, const QString &)),
    SLOT(onChatNotify(const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(chatError(const QString &, const QString &)),
    SLOT(onChatError(const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(chatClosed()),SLOT(onChatClosed()));
  connect(FMultiChat->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),
    SLOT(onStreamJidChanged(const Jid &, const Jid &)));
  connect(FMultiChat->instance(),SIGNAL(userPresence(IMultiUser *,int,const QString &)),
    SLOT(onUserPresence(IMultiUser *,int,const QString &)));
  connect(FMultiChat->instance(),SIGNAL(userDataChanged(IMultiUser *, int, const QVariant, const QVariant &)),
    SLOT(onUserDataChanged(IMultiUser *, int, const QVariant, const QVariant &)));
  connect(FMultiChat->instance(),SIGNAL(userNickChanged(IMultiUser *, const QString &, const QString &)),
    SLOT(onUserNickChanged(IMultiUser *, const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(presenceChanged(int, const QString &)),
    SLOT(onPresenceChanged(int, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(subjectChanged(const QString &, const QString &)),
    SLOT(onSubjectChanged(const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(serviceMessageReceived(const Message &)),
    SLOT(onServiceMessageReceived(const Message &)));
  connect(FMultiChat->instance(),SIGNAL(messageReceived(const QString &, const Message &)),
    SLOT(onMessageReceived(const QString &, const Message &)));
  connect(FMultiChat->instance(),SIGNAL(inviteDeclined(const Jid &, const QString &)),
    SLOT(onInviteDeclined(const Jid &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(userKicked(const QString &, const QString &, const QString &)),
    SLOT(onUserKicked(const QString &, const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(userBanned(const QString &, const QString &, const QString &)),
    SLOT(onUserBanned(const QString &, const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(affiliationListReceived(const QString &,const QList<IMultiUserListItem> &)),
    SLOT(onAffiliationListReceived(const QString &,const QList<IMultiUserListItem> &)));
  connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)), SLOT(onConfigFormReceived(const IDataForm &)));
  connect(FMultiChat->instance(),SIGNAL(roomDestroyed(const QString &)), SLOT(onRoomDestroyed(const QString &)));
}

void MultiUserChatWindow::createMessageWidgets()
{
  FViewWidget = FMessageWidgets->newViewWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
  FViewWidget->setShowKind(IViewWidget::GroupChatMessage);
  FViewWidget->document()->setDefaultFont(FMessageWidgets->defaultChatFont());
  ui.wdtView->setLayout(new QVBoxLayout);
  ui.wdtView->layout()->addWidget(FViewWidget->instance());
  ui.wdtView->layout()->setMargin(0);
  ui.wdtView->layout()->setSpacing(0);

  FEditWidget = FMessageWidgets->newEditWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
  FEditWidget->document()->setDefaultFont(FMessageWidgets->defaultChatFont());
  ui.wdtEdit->setLayout(new QVBoxLayout);
  ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
  ui.wdtEdit->layout()->setMargin(0);
  ui.wdtEdit->layout()->setSpacing(0);
  connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageSend()));
  connect(FEditWidget->instance(),SIGNAL(messageAboutToBeSend()),SLOT(onMessageAboutToBeSend()));
  connect(FEditWidget->instance(),SIGNAL(keyEventReceived(QKeyEvent *,bool &)),SLOT(onEditWidgetKeyEvent(QKeyEvent *,bool &)));

  FToolBarWidget = FMessageWidgets->newToolBarWidget(NULL,FViewWidget,FEditWidget,NULL);
  ui.wdtToolBar->setLayout(new QVBoxLayout);
  ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());
  ui.wdtToolBar->layout()->setMargin(0);
  ui.wdtToolBar->layout()->setSpacing(0);
}

void MultiUserChatWindow::createMenuBarActions()
{
  FRoomMenu = new Menu(this);
  FRoomMenu->setTitle(tr("Conference"));
  ui.mnbMenuBar->addMenu(FRoomMenu);

  FChangeNick = new Action(FRoomMenu);
  FChangeNick->setText(tr("Change room nick"));
  FChangeNick->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_NICK);
  connect(FChangeNick,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FRoomMenu->addAction(FChangeNick,AG_MURM_MULTIUSERCHAT,true);

  FChangeSubject = new Action(FRoomMenu);
  FChangeSubject->setText(tr("Change topic"));
  FChangeSubject->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_TOPIC);
  connect(FChangeSubject,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FRoomMenu->addAction(FChangeSubject,AG_MURM_MULTIUSERCHAT,true);

  FClearChat = new Action(FRoomMenu);
  FClearChat->setText(tr("Clear chat window"));
  FClearChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
  connect(FClearChat,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FRoomMenu->addAction(FClearChat,AG_MURM_MULTIUSERCHAT,true);

  FQuitRoom = new Action(FRoomMenu);
  FQuitRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT);
  FQuitRoom->setText(tr("Exit"));
  FQuitRoom->setShortcut(tr("Ctrl+X"));
  connect(FQuitRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FRoomMenu->addAction(FQuitRoom,AG_MURM_MULTIUSERCHAT_EXIT,true);

  FToolsMenu = new Menu(this);
  FToolsMenu->setTitle(tr("Tools"));
  ui.mnbMenuBar->addMenu(FToolsMenu);

  FInviteContact = new Action(FToolsMenu);
  FInviteContact->setText(tr("Invite to this room"));
  FInviteContact->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
  connect(FInviteContact,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FInviteContact,AG_MUTM_MULTIUSERCHAT,false);

  FRequestVoice = new Action(FToolsMenu);
  FRequestVoice->setText(tr("Request voice"));
  FRequestVoice->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_REQUEST_VOICE);
  connect(FRequestVoice,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FRequestVoice,AG_MUTM_MULTIUSERCHAT,false);

  FBanList = new Action(FToolsMenu);
  FBanList->setText(tr("Edit ban list"));
  FBanList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_BAN_LIST);
  connect(FBanList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FBanList,AG_MUTM_MULTIUSERCHAT,false);

  FMembersList = new Action(FToolsMenu);
  FMembersList->setText(tr("Edit members list"));
  FMembersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_MEMBERS_LIST);
  connect(FMembersList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FMembersList,AG_MUTM_MULTIUSERCHAT,false);

  FAdminsList = new Action(FToolsMenu);
  FAdminsList->setText(tr("Edit administrators list"));
  FAdminsList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_ADMINS_LIST);
  connect(FAdminsList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FAdminsList,AG_MUTM_MULTIUSERCHAT,false);

  FOwnersList = new Action(FToolsMenu);
  FOwnersList->setText(tr("Edit owners list"));
  FOwnersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_OWNERS_LIST);
  connect(FOwnersList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FOwnersList,AG_MUTM_MULTIUSERCHAT,false);

  FConfigRoom = new Action(FToolsMenu);
  FConfigRoom->setText(tr("Configure room"));
  FConfigRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFIGURE_ROOM);
  connect(FConfigRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FConfigRoom,AG_MUTM_MULTIUSERCHAT,false);

  FDestroyRoom = new Action(FToolsMenu);
  FDestroyRoom->setText(tr("Destroy room"));
  FDestroyRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_DESTROY_ROOM);
  connect(FDestroyRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FDestroyRoom,AG_MUTM_MULTIUSERCHAT_DESTROY,false);
}

void MultiUserChatWindow::updateMenuBarActions()
{
  QString role = FMultiChat->isOpen() ? FMultiChat->mainUser()->role() : MUC_ROLE_NONE;
  QString affiliation = FMultiChat->isOpen() ? FMultiChat->mainUser()->affiliation() : MUC_AFFIL_NONE;
  if (affiliation == MUC_AFFIL_OWNER)
  {
    FChangeSubject->setVisible(true);
    FInviteContact->setVisible(true);
    FRequestVoice->setVisible(false);
    FBanList->setVisible(true);
    FMembersList->setVisible(true);
    FAdminsList->setVisible(true);
    FOwnersList->setVisible(true);
    FConfigRoom->setVisible(true);
    FDestroyRoom->setVisible(true);
  }
  else if (affiliation == MUC_AFFIL_ADMIN)
  {
    FChangeSubject->setVisible(true);
    FInviteContact->setVisible(true);
    FRequestVoice->setVisible(false);
    FBanList->setVisible(true);
    FMembersList->setVisible(true);
    FAdminsList->setVisible(true);
    FOwnersList->setVisible(true);
    FConfigRoom->setVisible(false);
    FDestroyRoom->setVisible(false);
  }
  else if (role == MUC_ROLE_VISITOR)
  {
    FChangeSubject->setVisible(false);
    FInviteContact->setVisible(false);
    FRequestVoice->setVisible(true);
    FBanList->setVisible(false);
    FMembersList->setVisible(false);
    FAdminsList->setVisible(false);
    FOwnersList->setVisible(false);
    FConfigRoom->setVisible(false);
    FDestroyRoom->setVisible(false);
  }
  else
  {
    FChangeSubject->setVisible(true);
    FInviteContact->setVisible(true);
    FRequestVoice->setVisible(false);
    FBanList->setVisible(false);
    FMembersList->setVisible(false);
    FAdminsList->setVisible(false);
    FOwnersList->setVisible(false);
    FConfigRoom->setVisible(false);
    FDestroyRoom->setVisible(false);
  }
}

void MultiUserChatWindow::createRoomUtilsActions()
{
  FRoomUtilsMenu = new Menu(this);
  FRoomUtilsMenu->setTitle(tr("Room Utilities"));

  FInviteMenu = new Menu(FRoomUtilsMenu);
  FInviteMenu->setTitle(tr("Invite to"));
  FInviteMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
  FRoomUtilsMenu->addAction(FInviteMenu->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);

  FSetRoleNode = new Action(FRoomUtilsMenu);
  FSetRoleNode->setText(tr("Kick user"));
  connect(FSetRoleNode,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
  FRoomUtilsMenu->addAction(FSetRoleNode,AG_MULTIUSERCHAT_ROOM_UTILS,false);

  FSetAffilOutcast = new Action(FRoomUtilsMenu);
  FSetAffilOutcast->setText(tr("Ban user"));
  connect(FSetAffilOutcast,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
  FRoomUtilsMenu->addAction(FSetAffilOutcast,AG_MULTIUSERCHAT_ROOM_UTILS,false);

  FChangeRole = new Menu(FRoomUtilsMenu);
  FChangeRole->setTitle(tr("Change Role"));
  {
    FSetRoleVisitor = new Action(FChangeRole);
    FSetRoleVisitor->setCheckable(true);
    FSetRoleVisitor->setText(tr("Visitor"));
    connect(FSetRoleVisitor,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeRole->addAction(FSetRoleVisitor,AG_DEFAULT,false);

    FSetRoleParticipant = new Action(FChangeRole);
    FSetRoleParticipant->setCheckable(true);
    FSetRoleParticipant->setText(tr("Participant"));
    connect(FSetRoleParticipant,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeRole->addAction(FSetRoleParticipant,AG_DEFAULT,false);

    FSetRoleModerator = new Action(FChangeRole);
    FSetRoleModerator->setCheckable(true);
    FSetRoleModerator->setText(tr("Moderator"));
    connect(FSetRoleModerator,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeRole->addAction(FSetRoleModerator,AG_DEFAULT,false);
  }
  FRoomUtilsMenu->addAction(FChangeRole->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);

  FChangeAffiliation = new Menu(FRoomUtilsMenu);
  FChangeAffiliation->setTitle(tr("Change Affiliation"));
  {
    FSetAffilNone = new Action(FChangeAffiliation);
    FSetAffilNone->setCheckable(true);
    FSetAffilNone->setText(tr("None"));
    connect(FSetAffilNone,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeAffiliation->addAction(FSetAffilNone,AG_DEFAULT,false);

    FSetAffilMember = new Action(FChangeAffiliation);
    FSetAffilMember->setCheckable(true);
    FSetAffilMember->setText(tr("Member"));
    connect(FSetAffilMember,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeAffiliation->addAction(FSetAffilMember,AG_DEFAULT,false);

    FSetAffilAdmin = new Action(FChangeAffiliation);
    FSetAffilAdmin->setCheckable(true);
    FSetAffilAdmin->setText(tr("Administrator"));
    connect(FSetAffilAdmin,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeAffiliation->addAction(FSetAffilAdmin,AG_DEFAULT,false);

    FSetAffilOwner = new Action(FChangeAffiliation);
    FSetAffilOwner->setCheckable(true);
    FSetAffilOwner->setText(tr("Owner"));
    connect(FSetAffilOwner,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
    FChangeAffiliation->addAction(FSetAffilOwner,AG_DEFAULT,false);
  }
  FRoomUtilsMenu->addAction(FChangeAffiliation->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);
}

void MultiUserChatWindow::insertRoomUtilsActions(Menu *AMenu, IMultiUser *AUser)
{
  IMultiUser *muser = FMultiChat->mainUser();
  if (muser && muser->role()!=MUC_ROLE_VISITOR)
  {
    FInviteMenu->clear();
    QList<IMultiUserChatWindow *> windows = FChatPlugin->multiChatWindows();
    foreach(IMultiUserChatWindow *window,windows)
    {
      if ( window!=this && window->multiUserChat()->isOpen() &&
          (!AUser->data(MUDR_REAL_JID).toString().isEmpty() || window->roomJid().domain()==roomJid().domain()) )
      {
        Action *action = new Action(FInviteMenu);
        action->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_MESSAGE);
        action->setText(tr("%1 from %2").arg(window->roomJid().full()).arg(window->multiUserChat()->nickName()));
        action->setData(ADR_STREAM_JID,window->streamJid().full());
        action->setData(ADR_ROOM_JID,window->roomJid().full());
        if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
          action->setData(ADR_USER_JID,AUser->data(MUDR_REAL_JID));
        else
          action->setData(ADR_USER_JID,AUser->contactJid().full());
        connect(action,SIGNAL(triggered(bool)),SLOT(onInviteActionTriggered(bool)));
        FInviteMenu->addAction(action,AG_MULTIUSERCHAT_ROOM_UTILS,false);
      }
    }
    if (!FInviteMenu->isEmpty())
      AMenu->addAction(FInviteMenu->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);
  }
  if (muser && muser->role() == MUC_ROLE_MODERATOR)
  {
    FRoomUtilsMenu->menuAction()->setData(ADR_USER_NICK,AUser->nickName());

    FSetRoleVisitor->setChecked(AUser->role() == MUC_ROLE_VISITOR);
    FSetRoleParticipant->setChecked(AUser->role() == MUC_ROLE_PARTICIPANT);
    FSetRoleModerator->setChecked(AUser->role() == MUC_ROLE_MODERATOR);

    FSetAffilNone->setChecked(AUser->affiliation() == MUC_AFFIL_NONE);
    FSetAffilMember->setChecked(AUser->affiliation() == MUC_AFFIL_MEMBER);
    FSetAffilAdmin->setChecked(AUser->affiliation() == MUC_AFFIL_ADMIN);
    FSetAffilOwner->setChecked(AUser->affiliation() == MUC_AFFIL_OWNER);

    AMenu->addAction(FSetRoleNode,AG_MULTIUSERCHAT_ROOM_UTILS,false);
    AMenu->addAction(FSetAffilOutcast,AG_MULTIUSERCHAT_ROOM_UTILS,false);
    AMenu->addAction(FChangeRole->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);
    AMenu->addAction(FChangeAffiliation->menuAction(),AG_MULTIUSERCHAT_ROOM_UTILS,false);
  }
}

void MultiUserChatWindow::saveWindowState()
{
  if (FSettings && FMessageWidgets)
  {
    QString dataId = roomJid().pBare();
    if (isWindow() && isVisible())
      FSettings->saveBinaryData(BDI_WINDOW_GEOMETRY+dataId,saveGeometry());
    FSettings->saveBinaryData(BDI_WINDOW_HSPLITTER+dataId,ui.sprHSplitter->saveState());
    FSettings->saveBinaryData(BDI_WINDOW_VSPLITTER+dataId,ui.sprVSplitter->saveState());
  }
}

void MultiUserChatWindow::loadWindowState()
{
  if (FSettings && FMessageWidgets)
  {
    QString dataId = roomJid().pBare();
    if (isWindow())
      restoreGeometry(FSettings->loadBinaryData(BDI_WINDOW_GEOMETRY+dataId));
    if (!FSplitterLoaded)
    {
      ui.sprHSplitter->restoreState(FSettings->loadBinaryData(BDI_WINDOW_HSPLITTER+dataId));
      ui.sprVSplitter->restoreState(FSettings->loadBinaryData(BDI_WINDOW_VSPLITTER+dataId));
      FSplitterLoaded = true;
    }
    FEditWidget->textEdit()->setFocus();
  }
}

bool MultiUserChatWindow::showStatusCodes(const QString &ANick, const QList<int> &ACodes)
{
  if (ACodes.isEmpty())
  {
    return false;
  }

  bool shown = false;
  if (ACodes.contains(MUC_SC_NON_ANONYMOUS))
  {
    showServiceMessage(tr("Any occupant is allowed to see the user's full JID"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_AFFIL_CHANGED))
  {
    showServiceMessage(tr("%1 affiliation changed while not in the room").arg(ANick));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_CONFIG_CHANGED))
  {
    showServiceMessage(tr("Room configuration change has occurred"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NOW_LOGGING_ENABLED))
  {
    showServiceMessage(tr("Room logging is now enabled"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NOW_LOGGING_DISABLED))
  {
    showServiceMessage(tr("Room logging is now disabled"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NOW_NON_ANONYMOUS))
  {
    showServiceMessage(tr("The room is now non-anonymous"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NOW_SEMI_ANONYMOUS))
  {
    showServiceMessage(tr("The room is now semi-anonymous"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NOW_FULLY_ANONYMOUS))
  {
    showServiceMessage(tr("The room is now fully-anonymous"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_ROOM_CREATED))
  {
    showServiceMessage(tr("A new room has been created"));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_NICK_CHANGED))
  {
    shown = true;
  }
  if (ACodes.contains(MUC_SC_USER_BANNED))
  {
    shown = true;
  }
  if (ACodes.contains(MUC_SC_ROOM_ENTER))
  {
    shown = true;
  }
  if (ACodes.contains(MUC_SC_USER_KICKED))
  {
    shown = true;
  }
  if (ACodes.contains(MUC_SC_AFFIL_CHANGE))
  {
    showServiceMessage(tr("%1 has been removed from the room because of an affiliation change").arg(ANick));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_MEMBERS_ONLY))
  {
    showServiceMessage(tr("%1 has been removed from the room because the room has been changed to members-only").arg(ANick));
    shown = true;
  }
  if (ACodes.contains(MUC_SC_SYSTEM_SHUTDOWN))
  {
    showServiceMessage(tr("%1 is being removed from the room because of a system shutdown").arg(ANick));
    shown = true;
  }
  return shown;
}

void MultiUserChatWindow::showServiceMessage(const QString &AMessage)
{
  Message message;
  message.setBody("*** "+AMessage);

  QTextDocument doc;
  doc.setDefaultFont(FViewWidget->document()->defaultFont());
  if (FMessageProcessor)
    FMessageProcessor->messageToText(&doc,message);
  else
    doc.setPlainText(AMessage);

  QTextCursor cursor(&doc);
  cursor.select(QTextCursor::Document);
  QTextCharFormat format;
  format.setForeground(QBrush(Qt::darkGreen));
  cursor.mergeCharFormat(format);

  FViewWidget->showCustomMessage(doc.toHtml(),QDateTime::currentDateTime());
}

void MultiUserChatWindow::setViewColorForUser(IMultiUser *AUser)
{
  if (FColorQueue.isEmpty())
  {
    FColorQueue << Qt::blue << Qt::darkBlue << Qt::darkGreen << Qt::darkCyan << Qt::darkMagenta << Qt::darkYellow
                << Qt::green << Qt::cyan << Qt::magenta << Qt::darkRed;
  }

  QColor color;
  if (AUser != FMultiChat->mainUser())
  {
    if (!FColorLastOwner.values().contains(AUser->nickName()))
    {
      color = FColorQueue.takeFirst();
      FColorQueue.append(color);
    }
    else
      color = FColorLastOwner.key(AUser->nickName());
  }
  else
    color = Qt::red;

  FViewWidget->setColorForJid(AUser->contactJid(),color);
  AUser->setData(MUDR_VIEW_COLOR,color);
  FColorLastOwner.insert(color.name(),AUser->nickName());
}

void MultiUserChatWindow::setRoleColorForUser(IMultiUser *AUser)
{
  QListWidgetItem *listItem = FUsers.value(AUser);
  if (listItem)
  {
    QColor color;
    QString role = AUser->data(MUDR_ROLE).toString();
    if (role == MUC_ROLE_PARTICIPANT)
      color = Qt::black;
    else if (role == MUC_ROLE_MODERATOR)
      color = Qt::darkRed;
    else
      color = Qt::gray;
    listItem->setForeground(color);
    AUser->setData(MUDR_ROLE_COLOR,color);
  }
}

void MultiUserChatWindow::setAffilationLineForUser(IMultiUser *AUser)
{
  QListWidgetItem *listItem = FUsers.value(AUser);
  if (listItem)
  {
    QFont itemFont = listItem->font();
    QString affilation = AUser->data(MUDR_AFFILIATION).toString();
    if (affilation == MUC_AFFIL_OWNER)
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(true);
    }
    else if (affilation == MUC_AFFIL_OUTCAST)
    {
      itemFont.setStrikeOut(true);
      itemFont.setUnderline(false);
    }
    else
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(false);
    }
    listItem->setFont(itemFont);
  }
}

void MultiUserChatWindow::setToolTipForUser(IMultiUser *AUser)
{
  QListWidgetItem *listItem = FUsers.value(AUser);
  if (listItem)
  {
    QString toolTip;
    toolTip += AUser->nickName()+"<br>";
    if (!AUser->data(MUDR_REAL_JID).toString().isEmpty())
      toolTip += AUser->data(MUDR_REAL_JID).toString()+"<br>";
    toolTip += tr("Role: %1<br>").arg(AUser->data(MUDR_ROLE).toString());
    toolTip += tr("Affiliation: %1<br>").arg(AUser->data(MUDR_AFFILIATION).toString());
    toolTip += tr("Status: %1").arg(AUser->data(MUDR_STATUS).toString());
    listItem->setToolTip(toolTip);
  }
}

bool MultiUserChatWindow::execShortcutCommand(const QString &AText)
{
  bool hasCommand = false;
  if (AText.startsWith("/kick "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString nick = parts.takeFirst();
    if (FMultiChat->userByNick(nick))
      FMultiChat->setRole(nick,MUC_ROLE_NONE,parts.join(" "));
    else
      showServiceMessage(tr("User %1 is not present in the conference").arg(nick));
    hasCommand = true;
  }
  else if (AText.startsWith("/ban "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString nick = parts.takeFirst();
    if (FMultiChat->userByNick(nick))
      FMultiChat->setAffiliation(nick,MUC_AFFIL_OUTCAST,parts.join(" "));
    else
      showServiceMessage(tr("User %1 is not present in the conference").arg(nick));
    hasCommand = true;
  }
  else if (AText.startsWith("/invite "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    Jid  userJid = parts.takeFirst();
    if (userJid.isValid())
      FMultiChat->inviteContact(userJid,parts.join(" "));
    else
      showServiceMessage(tr("%1 is not valid contact JID").arg(userJid.full()));
    hasCommand = true;
  }
  else if (AText.startsWith("/join "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString roomName = parts.takeFirst();
    Jid roomJid(roomName,FMultiChat->roomJid().domain(),"");
    if (roomJid.isValid())
    {
      FChatPlugin->showJoinMultiChatDialog(streamJid(),roomJid,FMultiChat->nickName(),parts.join(" "));
    }
    else
      showServiceMessage(tr("%1 is not valid room JID").arg(roomJid.full()));
    hasCommand = true;
  }
  else if (AText.startsWith("/msg "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString nick = parts.takeFirst();
    if (FMultiChat->userByNick(nick))
    {
      Message message;
      message.setBody(parts.join(" "));
      FMultiChat->sendMessage(message,nick);
    }
    else
      showServiceMessage(tr("User %1 is not present in the conference").arg(nick));
    hasCommand = true;
  }
  else if (AText.startsWith("/nick "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString nick = parts.takeFirst();
    FMultiChat->setNickName(nick);
    hasCommand = true;
  }
  else if (AText.startsWith("/part ") || AText.startsWith("/leave ") || AText=="/part" || AText=="/leave")
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString status = parts.join(" ");
    FMultiChat->setPresence(IPresence::Offline,status);
    exitAndDestroy(tr("Disconnected"));
    hasCommand = true;
  }
  else if (AText.startsWith("/topic "))
  {
    QStringList parts = AText.split(" ");
    parts.removeFirst();
    QString subject = parts.join(" ");
    FMultiChat->setSubject(subject);
    hasCommand = true;
  }
  else if (AText == "/help")
  {
    showServiceMessage(tr("Supported list of commands:"));
    showServiceMessage(tr(" /ban <roomnick> [comment]"));
    showServiceMessage(tr(" /invite <jid> [comment]"));
    showServiceMessage(tr(" /join <roomname> [pass]"));
    showServiceMessage(tr(" /kick <roomnick> [comment]"));
    showServiceMessage(tr(" /msg <roomnick> <foo>"));
    showServiceMessage(tr(" /nick <newnick>"));
    showServiceMessage(tr(" /leave [comment]"));
    showServiceMessage(tr(" /topic <foo>"));
    hasCommand = true;
  }
  return hasCommand;
}

void MultiUserChatWindow::updateWindow()
{
  if (FActiveMessages.isEmpty())
    IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_CONFERENCE,0,0,"windowIcon");
  else
    IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_MUC_MESSAGE,0,0,"windowIcon");

  QString roomName = tr("%1 (%2)").arg(FMultiChat->roomJid().node()).arg(FUsers.count());
  setWindowIconText(roomName);
  setWindowTitle(tr("%1 - Conference").arg(roomName));

  emit windowChanged();
}

void MultiUserChatWindow::updateListItem(const Jid &AContactJid)
{
  IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
  QListWidgetItem *listItem = FUsers.value(user);
  if (listItem)
  {
    IChatWindow *window = findChatWindow(AContactJid);
    if (FActiveChatMessages.contains(window))
      listItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_PRIVATE_MESSAGE));
    else if (FStatusIcons)
      listItem->setIcon(FStatusIcons->iconByJidStatus(AContactJid,user->data(MUDR_SHOW).toInt(),"",false));
  }
}

void MultiUserChatWindow::removeActiveMessages()
{
  if (FMessageProcessor)
    foreach(int messageId, FActiveMessages)
      FMessageProcessor->removeMessage(messageId);
  FActiveMessages.clear();
  updateWindow();
}

IChatWindow *MultiUserChatWindow::getChatWindow(const Jid &AContactJid)
{
  IChatWindow *window = NULL;
  IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
  if (user && user != FMultiChat->mainUser())
  {
    window = FMessageWidgets!=NULL ? FMessageWidgets->newChatWindow(streamJid(),AContactJid) : NULL;
    if (window)
    {
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onChatMessageSend()));
      connect(window->instance(),SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
      connect(window->instance(),SIGNAL(windowClosed()),SLOT(onChatWindowClosed()));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
      window->viewWidget()->setNickForJid(user->contactJid(),user->nickName());
      window->viewWidget()->setColorForJid(user->contactJid(),user->data(MUDR_VIEW_COLOR).value<QColor>());
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactName,false);
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactShow,false);
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactStatus,false);
      window->infoWidget()->setField(IInfoWidget::ContactName,user->nickName());
      window->infoWidget()->setField(IInfoWidget::ContactShow,user->data(MUDR_SHOW));
      window->infoWidget()->setField(IInfoWidget::ContactStatus,user->data(MUDR_STATUS));
      window->infoWidget()->autoUpdateFields();
      FChatWindows.append(window);
      updateChatWindow(window);
      emit chatWindowCreated(window);
    }
  }
  return window == NULL ? findChatWindow(AContactJid) : window;
}

void MultiUserChatWindow::showChatWindow(IChatWindow *AWindow)
{
  if (AWindow->instance()->isWindow() && !AWindow->instance()->isVisible() && FMessageWidgets->checkOption(IMessageWidgets::UseTabWindow))
  {
    ITabWindow *tabWindow = FMessageWidgets->openTabWindow();
    tabWindow->addWidget(AWindow);
  }
  AWindow->showWindow();
}

void MultiUserChatWindow::removeActiveChatMessages(IChatWindow *AWindow)
{
  if (FActiveChatMessages.contains(AWindow))
  {
    QList<int> messageIds = FActiveChatMessages.values(AWindow);
    if (FMessageProcessor)
      foreach(int messageId, messageIds)
        FMessageProcessor->removeMessage(messageId);
    FActiveChatMessages.remove(AWindow);
    updateChatWindow(AWindow);
    updateListItem(AWindow->contactJid());
  }
}

void MultiUserChatWindow::updateChatWindow(IChatWindow *AWindow)
{
  QIcon icon;
  if (FActiveChatMessages.contains(AWindow))
    icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_PRIVATE_MESSAGE);
  else if (FStatusIcons)
    icon = FStatusIcons->iconByStatus(AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt(),"both",false);

  QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
  QString tabTitle = QString("[%1]").arg(contactName);
  AWindow->updateWindow(icon,tabTitle,tr("%1 - Private chat").arg(tabTitle));
}

bool MultiUserChatWindow::event(QEvent *AEvent)
{
  if (AEvent->type() == QEvent::WindowActivate)
    emit windowActivated();
  return QMainWindow::event(AEvent);
}

void MultiUserChatWindow::showEvent(QShowEvent *AEvent)
{
  loadWindowState();
  emit windowActivated();
  QMainWindow::showEvent(AEvent);
}

void MultiUserChatWindow::closeEvent(QCloseEvent *AEvent)
{
  saveWindowState();
  emit windowClosed();
  QMainWindow::closeEvent(AEvent);
}

bool MultiUserChatWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AObject == ui.ltwUsers)
  {
    if (AEvent->type() == QEvent::ContextMenu)
    {
      QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(AEvent);
      QListWidgetItem *listItem = ui.ltwUsers->itemAt(menuEvent->pos());
      IMultiUser *user = FUsers.key(listItem,NULL);
      if (user && user != FMultiChat->mainUser())
      {
        Menu *menu = new Menu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose,true);
        insertRoomUtilsActions(menu,user);
        emit multiUserContextMenu(user,menu);
        if (!menu->isEmpty())
          menu->popup(menuEvent->globalPos());
        else
          delete menu;
      }
    }
  }
  return QMainWindow::eventFilter(AObject,AEvent);
}

void MultiUserChatWindow::onChatOpened()
{
  FViewWidget->textBrowser()->clear();
  if (FMultiChat->statusCodes().contains(201))
    FMultiChat->requestConfigForm();
}

void MultiUserChatWindow::onChatNotify(const QString &ANick, const QString &ANotify)
{
  if (ANick.isEmpty())
    showServiceMessage(tr("Notify: %1").arg(ANotify));
  else
    showServiceMessage(tr("Notify from %1: %2").arg(ANick).arg(ANotify));
}

void MultiUserChatWindow::onChatError(const QString &ANick, const QString &AError)
{
  if (ANick.isEmpty())
    showServiceMessage(tr("Error: %1").arg(AError));
  else
    showServiceMessage(tr("Error from %1: %2").arg(ANick).arg(AError));
}

void MultiUserChatWindow::onChatClosed()
{
  FDestroyOnChatClosed ? deleteLater() : showServiceMessage(tr("Disconnected"));
}

void MultiUserChatWindow::onStreamJidChanged(const Jid &/*ABefour*/, const Jid &AAfter)
{
  FViewWidget->setStreamJid(AAfter);
  FEditWidget->setStreamJid(AAfter);
}

void MultiUserChatWindow::onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus)
{
  QListWidgetItem *listItem = FUsers.value(AUser);
  if (AShow!=IPresence::Offline && AShow!=IPresence::Error)
  {
    if (listItem == NULL)
    {
      listItem = new QListWidgetItem(ui.ltwUsers);
      listItem->setText(AUser->nickName());
      ui.ltwUsers->addItem(listItem);
      FUsers.insert(AUser,listItem);
      setViewColorForUser(AUser);
      setRoleColorForUser(AUser);
      setAffilationLineForUser(AUser);
      updateWindow();

      QString message = tr("%1 (%2) has joined the room. %3");
      message = message.arg(AUser->nickName());
      message = message.arg(AUser->data(MUDR_REAL_JID).toString());
      message = message.arg(AStatus);
      showServiceMessage(message);
    }
    showStatusCodes(AUser->nickName(),FMultiChat->statusCodes());
    setToolTipForUser(AUser);
    updateListItem(AUser->contactJid());
  }
  else if (listItem)
  {
    if (!showStatusCodes(AUser->nickName(),FMultiChat->statusCodes()))
    {
      QString message = tr("%1 (%2) has left the room. %3");
      message = message.arg(AUser->nickName());
      message = message.arg(AUser->data(MUDR_REAL_JID).toString());
      message = message.arg(AStatus.isEmpty() ? tr("Disconnected") : AStatus);
      showServiceMessage(message);
    }
    FUsers.remove(AUser);
    ui.ltwUsers->takeItem(ui.ltwUsers->row(listItem));
    delete listItem;
  }

  updateWindow();
  IChatWindow *window = findChatWindow(AUser->contactJid());
  if (window)
  {
    window->infoWidget()->setField(IInfoWidget::ContactShow,AShow);
    window->infoWidget()->setField(IInfoWidget::ContactStatus,AStatus);
    updateChatWindow(window);
  }
}

void MultiUserChatWindow::onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter)
{
  if (ARole == MUDR_ROLE)
  {
    if (AAfter!=MUC_ROLE_NONE && ABefour!=MUC_ROLE_NONE)
      showServiceMessage(tr("%1 role changed from %2 to %3").arg(AUser->nickName()).arg(ABefour.toString()).arg(AAfter.toString()));
    setRoleColorForUser(AUser);
  }
  else if (ARole==MUDR_AFFILIATION)
  {
    if (FUsers.contains(AUser))
      showServiceMessage(tr("%1 affiliation changed from %2 to %3").arg(AUser->nickName()).arg(ABefour.toString()).arg(AAfter.toString()));
    setAffilationLineForUser(AUser);
  }
}

void MultiUserChatWindow::onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick)
{
  QListWidgetItem *listItem = FUsers.value(AUser);
  if (listItem)
  {
    listItem->setText(ANewNick);
    QColor color = AUser->data(MUDR_VIEW_COLOR).value<QColor>();
    FColorLastOwner.insert(color.name(),ANewNick);
    FViewWidget->setColorForJid(AUser->contactJid(),color);

    Jid userOldJid = AUser->contactJid();
    userOldJid.setResource(AOldNick);
    IChatWindow *window = findChatWindow(userOldJid);
    if (window)
    {
      window->setContactJid(AUser->contactJid());
      window->infoWidget()->setField(IInfoWidget::ContactName,ANewNick);
      window->viewWidget()->setNickForJid(AUser->contactJid(),ANewNick);
      window->viewWidget()->setColorForJid(AUser->contactJid(),color);
    }
  }
  if (AUser == FMultiChat->mainUser())
    ui.lblNick->setText(Qt::escape(ANewNick));
  showServiceMessage(tr("%1 changed nick to %2").arg(AOldNick).arg(ANewNick));
}

void MultiUserChatWindow::onPresenceChanged(int /*AShow*/, const QString &/*AStatus*/)
{
  updateMenuBarActions();
}

void MultiUserChatWindow::onSubjectChanged(const QString &ANick, const QString &ASubject)
{
  Message message;
  message.setBody(ANick.isEmpty() ? tr("Subject: %1").arg(ASubject) : tr("%1 has changed the subject to: %2").arg(ANick).arg(ASubject));

  QTextDocument doc;
  doc.setDefaultFont(FViewWidget->document()->defaultFont());

  if (FMessageProcessor)
    FMessageProcessor->messageToText(&doc,message);
  else
    doc.setPlainText(ASubject);

  QTextCursor cursor(&doc);
  cursor.select(QTextCursor::Document);
  QTextCharFormat format;
  format.setForeground(QBrush(Qt::gray));
  cursor.mergeCharFormat(format);

  FViewWidget->showCustomMessage(doc.toHtml(),QDateTime::currentDateTime());
}

void MultiUserChatWindow::onServiceMessageReceived(const Message &AMessage)
{
  if (!showStatusCodes("",FMultiChat->statusCodes()) && !AMessage.body().isEmpty())
    onMessageReceived("",AMessage);
}

void MultiUserChatWindow::onMessageReceived(const QString &ANick, const Message &AMessage)
{
  if (AMessage.type() == Message::GroupChat || ANick.isEmpty())
    FViewWidget->showMessage(AMessage);
  else
  {
    IChatWindow *window = getChatWindow(AMessage.from());
    if (window)
      window->viewWidget()->showMessage(AMessage);
  }
}

void MultiUserChatWindow::onInviteDeclined(const Jid &AContactJid, const QString &AReason)
{
  QString nick = AContactJid && roomJid() ? AContactJid.resource() : AContactJid.hFull();
  showServiceMessage(tr("%1 has declined your invite to this room. %2").arg(nick).arg(AReason));
}

void MultiUserChatWindow::onUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser)
{
  showServiceMessage(tr("%1 has been kicked from the room%2. %3").arg(ANick)
    .arg(AByUser.isEmpty() ? "" : tr(" by %1").arg(AByUser)).arg(AReason));
}

void MultiUserChatWindow::onUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser)
{
  showServiceMessage(tr("%1 has been banned from the room%2. %3").arg(ANick)
    .arg(AByUser.isEmpty() ? "" : tr(" by %1").arg(AByUser)).arg(AReason));
}

void MultiUserChatWindow::onAffiliationListReceived(const QString &AAffiliation, const QList<IMultiUserListItem> &AList)
{
  EditUsersListDialog *dialog = new EditUsersListDialog(AAffiliation,AList,this);
  QString listName;
  if (AAffiliation == MUC_AFFIL_OUTCAST)
    listName = tr("Edit ban list - %1");
  else if (AAffiliation == MUC_AFFIL_MEMBER)
    listName = tr("Edit members list - %1");
  else if (AAffiliation == MUC_AFFIL_ADMIN)
    listName = tr("Edit administrators list - %1");
  else if (AAffiliation == MUC_AFFIL_OWNER)
    listName = tr("Edit owners list - %1");
  dialog->setTitle(listName.arg(roomJid().bare()));
  connect(dialog,SIGNAL(accepted()),SLOT(onAffiliationListDialogAccepted()));
  connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog,SLOT(reject()));
  dialog->show();
}

void MultiUserChatWindow::onConfigFormReceived(const IDataForm &AForm)
{
  if (FDataForms)
  {
    IDataDialogWidget *dialog = FDataForms->dialogWidget(FDataForms->localizeForm(AForm),this);
    connect(dialog->instance(),SIGNAL(accepted()),SLOT(onConfigFormDialogAccepted()));
    connect(FMultiChat->instance(),SIGNAL(chatClosed()),dialog->instance(),SLOT(reject()));
    connect(FMultiChat->instance(),SIGNAL(configFormReceived(const IDataForm &)),dialog->instance(),SLOT(reject()));
    dialog->instance()->show();
  }
}

void MultiUserChatWindow::onRoomDestroyed(const QString &AReason)
{
  showServiceMessage(tr("This room was destroyed by owner. %1").arg(AReason));
}

void MultiUserChatWindow::onMessageSend()
{
  if (FMultiChat->isOpen())
  {
    Message message;

    if (FMessageProcessor)
      FMessageProcessor->textToMessage(message,FEditWidget->document());
    else
      message.setBody(FEditWidget->document()->toPlainText());

    if (!message.body().isEmpty() && FMultiChat->sendMessage(message))
      FEditWidget->clearEditor();
  }
}

void MultiUserChatWindow::onMessageAboutToBeSend()
{
  if (execShortcutCommand(FEditWidget->textEdit()->toPlainText()))
    FEditWidget->clearEditor();
}

void MultiUserChatWindow::onEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHook)
{
  if (FMultiChat->isOpen() && AKeyEvent->modifiers()+AKeyEvent->key() == NICK_MENU_KEY)
  {
    Menu *nickMenu = new Menu(this);
    nickMenu->setAttribute(Qt::WA_DeleteOnClose,true);
    foreach(QListWidgetItem *listItem, FUsers)
    {
      if (listItem->text() != FMultiChat->mainUser()->nickName())
      {
        Action *action = new Action(nickMenu);
        action->setText(listItem->text());
        action->setIcon(listItem->icon());
        action->setData(ADR_USER_NICK,listItem->text());
        connect(action,SIGNAL(triggered(bool)),SLOT(onNickMenuActionTriggered(bool)));
        nickMenu->addAction(action,AG_DEFAULT,true);
      }
    }
    if (!nickMenu->isEmpty())
    {
      QTextEdit *textEdit = FEditWidget->textEdit();
      nickMenu->popup(textEdit->viewport()->mapToGlobal(textEdit->cursorRect().topLeft()));
    }
    else
      delete nickMenu;
    AHook = true;
  }
}

void MultiUserChatWindow::onWindowActivated()
{
  removeActiveMessages();
}

void MultiUserChatWindow::onChatMessageSend()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window && FMultiChat->isOpen())
  {
    Message message;
    message.setType(Message::Chat);

    if (FMessageProcessor)
      FMessageProcessor->textToMessage(message,window->editWidget()->document());
    else
      message.setBody(window->editWidget()->document()->toPlainText());

    if (!message.body().isEmpty() && FMultiChat->sendMessage(message,window->contactJid().resource()))
    {
      window->viewWidget()->showMessage(message);
      window->editWidget()->clearEditor();
    }
  }
}

void MultiUserChatWindow::onChatWindowActivated()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
    removeActiveChatMessages(window);
}

void MultiUserChatWindow::onChatWindowClosed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (FChatWindows.contains(window) && !FMultiChat->userByNick(window->contactJid().resource()))
    window->instance()->deleteLater();
}

void MultiUserChatWindow::onChatWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (FChatWindows.contains(window))
  {
    removeActiveChatMessages(window);
    FChatWindows.removeAt(FChatWindows.indexOf(window));
    emit chatWindowDestroyed(window);
  }
}

void MultiUserChatWindow::onNickMenuActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString nick = action->data(ADR_USER_NICK).toString();
    FEditWidget->textEdit()->textCursor().insertText(tr("%1: ").arg(nick));
  }
}

void MultiUserChatWindow::onMenuBarActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action == FChangeNick)
  {
    QString nick = QInputDialog::getText(this,tr("Change nick name"),tr("Enter your new nick name in room %1").arg(roomJid().node()),
      QLineEdit::Normal,FMultiChat->nickName());
    if (!nick.isEmpty())
      FMultiChat->setNickName(nick);
  }
  else if (action == FChangeSubject)
  {
    if (FMultiChat->isOpen())
    {
      bool ok = false;
      QString subject = QInputDialog::getText(this,tr("Change subject"),tr("Enter new subject for room %1").arg(roomJid().node()),
        QLineEdit::Normal,FMultiChat->subject(),&ok);
      if (ok)
        FMultiChat->setSubject(subject);
    }
  }
  else if (action == FClearChat)
  {
    FViewWidget->textBrowser()->clear();
  }
  else if (action == FQuitRoom)
  {
    deleteLater();
  }
  else if (action == FInviteContact)
  {
    if (FMultiChat->isOpen())
    {
      Jid contactJid = QInputDialog::getText(this,tr("Invite user"),tr("Enter user JID:"));
      if (contactJid.isValid())
      {
        QString reason = tr("You are welcome here");
        reason = QInputDialog::getText(this,tr("Invite user"),tr("Enter a reason:"),QLineEdit::Normal,reason);
        FMultiChat->inviteContact(contactJid,reason);
      }
    }
  }
  else if (action == FRequestVoice)
  {
    FMultiChat->requestVoice();
  }
  else if (action == FBanList)
  {
    FMultiChat->requestAffiliationList(MUC_AFFIL_OUTCAST);
  }
  else if (action == FMembersList)
  {
    FMultiChat->requestAffiliationList(MUC_AFFIL_MEMBER);
  }
  else if (action == FAdminsList)
  {
    FMultiChat->requestAffiliationList(MUC_AFFIL_ADMIN);
  }
  else if (action == FOwnersList)
  {
    FMultiChat->requestAffiliationList(MUC_AFFIL_OWNER);
  }
  else if (action == FConfigRoom)
  {
    FMultiChat->requestConfigForm();
  }
  else if (action == FDestroyRoom)
  {
    if (FMultiChat->isOpen())
    {
      bool ok = false;
      QString reason = QInputDialog::getText(this,tr("Destroying room"),tr("Enter a reason:"),QLineEdit::Normal,"",&ok);
      if (ok)
        FMultiChat->destroyRoom(reason);
    }
  }
}

void MultiUserChatWindow::onRoomUtilsActionTriggered(bool)
{
  Action *action =qobject_cast<Action *>(sender());
  if (action == FSetRoleNode)
  {
    bool ok;
    QString reason = QInputDialog::getText(this,tr("Kick reason"),tr("Enter reason for kick"),QLineEdit::Normal,"",&ok);
    if (ok)
      FMultiChat->setRole(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_NONE,reason);
  }
  else if (action == FSetRoleVisitor)
  {
    FMultiChat->setRole(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_VISITOR);
  }
  else if (action == FSetRoleParticipant)
  {
    FMultiChat->setRole(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_PARTICIPANT);
  }
  else if (action == FSetRoleModerator)
  {
    FMultiChat->setRole(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_MODERATOR);
  }
  else if (action == FSetAffilNone)
  {
    FMultiChat->setAffiliation(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_NONE);
  }
  else if (action == FSetAffilOutcast)
  {
    bool ok;
    QString reason = QInputDialog::getText(this,tr("Ban reason"),tr("Enter reason for ban"),QLineEdit::Normal,"",&ok);
    if (ok)
      FMultiChat->setAffiliation(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_OUTCAST,reason);
  }
  else if (action == FSetAffilMember)
  {
    FMultiChat->setAffiliation(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_MEMBER);
  }
  else if (action == FSetAffilAdmin)
  {
    FMultiChat->setAffiliation(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_ADMIN);
  }
  else if (action == FSetAffilOwner)
  {
    FMultiChat->setAffiliation(FRoomUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_OWNER);
  }
}

void MultiUserChatWindow::onInviteActionTriggered(bool)
{
  Action *action =qobject_cast<Action *>(sender());
  if (action)
  {
    bool ok;
    QString reason = tr("You are welcome here");
    reason = QInputDialog::getText(this,tr("Invite user"),tr("Enter a reason:"),QLineEdit::Normal,reason,&ok);
    if (ok)
    {
      Jid streamJid = action->data(ADR_STREAM_JID).toString();
      Jid roomJid = action->data(ADR_ROOM_JID).toString();
      IMultiUserChatWindow *window = FChatPlugin->multiChatWindow(streamJid,roomJid);
      if (window)
      {
        Jid userJid = action->data(ADR_USER_JID).toString();
        window->multiUserChat()->inviteContact(userJid,reason);
      }
    }
  }
}

void MultiUserChatWindow::onDataFormMessageDialogAccepted()
{
  IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
  if (dialog)
    FMultiChat->sendDataFormMessage(FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
}

void MultiUserChatWindow::onAffiliationListDialogAccepted()
{
  EditUsersListDialog *dialog = qobject_cast<EditUsersListDialog *>(sender());
  if (dialog)
    FMultiChat->changeAffiliationList(dialog->deltaList());
}

void MultiUserChatWindow::onConfigFormDialogAccepted()
{
  IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
  if (dialog)
    FMultiChat->sendConfigForm(FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
}

void MultiUserChatWindow::onListItemActivated(QListWidgetItem *AItem)
{
  IMultiUser *user = FUsers.key(AItem);
  if (user)
    openWindow(streamJid(),user->contactJid(),Message::Chat);
}

void MultiUserChatWindow::onDefaultChatFontChanged(const QFont &AFont)
{
  FViewWidget->document()->setDefaultFont(AFont);
  FEditWidget->document()->setDefaultFont(AFont);
}

void MultiUserChatWindow::onStatusIconsChanged()
{
  foreach(IChatWindow *window,FChatWindows)
    updateChatWindow(window);
  foreach(IMultiUser *user, FUsers.keys())
    updateListItem(user->contactJid());
  updateWindow();
}

void MultiUserChatWindow::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == AVN_NAME)
    ui.lblAccount->setText(Qt::escape(AValue.toString()));
}

