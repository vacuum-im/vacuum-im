#include "multiuserchatwindow.h"

#include <QTimer>
#include <QContextMenuEvent>

#define IN_EXIT                     "psi/cancel"
#define IN_CHAT_MESSAGE             "psi/start-chat"
#define IN_MULTICHAT_MESSAGE        "psi/groupChat"

#define SVN_WINDOW                  "windows:window[]"
#define SVN_WINDOW_GEOMETRY         SVN_WINDOW ":geometry"
#define SVN_WINDOW_HSPLITTER        SVN_WINDOW ":hsplitter"  
#define SVN_WINDOW_VSPLITTER        SVN_WINDOW ":vsplitter"  

MultiUserChatWindow::MultiUserChatWindow(IMessenger *AMessenger, IMultiUserChat *AMultiChat)
{
  ui.setupUi(this);
  ui.lblRoom->setText(AMultiChat->roomJid().hFull());
  ui.lblNick->setText(AMultiChat->nickName());
  ui.ltwUsers->installEventFilter(this);

  FSettings = NULL;
  FStatusIcons = NULL;

  FMessenger = AMessenger;
  FMultiChat = AMultiChat;
  FMultiChat->instance()->setParent(this);

  FSplitterLoaded = false;
  FExitOnChatClosed = false;

  FViewWidget = FMessenger->newViewWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
  FViewWidget->setShowKind(IViewWidget::GroupChatMessage);
  FViewWidget->document()->setDefaultFont(FMessenger->defaultChatFont());
  ui.wdtView->setLayout(new QVBoxLayout);
  ui.wdtView->layout()->addWidget(FViewWidget);
  ui.wdtView->layout()->setMargin(0);
  ui.wdtView->layout()->setSpacing(0);

  FEditWidget = FMessenger->newEditWidget(FMultiChat->streamJid(),FMultiChat->roomJid());
  FEditWidget->document()->setDefaultFont(FMessenger->defaultChatFont());
  ui.wdtEdit->setLayout(new QVBoxLayout);
  ui.wdtEdit->layout()->addWidget(FEditWidget);
  ui.wdtEdit->layout()->setMargin(0);
  ui.wdtEdit->layout()->setSpacing(0);
  connect(FEditWidget,SIGNAL(messageReady()),SLOT(onMessageSend()));

  FToolBarWidget = FMessenger->newToolBarWidget(NULL,FViewWidget,FEditWidget,NULL);
  ui.wdtToolBar->setLayout(new QVBoxLayout);
  ui.wdtToolBar->layout()->addWidget(FToolBarWidget);
  ui.wdtToolBar->layout()->setMargin(0);
  ui.wdtToolBar->layout()->setSpacing(0);

  Action *action = new Action(FToolBarWidget);
  action->setIcon(SYSTEM_ICONSETFILE,IN_EXIT);
  action->setText(tr("Exit"));
  action->setToolTip(tr("Exit groupchat"));
  connect(action,SIGNAL(triggered(bool)),SLOT(onExitMultiUserChat(bool)));
  FToolBarWidget->toolBarChanger()->addToolButton(action,Qt::ToolButtonTextBesideIcon,QToolButton::InstantPopup,AG_MAINWINDOW_MMENU_QUIT);

  connect(FMultiChat->instance(),SIGNAL(chatOpened()),SLOT(onChatOpened()));
  connect(FMultiChat->instance(),SIGNAL(userPresence(IMultiUser *,int,const QString &)),
    SLOT(onUserPresence(IMultiUser *,int,const QString &)));
  connect(FMultiChat->instance(),SIGNAL(userDataChanged(IMultiUser *, int, const QVariant, const QVariant &)),
    SLOT(onUserDataChanged(IMultiUser *, int, const QVariant, const QVariant &)));
  connect(FMultiChat->instance(),SIGNAL(userNickChanged(IMultiUser *, const QString &, const QString &)),
    SLOT(onUserNickChanged(IMultiUser *, const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(presenceChanged(int, const QString &)),
    SLOT(onPresenceChanged(int, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(topicChanged(const QString &)),SLOT(onTopicChanged(const QString &)));
  connect(FMultiChat->instance(),SIGNAL(messageReceived(const QString &, const Message &)),
    SLOT(onMessageReceived(const QString &, const Message &)));
  connect(FMultiChat->instance(),SIGNAL(chatError(const QString &, const QString &)),
    SLOT(onChatError(const QString &, const QString &)));
  connect(FMultiChat->instance(),SIGNAL(chatClosed()),SLOT(onChatClosed()));
  connect(FMultiChat->instance(),SIGNAL(streamJidChanged(const Jid &, const Jid &)),SLOT(onStreamJidChanged(const Jid &, const Jid &)));

  connect(FMessenger->instance(),SIGNAL(defaultChatFontChanged(const QFont &)), SLOT(onDefaultChatFontChanged(const QFont &)));
  
  connect(ui.ltwUsers,SIGNAL(itemActivated(QListWidgetItem *)),SLOT(onListItemActivated(QListWidgetItem *)));

  connect(this,SIGNAL(windowActivated()),SLOT(onWindowActivated()));

  initialize();
}

MultiUserChatWindow::~MultiUserChatWindow()
{
  exitMultiUserChat();
  foreach(IChatWindow *window,FChatWindows)
    window->deleteLater();
  FMessenger->removeMessageHandler(this,MHO_MULTIUSERCHAT);
  emit windowDestroyed();
}

void MultiUserChatWindow::showWindow()
{
  if (isWindow() && !isVisible())
  {
    if (FMessenger->checkOption(IMessenger::OpenChatInTabWindow))
    {
      ITabWindow *tabWindow = FMessenger->openTabWindow();
      tabWindow->addWidget(this);
    }
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

bool MultiUserChatWindow::openWindow(IRosterIndex * /*AIndex*/)
{
  return false;
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

bool MultiUserChatWindow::checkMessage(const Message &AMessage)
{
  return (streamJid() == AMessage.to()) && (roomJid() && AMessage.from());
}

bool MultiUserChatWindow::notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags)
{
  if (AMessage.type() == Message::GroupChat)
  {
    if (!isActive())
    {
      SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
      AIcon = iconset->iconByName(IN_MULTICHAT_MESSAGE);
      AToolTip = tr("Received groupchat message");
      AFlags = 0;
      return true;
    }
  }
  else 
  {
    IChatWindow *window = findChatWindow(AMessage.from());
    if (window == NULL || !window->isActive())
    {
      SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
      AIcon = iconset->iconByName(IN_CHAT_MESSAGE);
      AToolTip = tr("Received private message");
      AFlags = 0;
      return true;
    }
  }
  return false;
}

void MultiUserChatWindow::receiveMessage(int AMessageId)
{
  Message message = FMessenger->messageById(AMessageId);
  Jid contactJid = message.from();
  if (message.type() == Message::GroupChat)
  {
    if (!isActive())
    {
      FActiveMessages.append(AMessageId);
      updateWindow();
    }
    else
      FMessenger->removeMessage(AMessageId);
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
        FMessenger->removeMessage(AMessageId);
    }
  }
}

void MultiUserChatWindow::showMessage(int AMessageId)
{
  Message message = FMessenger->messageById(AMessageId);
  openWindow(message.to(),message.from(),message.type());
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

void MultiUserChatWindow::exitMultiUserChat()
{
  if (FMultiChat->isOpen())
  {
    FExitOnChatClosed = true;
    FMultiChat->setPresence(IPresence::Offline,tr("Disconnected"));
    closeWindow();
    QTimer::singleShot(5000,this,SLOT(deleteLater()));
  }
  else
    deleteLater();
}

void MultiUserChatWindow::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IAccountManager").value(0,NULL);
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

  FMessenger->insertMessageHandler(this,MHO_MULTIUSERCHAT);
}

void MultiUserChatWindow::saveWindowState()
{
  if (FSettings)
  {
    QString valueNameNS = roomJid().pBare();
    if (isWindow() && isVisible())
      FSettings->setValueNS(SVN_WINDOW_GEOMETRY,valueNameNS,saveGeometry());
    FSettings->setValueNS(SVN_WINDOW_HSPLITTER,valueNameNS,ui.sprHSplitter->saveState());
    FSettings->setValueNS(SVN_WINDOW_VSPLITTER,valueNameNS,ui.sprVSplitter->saveState());
  }
}

void MultiUserChatWindow::loadWindowState()
{
  QString valueNameNS = roomJid().pBare();
  if (isWindow())
    restoreGeometry(FSettings->valueNS(SVN_WINDOW_GEOMETRY,valueNameNS).toByteArray());
  if (!FSplitterLoaded)
  {
    ui.sprHSplitter->restoreState(FSettings->valueNS(SVN_WINDOW_HSPLITTER,valueNameNS).toByteArray());
    ui.sprVSplitter->restoreState(FSettings->valueNS(SVN_WINDOW_VSPLITTER,valueNameNS).toByteArray());
    FSplitterLoaded = true;
  }
  FEditWidget->textEdit()->setFocus();
}

void MultiUserChatWindow::showServiceMessage(const QString &AMessage)
{
  QString html = QString("<span style='color:green;'>*** %1</span>").arg(AMessage);
  FViewWidget->showCustomMessage(html,QDateTime::currentDateTime());
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
    if (!AUser->data(MUDR_REALJID).toString().isEmpty())
      toolTip += AUser->data(MUDR_REALJID).toString()+"<br>";
    toolTip += tr("Role: %1<br>").arg(AUser->data(MUDR_ROLE).toString());
    toolTip += tr("Affiliation: %1<br>").arg(AUser->data(MUDR_AFFILIATION).toString());
    toolTip += tr("Status: %1").arg(AUser->data(MUDR_STATUS).toString());
    listItem->setToolTip(toolTip);
  }
}

void MultiUserChatWindow::updateWindow()
{
  QIcon icon;
  SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  if (!FActiveMessages.isEmpty())
    icon = iconset->iconByName(IN_CHAT_MESSAGE);
  else
    icon = iconset->iconByName(IN_MULTICHAT_MESSAGE);

  setWindowIcon(icon);
  setWindowIconText(FMultiChat->roomJid().node());
  setWindowTitle(tr("%1 - Groupchat").arg(FMultiChat->roomJid().node()));

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
    {
      SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
      listItem->setIcon(iconset->iconByName(IN_CHAT_MESSAGE));
    }
    else if (FStatusIcons)
      listItem->setIcon(FStatusIcons->iconByJidStatus(AContactJid,user->data(MUDR_SHOW).toInt(),"",false));
  }
}

void MultiUserChatWindow::removeActiveMessages()
{
  foreach(int messageId, FActiveMessages)
    FMessenger->removeMessage(messageId);
  FActiveMessages.clear();
  updateWindow();
}

IChatWindow *MultiUserChatWindow::getChatWindow(const Jid &AContactJid)
{
  IChatWindow *window = NULL;
  IMultiUser *user = FMultiChat->userByNick(AContactJid.resource());
  if (user && user != FMultiChat->mainUser())
  {
    window = FMessenger->newChatWindow(streamJid(),AContactJid);
    if (window)
    {
      connect(window,SIGNAL(messageReady()),SLOT(onChatMessageSend()));
      connect(window,SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
      connect(window,SIGNAL(windowClosed()),SLOT(onChatWindowClosed()));
      connect(window,SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
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
    else
      window = findChatWindow(AContactJid);
  }
  return window;
}

void MultiUserChatWindow::showChatWindow(IChatWindow *AWindow)
{
  if (AWindow->isWindow() && FMessenger->checkOption(IMessenger::OpenChatInTabWindow))
  {
    ITabWindow *tabWindow = FMessenger->openTabWindow();
    tabWindow->addWidget(AWindow);
  }
  AWindow->showWindow();
}

void MultiUserChatWindow::removeActiveChatMessages(IChatWindow *AWindow)
{
  if (FActiveChatMessages.contains(AWindow))
  {
    QList<int> messageIds = FActiveChatMessages.values(AWindow);
    foreach(int messageId, messageIds)
      FMessenger->removeMessage(messageId);
    FActiveChatMessages.remove(AWindow);
    updateChatWindow(AWindow);
    updateListItem(AWindow->contactJid());
  }
}

void MultiUserChatWindow::updateChatWindow(IChatWindow *AWindow)
{
  QIcon icon;
  if (FActiveChatMessages.contains(AWindow))
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    icon = iconset->iconByName(IN_CHAT_MESSAGE);
  }
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
        emit multiUserContextMenu(user,menu);
        if (!menu->isEmpty())
          menu->popup(menuEvent->globalPos());
        else
          delete menu;
      }
    }
  }
  return IMultiUserChatWindow::eventFilter(AObject,AEvent);
}

void MultiUserChatWindow::onChatOpened()
{
  FViewWidget->textBrowser()->clear();
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
      
      QString message = tr("%1 (%2) has joined the room: %3");
      message = message.arg(AUser->nickName());
      message = message.arg(AUser->data(MUDR_REALJID).toString());
      message = message.arg(AStatus);
      showServiceMessage(message);
    }
    setToolTipForUser(AUser);
    updateListItem(AUser->contactJid());
  }
  else if (listItem)
  {
    QString message = tr("%1 (%2) has left the room: %3");
    message = message.arg(AUser->nickName());
    message = message.arg(AUser->data(MUDR_REALJID).toString());
    message = message.arg(AStatus.isEmpty() ? tr("Disconnected") : AStatus);
    showServiceMessage(message);

    FUsers.remove(AUser);
    ui.ltwUsers->takeItem(ui.ltwUsers->row(listItem));
    delete listItem;
  }

  IChatWindow *window = findChatWindow(AUser->contactJid());
  if (window)
  {
    window->infoWidget()->setField(IInfoWidget::ContactShow,AShow);
    window->infoWidget()->setField(IInfoWidget::ContactStatus,AStatus);
    updateChatWindow(window);
  }
}

