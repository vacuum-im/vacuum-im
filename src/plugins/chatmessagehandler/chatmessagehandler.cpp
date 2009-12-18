#include "chatmessagehandler.h"

#define HISTORY_MESSAGES          10
#define HISTORY_TIME_PAST         5

#define DESTROYWINDOW_TIMEOUT     30*60*1000
#define CONSECUTIVE_TIMEOUT       2*60

#define ADR_STREAM_JID            Action::DR_StreamJid
#define ADR_CONTACT_JID           Action::DR_Parametr1


#define CHAT_NOTIFICATOR_ID       "ChatMessages"

ChatMessageHandler::ChatMessageHandler()
{
  FMessageWidgets = NULL;
  FMessageProcessor = NULL;
  FMessageStyles = NULL;
  FPresencePlugin = NULL;
  FMessageArchiver = NULL;
  FRostersView = NULL;
  FRostersModel = NULL;
  FStatusIcons = NULL;
  FStatusChanger = NULL;
  FXmppUriQueries = NULL;
}

ChatMessageHandler::~ChatMessageHandler()
{

}

void ChatMessageHandler::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Chat Messages");
  APluginInfo->description = tr("Allows to exchange chat messages");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(MESSAGEPROCESSOR_UUID);
  APluginInfo->dependences.append(MESSAGESTYLES_UUID);
}


bool ChatMessageHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
  if (plugin)
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
  if (plugin)
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
  if (plugin)
  {
    FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());
    if (FMessageStyles)
    {
      connect(FMessageStyles->instance(),SIGNAL(styleOptionsChanged(const IMessageStyleOptions &, int, const QString &)),
        SLOT(onStyleOptionsChanged(const IMessageStyleOptions &, int, const QString &)));
    }
  }

  plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
  if (plugin)
  {
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
    if (FStatusIcons)
    {
      connect(FStatusIcons->instance(),SIGNAL(statusIconsChanged()),SLOT(onStatusIconsChanged()));
    }
  }

  plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
    }
  }

  plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
  if (plugin)
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());

  plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
  if (plugin)
  {
    INotifications *notifications = qobject_cast<INotifications *>(plugin->instance());
    if (notifications)
    {
      uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
      notifications->insertNotificator(CHAT_NOTIFICATOR_ID,tr("Chat Messages"),kindMask,kindMask);
    }
  }

  plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
  if (plugin) 
  {
    IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
    if (rostersViewPlugin)
    {
      FRostersView = rostersViewPlugin->rostersView();
      connect(FRostersView->instance(),SIGNAL(indexContextMenu(IRosterIndex *, Menu *)),SLOT(onRosterIndexContextMenu(IRosterIndex *, Menu *)));
    }
  }

  plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
  if (plugin)
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
  if (plugin)
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
  if (plugin)
    FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

  return FMessageProcessor!=NULL && FMessageWidgets!=NULL && FMessageStyles!=NULL;
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
  if (FXmppUriQueries)
  {
    FXmppUriQueries->insertUriHandler(this, XUHO_DEFAULT);
  }
  return true;
}

bool ChatMessageHandler::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
  if (AAction == "message")
  {
    QString type = AParams.value("type");
    if (type == "chat")
    {
      IChatWindow *window = getWindow(AStreamJid, AContactJid);
      window->editWidget()->textEdit()->setPlainText(AParams.value("body"));
      showWindow(window);
      return true;
    }
  }
  return false;
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
  IChatWindow *window = getWindow(message.to(),message.from());
  if (window)
  {
    showStyledMessage(window,message);
    if (!window->isActive())
    {
      FActiveMessages.insertMulti(window, AMessageId);
      updateWindow(window);
    }
    else
      FMessageProcessor->removeMessage(AMessageId);
  }
}

bool ChatMessageHandler::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType)
{
  if (AType == Message::Chat)
  {
    IChatWindow *window = getWindow(AStreamJid,AContactJid);
    if (window)
    {
      showWindow(window);
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

IChatWindow *ChatMessageHandler::getWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  IChatWindow *window = NULL;
  if (AStreamJid.isValid() && AContactJid.isValid())
  {
    window = FMessageWidgets->newChatWindow(AStreamJid,AContactJid);
    if (window)
    {
      window->infoWidget()->autoUpdateFields();
      connect(window->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));
      connect(window->infoWidget()->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
        SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));
      connect(window->instance(),SIGNAL(windowActivated()),SLOT(onWindowActivated()));
      connect(window->instance(),SIGNAL(windowClosed()),SLOT(onWindowClosed()));
      connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onWindowDestroyed()));

      FWindows.append(window);
      FWindowStatus[window->viewWidget()].createTime = QDateTime::currentDateTime();
      updateWindow(window);

      if (FRostersView && FRostersModel)
      {
        UserContextMenu *menu = new UserContextMenu(FRostersModel,FRostersView,window);
        window->menuBarWidget()->menuBarChanger()->insertMenu(menu,MBG_MWCW_USERCONTEXT);
      }
      setMessageStyle(window);
      showHistory(window);
    }
    else
      window = findWindow(AStreamJid,AContactJid);
  }
  return window;
}

