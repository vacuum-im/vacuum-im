#include "messagewindow.h"

#include <QHeaderView>

#define IN_NORMAL_MESSAGE           "psi/sendMessage"

#define SVN_MESSAGEWINDOWS          "messageWindows:window[]"
#define SVN_GEOMETRY                SVN_MESSAGEWINDOWS ":geometry"

MessageWindow::MessageWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
  setAttribute(Qt::WA_DeleteOnClose,true);
  ui.setupUi(this);

  FPresence = NULL;
  FStatusIcons = NULL;
  FSettings = NULL;

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FMode = AMode;
  FMessageId = 0;
  FCurrentThreadId = QUuid::createUuid().toString();

  FInfoWidget = FMessenger->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout);
  ui.wdtInfo->layout()->addWidget(FInfoWidget);
  ui.wdtInfo->layout()->setMargin(0);

  FViewWidget = FMessenger->newViewWidget(AStreamJid,AContactJid);
  FViewWidget->setShowKind(IViewWidget::SingleMessage);
  FViewWidget->document()->setDefaultFont(FMessenger->defaultMessageFont());

  FEditWidget = FMessenger->newEditWidget(AStreamJid,AContactJid);
  FEditWidget->setSendMessageKey(-1);
  FEditWidget->document()->setDefaultFont(FMessenger->defaultMessageFont());

  FReceiversWidget = FMessenger->newReceiversWidget(FStreamJid);
  connect(FReceiversWidget,SIGNAL(receiverAdded(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
  connect(FReceiversWidget,SIGNAL(receiverRemoved(const Jid &)),SLOT(onReceiversChanged(const Jid &)));

  FViewToolBarWidget = FMessenger->newToolBarWidget(FInfoWidget,FViewWidget,NULL,NULL);
  FEditToolBarWidget = FMessenger->newToolBarWidget(FInfoWidget,NULL,FEditWidget,NULL);
  ui.wdtToolBar->setLayout(new QVBoxLayout);
  ui.wdtToolBar->layout()->setMargin(0);

  ui.wdtMessage ->setLayout(new QVBoxLayout);
  ui.wdtMessage->layout()->setMargin(0);

  ui.wdtTabs->setCurrentWidget(ui.tabMessage);

  connect(FMessenger->instance(),SIGNAL(messageReceived(const Message &)),SLOT(onMessageReceived(const Message &)));
  connect(FMessenger->instance(),SIGNAL(defaultMessageFontChanged(const QFont &)), SLOT(onDefaultMessageFontChanged(const QFont &)));

  connect(ui.pbtSend,SIGNAL(clicked()),SLOT(onSendButtonClicked()));
  connect(ui.pbtReply,SIGNAL(clicked()),SLOT(onReplyButtonClicked()));
  connect(ui.pbtForward,SIGNAL(clicked()),SLOT(onForwardButtonClicked()));
  connect(ui.pbtChat,SIGNAL(clicked()),SLOT(onChatButtonClicked()));
  connect(ui.pbtNext,SIGNAL(clicked()),SLOT(onNextButtonClicked()));

  initialize();
}

MessageWindow::~MessageWindow()
{
  emit windowDestroyed();
}

void MessageWindow::addTabWidget(QWidget *AWidget)
{
  ui.wdtTabs->addTab(AWidget,AWidget->windowIcon(),AWidget->windowIconText());
}

void MessageWindow::removeTabWidget(QWidget *AWidget)
{
  ui.wdtTabs->removeTab(ui.wdtTabs->indexOf(AWidget));
}

void MessageWindow::setMode(Mode AMode)
{
  FMode = AMode;
  if (AMode == ReadMode)
  {
    ui.wdtMessage->layout()->removeWidget(FEditWidget);
    ui.wdtMessage->layout()->addWidget(FViewWidget);
    ui.wdtToolBar->layout()->removeWidget(FEditToolBarWidget);
    ui.wdtToolBar->layout()->addWidget(FViewToolBarWidget);
    removeTabWidget(FReceiversWidget);
  }
  else
  {
    FReceiversWidget->addReceiver(FContactJid);
    ui.wdtMessage->layout()->removeWidget(FViewWidget);
    ui.wdtMessage->layout()->addWidget(FEditWidget);
    ui.wdtToolBar->layout()->removeWidget(FViewToolBarWidget);
    ui.wdtToolBar->layout()->addWidget(FEditToolBarWidget);
    addTabWidget(FReceiversWidget);
  }
  ui.wdtReceivers->setVisible(FMode == WriteMode);
  ui.wdtInfo->setVisible(FMode == ReadMode);
  ui.lneSubject->setReadOnly(FMode == ReadMode);
  ui.lblReceived->setVisible(FMode == ReadMode);
  ui.lblDateTime->setVisible(FMode == ReadMode);
  ui.pbtSend->setVisible(FMode == WriteMode);
  ui.pbtReply->setVisible(FMode == ReadMode);
  ui.pbtForward->setVisible(FMode == ReadMode);
  ui.pbtChat->setVisible(FMode == ReadMode);
}

void MessageWindow::showWindow()
{
  if (isWindow())
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
  else
    emit windowShow();
}

void MessageWindow::closeWindow()
{
  if (isWindow())
    close();
  else
    emit windowClose();
}

void MessageWindow::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    IPresencePlugin *presencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (presencePlugin)
    {
      FPresence = presencePlugin->getPresence(FStreamJid);
      if (FPresence)
      {
        connect(FPresence->instance(),SIGNAL(presenceItem(IPresenceItem *)),SLOT(onPresenceItem(IPresenceItem *)));
        connect(FPresence->xmppStream()->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
          SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
      }
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

  plugin = FMessenger->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSENGER_UUID);
    }
  }

  FInfoWidget->autoSetFields();
  setMode(FMode);
  loadActiveMessages();
  updateWindow();
}

