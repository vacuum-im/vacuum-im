#include <QtDebug>
#include "messenger.h"

#define IN_NORMAL_MESSAGE                     "psi/sendMessage"
#define IN_CHAT_MESSAGE                       "psi/start-chat"

#define SVN_INFO                              "info"
#define SVN_VIEW                              "view"
#define SVN_EDIT                              "edit"
#define SVN_CHAT                              "chat"
#define SVN_OPEN_CHAT_IN_TABWINDOW            "openChatInTabWindow"
#define SVN_OPEN_CHAT_WINDOW                  "openChatWindow"
#define SVN_OPEN_MESSAGE_WINDOW               "openMessageWindow"
#define SVN_VIEW_HTML                         SVN_VIEW ":" "showHtml"
#define SVN_VIEW_DATETIME                     SVN_VIEW ":" "showDateTime"
#define SVN_CHAT_STATUS                       SVN_CHAT ":" "showStatus"
#define SVN_CHAT_FONT                         "defaultChatFont"
#define SVN_MESSAGE_FONT                      "defaultMessageFont"

#define SHC_MESSAGE                           "/message"

#define WT_MessageWindow                      0
#define WT_ChatWindow                         1
#define ADR_StreamJid                         Action::DR_StreamJid
#define ADR_ContactJid                        Action::DR_Parametr1
#define ADR_WindowType                        Action::DR_Parametr2  
#define ADR_Group                             Action::DR_Parametr3


Messenger::Messenger()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FRostersView = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FRostersModel = NULL;
  FTrayManager = NULL;
  FSettingsPlugin = NULL;

  FMessengerOptions = NULL;
  FMessageHandler = NULL;
  FMessageId = 0;
  FOptions = 0;
}

Messenger::~Messenger()
{

}

void Messenger::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Send and receive messages");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Messenger"); 
  APluginInfo->uid = MESSENGER_UUID;
  APluginInfo->version = "0.1";
}

bool Messenger::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }
  
  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin) 
    FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin) 
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin) 
  {
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
    if (FTrayManager)
    {
      connect(FTrayManager->instance(),SIGNAL(notifyActivated(int,QSystemTrayIcon::ActivationReason)),
        SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
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
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
    }
  }

  return true;
}

bool Messenger::initObjects()
{
  FMessageHandler = new MessageHandler(this,this);
  insertMessageHandler(FMessageHandler,MHO_MESSENGER);
  insertMessageWriter(this,MWO_MESSENGER);
  insertMessageWriter(this,MWO_MESSENGER_ANCHORS);

  if (FRostersViewPlugin)
  {
    FRostersView = FRostersViewPlugin->rostersView();
    FRostersView->insertClickHooker(RCHO_MESSENGER,this);
    connect(FRostersView,SIGNAL(contextMenu(IRosterIndex *, Menu *)),SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
    connect(FRostersView,SIGNAL(notifyActivated(IRosterIndex *, int)), SLOT(onRosterNotifyActivated(IRosterIndex *, int)));
  }

  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_MESSAGES,tr("Messages"), tr("Message window options"),QIcon());
    FSettingsPlugin->appendOptionsHolder(this);
  }

  return true;
}

bool Messenger::readStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, const Stanza &AStanza, bool &AAccept)
{
  Message message(AStanza);
  bool received = receiveMessage(message) > 0;
  AAccept = AAccept || received;
  return received;
}

bool Messenger::rosterIndexClicked(IRosterIndex *AIndex, int /*AOrder*/)
{
  static QList<int> hookerTypes = QList<int>() << RIT_Contact << RIT_MyResource;
  if (hookerTypes.contains(AIndex->type()))
  {
    QList<int> notifyIds = FRostersView->indexNotifies(AIndex,RLO_MESSAGE);
    if (notifyIds.isEmpty())
    {
      foreach(IMessageHandler *handler, FMessageHandlers)
        if (handler->openWindow(AIndex))
          return true;
    }
    else
    {
      showMessage(FRosterId2MessageId.value(notifyIds.first()));
      return true;
    }
  }
  return false;
}

