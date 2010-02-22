#include "multiuserchatwindow.h"

#include <QTimer>
#include <QKeyEvent>
#include <QInputDialog>
#include <QCoreApplication>
#include <QContextMenuEvent>

#define BDI_WINDOW_GEOMETRY         "MultiChatWindowGeometry"
#define BDI_WINDOW_HSPLITTER        "MultiChatWindowHSplitterState"

#define ADR_STREAM_JID              Action::DR_StreamJid
#define ADR_ROOM_JID                Action::DR_Parametr1
#define ADR_USER_JID                Action::DR_Parametr2
#define ADR_USER_REAL_JID           Action::DR_Parametr3
#define ADR_USER_NICK               Action::DR_Parametr4

#define NICK_MENU_KEY               Qt::Key_Tab

#define CONSECUTIVE_TIMEOUT         2*60

#define HISTORY_MESSAGES            10
#define HISTORY_TIME_PAST           5

MultiUserChatWindow::MultiUserChatWindow(IMultiUserChatPlugin *AChatPlugin, IMultiUserChat *AMultiChat)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose, false);

  ui.lblRoom->setText(AMultiChat->roomJid().hFull());
  ui.lblNick->setText(Qt::escape(AMultiChat->nickName()));

  FSettings = NULL;
  FStatusIcons = NULL;
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;
  FMessageStyles = NULL;
  FStatusChanger = NULL;
  FMessageArchiver = NULL;

  FMultiChat = AMultiChat;
  FChatPlugin = AChatPlugin;
  FMultiChat->instance()->setParent(this);

  FViewWidget = NULL;
  FEditWidget = NULL;
  FMenuBarWidget = NULL;
  FToolBarWidget = NULL;
  FStatusBarWidget = NULL;
  FShownDetached = false;
  FDestroyOnChatClosed = false;

  initialize();
  createMessageWidgets();
  setMessageStyle();
  connectMultiChat();
  createStaticRoomActions();
  updateStaticRoomActions();
  createStaticUserContextActions();
  loadWindowState();

  FUsersModel = new QStandardItemModel(0,1,ui.ltvUsers);

  FUsersProxy = new UsersProxyModel(AMultiChat,FUsersModel);
  FUsersProxy->setSortLocaleAware(true);
  FUsersProxy->setDynamicSortFilter(true);
  FUsersProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
  FUsersProxy->setSourceModel(FUsersModel);
  FUsersProxy->sort(0, Qt::AscendingOrder);
  
  ui.ltvUsers->setModel(FUsersProxy);
  ui.ltvUsers->viewport()->installEventFilter(this);
  connect(ui.ltvUsers,SIGNAL(activated(const QModelIndex &)),SLOT(onUserItemActivated(const QModelIndex &)));

  connect(this,SIGNAL(windowActivated()),SLOT(onWindowActivated()));
}

MultiUserChatWindow::~MultiUserChatWindow()
{
  QList<IChatWindow *> chatWindows = FChatWindows;
  foreach(IChatWindow *window,chatWindows)
    delete window->instance();

  if (FMessageProcessor)
    FMessageProcessor->removeMessageHandler(this,MHO_MULTIUSERCHAT);

  saveWindowState();
  emit windowDestroyed();
}

QString MultiUserChatWindow::tabPageId() const
{
  return "MessageWindow|"+streamJid().pBare()+"|"+roomJid().pBare();
}

