#include "messagehandler.h"

#define IN_NORMAL_MESSAGE                     "psi/sendMessage"
#define IN_CHAT_MESSAGE                       "psi/start-chat"

#define HISTORY_MESSAGES                      10

#define CHAT_NOTIFICATOR_ID                   "ChatMessages"
#define NORMAL_NOTIFICATOR_ID                 "NormalMessages"


MessageHandler::MessageHandler(IMessenger *AMessenger, QObject *AParent) : QObject(AParent)
{
  FMessenger = AMessenger;
  FStatusIcons = NULL;
  FPresencePlugin = NULL;
  FMessageArchiver = NULL;
  FVCardPlugin = NULL;
  initialize();
}

MessageHandler::~MessageHandler()
{

}

bool MessageHandler::openWindow(IRosterIndex *AIndex)
{
  if (AIndex->type() == RIT_Contact || AIndex->type() == RIT_MyResource)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    return openWindow(streamJid,contactJid,Message::Chat);
  }
  return false;
}

bool MessageHandler::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType)
{
  if (AType == Message::Chat)
  {
    IChatWindow *window = getChatWindow(AStreamJid,AContactJid);
    if (window)
    {
      showChatWindow(window);
      return true;
    }
  }
  else
  {
    IMessageWindow *window = getMessageWindow(AStreamJid,AContactJid,IMessageWindow::WriteMode);
    if (window)
    {
      showMessageWindow(window);
      return true;
    }
  }
  return false;
}

bool MessageHandler::checkMessage(const Message &AMessage)
{
  if (!AMessage.body().isEmpty() || !AMessage.subject().isEmpty())
    return true;
  return false;
}

INotification MessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
  SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  QIcon icon =  iconset->iconByName(AMessage.type() == Message::Chat ? IN_CHAT_MESSAGE : IN_NORMAL_MESSAGE);
  QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

  INotification notify;
  notify.kinds = ANotifications->notificatorKinds(AMessage.type()==Message::Normal ? NORMAL_NOTIFICATOR_ID : CHAT_NOTIFICATOR_ID);
  notify.data.insert(NDR_ICON,icon);
  notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
  notify.data.insert(NDR_ROSTER_STREAM_JID,AMessage.to());
  notify.data.insert(NDR_ROSTER_CONTACT_JID,AMessage.from());
  notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
  notify.data.insert(NDR_WINDOW_IMAGE,ANotifications->contactAvatar(AMessage.from()));
  notify.data.insert(NDR_WINDOW_CAPTION, tr("Message received"));
  notify.data.insert(NDR_WINDOW_TITLE,name);
  notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
  notify.data.insert(NDR_SOUNDSET_DIR_NAME,SSD_COMMON);
  notify.data.insert(NDR_SOUND_NAME,SN_COMMON_MESSAGE);

  return notify;
}

void MessageHandler::receiveMessage(int AMessageId)
{
  Message message = FMessenger->messageById(AMessageId);
  Jid streamJid = message.to();
  Jid contactJid = message.from();
  if (message.type() == Message::Chat)
  {
    IChatWindow *window = getChatWindow(streamJid,contactJid);
    if (window)
    {
      window->showMessage(message);
      if (!window->isActive())
      {
        FActiveChatMessages.insertMulti(window, AMessageId);
        updateChatWindow(window);
      }
      else
        FMessenger->removeMessage(AMessageId);
    }
  }
  else
  {
    IMessageWindow *window = findMessageWindow(streamJid,contactJid);
    if (window)
    {
      FActiveNormalMessages.insertMulti(window,AMessageId);
      updateMessageWindow(window);
    }
    else
      FActiveNormalMessages.insertMulti(NULL,AMessageId);
  }
}

void MessageHandler::showMessage(int AMessageId)
{
  Message message = FMessenger->messageById(AMessageId);
  Jid streamJid = message.to();
  Jid contactJid = message.from();
  openWindow(streamJid,contactJid,message.type());
}