QWidget *Messenger::optionsWidget(const QString &ANode, int &/*AOrder*/)
{
  if (ANode == ON_MESSAGES)
  {
    FMessengerOptions = new MessengerOptions;
    FMessengerOptions->setOption(OpenChatInTabWindow,checkOption(OpenChatInTabWindow));
    FMessengerOptions->setOption(OpenMessageWindow,checkOption(OpenMessageWindow));
    FMessengerOptions->setOption(OpenChatWindow,checkOption(OpenChatWindow));
    FMessengerOptions->setOption(ShowHTML,checkOption(ShowHTML));
    FMessengerOptions->setOption(ShowDateTime,checkOption(ShowDateTime));
    FMessengerOptions->setOption(ShowStatus,checkOption(ShowStatus));
    FMessengerOptions->setChatFont(FChatFont);
    FMessengerOptions->setMessageFont(FMessageFont);
    return FMessengerOptions;
  }
  return NULL;
}

void Messenger::writeMessage(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder)
{
  if (AOrder == MWO_MESSENGER)
  {
    AMessage.setBody(prepareBodyForSend(ADocument->toPlainText()),ALang);
  }
}

void Messenger::writeText(Message &AMessage, QTextDocument *ADocument, const QString &ALang, int AOrder)
{
  if (AOrder == MWO_MESSENGER)
  {
    QTextCursor cursor(ADocument);
    cursor.insertText(prepareBodyForReceive(AMessage.body(ALang)));
  }
  else if (AOrder == MWO_MESSENGER_ANCHORS)
  {
    QRegExp regexp("\\b((https?|ftp)://|www.)[a-z0-9/\\?.=:@&%#_;\\+\\-]+");
    regexp.setCaseSensitivity(Qt::CaseInsensitive);
    for (QTextCursor cursor = ADocument->find(regexp); !cursor.isNull();  cursor = ADocument->find(regexp,cursor))
    {
      QTextCharFormat linkFormat = cursor.charFormat();
      linkFormat.setAnchor(true);
      linkFormat.setAnchorHref(cursor.selectedText());
      cursor.setCharFormat(linkFormat);
    }
  }
}

void Messenger::insertMessageHandler(IMessageHandler *AHandler, int AOrder)
{
  if (!FMessageHandlers.values(AOrder).contains(AHandler))  
  {
    FMessageHandlers.insert(AOrder,AHandler);
    emit messageHandlerInserted(AHandler,AOrder);
  }
}

void Messenger::removeMessageHandler(IMessageHandler *AHandler, int AOrder)
{
  if (FMessageHandlers.values(AOrder).contains(AHandler))  
  {
    FMessageHandlers.remove(AOrder,AHandler);
    emit messageHandlerRemoved(AHandler,AOrder);
  }
}

void Messenger::insertMessageWriter(IMessageWriter *AWriter, int AOrder)
{
  if (!FMessageWriters.values(AOrder).contains(AWriter))  
  {
    FMessageWriters.insert(AOrder,AWriter);
    emit messageWriterInserted(AWriter,AOrder);
  }
}

void Messenger::removeMessageWriter(IMessageWriter *AWriter, int AOrder)
{
  if (FMessageWriters.values(AOrder).contains(AWriter))  
  {
    FMessageWriters.remove(AOrder,AWriter);
    emit messageWriterRemoved(AWriter,AOrder);
  }
}

void Messenger::insertResourceLoader(IResourceLoader *ALoader, int AOrder)
{
  if (!FResourceLoaders.values(AOrder).contains(ALoader))  
  {
    FResourceLoaders.insert(AOrder,ALoader);
    emit resourceLoaderInserted(ALoader,AOrder);
  }
}

void Messenger::removeResourceLoader(IResourceLoader *ALoader, int AOrder)
{
  if (FResourceLoaders.values(AOrder).contains(ALoader))  
  {
    FResourceLoaders.remove(AOrder,ALoader);
    emit resourceLoaderRemoved(ALoader,AOrder);
  }
}

void Messenger::textToMessage(Message &AMessage, const QTextDocument *ADocument, const QString &ALang) const
{
  QTextDocument *documentCopy = ADocument->clone();
  QMapIterator<int,IMessageWriter *> it(FMessageWriters);
  it.toBack();
  while(it.hasPrevious())
  {
    it.previous();
    it.value()->writeMessage(AMessage,documentCopy,ALang,it.key());
  }
  delete documentCopy;
}