void MultiUserChatWindow::showWindow()
{
  if (FMessageWidgets && isWindow() && !isVisible())
    FMessageWidgets->assignTabWindowPage(this);

  if (isWindow())
  {
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
    WidgetManager::raiseWidget(this);
  }
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
    showMessage(tr("Data form received: %1").arg(form.title),IMessageContentOptions::Notification);
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
  if (AMessage.type() != Message::Error)
  {
    Jid contactJid = AMessage.from();
    IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
    if (!contactJid.resource().isEmpty())
    {
      if (AMessage.type() == Message::GroupChat)
      {
        if (!AMessage.body().isEmpty() && !isActive() && !AMessage.isDelayed())
        {
          if (isMentionMessage(AMessage))
          {
            notify.kinds = ANotifications->notificatorKinds(MENTION_NOTIFICATOR_ID);
            notify.data.insert(NDR_TOOLTIP,tr("Mention message in conference: %1").arg(contactJid.node()));
            notify.data.insert(NDR_WINDOW_CAPTION,tr("Mention in conference"));
          }
          else
          {
            notify.kinds = ANotifications->notificatorKinds(GROUP_NOTIFICATOR_ID);
            notify.data.insert(NDR_TOOLTIP,tr("New message in conference: %1").arg(contactJid.node()));
            notify.data.insert(NDR_WINDOW_CAPTION,tr("Conference message"));
          }
          notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_MESSAGE));
          notify.data.insert(NDR_WINDOW_TITLE,tr("[%1] in conference %2").arg(contactJid.resource()).arg(contactJid.node()));
          notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
          notify.data.insert(NDR_SOUND_FILE,SDF_MUC_MESSAGE);
        }
      }
      else if (!AMessage.body().isEmpty())
      {
        IChatWindow *window = findChatWindow(AMessage.from());
        if (window == NULL || !window->isActive())
        {
          notify.kinds = ANotifications->notificatorKinds(PRIVATE_NOTIFICATOR_ID);
          notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_PRIVATE_MESSAGE));
          notify.data.insert(NDR_TOOLTIP,tr("Private message from: [%1]").arg(contactJid.resource()));
          notify.data.insert(NDR_WINDOW_CAPTION,tr("Private message"));
          notify.data.insert(NDR_WINDOW_TITLE,tr("[%1] in conference %2").arg(contactJid.resource()).arg(contactJid.node()));
          notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
          notify.data.insert(NDR_SOUND_FILE,SDF_MUC_PRIVATE_MESSAGE);
        }
      }
    }
    else
    {
      if (!AMessage.stanza().firstElement("x",NS_JABBER_DATA).isNull())
      {
        notify.kinds = ANotifications->notificatorKinds(PRIVATE_NOTIFICATOR_ID);
        notify.data.insert(NDR_ICON,storage->getIcon(MNI_MUC_DATA_MESSAGE));
        notify.data.insert(NDR_TOOLTIP,tr("Data form received from: %1").arg(contactJid.node()));
        notify.data.insert(NDR_WINDOW_CAPTION,tr("Data form received"));
        notify.data.insert(NDR_WINDOW_TITLE,ANotifications->contactName(FMultiChat->streamJid(),contactJid));
        notify.data.insert(NDR_WINDOW_IMAGE,ANotifications->contactAvatar(contactJid));
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

void MultiUserChatWindow::contextMenuForUser(IMultiUser *AUser, Menu *AMenu)
{
  if (FUsers.contains(AUser) && AUser!=FMultiChat->mainUser())
  {
    insertStaticUserContextActions(AMenu,AUser);
    emit multiUserContextMenu(AUser,AMenu);
  }
}

void MultiUserChatWindow::exitAndDestroy(const QString &AStatus, int AWaitClose)
{
  closeWindow();

  FDestroyOnChatClosed = true;
  if (FMultiChat->isOpen())
    FMultiChat->setPresence(IPresence::Offline,AStatus);

  if (AWaitClose>0)
    QTimer::singleShot(FMultiChat->isOpen() ? AWaitClose : 0, this, SLOT(deleteLater()));
  else
    delete this;
}

void MultiUserChatWindow::initialize()
{
  IPlugin *plugin = FChatPlugin->pluginManager()->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
    }
  }

  plugin = FChatPlugin->pluginManager()->pluginInterface("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = FChatPlugin->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
  if (plugin)
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

  plugin = FChatPlugin->pluginManager()->pluginInterface("IAccountManager").value(0,NULL);
  if (plugin)
  {
    IAccountManager *accountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (accountManager)
    {
      IAccount *account = accountManager->accountByStream(streamJid());
      if (account)
      {
        ui.lblAccount->setText(Qt::escape(account->name()));
        connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          SLOT(onAccountChanged(const QString &, const QVariant &)));
      }
    }
  }

  plugin = FChatPlugin->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageWidgets").value(0,NULL);
  if (plugin)
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

  plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageProcessor").value(0,NULL);
  if (plugin)
  {
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
    if (FMessageProcessor)
    {
      FMessageProcessor->insertMessageHandler(this,MHO_MULTIUSERCHAT);
    }
  }

  plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageStyles").value(0,NULL);
  if (plugin)
  {
    FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
    if (FMessageStyles)
    {
      connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
        SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
    }
  }

  plugin = FChatPlugin->pluginManager()->pluginInterface("IMessageArchiver").value(0,NULL);
  if (plugin)
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
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
  if (FMessageWidgets)
  {
    FViewWidget = FMessageWidgets->newViewWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
    ui.wdtView->setLayout(new QVBoxLayout);
    ui.wdtView->layout()->addWidget(FViewWidget->instance());
    ui.wdtView->layout()->setMargin(0);
    FWindowStatus[FViewWidget].createTime = QDateTime::currentDateTime();

    FEditWidget = FMessageWidgets->newEditWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
    ui.wdtEdit->setLayout(new QVBoxLayout);
    ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
    ui.wdtEdit->layout()->setMargin(0);
    connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
    connect(FEditWidget->instance(),SIGNAL(messageAboutToBeSend()),SLOT(onMessageAboutToBeSend()));
    connect(FEditWidget->instance(),SIGNAL(keyEventReceived(QKeyEvent *,bool &)),SLOT(onEditWidgetKeyEvent(QKeyEvent *,bool &)));

    FMenuBarWidget = FMessageWidgets->newMenuBarWidget(NULL,FViewWidget,FEditWidget,NULL);
    setMenuBar(FMenuBarWidget->instance());

    FToolBarWidget = FMessageWidgets->newToolBarWidget(NULL,FViewWidget,FEditWidget,NULL);
    ui.wdtToolBar->setLayout(new QVBoxLayout);
    ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());
    ui.wdtToolBar->layout()->setMargin(0);
    FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

    FStatusBarWidget = FMessageWidgets->newStatusBarWidget(NULL,FViewWidget,FEditWidget,NULL);
    setStatusBar(FStatusBarWidget->instance());
  }
}

void MultiUserChatWindow::createStaticRoomActions()
{
  FToolsMenu = new Menu(FToolBarWidget->toolBarChanger()->toolBar());
  FToolsMenu->setTitle(tr("Tools"));
  FToolsMenu->setIcon(RSR_STORAGE_MENUICONS, MNI_MUC_TOOLS_MENU);
  QToolButton *toolsButton = FToolBarWidget->toolBarChanger()->insertAction(FToolsMenu->menuAction(), TBG_MCWTBW_ROOM_TOOLS);
  toolsButton->setPopupMode(QToolButton::InstantPopup);

  FChangeNick = new Action(FToolsMenu);
  FChangeNick->setText(tr("Change room nick"));
  FChangeNick->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_NICK);
  connect(FChangeNick,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FChangeNick,AG_MUTM_MULTIUSERCHAT_COMMON,false);

  FInviteContact = new Action(FToolsMenu);
  FInviteContact->setText(tr("Invite to this room"));
  FInviteContact->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_INVITE);
  connect(FInviteContact,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FInviteContact,AG_MUTM_MULTIUSERCHAT_COMMON,false);

  FRequestVoice = new Action(FToolsMenu);
  FRequestVoice->setText(tr("Request voice"));
  FRequestVoice->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_REQUEST_VOICE);
  connect(FRequestVoice,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FRequestVoice,AG_MUTM_MULTIUSERCHAT_COMMON,false);

  FClearChat = new Action(FToolsMenu);
  FClearChat->setText(tr("Clear chat window"));
  FClearChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CLEAR_CHAT);
  connect(FClearChat,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FClearChat,AG_MUTM_MULTIUSERCHAT_COMMON,false);

  FChangeSubject = new Action(FToolsMenu);
  FChangeSubject->setText(tr("Change topic"));
  FChangeSubject->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CHANGE_TOPIC);
  connect(FChangeSubject,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FChangeSubject,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FBanList = new Action(FToolsMenu);
  FBanList->setText(tr("Edit ban list"));
  FBanList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_BAN_LIST);
  connect(FBanList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FBanList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FMembersList = new Action(FToolsMenu);
  FMembersList->setText(tr("Edit members list"));
  FMembersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_MEMBERS_LIST);
  connect(FMembersList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FMembersList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FAdminsList = new Action(FToolsMenu);
  FAdminsList->setText(tr("Edit administrators list"));
  FAdminsList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_ADMINS_LIST);
  connect(FAdminsList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FAdminsList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FOwnersList = new Action(FToolsMenu);
  FOwnersList->setText(tr("Edit owners list"));
  FOwnersList->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EDIT_OWNERS_LIST);
  connect(FOwnersList,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FOwnersList,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FConfigRoom = new Action(FToolsMenu);
  FConfigRoom->setText(tr("Configure room"));
  FConfigRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_CONFIGURE_ROOM);
  connect(FConfigRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FConfigRoom,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FDestroyRoom = new Action(FToolsMenu);
  FDestroyRoom->setText(tr("Destroy room"));
  FDestroyRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_DESTROY_ROOM);
  connect(FDestroyRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FDestroyRoom,AG_MUTM_MULTIUSERCHAT_TOOLS,false);

  FEnterRoom = new Action(FToolsMenu);
  FEnterRoom->setText(tr("Enter room"));
  FEnterRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_ENTER_ROOM);
  connect(FEnterRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FEnterRoom,AG_MUTM_MULTIUSERCHAT_EXIT,false);

  FExitRoom = new Action(FToolsMenu);
  FExitRoom->setIcon(RSR_STORAGE_MENUICONS,MNI_MUC_EXIT_ROOM);
  FExitRoom->setText(tr("Exit room"));
  FExitRoom->setShortcut(tr("Ctrl+F4"));
  connect(FExitRoom,SIGNAL(triggered(bool)),SLOT(onMenuBarActionTriggered(bool)));
  FToolsMenu->addAction(FExitRoom,AG_MUTM_MULTIUSERCHAT_EXIT,false);
}