void MessageHandler::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IMessageArchiver").value(0,NULL);
  if (plugin)
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());

  plugin = FMessenger->pluginManager()->getPlugins("INotifications").value(0,NULL);
  if (plugin)
  {
    INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
    if (notifications)
    {
      uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
      notifications->insertNotificator(NORMAL_NOTIFICATOR_ID,tr("Single Messages"),kindMask,kindMask);
      notifications->insertNotificator(CHAT_NOTIFICATOR_ID,tr("Chat Messages"),kindMask,kindMask);
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
    }
  }
}

IMessageWindow *MessageHandler::getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
  IMessageWindow *window = NULL;
  if (AStreamJid.isValid() && (AContactJid.isValid() || AMode == IMessageWindow::WriteMode))
  {
    window = FMessenger->newMessageWindow(AStreamJid,AContactJid,AMode);
    if (window)
    {
      connect(window,SIGNAL(messageReady()),SLOT(onMessageWindowSend()));
      connect(window,SIGNAL(showNextMessage()),SLOT(onMessageWindowShowNext()));
      connect(window,SIGNAL(replyMessage()),SLOT(onMessageWindowReply()));
      connect(window,SIGNAL(forwardMessage()),SLOT(onMessageWindowForward()));
      connect(window,SIGNAL(showChatWindow()),SLOT(onMessageWindowShowChat()));
      connect(window,SIGNAL(windowDestroyed()),SLOT(onMessageWindowDestroyed()));
      FMessageWindows.append(window);
      //window->infoWidget()->setFieldVisible(IInfoWidget::ContactStatus,false);
      window->infoWidget()->autoUpdateFields();
      loadActiveNormalMessages(window);
      showNextNormalMessage(window);
    }
    else
      window = findMessageWindow(AStreamJid,AContactJid);
  }
  return window;
}

IMessageWindow *MessageHandler::findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IMessageWindow *window,FMessageWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void MessageHandler::showMessageWindow(IMessageWindow *AWindow)
{
  AWindow->showWindow();
}

void MessageHandler::showNextNormalMessage(IMessageWindow *AWindow)
{
  if (FActiveNormalMessages.contains(AWindow))
  {
    int messageId = FActiveNormalMessages.value(AWindow);
    Message message = FMessenger->messageById(messageId);
    AWindow->showMessage(message);
    FMessenger->removeMessage(messageId);
    FActiveNormalMessages.remove(AWindow,messageId);
  }
  updateMessageWindow(AWindow);
}

void MessageHandler::loadActiveNormalMessages(IMessageWindow *AWindow)
{
  QList<int> messagesId = FActiveNormalMessages.values(NULL);
  foreach(int messageId, messagesId)
  {
    Message message = FMessenger->messageById(messageId);
    if (AWindow->streamJid() == message.to() && AWindow->contactJid() == message.from())
    {
      FActiveNormalMessages.insertMulti(AWindow,messageId);
      FActiveNormalMessages.remove(NULL,messageId);
    }
  }
}

void MessageHandler::updateMessageWindow(IMessageWindow *AWindow)
{
  QIcon icon;
  if (FActiveNormalMessages.contains(AWindow))
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    icon = iconset->iconByName(IN_NORMAL_MESSAGE);
  }
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());
  
  QString title = tr("Composing message");
  if (AWindow->mode() == IMessageWindow::ReadMode)
    title = tr("%1 - Message").arg(AWindow->infoWidget()->field(IInfoWidget::ContactName).toString());
  AWindow->updateWindow(icon,title,title);
  AWindow->setNextCount(FActiveNormalMessages.count(AWindow));
}