void Messenger::messageToText(QTextDocument *ADocument, const Message &AMessage, const QString &ALang) const
{
  Message messageCopy = AMessage;
  QMapIterator<int,IMessageWriter *> it(FMessageWriters);
  it.toFront();
  while(it.hasNext())
  {
    it.next();
    it.value()->writeText(messageCopy,ADocument,ALang,it.key());
  }
}

bool Messenger::openWindow(const Jid &AStreamJid, const Jid &AContactJid, Message::MessageType AType) const
{
  foreach(IMessageHandler *handler, FMessageHandlers)
    if (handler->openWindow(AStreamJid,AContactJid,AType))
      return true;
  return false;
}

bool Messenger::sendMessage(const Message &AMessage, const Jid &AStreamJid)
{
  Message message = AMessage;

  emit messageSend(message);
  
  if (FStanzaProcessor->sendStanzaOut(AStreamJid,message.stanza()))
  {
    emit messageSent(message);
    return true;
  }
  return false;
}

int Messenger::receiveMessage(const Message &AMessage)
{
  int messageId = 0; 
  IMessageHandler *handler = getMessageHandler(AMessage);
  if (handler)
  {
    Message message = AMessage;
    messageId = newMessageId();
    message.setData(MDR_MESSAGEID,messageId);
    FMessages.insert(messageId,message);
    FHandlerForMessage.insert(messageId,handler);

    emit messageReceive(message);
    notifyMessage(messageId);
    handler->receiveMessage(messageId);
    emit messageReceived(message);
  }
  return messageId;
}

void Messenger::showMessage(int AMessageId)
{
  IMessageHandler *handler = FHandlerForMessage.value(AMessageId,NULL);
  if (handler)
    handler->showMessage(AMessageId);
}

void Messenger::removeMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    unNotifyMessage(AMessageId);
    FHandlerForMessage.remove(AMessageId);
    Message message = FMessages.take(AMessageId);
    emit messageRemoved(message);
  }
}

QList<int> Messenger::messages(const Jid &AStreamJid, const Jid &AFromJid, int AMesTypes)
{
  QList<int> mIds;
  QMap<int,Message>::const_iterator it = FMessages.constBegin();
  while(it != FMessages.constEnd())
  {
    if (AStreamJid == it.value().to() 
        && (!AFromJid.isValid() || AFromJid == it.value().from())
        && (AMesTypes == Message::AnyType || (AMesTypes & it.value().type())>0) 
       )
    {
      mIds.append(it.key());
    }
    it++;
  }
  return mIds;
}

bool Messenger::checkOption(IMessenger::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void Messenger::setOption(IMessenger::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    emit optionChanged(AOption,AValue);
  }
}

void Messenger::setDefaultChatFont(const QFont &AFont)
{
  if (FChatFont != AFont)
  {
    FChatFont = AFont;
    emit defaultChatFontChanged(FChatFont);
  }
}

void Messenger::setDefaultMessageFont(const QFont &AFont)
{
  if (FMessageFont != AFont)
  {
    FMessageFont = AFont;
    emit defaultMessageFontChanged(FMessageFont);
  }
}

IInfoWidget *Messenger::newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IInfoWidget *widget = new InfoWidget(this,AStreamJid,AContactJid);
  emit infoWidgetCreated(widget);
  return widget;
}

IViewWidget *Messenger::newViewWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IViewWidget *widget = new ViewWidget(this,AStreamJid,AContactJid);
  connect(widget->textBrowser(),SIGNAL(loadCustomResource(int, const QUrl &, QVariant &)),
    SLOT(onTextLoadResource(int, const QUrl &, QVariant &)));
  emit viewWidgetCreated(widget);
  return widget;
}

IEditWidget *Messenger::newEditWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IEditWidget *widget = new EditWidget(this,AStreamJid,AContactJid);
  connect(widget->textEdit(),SIGNAL(loadCustomResource(int, const QUrl &, QVariant &)),
    SLOT(onTextLoadResource(int, const QUrl &, QVariant &)));
  emit editWidgetCreated(widget);
  return widget;
}

IReceiversWidget *Messenger::newReceiversWidget(const Jid &AStreamJid)
{
  IReceiversWidget *widget = new ReceiversWidget(this,AStreamJid);
  emit receiversWidgetCreated(widget);
  return widget;
}