void MultiUserChatWindow::updateStaticRoomActions()
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
  FEnterRoom->setVisible(!FMultiChat->isOpen());
}

void MultiUserChatWindow::createStaticUserContextActions()
{
  FModeratorUtilsMenu = new Menu(this);
  FModeratorUtilsMenu->setTitle(tr("Room Utilities"));

  FSetRoleNode = new Action(FModeratorUtilsMenu);
  FSetRoleNode->setText(tr("Kick user"));
  connect(FSetRoleNode,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
  FModeratorUtilsMenu->addAction(FSetRoleNode,AG_MUCM_MULTIUSERCHAT_UTILS,false);

  FSetAffilOutcast = new Action(FModeratorUtilsMenu);
  FSetAffilOutcast->setText(tr("Ban user"));
  connect(FSetAffilOutcast,SIGNAL(triggered(bool)),SLOT(onRoomUtilsActionTriggered(bool)));
  FModeratorUtilsMenu->addAction(FSetAffilOutcast,AG_MUCM_MULTIUSERCHAT_UTILS,false);

  FChangeRole = new Menu(FModeratorUtilsMenu);
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
  FModeratorUtilsMenu->addAction(FChangeRole->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);

  FChangeAffiliation = new Menu(FModeratorUtilsMenu);
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
  FModeratorUtilsMenu->addAction(FChangeAffiliation->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);
}

void MultiUserChatWindow::insertStaticUserContextActions(Menu *AMenu, IMultiUser *AUser)
{
  IMultiUser *muser = FMultiChat->mainUser();
  if (muser && muser->role() == MUC_ROLE_MODERATOR)
  {
    FModeratorUtilsMenu->menuAction()->setData(ADR_USER_NICK,AUser->nickName());

    FSetRoleVisitor->setChecked(AUser->role() == MUC_ROLE_VISITOR);
    FSetRoleParticipant->setChecked(AUser->role() == MUC_ROLE_PARTICIPANT);
    FSetRoleModerator->setChecked(AUser->role() == MUC_ROLE_MODERATOR);

    FSetAffilNone->setChecked(AUser->affiliation() == MUC_AFFIL_NONE);
    FSetAffilMember->setChecked(AUser->affiliation() == MUC_AFFIL_MEMBER);
    FSetAffilAdmin->setChecked(AUser->affiliation() == MUC_AFFIL_ADMIN);
    FSetAffilOwner->setChecked(AUser->affiliation() == MUC_AFFIL_OWNER);

    AMenu->addAction(FSetRoleNode,AG_MUCM_MULTIUSERCHAT_UTILS,false);
    AMenu->addAction(FSetAffilOutcast,AG_MUCM_MULTIUSERCHAT_UTILS,false);
    AMenu->addAction(FChangeRole->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);
    AMenu->addAction(FChangeAffiliation->menuAction(),AG_MUCM_MULTIUSERCHAT_UTILS,false);
  }
}

void MultiUserChatWindow::saveWindowState()
{
  if (FSettings)
  {
    QString dataId = streamJid().pBare()+"|"+roomJid().pBare();
    FSettings->saveBinaryData(BDI_WINDOW_HSPLITTER"|"+dataId,ui.sprHSplitter->saveState());
  }
}

void MultiUserChatWindow::loadWindowState()
{
  if (FSettings)
  {
    QString dataId = streamJid().pBare()+"|"+roomJid().pBare();
    ui.sprHSplitter->restoreState(FSettings->loadBinaryData(BDI_WINDOW_HSPLITTER"|"+dataId));
  }
}

void MultiUserChatWindow::saveWindowGeometry()
{
  if (FSettings && isWindow())
  {
    QString dataId = streamJid().pBare()+"|"+roomJid().pBare();
    FSettings->saveBinaryData(BDI_WINDOW_GEOMETRY"|"+dataId,saveGeometry());
  }
}

void MultiUserChatWindow::loadWindowGeometry()
{
  if (FSettings && isWindow())
  {
    QString dataId = streamJid().pBare()+"|"+roomJid().pBare();
    restoreGeometry(FSettings->loadBinaryData(BDI_WINDOW_GEOMETRY"|"+dataId));
  }
}

bool MultiUserChatWindow::showStatusCodes(const QString &ANick, const QList<int> &ACodes)
{
  bool shown = false;
  if (!ACodes.isEmpty())
  {
    if (ACodes.contains(MUC_SC_NON_ANONYMOUS))
    {
      showMessage(tr("Any occupant is allowed to see the user's full JID"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_AFFIL_CHANGED))
    {
      showMessage(tr("%1 affiliation changed while not in the room").arg(ANick),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_CONFIG_CHANGED))
    {
      showMessage(tr("Room configuration change has occurred"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_NOW_LOGGING_ENABLED))
    {
      showMessage(tr("Room logging is now enabled"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_NOW_LOGGING_DISABLED))
    {
      showMessage(tr("Room logging is now disabled"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_NOW_NON_ANONYMOUS))
    {
      showMessage(tr("The room is now non-anonymous"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_NOW_SEMI_ANONYMOUS))
    {
      showMessage(tr("The room is now semi-anonymous"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_NOW_FULLY_ANONYMOUS))
    {
      showMessage(tr("The room is now fully-anonymous"),IMessageContentOptions::Notification);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_ROOM_CREATED))
    {
      showMessage(tr("A new room has been created"),IMessageContentOptions::Notification);
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
      showMessage(tr("%1 has been removed from the room because of an affiliation change").arg(ANick),IMessageContentOptions::Event);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_MEMBERS_ONLY))
    {
      showMessage(tr("%1 has been removed from the room because the room has been changed to members-only").arg(ANick),IMessageContentOptions::Event);
      shown = true;
    }
    if (ACodes.contains(MUC_SC_SYSTEM_SHUTDOWN))
    {
      showMessage(tr("%1 is being removed from the room because of a system shutdown").arg(ANick),IMessageContentOptions::Event);
      shown = true;
    }
  }
  return shown;
}

void MultiUserChatWindow::highlightUserRole(IMultiUser *AUser)
{
  QStandardItem *userItem = FUsers.value(AUser);
  if (userItem)
  {
    QColor itemColor;
    QFont itemFont = userItem->font();
    QString role = AUser->data(MUDR_ROLE).toString();
    if (role == MUC_ROLE_MODERATOR)
    {
      itemFont.setBold(true);
      itemColor = ui.ltvUsers->palette().color(QPalette::Active, QPalette::Text);
    }
    else if (role == MUC_ROLE_PARTICIPANT)
    {
      itemFont.setBold(false);
      itemColor = ui.ltvUsers->palette().color(QPalette::Active, QPalette::Text);
    }
    else
    {
      itemFont.setBold(false);
      itemColor = ui.ltvUsers->palette().color(QPalette::Disabled, QPalette::Text);
    }
    userItem->setFont(itemFont);
    userItem->setForeground(itemColor);
  }
}

void MultiUserChatWindow::highlightUserAffiliation(IMultiUser *AUser)
{
  QStandardItem *userItem = FUsers.value(AUser);
  if (userItem)
  {
    QFont itemFont = userItem->font();
    QString affilation = AUser->data(MUDR_AFFILIATION).toString();
    if (affilation == MUC_AFFIL_OWNER)
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(true);
      itemFont.setItalic(false);
    }
    else if (affilation == MUC_AFFIL_ADMIN)
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(false);
      itemFont.setItalic(false);
    }
    else if (affilation == MUC_AFFIL_MEMBER)
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(false);
      itemFont.setItalic(false);
    }
    else if (affilation == MUC_AFFIL_OUTCAST)
    {
      itemFont.setStrikeOut(true);
      itemFont.setUnderline(false);
      itemFont.setItalic(false);
    }
    else
    {
      itemFont.setStrikeOut(false);
      itemFont.setUnderline(false);
      itemFont.setItalic(true);
    }
    userItem->setFont(itemFont);
  }
}

void MultiUserChatWindow::setToolTipForUser(IMultiUser *AUser)
{
  QStandardItem *userItem = FUsers.value(AUser);
  if (userItem)
  {
    QStringList toolTips;
    toolTips.append(Qt::escape(AUser->nickName()));

    QString realJid = AUser->data(MUDR_REAL_JID).toString();
    if (!realJid.isEmpty())
      toolTips.append(Qt::escape(realJid));
    
    QString role = AUser->data(MUDR_ROLE).toString();
    if (!role.isEmpty())
      toolTips.append(tr("Role: %1").arg(Qt::escape(role)));

    QString affiliation = AUser->data(MUDR_AFFILIATION).toString();
    if (!affiliation.isEmpty())
      toolTips.append(tr("Affiliation: %1").arg(Qt::escape(affiliation)));

    QString status = AUser->data(MUDR_STATUS).toString();
    if (!status.isEmpty())
      toolTips.append(QString("%1 <div style='margin-left:10px;'>%2</div>").arg(tr("Status:")).arg(Qt::escape(status).replace("\n","<br>")));

    userItem->setToolTip("<span>"+toolTips.join("<p/>")+"</span>");
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
      showMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::Notification);
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
      showMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::Notification);
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
      showMessage(tr("%1 is not valid contact JID").arg(userJid.full()),IMessageContentOptions::Notification);
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
      showMessage(tr("%1 is not valid room JID").arg(roomJid.full()),IMessageContentOptions::Notification);
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
      showMessage(tr("User %1 is not present in the conference").arg(nick),IMessageContentOptions::Notification);
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
    exitAndDestroy(QString::null);
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
    showMessage(tr("Supported list of commands: \n"
                   " /ban <roomnick> [comment] \n"
                   " /invite <jid> [comment] \n"
                   " /join <roomname> [pass] \n"
                   " /kick <roomnick> [comment] \n"
                   " /msg <roomnick> <foo> \n"
                   " /nick <newnick> \n"
                   " /leave [comment] \n"
                   " /topic <foo>"),IMessageContentOptions::Notification);
    hasCommand = true;
  }
  return hasCommand;
}