IChatWindow *MessageHandler::getChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  IChatWindow *window = NULL;
  if (AStreamJid.isValid() && AContactJid.isValid())
  {
    window = FMessenger->newChatWindow(AStreamJid,AContactJid);
    if (window)
    {
      connect(window,SIGNAL(messageReady()),SLOT(onChatMessageSend()));
      connect(window,SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
      connect(window->infoWidget(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
        SLOT(onChatInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
      connect(window,SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
      FChatWindows.append(window);
      window->infoWidget()->autoUpdateFields();
      window->viewWidget()->setNickForJid(AContactJid.bare(),window->viewWidget()->nickForJid(AContactJid));
      window->viewWidget()->setColorForJid(AContactJid.bare(),window->viewWidget()->colorForJid(AContactJid));
      IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AStreamJid.bare()) : NULL;
      if (vcard)
      {
        QString nickName = vcard->value(VVN_NICKNAME);
        if (!nickName.isEmpty())
        {
          window->viewWidget()->setNickForJid(AStreamJid,nickName);
          window->viewWidget()->setColorForJid(AStreamJid,window->viewWidget()->colorForJid(AStreamJid));
          window->viewWidget()->setNickForJid(AStreamJid.bare(),nickName);
          window->viewWidget()->setColorForJid(AStreamJid.bare(),window->viewWidget()->colorForJid(AStreamJid));
        }
        vcard->unlock();
      }
      showChatHistory(window);
      updateChatWindow(window);
    }
    else
      window = findChatWindow(AStreamJid,AContactJid);
  }
  return window;
}

IChatWindow *MessageHandler::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IChatWindow *window,FChatWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void MessageHandler::showChatHistory(IChatWindow *AWindow)
{
  if (FMessageArchiver)
  {
    IArchiveRequest request;
    request.with = AWindow->contactJid();
    request.count = HISTORY_MESSAGES;
    request.order = Qt::DescendingOrder;
    QList<Message> history = FMessageArchiver->findLocalMessages(AWindow->streamJid(),request);
    qSort(history);
    foreach (Message message, history)
    {
      if (AWindow->viewWidget()->nickForJid(message.from()).isNull())
      {
        AWindow->viewWidget()->setNickForJid(message.from(),FMessageArchiver->gateNick(AWindow->streamJid(),message.from()));
        AWindow->viewWidget()->setColorForJid(message.from(),AWindow->viewWidget()->colorForJid(AWindow->contactJid()));
      }
      AWindow->showMessage(message);
    }
  }
}

void MessageHandler::showChatWindow(IChatWindow *AWindow)
{
  if (AWindow->isWindow() && FMessenger->checkOption(IMessenger::UseTabWindow))
  {
    ITabWindow *tabWindow = FMessenger->openTabWindow();
    tabWindow->addWidget(AWindow);
  }
  AWindow->showWindow();
}

void MessageHandler::removeActiveChatMessages(IChatWindow *AWindow)
{
  if (FActiveChatMessages.contains(AWindow))
  {
    QList<int> messageIds = FActiveChatMessages.values(AWindow);
    foreach(int messageId, messageIds)
      FMessenger->removeMessage(messageId);
    FActiveChatMessages.remove(AWindow);
    updateChatWindow(AWindow);
  }
}

void MessageHandler::updateChatWindow(IChatWindow *AWindow)
{
  QIcon icon;
  if (FActiveChatMessages.contains(AWindow))
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    icon = iconset->iconByName(IN_CHAT_MESSAGE);
  }
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

  QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
  AWindow->updateWindow(icon,contactName,tr("%1 - Chat").arg(contactName));
}

void MessageHandler::onChatMessageSend()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    Message message;
    message.setTo(window->contactJid().eFull()).setType(Message::Chat);
    FMessenger->textToMessage(message,window->editWidget()->document());
    if (!message.body().isEmpty() && FMessenger->sendMessage(message,window->streamJid()))
    {
      window->viewWidget()->showMessage(message);
      window->editWidget()->clearEditor();
    }
  }
}

void MessageHandler::onChatWindowActivated()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
    removeActiveChatMessages(window);
}

void MessageHandler::onChatInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
  if (AField == IInfoWidget::ContactName) 
  {
    IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
    IChatWindow *window = widget!=NULL ? FMessenger->findChatWindow(widget->streamJid(),widget->contactJid()) : NULL;
    if (window)
    {
      Jid streamJid = window->streamJid();
      Jid contactJid = window->contactJid();
      QString selfName = streamJid && contactJid ? streamJid.resource() : streamJid.node();
      window->viewWidget()->setNickForJid(streamJid,selfName);
      window->viewWidget()->setNickForJid(contactJid,AValue.toString());
      updateChatWindow(window);
    }
  }
}