IToolBarWidget *Messenger::newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
  IToolBarWidget *widget = new ToolBarWidget(AInfo,AView,AEdit,AReceivers);
  emit toolBarWidgetCreated(widget);
  return widget;
}

IMessageWindow *Messenger::newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
  IMessageWindow *window = findMessageWindow(AStreamJid,AContactJid);
  if (!window)
  {
    window = new MessageWindow(this,AStreamJid,AContactJid,AMode);
    FMessageWindows.append(window);
    connect(window,SIGNAL(windowDestroyed()),SLOT(onMessageWindowDestroyed()));
    emit messageWindowCreated(window);
    return window;
  }
  return NULL;
}

IMessageWindow *Messenger::findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IMessageWindow *window,FMessageWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

IChatWindow *Messenger::newChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  IChatWindow *window = findChatWindow(AStreamJid,AContactJid);
  if (!window)
  {
    window = new ChatWindow(this,AStreamJid,AContactJid);
    FChatWindows.append(window);
    connect(window,SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
    emit chatWindowCreated(window);
    return window;
  }
  return NULL;
}

IChatWindow *Messenger::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  foreach(IChatWindow *window,FChatWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

ITabWindow *Messenger::openTabWindow(int AWindowId)
{
  ITabWindow *window = findTabWindow(AWindowId);
  if (!window)
  {
    window = new TabWindow(this,AWindowId);
    FTabWindows.insert(AWindowId,window);
    connect(window,SIGNAL(windowDestroyed()),SLOT(onTabWindowDestroyed()));
    emit tabWindowCreated(window);
  }
  window->showWindow();
  return window;
}

ITabWindow *Messenger::findTabWindow(int AWindowId)
{
  return FTabWindows.value(AWindowId,NULL);
}

IMessageHandler *Messenger::getMessageHandler(const Message &AMessage)
{
  foreach(IMessageHandler *handler, FMessageHandlers)
    if (handler->checkMessage(AMessage))
      return handler;
  return NULL;
}

void Messenger::notifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    QIcon icon;
    QString toolTip;
    int flags = 0;
    Message &message = FMessages[AMessageId];
    IMessageHandler *handler = FHandlerForMessage.value(AMessageId);
    if (handler->notifyOptions(message,icon,toolTip,flags))
    {
      if (FRostersView && FRostersModel && (flags&IRostersView::LabelVisible))
      {
        IRosterIndexList indexList = FRostersModel->getContactIndexList(message.to(),message.from(),true);
        int rosterId = FRostersView->appendNotify(indexList,RLO_MESSAGE,icon,toolTip,flags);
        FRosterId2MessageId.insert(rosterId,AMessageId);
      }
      if (FTrayManager)
      {
        int trayId = FTrayManager->appendNotify(icon,toolTip,true);
        FTrayId2MessageId.insert(trayId,AMessageId);
      }
      emit messageNotified(AMessageId);
    }
  }
}

void Messenger::unNotifyMessage(int AMessageId)
{
  if (FMessages.contains(AMessageId))
  {
    if (FRostersView)
    {
      int rosterId = FRosterId2MessageId.key(AMessageId);
      FRostersView->removeNotify(rosterId);
      FRosterId2MessageId.remove(rosterId);
    }
    if (FTrayManager)
    {
      int trayId = FTrayId2MessageId.key(AMessageId);
      FTrayManager->removeNotify(trayId);
      FTrayId2MessageId.remove(trayId);
    }
    emit messageUnNotified(AMessageId);
  }
}

void Messenger::removeStreamMessages(const Jid &AStreamJid)
{
  QList<int> mIds = messages(AStreamJid);
  foreach (int messageId, mIds)
    removeMessage(messageId);
}

void Messenger::deleteStreamWindows(const Jid &AStreamJid)
{
  foreach(IChatWindow *window, FChatWindows)
    if (window->streamJid() == AStreamJid)
    {
      window->close();
      window->deleteLater();
    }

  foreach(IMessageWindow *window, FMessageWindows)
    if (window->streamJid() == AStreamJid)
    {
      window->close();
      window->deleteLater();
    }
}

