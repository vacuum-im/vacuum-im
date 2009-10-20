#include "normalmessagehandler.h"

#define NORMAL_NOTIFICATOR_ID     "NormalMessages"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_GROUP                 Action::DR_Parametr3

NormalMessageHandler::NormalMessageHandler()
{
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;
  FMessageStyles = NULL;
  FStatusIcons = NULL;
  FPresencePlugin = NULL;
  FRostersView = NULL;
}

NormalMessageHandler::~NormalMessageHandler()
{

}

void NormalMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Normal Message Handler"); 
  APluginInfo->description = tr("Normal message handling");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
  APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}

bool NormalMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IMessageWidgets").value(0,NULL);
  if (plugin)
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMessageProcessor").value(0,NULL);
  if (plugin)
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMessageStyles").value(0,NULL);
  if (plugin)
  {
    FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
    if (FMessageStyles)
    {
      connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
        SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
    }
  }

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

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin)
  {
    INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
    if (notifications)
    {
      uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
      notifications->insertNotificator(NORMAL_NOTIFICATOR_ID,tr("Single Messages"),kindMask,kindMask);
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

  return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
}

bool NormalMessageHandler::initObjects()
{
  if (FMessageProcessor)
  {
    FMessageProcessor->insertMessageHandler(this,MHO_NORMALMESSAGEHANDLER);
  }
  return true;
}

bool NormalMessageHandler::checkMessage(const Message &AMessage)
{
  if (!AMessage.body().isEmpty() || !AMessage.subject().isEmpty())
    return true;
  return false;
}

void NormalMessageHandler::showMessage(int AMessageId)
{
  Message message = FMessageProcessor->messageById(AMessageId);
  Jid streamJid = message.to();
  Jid contactJid = message.from();
  openWindow(streamJid,contactJid,message.type());
}

void NormalMessageHandler::receiveMessage(int AMessageId)
{
  Message message = FMessageProcessor->messageById(AMessageId);
  IMessageWindow *window = findWindow(message.to(),message.from());
  if (window)
  {
    FActiveMessages.insertMulti(window,AMessageId);
    updateWindow(window);
  }
  else
    FActiveMessages.insertMulti(NULL,AMessageId);
}

bool NormalMessageHandler::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType /*AType*/)
{
  IMessageWindow *window = getWindow(AStreamJid,AContactJid,IMessageWindow::WriteMode);
  if (window)
  {
    showWindow(window);
    return true;
  }
  return false;
}

INotification NormalMessageHandler::notification(INotifications *ANotifications, const Message &AMessage)
{
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
  QIcon icon =  storage->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
  QString name= ANotifications->contactName(AMessage.to(),AMessage.from());

  INotification notify;
  notify.kinds = ANotifications->notificatorKinds(NORMAL_NOTIFICATOR_ID);
  notify.data.insert(NDR_ICON,icon);
  notify.data.insert(NDR_TOOLTIP,tr("Message from %1").arg(name));
  notify.data.insert(NDR_ROSTER_STREAM_JID,AMessage.to());
  notify.data.insert(NDR_ROSTER_CONTACT_JID,AMessage.from());
  notify.data.insert(NDR_ROSTER_NOTIFY_ORDER,RLO_MESSAGE);
  notify.data.insert(NDR_WINDOW_IMAGE,ANotifications->contactAvatar(AMessage.from()));
  notify.data.insert(NDR_WINDOW_CAPTION, tr("Message received"));
  notify.data.insert(NDR_WINDOW_TITLE,name);
  notify.data.insert(NDR_WINDOW_TEXT,AMessage.body());
  notify.data.insert(NDR_SOUND_FILE,SDF_NORMAL_MHANDLER_MESSAGE);

  return notify;
}

IMessageWindow *NormalMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
  IMessageWindow *window = NULL;
  if (AStreamJid.isValid() && (AContactJid.isValid() || AMode == IMessageWindow::WriteMode))
  {
    window = FMessageWidgets->newMessageWindow(AStreamJid,AContactJid,AMode);
    if (window)
    {
      window->infoWidget()->autoUpdateFields();
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
      connect(window->instance(),SIGNAL(showNextMessage()),SLOT(onShowNextMessage()));
      connect(window->instance(),SIGNAL(replyMessage()),SLOT(onReplyMessage()));
      connect(window->instance(),SIGNAL(forwardMessage()),SLOT(onForwardMessage()));
      connect(window->instance(),SIGNAL(showChatWindow()),SLOT(onShowChatWindow()));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onWindowDestroyed()));
      FWindows.append(window);
      loadActiveMessages(window);
      showNextMessage(window);
    }
    else
      window = findWindow(AStreamJid,AContactJid);
  }
  return window;
}