bool MultiUserChatWindow::isMentionMessage(const Message &AMessage) const
{
  QRegExp mention(QString("\\b%1\\b").arg(QRegExp::escape(FMultiChat->nickName())));
  return AMessage.body().indexOf(mention)>=0;
}

void MultiUserChatWindow::setMessageStyle()
{
  if (FMessageStyles)
  {
    IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::GroupChat);
    if (FViewWidget->messageStyle()==NULL || !FViewWidget->messageStyle()->changeOptions(FViewWidget->styleWidget(),soptions,true))
    {
      IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
      FViewWidget->setMessageStyle(style,soptions);
    }
  }
}

void MultiUserChatWindow::showTopic(const QString &ATopic)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Topic;
  options.type |= IMessageContentOptions::Groupchat;
  options.direction = IMessageContentOptions::DirectionIn;

  options.time = QDateTime::currentDateTime();
  options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;
  
  FViewWidget->appendText(ATopic,options);
}

void MultiUserChatWindow::showMessage(const QString &AMessage, int AContentType)
{
  IMessageContentOptions options;
  options.direction = IMessageContentOptions::DirectionIn;
  options.kind = IMessageContentOptions::Status;
  options.type |= AContentType;

  options.time = QDateTime::currentDateTime();
  options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;
  
  FViewWidget->appendText(AMessage,options);
}