QString Messenger::prepareBodyForSend(const QString &AString) const
{
  QString result = AString.trimmed();
  result.remove(QChar::ObjectReplacementCharacter);
  return result;
}

QString Messenger::prepareBodyForReceive(const QString &AString) const
{
  QString result = AString.trimmed();
  result.remove(QChar::ObjectReplacementCharacter);
  return result;
}

void Messenger::onStreamAdded(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && !FStanzaHandlers.contains(AXmppStream))
  {
    int handler = FStanzaProcessor->insertHandler(this,SHC_MESSAGE,IStanzaProcessor::DirectionIn,SHP_DEFAULT,AXmppStream->jid());
    FStanzaHandlers.insert(AXmppStream,handler);
  }
}

void Messenger::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  if (AAfter && AXmppStream->jid())
  {
    QMap<int,Message>::iterator it = FMessages.begin();
    while (it != FMessages.end())
    {
      if (AXmppStream->jid() == it.value().to())
      {
        unNotifyMessage(it.key());
        it.value().setTo(AAfter.eFull());
      }
      it++;
    }
  }
  else
  {
    deleteStreamWindows(AXmppStream->jid());
    removeStreamMessages(AXmppStream->jid());
  }
}

void Messenger::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &/*ABefour*/)
{
  QMap<int,Message>::iterator it = FMessages.begin();
  while (it != FMessages.end())
  {
    if (AXmppStream->jid() == it.value().to())
      notifyMessage(it.key());
    it++;
  }
}

void Messenger::onStreamRemoved(IXmppStream *AXmppStream)
{
  deleteStreamWindows(AXmppStream->jid());
  removeStreamMessages(AXmppStream->jid());
  if (FStanzaProcessor && FStanzaHandlers.contains(AXmppStream))
  {
    int handler = FStanzaHandlers.value(AXmppStream);
    FStanzaProcessor->removeHandler(handler);
    FStanzaHandlers.remove(AXmppStream);
  }
}

void Messenger::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  static QList<int> chatActionTypes = QList<int>() << RIT_Contact << RIT_Agent << RIT_MyResource;
  static QList<int> messageActionTypes = QList<int>() << RIT_StreamRoot << RIT_Group << RIT_Contact << RIT_Agent << RIT_MyResource;

  Jid streamJid = AIndex->data(RDR_StreamJid).toString();
  Jid contactJid = AIndex->data(RDR_Jid).toString();
  IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->getStream(streamJid) : NULL;
  if (stream && stream->isOpen())
  {
    if (chatActionTypes.contains(AIndex->type()))
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Chat"));
      //action->setIcon(SYSTEM_ICONSETFILE,IN_CHAT_MESSAGE);
      action->setData(ADR_StreamJid,streamJid.full());
      action->setData(ADR_ContactJid,contactJid.full());
      action->setData(ADR_WindowType,WT_ChatWindow);
      AMenu->addAction(action,AG_MESSENGER,true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
    }
    if (messageActionTypes.contains(AIndex->type()))
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Message"));
      //action->setIcon(SYSTEM_ICONSETFILE,IN_NORMAL_MESSAGE);
      action->setData(ADR_StreamJid,streamJid.full());
      if (AIndex->type() == RIT_Group)
        action->setData(ADR_Group,AIndex->data(RDR_Group));
      else if (AIndex->type() != RIT_StreamRoot)
        action->setData(ADR_ContactJid,contactJid.full());
      action->setData(ADR_WindowType,WT_MessageWindow);
      AMenu->addAction(action,AG_MESSENGER,true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowWindowAction(bool)));
    }
  }
}

void Messenger::onRosterNotifyActivated(IRosterIndex * /*AIndex*/, int ANotifyId)
{
  if (FRosterId2MessageId.contains(ANotifyId))
    showMessage(FRosterId2MessageId.value(ANotifyId));
}

void Messenger::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
  if (AReason == QSystemTrayIcon::DoubleClick && FTrayId2MessageId.contains(ANotifyId))
    showMessage(FTrayId2MessageId.value(ANotifyId));
}

void Messenger::onMessageWindowDestroyed()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    FMessageWindows.removeAt(FMessageWindows.indexOf(window));
    emit messageWindowDestroyed(window);
  }
}