void MessageWindow::saveWindowState()
{
  if (FSettings)
  {
    FSettingsValueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->setValueNS(SVN_GEOMETRY,FSettingsValueNS,saveGeometry());
  }
}

void MessageWindow::loadWindowState()
{
  if (FSettings)
  {
    FSettingsValueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->valueNS(SVN_GEOMETRY,FSettingsValueNS).toByteArray());
  }
  if (FMode == WriteMode)
    FEditWidget->textEdit()->setFocus();
}

void MessageWindow::loadActiveMessages()
{
  QList<int> messagesId = FMessenger->messages(FStreamJid,FContactJid);
  foreach(int messageId, messagesId)
  {
    Message message = FMessenger->messageById(messageId);
    onMessageReceived(message);
  }
}

void MessageWindow::removeActiveMessage(int AMessageId)
{
  if (FActiveMessages.contains(AMessageId))
  {
    FMessenger->removeMessage(AMessageId);
    FActiveMessages.removeAt(FActiveMessages.indexOf(AMessageId));
    updateWindow();
  }
}

void MessageWindow::updateWindow()
{
  if (!FActiveMessages.isEmpty())
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    setWindowIcon(iconset->iconByName(IN_NORMAL_MESSAGE));
  }
  else if (FStatusIcons)
    setWindowIcon(FStatusIcons->iconByJid(FStreamJid,FContactJid));

  if (FMode == ReadMode)
  {
    QString contactName = FInfoWidget->field(IInfoWidget::ContactName).toString();
    setWindowTitle(tr("%1 - Message").arg(contactName));
    setWindowIconText(windowTitle());
  }
  else
  {
    setWindowTitle(tr("Composing message"));
    setWindowIconText(windowTitle());
  }

  ui.pbtNext->setVisible(!FActiveMessages.isEmpty());
  ui.pbtNext->setText(tr("Next - %1").arg(FActiveMessages.count()));

  emit windowChanged();
}

void MessageWindow::showMessage(const Message &AMessage)
{
  FMessage = AMessage;
  FMessageId = FMessage.data(MDR_MESSAGEID).toInt();
  FCurrentThreadId = FMessage.threadId();
  ui.lneSubject->setText(FMessage.subject());
  if (QDateTime::currentDateTime().date() == FMessage.dateTime().date())
    ui.lblDateTime->setText(FMessage.dateTime().toString(tr("hh:mm:ss")));
  else
    ui.lblDateTime->setText(FMessage.dateTime().toString(tr("dd MMM hh:mm")));
  if (FMessage.type() == Message::Error)
    showErrorMessage(FMessage);
  else
    FViewWidget->showMessage(FMessage);
  FMessenger->removeMessage(FMessageId);
}

void MessageWindow::showErrorMessage(const Message &AMessage)
{
  QTextDocument doc;
  doc.setDefaultFont(FViewWidget->document()->defaultFont());
  FMessenger->messageToText(&doc,AMessage);
  ErrorHandler err(AMessage.stanza().element());
  
  QTextCursor cursor(&doc);
  QTextCharFormat format;
  format.setFontWeight(QFont::Bold);
  cursor.insertText(tr("The message with a error code %1 is received.").arg(err.code()),format);
  cursor.insertBlock();
  cursor.insertBlock();
  format.setForeground(Qt::red);
  cursor.insertText(err.message(),format);
  cursor.insertBlock();
  format.setForeground(Qt::black);
  cursor.insertText("______________");
  cursor.insertBlock();

  FViewWidget->document()->clear();
  FViewWidget->showCustomHtml(doc.toHtml());
}

