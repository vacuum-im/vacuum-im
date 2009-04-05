#include "normalmessagehandler.h"

#define NORMAL_NOTIFICATOR_ID     "NormalMessages"

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1
#define ADR_GROUP                 Action::DR_Parametr3

NormalMessageHandler::NormalMessageHandler()
{
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;
  FStatusIcons = NULL;
  FPresencePlugin = NULL;
  FRostersView = NULL;
}

NormalMessageHandler::~NormalMessageHandler()
{

}

void NormalMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Normal message handling");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Normal Message Handler"); 
  APluginInfo->uid = NORMALMESSAGEHANDLER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
  APluginInfo->conflicts.append("{153A4638-B468-496f-B57C-9F30CEDFCC2E}");  //Messenger
  APluginInfo->conflicts.append("{f118ccf4-8535-4302-8fda-0f6487c6db01}");  //MessageHandler
}

bool NormalMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
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

  return FMessageProcessor!=NULL && FMessageWidgets!=NULL;
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
  IMessageWindow *window = findMessageWindow(message.to(),message.from());
  if (window)
  {
    FActiveNormalMessages.insertMulti(window,AMessageId);
    updateMessageWindow(window);
  }
  else
    FActiveNormalMessages.insertMulti(NULL,AMessageId);
}

bool NormalMessageHandler::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType)
{
  IMessageWindow *window = getMessageWindow(AStreamJid,AContactJid,IMessageWindow::WriteMode);
  if (window)
  {
    showMessageWindow(window);
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

IMessageWindow *NormalMessageHandler::getMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
  IMessageWindow *window = NULL;
  if (AStreamJid.isValid() && (AContactJid.isValid() || AMode == IMessageWindow::WriteMode))
  {
    window = FMessageWidgets->newMessageWindow(AStreamJid,AContactJid,AMode);
    if (window)
    {
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageWindowSend()));
      connect(window->instance(),SIGNAL(showNextMessage()),SLOT(onMessageWindowShowNext()));
      connect(window->instance(),SIGNAL(replyMessage()),SLOT(onMessageWindowReply()));
      connect(window->instance(),SIGNAL(forwardMessage()),SLOT(onMessageWindowForward()));
      connect(window->instance(),SIGNAL(showChatWindow()),SLOT(onMessageWindowShowChat()));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onMessageWindowDestroyed()));
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

IMessageWindow *NormalMessageHandler::findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IMessageWindow *window,FMessageWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void NormalMessageHandler::showMessageWindow(IMessageWindow *AWindow)
{
  AWindow->showWindow();
}

void NormalMessageHandler::showNextNormalMessage(IMessageWindow *AWindow)
{
  if (FActiveNormalMessages.contains(AWindow))
  {
    int messageId = FActiveNormalMessages.value(AWindow);
    Message message = FMessageProcessor->messageById(messageId);
    AWindow->showMessage(message);
    FMessageProcessor->removeMessage(messageId);
    FActiveNormalMessages.remove(AWindow,messageId);
  }
  updateMessageWindow(AWindow);
}

void NormalMessageHandler::loadActiveNormalMessages(IMessageWindow *AWindow)
{
  QList<int> messagesId = FActiveNormalMessages.values(NULL);
  foreach(int messageId, messagesId)
  {
    Message message = FMessageProcessor->messageById(messageId);
    if (AWindow->streamJid() == message.to() && AWindow->contactJid() == message.from())
    {
      FActiveNormalMessages.insertMulti(AWindow,messageId);
      FActiveNormalMessages.remove(NULL,messageId);
    }
  }
}

void NormalMessageHandler::updateMessageWindow(IMessageWindow *AWindow)
{
  QIcon icon;
  if (FActiveNormalMessages.contains(AWindow))
    icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_NORMAL_MHANDLER_MESSAGE);
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

  QString title = tr("Composing message");
  if (AWindow->mode() == IMessageWindow::ReadMode)
    title = tr("%1 - Message").arg(AWindow->infoWidget()->field(IInfoWidget::ContactName).toString());
  AWindow->updateWindow(icon,title,title);
  AWindow->setNextCount(FActiveNormalMessages.count(AWindow));
}

void NormalMessageHandler::onMessageWindowSend()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    Message message;
    message/*.setFrom(window->streamJid().eFull())*/.setType(Message::Normal);
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
        if (FActiveNormalMessages.contains(window))
          showNextNormalMessage(window);
        else
          window->closeWindow();
      }
    }
  }
}

void NormalMessageHandler::onMessageWindowShowNext()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    showNextNormalMessage(window);
    updateMessageWindow(window);
  }
}

void NormalMessageHandler::onMessageWindowReply()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Re: %1").arg(window->subject()));
    window->editWidget()->clearEditor();
    window->editWidget()->instance()->setFocus();
    updateMessageWindow(window);
  }
}

void NormalMessageHandler::onMessageWindowForward()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    window->setMode(IMessageWindow::WriteMode);
    window->setSubject(tr("Fw: %1").arg(window->subject()));
    window->setThreadId(window->currentMessage().threadId());
    FMessageProcessor->messageToText(window->editWidget()->document(),window->currentMessage());
    window->receiversWidget()->clear();
    window->setCurrentTabWidget(window->receiversWidget()->instance());
    updateMessageWindow(window);
  }
}

void NormalMessageHandler::onMessageWindowShowChat()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (FMessageProcessor && window)
    FMessageProcessor->openWindow(window->streamJid(),window->contactJid(),Message::Chat);
}

void NormalMessageHandler::onMessageWindowDestroyed()
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

void NormalMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  IMessageWindow *messageWindow = findMessageWindow(APresence->streamJid(),APresenceItem.itemJid);
  if (messageWindow)
    updateMessageWindow(messageWindow);
}

void NormalMessageHandler::onStatusIconsChanged()
{
  foreach(IMessageWindow *window, FMessageWindows)
    updateMessageWindow(window);
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

Q_EXPORT_PLUGIN2(NormalMessageHandlerPlugin, NormalMessageHandler)