void Messenger::onChatWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    FChatWindows.removeAt(FChatWindows.indexOf(window));
    emit chatWindowDestroyed(window);
  }
}

void Messenger::onTabWindowDestroyed()
{
  ITabWindow *window = qobject_cast<ITabWindow *>(sender());
  if (window)
  {
    FTabWindows.remove(window->windowId());
    emit tabWindowDestroyed(window);
  }
}

void Messenger::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSENGER_UUID);
  setOption(OpenChatInTabWindow, settings->value(SVN_OPEN_CHAT_IN_TABWINDOW,true).toBool());
  setOption(OpenMessageWindow, settings->value(SVN_OPEN_MESSAGE_WINDOW,false).toBool());
  setOption(OpenChatWindow, settings->value(SVN_OPEN_CHAT_WINDOW,false).toBool());
  setOption(ShowHTML, settings->value(SVN_VIEW_HTML,true).toBool());
  setOption(ShowDateTime, settings->value(SVN_VIEW_DATETIME,true).toBool());
  setOption(ShowStatus, settings->value(SVN_CHAT_STATUS,true).toBool());
  FChatFont.fromString(settings->value(SVN_CHAT_FONT,QFont().toString()).toString());
  FMessageFont.fromString(settings->value(SVN_MESSAGE_FONT,QFont().toString()).toString());
}

void Messenger::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSENGER_UUID);
  settings->setValue(SVN_OPEN_CHAT_IN_TABWINDOW,checkOption(OpenChatInTabWindow));
  settings->setValue(SVN_OPEN_MESSAGE_WINDOW,checkOption(OpenMessageWindow));
  settings->setValue(SVN_OPEN_CHAT_WINDOW,checkOption(OpenChatWindow));
  settings->setValue(SVN_VIEW_HTML,checkOption(ShowHTML));
  settings->setValue(SVN_VIEW_DATETIME,checkOption(ShowDateTime));
  settings->setValue(SVN_CHAT_STATUS,checkOption(ShowStatus));

  if (FChatFont != QFont())
    settings->setValue(SVN_CHAT_FONT,FChatFont.toString());
  else
    settings->deleteValue(SVN_CHAT_FONT);
  
  if (FMessageFont != QFont())
    settings->setValue(SVN_MESSAGE_FONT,FMessageFont.toString());
  else
    settings->deleteValue(SVN_MESSAGE_FONT);
}

void Messenger::onOptionsDialogAccepted()
{
  setOption(OpenChatInTabWindow,FMessengerOptions->checkOption(OpenChatInTabWindow));
  setOption(OpenMessageWindow,FMessengerOptions->checkOption(OpenMessageWindow));
  setOption(OpenChatWindow,FMessengerOptions->checkOption(OpenChatWindow));
  setOption(ShowHTML,FMessengerOptions->checkOption(ShowHTML));
  setOption(ShowDateTime,FMessengerOptions->checkOption(ShowDateTime));
  setOption(ShowStatus,FMessengerOptions->checkOption(ShowStatus));
  setDefaultChatFont(FMessengerOptions->chatFont());
  setDefaultMessageFont(FMessengerOptions->messageFont());
  emit optionsAccepted();
}

void Messenger::onOptionsDialogRejected()
{
  emit optionsRejected();
}

void Messenger::onShowWindowAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid contactJid = action->data(ADR_ContactJid).toString();
    if (action->data(ADR_WindowType).toInt() == WT_MessageWindow)
    {
      openWindow(streamJid,contactJid,Message::Normal);
      QString group = action->data(ADR_Group).toString();
      if (!group.isEmpty())
      {
        IMessageWindow *window = findMessageWindow(streamJid,contactJid);
        if (window)
          window->receiversWidget()->addReceiversGroup(group);
      }
    }
    else
      openWindow(streamJid,contactJid,Message::Chat);
  }
}

void Messenger::onTextLoadResource(int AType, const QUrl &AName, QVariant &AValue)
{
  QMultiMap<int,IResourceLoader *>::const_iterator it = FResourceLoaders.constBegin();
  while(!AValue.isValid() && it!=FResourceLoaders.constEnd())
  {
    it.value()->loadResource(AType,AName,AValue);
    it++;
  }
}

Q_EXPORT_PLUGIN2(MessengerPlugin, Messenger)