IChatWindow *ChatMessageHandler::findWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IChatWindow *window,FWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

void ChatMessageHandler::showWindow(IChatWindow *AWindow)
{
  if (FMessageWidgets && AWindow->instance()->isWindow() && !AWindow->instance()->isVisible())
    FMessageWidgets->assignTabWindow(AWindow);
  AWindow->showWindow();
}

void ChatMessageHandler::updateWindow(IChatWindow *AWindow)
{
  QIcon icon;
  if (FActiveMessages.contains(AWindow))
    icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CHAT_MHANDLER_MESSAGE);
  else if (FStatusIcons)
    icon = FStatusIcons->iconByJid(AWindow->streamJid(),AWindow->contactJid());

  QString contactName = AWindow->infoWidget()->field(IInfoWidget::ContactName).toString();
  AWindow->updateWindow(icon,contactName,tr("%1 - Chat").arg(contactName));
}

void ChatMessageHandler::removeActiveMessages(IChatWindow *AWindow)
{
  if (FActiveMessages.contains(AWindow))
  {
    QList<int> messageIds = FActiveMessages.values(AWindow);
    foreach(int messageId, messageIds)
      FMessageProcessor->removeMessage(messageId);
    FActiveMessages.remove(AWindow);
    updateWindow(AWindow);
  }
}

void ChatMessageHandler::showHistory(IChatWindow *AWindow)
{
  if (FMessageArchiver)
  {
    IArchiveRequest request;
    request.with = AWindow->contactJid().bare();
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
      showStyledMessage(AWindow,message);
    }

    wstatus.startTime = history.value(0).dateTime();
  }
}

void ChatMessageHandler::setMessageStyle(IChatWindow *AWindow)
{
  IMessageStyleOptions soptions = FMessageStyles->styleOptions(Message::Chat);
  IMessageStyle *style = FMessageStyles->styleForOptions(soptions);
  AWindow->viewWidget()->setMessageStyle(style,soptions);
}

void ChatMessageHandler::fillContentOptions(IChatWindow *AWindow, IMessageContentOptions &AOptions) const
{
  if (AOptions.direction == IMessageContentOptions::DirectionIn)
  {
    AOptions.senderId = AWindow->contactJid().full();
    AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid(),AWindow->contactJid()));
    AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->contactJid());
    AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid(),AWindow->contactJid());
    AOptions.senderColor = "blue";
  }
  else
  {
    AOptions.senderId = AWindow->streamJid().full();
    if (AWindow->streamJid() && AWindow->contactJid())
      AOptions.senderName = Qt::escape(!AWindow->streamJid().resource().isEmpty() ? AWindow->streamJid().resource() : AWindow->streamJid().node());
    else
      AOptions.senderName = Qt::escape(FMessageStyles->userName(AWindow->streamJid()));
    AOptions.senderAvatar = FMessageStyles->userAvatar(AWindow->streamJid());
    AOptions.senderIcon = FMessageStyles->userIcon(AWindow->streamJid());
    AOptions.senderColor = "red";
  }
}

void ChatMessageHandler::showStyledStatus(IChatWindow *AWindow, const QString &AMessage)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Status;
  options.time = QDateTime::currentDateTime();
  options.timeFormat = FMessageStyles->timeFormat(options.time);
  options.direction = IMessageContentOptions::DirectionIn;
  fillContentOptions(AWindow,options);
  AWindow->viewWidget()->appendText(AMessage,options);
}