IMessageWindow *NormalMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IMessageWindow *window,FWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void NormalMessageHandler::showWindow(IMessageWindow *AWindow)
{
  AWindow->showWindow();
}

void NormalMessageHandler::showNextMessage(IMessageWindow *AWindow)
{
  if (FActiveMessages.contains(AWindow))
  {
    int messageId = FActiveMessages.value(AWindow);
    Message message = FMessageProcessor->messageById(messageId);
    showStyledMessage(AWindow,message);
    FLastMessages.insert(AWindow,message);
    FMessageProcessor->removeMessage(messageId);
    FActiveMessages.remove(AWindow,messageId);
  }
  updateWindow(AWindow);
}

void NormalMessageHandler::loadActiveMessages(IMessageWindow *AWindow)
{
  QList<int> messagesId = FActiveMessages.values(NULL);
  foreach(int messageId, messagesId)
  {
    Message message = FMessageProcessor->messageById(messageId);
    if (AWindow->streamJid() == message.to() && AWindow->contactJid() == message.from())
    {
      FActiveMessages.insertMulti(AWindow,messageId);
      FActiveMessages.remove(NULL,messageId);
    }
  }
}

void NormalMessageHandler::updateWindow(IMessageWindow *AWindow)
{
  QIcon icon;
  if (FActiveMessages.contains(AWindow))
    icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

  QString title = tr("Composing message");
  if (AWindow->mode() == IMessageWindow::ReadMode)
    title = tr("%1 - Message").arg(AWindow->infoWidget()->field(IInfoWidget::ContactName).toString());
  AWindow->updateWindow(icon,title,title);
  AWindow->setNextCount(FActiveMessages.count(AWindow));
}

void NormalMessageHandler::setMessageStyle(IMessageWindow *AWindow)
{
  IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Normal);
  IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
  if (style != AWindow->viewWidget()->messageStyle())
    AWindow->viewWidget()->setMessageStyle(style,soptions);
  else if (AWindow->viewWidget()->messageStyle() != NULL)
    AWindow->viewWidget()->messageStyle()->changeOptions(AWindow->viewWidget()->styleWidget(),soptions);
}

void NormalMessageHandler::fillContentOptions(IMessageWindow *AWindow, IMessageContentOptions &AOptions) const
{
  AOptions.senderColor = "blue";
  AOptions.senderId = AWindow->contactJid().full();
  AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid(),AWindow->contactJid()));
  AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->contactJid());
  AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid(),AWindow->contactJid());
}

void NormalMessageHandler::showStyledMessage(IMessageWindow *AWindow, const Message &AMessage)
{
  IMessageContentOptions options;
  options.time = AMessage.dateTime();
  options.timeFormat = FMessageStyles->timeFormat(options.time);
  options.direction = IMessageContentOptions::DirectionIn;
  options.noScroll = true;
  fillContentOptions(AWindow,options);

  AWindow->setMode(IMessageWindow::ReadMode);
  AWindow->setSubject(AMessage.subject());
  AWindow->setThreadId(AMessage.threadId());

  setMessageStyle(AWindow);
  
  if (AMessage.type() == Message::Error)
  {
    ErrorHandler err(AMessage.stanza().element());
    QString html = tr("<b>The message with a error code %1 is received</b>").arg(err.code());
    html += "<p style='color:red;'>"+Qt::escape(err.message())+"</p>";
    html += "<hr>";
    options.kind = IMessageContentOptions::Message;
    AWindow->viewWidget()->appendHtml(html,options);
  }

  options.kind = IMessageContentOptions::Topic;
  AWindow->viewWidget()->appendText(tr("Subject: %1").arg(!AMessage.subject().isEmpty() ? AMessage.subject() : tr("<no subject>")),options);
  options.kind = IMessageContentOptions::Message;
  AWindow->viewWidget()->appendMessage(AMessage,options);
}