void MultiUserChatWindow::showUserMessage(const Message &AMessage, const QString &ANick)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Message;
  options.type |= IMessageContentOptions::Groupchat;

  options.time = AMessage.dateTime();
  options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time) : QString::null;
  
  if (AMessage.isDelayed())
    options.type |= IMessageContentOptions::History;

  options.senderName = Qt::escape(ANick);
  options.senderId = options.senderName;

  if (FMessageStyles!=NULL)
  {
    IMultiUser *user = FMultiChat->nickName()!=ANick ? FMultiChat->userByNick(ANick) : FMultiChat->mainUser();
    if (user)
      options.senderIcon = FMessageStyles->userIcon(user->contactJid(),user->data(MUDR_SHOW).toInt(),SUBSCRIPTION_BOTH,false);
    else
      options.senderIcon = FMessageStyles->userIcon(Jid(),IPresence::Offline,SUBSCRIPTION_BOTH,false);
  }

  if (FMultiChat->nickName()!=ANick)
  {
    options.direction = IMessageContentOptions::DirectionIn;
    if (isMentionMessage(AMessage))
      options.type |= IMessageContentOptions::Mention;
  }
  else
  {
    options.direction = IMessageContentOptions::DirectionOut;
  }

  FViewWidget->appendMessage(AMessage,options);
}

void MultiUserChatWindow::showHistory()
{
  if (FMessageArchiver)
  {
    IArchiveRequest request;
    request.with = FMultiChat->roomJid();
    request.order = Qt::DescendingOrder;
    request.start = FWindowStatus.value(FViewWidget).createTime;
    request.end = QDateTime::currentDateTime();

    QList<Message> history;
    QList<IArchiveHeader> headers = FMessageArchiver->loadLocalHeaders(FMultiChat->streamJid(), request);
    for (int i=0; history.count()<HISTORY_MESSAGES && i<headers.count(); i++)
    {
      if (headers.at(i).with.resource().isEmpty())
      {
        IArchiveCollection collection = FMessageArchiver->loadLocalCollection(FMultiChat->streamJid(), headers.at(i));
        history = collection.messages + history;
      }
    }

    showTopic(FMultiChat->subject());
    for (int i=0; i<history.count(); i++)
    {
      Message message = history.at(i);
      showUserMessage(message, Jid(message.from()).resource());
    }
  }
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
  QStandardItem *userItem = FUsers.value(user);
  if (userItem)
  {
    IChatWindow *window = findChatWindow(AContactJid);
    if (FActiveChatMessages.contains(window))
      userItem->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_MUC_PRIVATE_MESSAGE));
    else if (FStatusIcons)
      userItem->setIcon(FStatusIcons->iconByJidStatus(AContactJid,user->data(MUDR_SHOW).toInt(),"",false));
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

void MultiUserChatWindow::setChatMessageStyle(IChatWindow *AWindow)
{
  if (FMessageStyles && AWindow)
  {
    IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
    IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
    AWindow->viewWidget()->setMessageStyle(style,soptions);
  }
}

void MultiUserChatWindow::fillChatContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
  IMultiUser *user = AOptions.direction==IMessageContentOptions::DirectionIn ? FMultiChat->userByNick(AWindow->contactJid().resource()) : FMultiChat->mainUser();
  if (user)
    AOptions.senderIcon = FMessageStyles->userIcon(user->contactJid(),user->data(MUDR_SHOW).toInt(),SUBSCRIPTION_BOTH,false);

  if (AOptions.direction == IMessageContentOptions::DirectionIn)
  {
    AOptions.senderColor = "blue";
    AOptions.senderName = Qt::escape(AWindow->contactJid().resource());
  }
  else
  {
    AOptions.senderColor = "red";
    AOptions.senderName = Qt::escape(FMultiChat->nickName());
  }
  AOptions.senderId = AOptions.senderName;
}

void MultiUserChatWindow::showChatStatus(IChatWindow *AWindow, const QString &AMessage)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Status;
  options.direction = IMessageContentOptions::DirectionIn;
  
  options.time = QDateTime::currentDateTime();
  options.timeFormat = FMessageStyles->timeFormat(options.time);

  fillChatContentOptions(AWindow,options);
  AWindow->viewWidget()->appendText(AMessage,options);
}

void MultiUserChatWindow::showChatMessage(IChatWindow *AWindow, const Message &AMessage)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Message;

  options.time = AMessage.dateTime();
  options.timeFormat = FMessageStyles->timeFormat(options.time);
  
  options.direction = AWindow->contactJid()!=AMessage.to() ? IMessageContentOptions::DirectionIn : IMessageContentOptions::DirectionOut;

  if (options.time.secsTo(FWindowStatus.value(AWindow->viewWidget()).createTime)>HISTORY_TIME_PAST)
    options.type |= IMessageContentOptions::History;

  fillChatContentOptions(AWindow,options);
  AWindow->viewWidget()->appendMessage(AMessage,options);
}

void MultiUserChatWindow::showChatHistory(IChatWindow *AWindow)
{
  if (FMessageArchiver)
  {
    IArchiveRequest request;
    request.with = AWindow->contactJid();
    request.order = Qt::DescendingOrder;

    WindowStatus &wstatus = FWindowStatus[AWindow->viewWidget()];
    if (wstatus.createTime.secsTo(QDateTime::currentDateTime()) < HISTORY_TIME_PAST)
    {
      request.count = HISTORY_MESSAGES;
      request.end = QDateTime::currentDateTime().addSecs(-HISTORY_TIME_PAST);
    }
    else
    {
      request.start = wstatus.startTime.isValid() ? wstatus.startTime : wstatus.createTime;
      request.end = QDateTime::currentDateTime();
    }

    QList<Message> history;
    QList<IArchiveHeader> headers = FMessageArchiver->loadLocalHeaders(AWindow->streamJid(), request);
    for (int i=0; history.count()<HISTORY_MESSAGES && i<headers.count(); i++)
    {
      IArchiveCollection collection = FMessageArchiver->loadLocalCollection(AWindow->streamJid(), headers.at(i));
      history = collection.messages + history;
    }

    for (int i=0; i<history.count(); i++)
    {
      Message message = history.at(i);
      showChatMessage(AWindow,message);
    }

    wstatus.startTime = history.value(0).dateTime();
  }
}

