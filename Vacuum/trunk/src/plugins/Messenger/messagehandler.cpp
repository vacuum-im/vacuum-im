#include "messagehandler.h"

#define IN_NORMAL_MESSAGE                     "psi/sendMessage"
#define IN_CHAT_MESSAGE                       "psi/start-chat"

MessageHandler::MessageHandler(IMessenger *AMessenger, QObject *AParent) : QObject(AParent)
{
  FStatusIcons = NULL;
  FPresencePlugin = NULL;
  FMessenger = AMessenger;
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

bool MessageHandler::notifyOptions(const Message &AMessage, QIcon &AIcon, QString &AToolTip, int &AFlags)
{
  Jid fromJid = AMessage.from();
  SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  if (AMessage.type() == Message::Chat)
    AIcon = iconset->iconByName(IN_CHAT_MESSAGE);
  else
    AIcon = iconset->iconByName(IN_NORMAL_MESSAGE);
  AToolTip = tr("Message from %1%2").arg(fromJid.bare()).arg(!fromJid.resource().isEmpty() ? "/"+fromJid.resource() : "");
  AFlags = IRostersView::LabelBlink|IRostersView::LabelVisible;
  return true;
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
      connect(FPresencePlugin->instance(),SIGNAL(presenceItem(IPresence *, IPresenceItem *)),SLOT(onPresenceItem(IPresence *, IPresenceItem *)));
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
      window->infoWidget()->setFieldVisible(IInfoWidget::ContactStatus,false);
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

void MessageHandler::showChatWindow(IChatWindow *AWindow)
{
  if (AWindow->isWindow() && FMessenger->checkOption(IMessenger::OpenChatInTabWindow))
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
    message.setFrom(window->streamJid().eFull()).setTo(window->contactJid().eFull()).setType(Message::Chat);
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
    message.setFrom(window->streamJid().eFull()).setType(Message::Normal);
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

void MessageHandler::onPresenceItem(IPresence *APresence, IPresenceItem *APresenceItem)
{
  Jid streamJid = APresence->streamJid();
  Jid contactJid = APresenceItem->jid();
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

