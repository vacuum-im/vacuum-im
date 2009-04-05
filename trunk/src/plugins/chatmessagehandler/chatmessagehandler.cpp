#include "chatmessagehandler.h"

#define HISTORY_MESSAGES          10

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1

#define CHAT_NOTIFICATOR_ID       "ChatMessages"

ChatMessageHandler::ChatMessageHandler()
{
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;
  FStatusIcons = NULL;
  FPresencePlugin = NULL;
  FMessageArchiver = NULL;
  FVCardPlugin = NULL;
  FRostersView = NULL;
}

ChatMessageHandler::~ChatMessageHandler()
{

}

void ChatMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Chat message handling");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Chat Message Handler"); 
  APluginInfo->uid = CHATMESSAGEHANDLER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
  APluginInfo->conflicts.append("{153A4638-B468-496f-B57C-9F30CEDFCC2E}");  //Messenger
  APluginInfo->conflicts.append("{f118ccf4-8535-4302-8fda-0f6487c6db01}");  //MessageHandler
}


bool ChatMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IMessageWidgets").value(0,NULL);
  if (plugin)
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMessageProcessor").value(0,NULL);
  if (plugin)
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
    }
  }

  plugin = APluginManager->getPlugins("IMessageArchiver").value(0,NULL);
  if (plugin)
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin)
  {
    INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
    if (notifications)
    {
      uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
      notifications->insertNotificator(CHAT_NOTIFICATOR_ID,tr("Chat Messages"),kindMask,kindMask);
    }
  }

  plugin = APluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardChanged(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardPublished(const Jid &)),SLOT(onVCardChanged(const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin) 
  {
    IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (rostersViewPlugin)
    {
      FRostersView = rostersViewPlugin->rostersView();
      connect(FRostersView->instance(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    }
  }

  return FMessageProcessor!=NULL && FMessageWidgets!=NULL;
}

bool ChatMessageHandler::initObjects()
{
  if (FRostersView)
  {
    FRostersView->insertClickHooker(RCHO_CHATMESSAGEHANDLER,this);
  }
  if (FMessageProcessor)
  {
    FMessageProcessor->insertMessageHandler(this,MHO_CHATMESSAGEHANDLER);
  }
  return true;
}

bool ChatMessageHandler::rosterIndexClicked(IRosterIndex *AIndex, int /*AOrder*/)
{
  if (AIndex->type()==RIT_CONTACT || AIndex->type()==RIT_MY_RESOURCE)
  {
    Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
    Jid contactJid = AIndex->data(RDR_JID).toString();
    return openWindow(streamJid,contactJid,Message::Chat);
  }
  return false;
}

bool ChatMessageHandler::checkMessage(const Message &AMessage)
{
  if (AMessage.type()==Message::Chat && !AMessage.body().isEmpty())
    return true;
  return false;
}

void ChatMessageHandler::showMessage(int AMessageId)
{
  Message message = FMessageProcessor->messageById(AMessageId);
  Jid streamJid = message.to();
  Jid contactJid = message.from();
  openWindow(streamJid,contactJid,message.type());
}

void ChatMessageHandler::receiveMessage(int AMessageId)
{
  Message message = FMessageProcessor->messageById(AMessageId);
  IChatWindow *window = getChatWindow(message.to(),message.from());
  if (window)
  {
    window->showMessage(message);
    if (!window->isActive())
    {
      FActiveChatMessages.insertMulti(window, AMessageId);
      updateChatWindow(window);
    }
    else
      FMessageProcessor->removeMessage(AMessageId);
  }
}

bool ChatMessageHandler::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType)
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
  return false;
}

INotification ChatMessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
  QIcon icon =  storage->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
  QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

  INotification notify;
  notify.kinds = ANotifications->notificatorKinds(CHAT_NOTIFICATOR_ID);
  notify.data.insert(NDR_ICON,icon);
  notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
  notify.data.insert(NDR_ROSTER_STREAM_JID,AMessage.to());
  notify.data.insert(NDR_ROSTER_CONTACT_JID,AMessage.from());
  notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
  notify.data.insert(NDR_WINDOW_IMAGE,ANotifications->contactAvatar(AMessage.from()));
  notify.data.insert(NDR_WINDOW_CAPTION, tr("Message received"));
  notify.data.insert(NDR_WINDOW_TITLE,name);
  notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
  notify.data.insert(NDR_SOUND_FILE,SDF_CHAT_MHANDLER_MESSAGE);

  return notify;
}