void MessageHandler::onChatWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (FChatWindows.contains(window))
  {
    removeActiveChatMessages(window);
    FChatWindows.removeAt(FChatWindows.indexOf(window));
  }
}

void MessageHandler::onMessageWindowSend()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    Message message;
    message/*.setFrom(window->streamJid().eFull())*/.setType(Message::Normal);
    message.setSubject(window->subject());
    message.setThreadId(window->threadId());
    FMessenger->textToMessage(message,window->editWidget()->document());
    if (!message.body().isEmpty())
    {
      bool sended = false;
      QList<Jid> receiversList = window->receiversWidget()->receivers();
      foreach(Jid receiver, receiversList)
      {
        message.setTo(receiver.eFull());
        sended = FMessenger->sendMessage(message,window->streamJid()) ? true : sended;
      }
      if (sended)
      {
        if (FActiveNormalMessages.contains(window))
          showNextNormalMessage(window);
        else
          window->close();
      }
    }
  }
}

void MessageHandler::onMessageWindowShowNext()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    showNextNormalMessage(window);
    updateMessageWindow(window);
  }
}

void MessageHandler::onMessageWindowReply()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Re: ")+window->subject());
    window->editWidget()->clearEditor();
    window->editWidget()->setFocus();
    updateMessageWindow(window);
  }
}

void MessageHandler::onMessageWindowForward()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Fw: ")+window->subject());
    window->setThreadId(window->currentMessage().threadId());
    FMessenger->messageToText(window->editWidget()->document(),window->currentMessage());
    window->receiversWidget()->clear();
    window->setCurrentTabWidget(window->receiversWidget());
    updateMessageWindow(window);
  }
}

void MessageHandler::onMessageWindowShowChat()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
    openWindow(window->streamJid(),window->contactJid(),Message::Chat);
}

void MessageHandler::onMessageWindowDestroyed()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (FMessageWindows.contains(window))
  {
    QList<int> messagesId = FActiveNormalMessages.values(window);
    foreach(int messageId, messagesId)
      FActiveNormalMessages.insertMulti(NULL,messageId);
    FActiveNormalMessages.remove(window);
    FMessageWindows.removeAt(FMessageWindows.indexOf(window));
  }
}

void MessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  Jid streamJid = APresence->streamJid();
  Jid contactJid = APresenceItem.itemJid;
  IChatWindow *chatWindow = findChatWindow(streamJid,contactJid);
  if (!chatWindow && !contactJid.resource().isEmpty())
  {
    chatWindow = findChatWindow(streamJid,contactJid.bare());
    if (chatWindow)
      chatWindow->setContactJid(contactJid);
  }
  else if (chatWindow && !contactJid.resource().isEmpty())
  {
    IChatWindow *bareWindow = findChatWindow(streamJid,contactJid.bare());
    if (bareWindow)
      bareWindow->deleteLater();
  }
  if (chatWindow)
    updateChatWindow(chatWindow);

  IMessageWindow *messageWindow = findMessageWindow(streamJid,contactJid);
  if (messageWindow)
    updateMessageWindow(messageWindow);
}

void MessageHandler::onStatusIconsChanged()
{
  foreach(IChatWindow *window, FChatWindows)
    updateChatWindow(window);
  foreach(IMessageWindow *window, FMessageWindows)
    updateMessageWindow(window);
}

void MessageHandler::onVCardChanged(const Jid &AContactJid)
{
  IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AContactJid.bare()) : NULL;
  if (vcard)
  {
    QString nickName = vcard->value(VVN_NICKNAME);
    foreach(IChatWindow *window, FMessenger->chatWindows())
    {
      if (window->streamJid() && AContactJid)
      {
        window->viewWidget()->setNickForJid(window->streamJid(),nickName);
        window->viewWidget()->setNickForJid(window->streamJid().bare(),nickName);
      }
    }
    vcard->unlock();
  }
}