IChatWindow *MultiUserChatWindow::getChatWindow(const Jid &AContactJid)
{
  IChatWindow *window = findChatWindow(AContactJid);
  IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
  if (!window && user && user!=FMultiChat->mainUser())
  {
    window = FMessageWidgets!=NULL ? FMessageWidgets->newChatWindow(streamJid(),AContactJid) : NULL;
    if (window)
    {
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onChatMessageReady()));
      connect(window->instance(),SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
      connect(window->instance(),SIGNAL(windowClosed()),SLOT(onChatWindowClosed()));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
      
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactName,false);
      window->infoWidget()->setField(IInfoWidget::ContactName,user->nickName());
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactShow,false);
      window->infoWidget()->setField(IInfoWidget::ContactShow,user->data(MUDR_SHOW));
      window->infoWidget()->setFieldAutoUpdated(IInfoWidget::ContactStatus,false);
      window->infoWidget()->setField(IInfoWidget::ContactStatus,user->data(MUDR_STATUS));
      window->infoWidget()->autoUpdateFields();

      FChatWindows.append(window);
      FWindowStatus[window->viewWidget()].createTime = QDateTime::currentDateTime();
      updateChatWindow(window);
      
      UserContextMenu *menu = new UserContextMenu(this,window);
      menu->menuAction()->setIcon(RSR_STORAGE_MENUICONS, MNI_MUC_USER_MENU);
      QToolButton *button = window->toolBarWidget()->toolBarChanger()->insertAction(menu->menuAction(),TBG_CWTBW_USER_TOOLS);
      button->setPopupMode(QToolButton::InstantPopup);

      setChatMessageStyle(window);
      showChatHistory(window);
      emit chatWindowCreated(window);
    }
  }
  return window;
}

void MultiUserChatWindow::showChatWindow(IChatWindow *AWindow)
{
  if (FMessageWidgets && AWindow->instance()->isWindow() && !AWindow->instance()->isVisible())
    FMessageWidgets->assignTabWindowPage(AWindow);
  AWindow->showWindow();
}

void MultiUserChatWindow::removeActiveChatMessages(IChatWindow *AWindow)
{
  if (FActiveChatMessages.contains(AWindow))
  {
    if (FMessageProcessor)
      foreach(int messageId, FActiveChatMessages.values(AWindow))
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
    icon = FStatusIcons->iconByJidStatus(AWindow->contactJid(),AWindow->infoWidget()->field(IInfoWidget::ContactShow).toInt(),"",false);
    
  QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
  QString tabTitle = QString("[%1]").arg(contactName);
  AWindow->updateWindow(icon,tabTitle,tr("%1 - Private chat").arg(tabTitle));
}

bool MultiUserChatWindow::event(QEvent *AEvent)
{
  if (FEditWidget && AEvent->type()==QEvent::KeyPress)
  {
    static QKeyEvent *sentEvent = NULL;
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
    if (sentEvent!=keyEvent && !keyEvent->text().isEmpty())
    {
      sentEvent = keyEvent;
      FEditWidget->textEdit()->setFocus();
      QCoreApplication::sendEvent(FEditWidget->textEdit(),AEvent);
      sentEvent = NULL;
    }
  }
  else if (AEvent->type() == QEvent::WindowActivate)
  {
    emit windowActivated();
  }
  return QMainWindow::event(AEvent);
}

void MultiUserChatWindow::showEvent(QShowEvent *AEvent)
{
  if (!FShownDetached && isWindow())
    loadWindowGeometry();
  FShownDetached = isWindow();
  QMainWindow::showEvent(AEvent);
  if (FEditWidget)
    FEditWidget->textEdit()->setFocus();
  emit windowActivated();
}

void MultiUserChatWindow::closeEvent(QCloseEvent *AEvent)
{
  if (FShownDetached)
    saveWindowGeometry();
  QMainWindow::closeEvent(AEvent);
  emit windowClosed();
}

bool MultiUserChatWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AObject == ui.ltvUsers->viewport())
  {
    if (AEvent->type() == QEvent::ContextMenu)
    {
      QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(AEvent);
      QStandardItem *userItem = FUsersModel->itemFromIndex(FUsersProxy->mapToSource(ui.ltvUsers->indexAt(menuEvent->pos())));
      IMultiUser *user = FUsers.key(userItem,NULL);
      if (user && user!=FMultiChat->mainUser())
      {
        Menu *menu = new Menu(this);
        menu->setAttribute(Qt::WA_DeleteOnClose,true);
        contextMenuForUser(user,menu);
        if (!menu->isEmpty())
          menu->popup(menuEvent->globalPos());
        else
          delete menu;
      }
    }
    else if (AEvent->type() == QEvent::MouseButtonPress)
    {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(AEvent);
      if (FEditWidget && mouseEvent->button()==Qt::MidButton)
      {
        QStandardItem *userItem = FUsersModel->itemFromIndex(FUsersProxy->mapToSource(ui.ltvUsers->indexAt(mouseEvent->pos())));
        if (userItem)
        {
          QString sufix = FEditWidget->textEdit()->textCursor().atBlockStart() ? ": " : " ";
          FEditWidget->textEdit()->textCursor().insertText(userItem->text() + sufix);
          FEditWidget->textEdit()->setFocus();
        }
      }
    }
  }
  return QMainWindow::eventFilter(AObject,AEvent);
}

void MultiUserChatWindow::onChatOpened()
{
  if (FMultiChat->statusCodes().contains(MUC_SC_ROOM_CREATED))
    FMultiChat->requestConfigForm();
  setMessageStyle();
}

void MultiUserChatWindow::onChatNotify(const QString &ANick, const QString &ANotify)
{
  if (ANick.isEmpty())
    showMessage(tr("Notify: %1").arg(ANotify),IMessageContentOptions::Notification);
  else
    showMessage(tr("Notify from %1: %2").arg(ANick).arg(ANotify),IMessageContentOptions::Notification);
}

void MultiUserChatWindow::onChatError(const QString &ANick, const QString &AError)
{
  if (ANick.isEmpty())
    showMessage(tr("Error: %1").arg(AError),IMessageContentOptions::Notification);
  else
    showMessage(tr("Error from %1: %2").arg(ANick).arg(AError),IMessageContentOptions::Notification);
}

void MultiUserChatWindow::onChatClosed()
{
  FDestroyOnChatClosed ? deleteLater() : showMessage(tr("Disconnected"));
  updateWindow();
}

void MultiUserChatWindow::onStreamJidChanged(const Jid &ABefour, const Jid &AAfter)
{
  Q_UNUSED(ABefour);
  FViewWidget->setStreamJid(AAfter);
  FEditWidget->setStreamJid(AAfter);
}