IChatWindow *ChatMessageHandler::getChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  IChatWindow *window = NULL;
  if (AStreamJid.isValid() && AContactJid.isValid())
  {
    window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
    if (window)
    {
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onChatMessageSend()));
      connect(window->instance(),SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
      connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
        SLOT(onChatInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
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

IChatWindow *ChatMessageHandler::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IChatWindow *window,FChatWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void ChatMessageHandler::showChatHistory(IChatWindow *AWindow)
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

void ChatMessageHandler::showChatWindow(IChatWindow *AWindow)
{
  if (AWindow->instance()->isWindow() && !AWindow->instance()->isVisible() && FMessageWidgets->checkOption(IMessageWidgets::UseTabWindow))
  {
    ITabWindow *tabWindow = FMessageWidgets->openTabWindow();
    tabWindow->addWidget(AWindow);
  }
  AWindow->showWindow();
}

void ChatMessageHandler::removeActiveChatMessages(IChatWindow *AWindow)
{
  if (FActiveChatMessages.contains(AWindow))
  {
    QList<int> messageIds = FActiveChatMessages.values(AWindow);
    foreach(int messageId, messageIds)
      FMessageProcessor->removeMessage(messageId);
    FActiveChatMessages.remove(AWindow);
    updateChatWindow(AWindow);
  }
}

void ChatMessageHandler::updateChatWindow(IChatWindow *AWindow)
{
  QIcon icon;
  if (FActiveChatMessages.contains(AWindow))
    icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

  QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
  AWindow->updateWindow(icon,contactName,tr("%1 - Chat").arg(contactName));
}

void ChatMessageHandler::onChatMessageSend()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    Message message;
    message.setTo(window->contactJid().eFull()).setType(Message::Chat);
    FMessageProcessor->textToMessage(message,window->editWidget()->document());
    if (!message.body().isEmpty() && FMessageProcessor->sendMessage(window->streamJid(),message))
    {
      window->viewWidget()->showMessage(message);
      window->editWidget()->clearEditor();
    }
  }
}

void ChatMessageHandler::onChatWindowActivated()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
    removeActiveChatMessages(window);
}

void ChatMessageHandler::onChatInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
  if (AField == IInfoWidget::ContactName) 
  {
    IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
    IChatWindow *window = widget!=NULL ? FMessageWidgets->findChatWindow(widget->streamJid(),widget->contactJid()) : NULL;
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

void ChatMessageHandler::onChatWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (FChatWindows.contains(window))
  {
    removeActiveChatMessages(window);
    FChatWindows.removeAt(FChatWindows.indexOf(window));
  }
}

void ChatMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
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
      bareWindow->instance()->deleteLater();
  }
  if (chatWindow)
    updateChatWindow(chatWindow);
}

void ChatMessageHandler::onStatusIconsChanged()
{
  foreach(IChatWindow *window, FChatWindows)
    updateChatWindow(window);
}

void ChatMessageHandler::onVCardChanged(const Jid &AContactJid)
{
  IVCard *vcard = FVCardPlugin!=NULL ? FVCardPlugin->vcard(AContactJid.bare()) : NULL;
  if (vcard)
  {
    QString nickName = vcard->value(VVN_NICKNAME);
    foreach(IChatWindow *window, FMessageWidgets->chatWindows())
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

void ChatMessageHandler::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  static QList<int> chatActionTypes = QList<int>() << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

  Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
  if (presence && presence->isOpen())
  {
    Jid contactJid = AIndex->data(RDR_JID).toString();
    if (chatActionTypes.contains(AIndex->type()))
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Chat"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
      action->setData(ADR_STREAM_JID,streamJid.full());
      action->setData(ADR_CONTACT_JID,contactJid.full());
      AMenu->addAction(action,AG_RVCM_CHATMESSAGEHANDLER,true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
    }
  }
}

void ChatMessageHandler::onShowWindowAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    openWindow(streamJid,contactJid,Message::Chat);
  }
}

Q_EXPORT_PLUGIN2(ChatMessageHandlerPlugin, ChatMessageHandler)