void ChatMessageHandler::showStyledMessage(IChatWindow *AWindow, const Message &AMessage)
{
  IMessageContentOptions options;
  options.kind = IMessageContentOptions::Message;
  options.time = AMessage.dateTime();
  options.timeFormat = FMessageStyles->timeFormat(options.time);
  if (AWindow->streamJid() && AWindow->contactJid() ? AWindow->contactJid() != AMessage.to() : !(AWindow->contactJid() && AMessage.to()))
    options.direction = IMessageContentOptions::DirectionIn;
  else
    options.direction = IMessageContentOptions::DirectionOut;

  if (options.time.secsTo(FWindowStatus.value(AWindow->viewWidget()).createTime)>HISTORY_TIME_PAST)
    options.type |= IMessageContentOptions::History;
  
  fillContentOptions(AWindow,options);
  AWindow->viewWidget()->appendMessage(AMessage,options);
}

void ChatMessageHandler::onMessageReady()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    Message message;
    message.setTo(window->contactJid().eFull()).setType(Message::Chat);
    FMessageProcessor->textToMessage(message,window->editWidget()->document());
    if (!message.body().isEmpty() && FMessageProcessor->sendMessage(window->streamJid(),message))
    {
      window->editWidget()->clearEditor();
      showStyledMessage(window,message);
    }
  }
}

void ChatMessageHandler::onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
  if ((AField & (IInfoWidget::ContactStatus|IInfoWidget::ContactName))>0)
  {
    IInfoWidget *widget = qobject_cast<IInfoWidget *>(sender());
    IChatWindow *window = widget!=NULL ? findWindow(widget->streamJid(),widget->contactJid()) : NULL;
    if (window)
    {
      Jid streamJid = window->streamJid();
      Jid contactJid = window->contactJid();
      if (AField == IInfoWidget::ContactStatus && FMessageWidgets->chatWindowShowStatus())
      {
        QString status = AValue.toString();
        QString show = FStatusChanger ? FStatusChanger->nameByShow(widget->field(IInfoWidget::ContactShow).toInt()) : QString::null;
        WindowStatus &wstatus = FWindowStatus[window->viewWidget()];
        if (wstatus.lastStatusShow != status+show)
        {
          wstatus.lastStatusShow = status+show;
          QString message = tr("%1 changed status to [%2] %3").arg(widget->field(IInfoWidget::ContactName).toString()).arg(show).arg(status);
          showStyledStatus(window,message);
        }
      }
      updateWindow(window);
    }
  }
}

void ChatMessageHandler::onWindowActivated()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    removeActiveMessages(window);
    if (FWindowTimers.contains(window))
      delete FWindowTimers.take(window);
  }
}

void ChatMessageHandler::onWindowClosed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    if (!FWindowTimers.contains(window))
    {
      QTimer *timer = new QTimer;
      timer->setSingleShot(true);
      connect(timer,SIGNAL(timeout()),window->instance(),SLOT(deleteLater()));
      FWindowTimers.insert(window,timer);
    }
    FWindowTimers[window]->start(DESTROYWINDOW_TIMEOUT);
  }
}

void ChatMessageHandler::onWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (FWindows.contains(window))
  {
    removeActiveMessages(window);
    if (FWindowTimers.contains(window))
      delete FWindowTimers.take(window);
    FWindows.removeAt(FWindows.indexOf(window));
    FWindowStatus.remove(window->viewWidget());
  }
}

void ChatMessageHandler::onStatusIconsChanged()
{
  foreach(IChatWindow *window, FWindows)
    updateWindow(window);
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

void ChatMessageHandler::onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu)
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

void ChatMessageHandler::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  Jid streamJid = APresence->streamJid();
  Jid contactJid = APresenceItem.itemJid;
  IChatWindow *window = findWindow(streamJid,contactJid);
  if (!window && !contactJid.resource().isEmpty())
  {
    window = findWindow(streamJid,contactJid.bare());
    if (window)
      window->setContactJid(contactJid);
  }
  else if (window && !contactJid.resource().isEmpty())
  {
    IChatWindow *bareWindow = findWindow(streamJid,contactJid.bare());
    if (bareWindow)
      bareWindow->instance()->deleteLater();
  }
}

void ChatMessageHandler::onStyleOptionsChanged(const IMessageStyleOptions &AOptions, int AMessageType, const QString &AContext)
{
  if (AMessageType==Message::Chat && AContext.isEmpty())
  {
    foreach (IChatWindow *window, FWindows)
    {
      IMessageStyle *style = window->viewWidget()!=NULL ? window->viewWidget()->messageStyle() : NULL;
      if (style==NULL || !style->changeOptions(window->viewWidget()->styleWidget(),AOptions,false))
      {
        setMessageStyle(window);
        showHistory(window);
      }
    }
  }
}

Q_EXPORT_PLUGIN2(plg_chatmessagehandler, ChatMessageHandler)