void MultiUserChatWindow::onUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus)
{
  QStandardItem *userItem = FUsers.value(AUser);
  if (AShow!=IPresence::Offline && AShow!=IPresence::Error)
  {
    if (userItem == NULL)
    {
      userItem = new QStandardItem(AUser->nickName());
      FUsersModel->appendRow(userItem);
      FUsers.insert(AUser,userItem);
      highlightUserRole(AUser);
      highlightUserAffiliation(AUser);

      if (FMultiChat->isOpen())
      {
        QString message;
        QString realJid = AUser->data(MUDR_REAL_JID).toString();
        if (!realJid.isEmpty())
          message = tr("%1 <%2> has joined the room. %3").arg(AUser->nickName()).arg(realJid).arg(AStatus);
        else
          message = tr("%1 has joined the room. %2").arg(AUser->nickName()).arg(AStatus);
        showMessage(message);
      }
    }
    showStatusCodes(AUser->nickName(),FMultiChat->statusCodes());
    setToolTipForUser(AUser);
    updateListItem(AUser->contactJid());
  }
  else if (userItem)
  {
    if (!showStatusCodes(AUser->nickName(),FMultiChat->statusCodes()) && FMultiChat->isOpen())
    {
      QString message;
      QString realJid = AUser->data(MUDR_REAL_JID).toString();
      if (!realJid.isEmpty())
        message = tr("%1 <%2> has left the room. %3").arg(AUser->nickName()).arg(realJid).arg(AStatus);
      else
        message = tr("%1 has left the room. %2").arg(AUser->nickName()).arg(AStatus);
      showMessage(message);
    }
    FUsers.remove(AUser);
    qDeleteAll(FUsersModel->takeRow(userItem->row()));
  }

  if (FMultiChat->isOpen())
    updateWindow();

  IChatWindow *window = findChatWindow(AUser->contactJid());
  if (window)
  {
    window->infoWidget()->setField(IInfoWidget::ContactShow,AShow);
    window->infoWidget()->setField(IInfoWidget::ContactStatus,AStatus);
    if (FMessageWidgets->chatWindowShowStatus())
    {
      QString show = FStatusChanger ? FStatusChanger->nameByShow(AShow) : QString::null;
      WindowStatus &wstatus = FWindowStatus[window->viewWidget()];
      if (wstatus.lastStatusShow != AStatus+show)
      {
        wstatus.lastStatusShow = AStatus+show;
        QString message = tr("%1 changed status to [%2] %3").arg(AUser->nickName()).arg(show).arg(AStatus);
        showChatStatus(window,message);
      }
    }
    updateChatWindow(window);
  }
}

void MultiUserChatWindow::onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &ABefour, const QVariant &AAfter)
{
  if (ARole == MUDR_ROLE)
  {
    if (AAfter!=MUC_ROLE_NONE && ABefour!=MUC_ROLE_NONE)
      showMessage(tr("%1 role changed from %2 to %3").arg(AUser->nickName()).arg(ABefour.toString()).arg(AAfter.toString()),IMessageContentOptions::Event);
    highlightUserRole(AUser);
  }
  else if (ARole == MUDR_AFFILIATION)
  {
    if (FUsers.contains(AUser))
      showMessage(tr("%1 affiliation changed from %2 to %3").arg(AUser->nickName()).arg(ABefour.toString()).arg(AAfter.toString()),IMessageContentOptions::Event);
    highlightUserAffiliation(AUser);
  }
}

void MultiUserChatWindow::onUserNickChanged(IMultiUser *AUser, const QString &AOldNick, const QString &ANewNick)
{
  QStandardItem *userItem = FUsers.value(AUser);
  if (userItem)
  {
    userItem->setText(ANewNick);
    Jid userOldJid = AUser->contactJid();
    userOldJid.setResource(AOldNick);
    IChatWindow *window = findChatWindow(userOldJid);
    if (window)
    {
      window->setContactJid(AUser->contactJid());
      window->infoWidget()->setField(IInfoWidget::ContactName,ANewNick);
      updateChatWindow(window);
    }
  }
  if (AUser == FMultiChat->mainUser())
    ui.lblNick->setText(Qt::escape(ANewNick));
  showMessage(tr("%1 changed nick to %2").arg(AOldNick).arg(ANewNick),IMessageContentOptions::Event);
}

void MultiUserChatWindow::onPresenceChanged(int AShow, const QString &AStatus)
{
  Q_UNUSED(AShow);
  Q_UNUSED(AStatus);
  updateStaticRoomActions();
}

void MultiUserChatWindow::onSubjectChanged(const QString &ANick, const QString &ASubject)
{
  QString topic = ANick.isEmpty() ? tr("Subject: %1").arg(ASubject) : tr("%1 has changed the subject to: %2").arg(ANick).arg(ASubject);
  showTopic(topic);
}

void MultiUserChatWindow::onServiceMessageReceived(const Message &AMessage)
{
  if (!showStatusCodes("",FMultiChat->statusCodes()) && !AMessage.body().isEmpty())
    onMessageReceived("",AMessage);
}

void MultiUserChatWindow::onMessageReceived(const QString &ANick, const Message &AMessage)
{
  if (AMessage.type() == Message::GroupChat || ANick.isEmpty())
  {
    showUserMessage(AMessage,ANick);
  }
  else
  {
    IChatWindow *window = getChatWindow(AMessage.from());
    if (window)
      showChatMessage(window,AMessage);
  }
}

void MultiUserChatWindow::onInviteDeclined(const Jid &AContactJid, const QString &AReason)
{
  QString nick = AContactJid && roomJid() ? AContactJid.resource() : AContactJid.hFull();
  showMessage(tr("%1 has declined your invite to this room. %2").arg(nick).arg(AReason),IMessageContentOptions::Notification);
}

void MultiUserChatWindow::onUserKicked(const QString &ANick, const QString &AReason, const QString &AByUser)
{
  IMultiUser *user = FMultiChat->userByNick(ANick);
  QString realJid = user!=NULL ? user->data(MUDR_REAL_JID).toString() : QString::null;
  showMessage(tr("%1 has been kicked from the room%2. %3")
    .arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid) : ANick)
    .arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
    .arg(AReason),
    IMessageContentOptions::Event);
}