void MessageWindow::showNextOrClose()
{
  if (FActiveMessages.isEmpty())
    closeWindow();
  else
    onNextButtonClicked();
}

void MessageWindow::setContactJid( const Jid &AContactJid )
{
  Jid befour = FContactJid;
  FContactJid = AContactJid;
  FInfoWidget->setContactJid(FContactJid);
  FViewWidget->setContactJid(FContactJid);
  FEditWidget->setContactJid(FContactJid);
  emit contactJidChanged(befour);
}

void MessageWindow::showEvent(QShowEvent *AEvent)
{
  loadWindowState();
  QMainWindow::showEvent(AEvent);
}

void MessageWindow::closeEvent(QCloseEvent *AEvent)
{
  saveWindowState();
  emit windowClosed();
  QMainWindow::closeEvent(AEvent);
}

void MessageWindow::onMessageReceived(const Message &AMessage)
{
  Jid fromJid = AMessage.from();
  Jid toJid = AMessage.to();
  if ((AMessage.type() == Message::Normal || AMessage.type() == Message::Headline ||  AMessage.type() == Message::Error) 
    && fromJid == FContactJid && toJid == FStreamJid)
  {
    if (FMessageId == 0 && FMode == ReadMode)
    {
      showMessage(AMessage);
    }
    else
    {
      FActiveMessages.append(AMessage.data(MDR_MESSAGEID).toInt());
      updateWindow();
    }
  }
}

void MessageWindow::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  if (FStreamJid && AXmppStream->jid())
  {
    FStreamJid = AXmppStream->jid();
    FInfoWidget->setStreamJid(FStreamJid);
    FViewWidget->setStreamJid(FStreamJid);
    FEditWidget->setStreamJid(FStreamJid);
    emit streamJidChanged(ABefour);
  }
  else
    deleteLater();
}

void MessageWindow::onPresenceItem(IPresenceItem *APresenceItem)
{
  if (FContactJid && APresenceItem->jid())
  {
    if (FContactJid.resource().isEmpty())
      setContactJid(APresenceItem->jid());
    updateWindow();
  }
}

void MessageWindow::onStatusIconsChanged()
{
  updateWindow();
}

void MessageWindow::onSendButtonClicked()
{
  Message message;
  message.setFrom(FStreamJid.eFull()).setType(Message::Normal).setThreadId(FCurrentThreadId);
  message.setSubject(ui.lneSubject->text());
  FMessenger->textToMessage(message,FEditWidget->document());
  if (!message.body().isEmpty())
  {
    bool sended = false;
    QList<Jid> receiversList = FReceiversWidget->receivers();
    foreach(Jid receiver, receiversList)
    {
      message.setTo(receiver.eFull());
      sended = FMessenger->sendMessage(message,FStreamJid) ? true : sended;
    }
    if (sended)
    {
      ui.lneSubject->clear();
      FEditWidget->clearEditor();
      showNextOrClose();
    }
  }
}

void MessageWindow::onReplyButtonClicked()
{
  setMode(WriteMode);
  updateWindow();
  ui.lneSubject->setText(tr("Re: ")+ui.lneSubject->text());
  FEditWidget->clearEditor();
  FEditWidget->textEdit()->setFocus();
}

void MessageWindow::onForwardButtonClicked()
{
  FReceiversWidget->removeReceiver(FContactJid);
  setContactJid(Jid());
  setMode(WriteMode);
  updateWindow();
  ui.lneSubject->setText(tr("Fw: ")+ui.lneSubject->text());
  FEditWidget->clearEditor();
  FMessenger->messageToText(FEditWidget->document(),FMessage);
  ui.wdtTabs->setCurrentWidget(FReceiversWidget);
}

void MessageWindow::onChatButtonClicked()
{
  FMessenger->openChatWindow(FStreamJid,FContactJid);
  showNextOrClose();
}

void MessageWindow::onNextButtonClicked()
{
  if (!FActiveMessages.isEmpty())
  {
    Message message = FMessenger->messageById(FActiveMessages.first());
    FActiveMessages.removeAt(0);
    setMode(ReadMode);
    showMessage(message);
    updateWindow();
  }
}

void MessageWindow::onReceiversChanged(const Jid &/*AReceiver*/)
{
  QString receiversStr;
  foreach(Jid contactJid,FReceiversWidget->receivers())
    receiversStr += QString("%1; ").arg(FReceiversWidget->receiverName(contactJid));
  ui.lblReceivers->setText(receiversStr);
}

void MessageWindow::onDefaultMessageFontChanged(const QFont &AFont)
{
  FViewWidget->document()->setDefaultFont(AFont);
  FEditWidget->document()->setDefaultFont(AFont);
}