void MultiUserChatWindow::onUserDataChanged(IMultiUser *AUser, int ARole, const QVariant &/*ABefour*/, const QVariant &/*AAfter*/)
{
  if (ARole == MUDR_ROLE)
    setRoleColorForUser(AUser);
  else if (ARole == MUDR_AFFILIATION)
    setAffilationLineForUser(AUser);
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
    ui.lblNick->setText(ANewNick);
  showServiceMessage(tr("%1 changed nick to %2").arg(AOldNick).arg(ANewNick));
}

void MultiUserChatWindow::onPresenceChanged(int /*AShow*/, const QString &/*AStatus*/)
{

}

void MultiUserChatWindow::onTopicChanged(const QString &ATopic)
{
  QString html = QString("<span style='color:gray;'>The topic has been set to: %1</span>").arg(ATopic);
  FViewWidget->showCustomMessage(html,QDateTime::currentDateTime());
}

void MultiUserChatWindow::onMessageReceived(const QString &/*ANick*/, const Message &AMessage)
{
  if (AMessage.type() == Message::GroupChat)
    FViewWidget->showMessage(AMessage);
  else
  {
    IChatWindow *window = getChatWindow(AMessage.from());
    if (window)
      window->viewWidget()->showMessage(AMessage);
  }
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
  FExitOnChatClosed ? deleteLater() : showServiceMessage(tr("Disconnected"));
}

void MultiUserChatWindow::onMessageSend()
{
  if (FMultiChat->isOpen())
  {
    Message message;
    FMessenger->textToMessage(message,FEditWidget->document());
    if (!message.body().isEmpty() && FMultiChat->sendMessage(message))
      FEditWidget->clearEditor();
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
    FMessenger->textToMessage(message,window->editWidget()->document());
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
    window->deleteLater();
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
  if (AName == "name")
    ui.lblAccount->setText(Qt::escape(AValue.toString()));
}

void MultiUserChatWindow::onStreamJidChanged(const Jid &/*ABefour*/, const Jid &AAfter)
{
  FViewWidget->setStreamJid(AAfter);
  FEditWidget->setStreamJid(AAfter);
}

void MultiUserChatWindow::onExitMultiUserChat(bool)
{
  exitMultiUserChat();
}