void NormalMessageHandler::onMessageReady()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    Message message;
    message.setType(Message::Normal);
    message.setSubject(window->subject());
    message.setThreadId(window->threadId());
    FMessageProcessor->textToMessage(message,window->editWidget()->document());
    if (!message.body().isEmpty())
    {
      bool sended = false;
      QList<Jid> receiversList = window->receiversWidget()->receivers();
      foreach(Jid receiver, receiversList)
      {
        message.setTo(receiver.eFull());
        sended = FMessageProcessor->sendMessage(window->streamJid(),message) ? true : sended;
      }
      if (sended)
      {
        if (FActiveMessages.contains(window))
          showNextMessage(window);
        else
          window->closeWindow();
      }
    }
  }
}

void NormalMessageHandler::onShowNextMessage()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    showNextMessage(window);
    updateWindow(window);
  }
}

void NormalMessageHandler::onReplyMessage()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Re: %1").arg(window->subject()));
    window->editWidget()->clearEditor();
    window->editWidget()->instance()->setFocus();
    updateWindow(window);
  }
}

void NormalMessageHandler::onForwardMessage()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (FLastMessages.contains(window))
  {
    Message message = FLastMessages.value(window);
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Fw: %1").arg(message.subject()));
    window->setThreadId(message.threadId());
    FMessageProcessor->messageToText(window->editWidget()->document(),message);
    window->receiversWidget()->clear();
    window->setCurrentTabWidget(window->receiversWidget()->instance());
    updateWindow(window);
  }
}

void NormalMessageHandler::onShowChatWindow()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (FMessageProcessor && window)
    FMessageProcessor->openWindow(window->streamJid(),window->contactJid(),Message::Chat);
}

void NormalMessageHandler::onWindowDestroyed()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (FWindows.contains(window))
  {
    QList<int> messagesId = FActiveMessages.values(window);
    foreach(int messageId, messagesId)
      FActiveMessages.insertMulti(NULL,messageId);
    FActiveMessages.remove(window);
    FLastMessages.remove(window);
    FWindows.removeAt(FWindows.indexOf(window));
  }
}

void NormalMessageHandler::onStatusIconsChanged()
{
  foreach(IMessageWindow *window, FWindows)
    updateWindow(window);
}

void NormalMessageHandler::onShowWindowAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    openWindow(streamJid,contactJid,Message::Normal);

    QString group = action->data(ADR_GROUP).toString();
    if (!group.isEmpty())
    {
      IMessageWindow *window = FMessageWidgets->findMessageWindow(streamJid,contactJid);
      if (window)
        window->receiversWidget()->addReceiversGroup(group);
    }
  }
}

void NormalMessageHandler::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  static QList<int> messageActionTypes = QList<int>() << RIT_STREAM_ROOT << RIT_GROUP << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;

  Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
  if (presence && presence->isOpen())
  {
    if (messageActionTypes.contains(AIndex->type()))
    {
      Jid contactJid = AIndex->data(RDR_JID).toString();
      Action *action = new Action(AMenu);
      action->setText(tr("Message"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMAL_MHANDLER_MESSAGE);
      action->setData(ADR_STREAM_JID,streamJid.full());
      if (AIndex->type() == RIT_GROUP)
        action->setData(ADR_GROUP,AIndex->data(RDR_GROUP));
      else if (AIndex->type() != RIT_STREAM_ROOT)
        action->setData(ADR_CONTACT_JID,contactJid.full());
      AMenu->addAction(action,AG_RVCM_NORMALMESSAGEHANDLER,true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
    }
  }
}

void NormalMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  IMessageWindow *messageWindow = findWindow(APresence->streamJid(),APresenceItem.itemJid);
  if (messageWindow)
    updateWindow(messageWindow);
}

void NormalMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (AContext.isEmpty())
  {
    foreach (IMessageWindow *window, FWindows)
    {
      if (FLastMessages.value(window).type()==AMessageType && window->viewWidget() && window->viewWidget()->messageStyle())
      {
        window->viewWidget()->messageStyle()->changeOptions(window->viewWidget()->styleWidget(),AOptions,false);
      }
    }
  }
}

Q_EXPORT_PLUGIN2(NormalMessageHandlerPlugin, NormalMessageHandler)