void MultiUserChatWindow::onUserBanned(const QString &ANick, const QString &AReason, const QString &AByUser)
{
  IMultiUser *user = FMultiChat->userByNick(ANick);
  QString realJid = user!=NULL ? user->data(MUDR_REAL_JID).toString() : QString::null;
  showMessage(tr("%1 has been banned from the room%2. %3")
    .arg(!realJid.isEmpty() ? ANick + QString(" <%1>").arg(realJid) : ANick)
    .arg(!AByUser.isEmpty() ? tr(" by %1").arg(AByUser) : QString::null)
    .arg(AReason),
    IMessageContentOptions::Event);
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
  showMessage(tr("This room was destroyed by owner. %1").arg(AReason),IMessageContentOptions::Event);
}

void MultiUserChatWindow::onMessageReady()
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

void MultiUserChatWindow::onEditWidgetKeyEvent(QKeyEvent *AKeyEvent, bool &AHooked)
{
  if (FMultiChat->isOpen() && AKeyEvent->modifiers()+AKeyEvent->key() == NICK_MENU_KEY)
  {
    QTextEdit *textEdit = FEditWidget->textEdit();
    QTextCursor cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);

    QList<QString> nicks;
    QString nickStarts = cursor.selectedText().toLower();

    foreach(IMultiUser *user, FUsers.keys())
    {
      if (user != FMultiChat->mainUser())
        if (nickStarts.isEmpty() || user->nickName().toLower().startsWith(nickStarts))
          nicks.append(user->nickName());
    }

    if (nicks.count() > 1)
    {
      Menu *nickMenu = new Menu(this);
      nickMenu->setAttribute(Qt::WA_DeleteOnClose,true);
      foreach(QString nick, nicks)
      {
        Action *action = new Action(nickMenu);
        action->setText(nick);
        action->setIcon(FUsers.value(FMultiChat->userByNick(nick))->icon());
        action->setData(ADR_USER_NICK,nick);
        connect(action,SIGNAL(triggered(bool)),SLOT(onNickMenuActionTriggered(bool)));
        nickMenu->addAction(action,AG_DEFAULT,true);
      }
      nickMenu->popup(textEdit->viewport()->mapToGlobal(textEdit->cursorRect().topLeft()));
    }
    else if (!nicks.isEmpty())
    {
      QString sufix = cursor.atBlockStart() ? ": " : " ";
      cursor.insertText(nicks.first() + sufix);
    }

    AHooked = true;
  }
}

void MultiUserChatWindow::onWindowActivated()
{
  removeActiveMessages();
}

void MultiUserChatWindow::onChatMessageReady()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window && FMultiChat->isOpen())
  {
    Message message;
    message.setType(Message::Chat).setTo(window->contactJid().eFull());

    if (FMessageProcessor)
      FMessageProcessor->textToMessage(message,window->editWidget()->document());
    else
      message.setBody(window->editWidget()->document()->toPlainText());

    if (!message.body().isEmpty() && FMultiChat->sendMessage(message,window->contactJid().resource()))
    {
      showChatMessage(window,message);
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
    FWindowStatus.remove(window->viewWidget());
    emit chatWindowDestroyed(window);
  }
}

void MultiUserChatWindow::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (AMessageType==Message::Chat && AContext.isEmpty())
  {
    foreach (IChatWindow *window, FChatWindows)
    {
      IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
      if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
      {
        setChatMessageStyle(window);
        showChatHistory(window);
      }
    }
  }
  else if (AMessageType==Message::GroupChat && AContext.isEmpty())
  {
    IMessageStyle *style = FViewWidget!=NULL ? FViewWidget->messageStyle() : NULL;
    if (style==NULL || !style->changeOptions(FViewWidget->styleWidget(),AOptions,false))
    {
      setMessageStyle();
      showHistory();
    }
  }
}

void MultiUserChatWindow::onNickMenuActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    QString nick = action->data(ADR_USER_NICK).toString();
    QTextCursor cursor = FEditWidget->textEdit()->textCursor();
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    QString sufix = cursor.atBlockStart() ? ": " : " ";
    cursor.insertText(nick + sufix);
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
      QString newSubject = FMultiChat->subject();
      InputTextDialog *dialog = new InputTextDialog(this,tr("Change subject"),tr("Enter new subject for room %1").arg(roomJid().node()), newSubject);
      if (dialog->exec() == QDialog::Accepted)
        FMultiChat->setSubject(newSubject);
    }
  }
  else if (action == FClearChat)
  {
    setMessageStyle();
  }
  else if (action == FEnterRoom)
  {
    FMultiChat->setAutoPresence(false);
    FMultiChat->setAutoPresence(true);
  }
  else if (action == FExitRoom)
  {
    exitAndDestroy(QString::null);
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
      FMultiChat->setRole(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_NONE,reason);
  }
  else if (action == FSetRoleVisitor)
  {
    FMultiChat->setRole(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_VISITOR);
  }
  else if (action == FSetRoleParticipant)
  {
    FMultiChat->setRole(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_PARTICIPANT);
  }
  else if (action == FSetRoleModerator)
  {
    FMultiChat->setRole(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_ROLE_MODERATOR);
  }
  else if (action == FSetAffilNone)
  {
    FMultiChat->setAffiliation(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_NONE);
  }
  else if (action == FSetAffilOutcast)
  {
    bool ok;
    QString reason = QInputDialog::getText(this,tr("Ban reason"),tr("Enter reason for ban"),QLineEdit::Normal,"",&ok);
    if (ok)
      FMultiChat->setAffiliation(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_OUTCAST,reason);
  }
  else if (action == FSetAffilMember)
  {
    FMultiChat->setAffiliation(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_MEMBER);
  }
  else if (action == FSetAffilAdmin)
  {
    FMultiChat->setAffiliation(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_ADMIN);
  }
  else if (action == FSetAffilOwner)
  {
    FMultiChat->setAffiliation(FModeratorUtilsMenu->menuAction()->data(ADR_USER_NICK).toString(),MUC_AFFIL_OWNER);
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

void MultiUserChatWindow::onUserItemActivated(const QModelIndex &AIndex)
{
  IMultiUser *user = FUsers.key(FUsersModel->itemFromIndex(FUsersProxy->mapToSource(AIndex)));
  if (user)
    openWindow(streamJid(),user->contactJid(),Message::Chat);
}

void MultiUserChatWindow::onStatusIconsChanged()
{
  foreach(IChatWindow *window, FChatWindows) {
    updateChatWindow(window); }
  foreach(IMultiUser *user, FUsers.keys()) {
    updateListItem(user->contactJid()); }
  updateWindow();
}

void MultiUserChatWindow::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  if (AName == AVN_NAME)
    ui.lblAccount->setText(Qt::escape(AValue.toString()));
}
