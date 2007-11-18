#include <Qdebug>
#include "chatwindow.h"

#include <QTextDocumentFragment>

#define IN_CHAT_MESSAGE             "psi/start-chat"

#define SVN_CHATWINDOWS             "chatWindows:window[]"
#define SVN_GEOMETRY                SVN_CHATWINDOWS ":geometry"
#define SVN_SPLITTER                SVN_CHATWINDOWS ":splitter"  
#define SVN_SHOW_STATUS             SVN_CHATWINDOWS ":showStatus"


ChatWindow::ChatWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  FPresence = NULL;
  FStatusIcons = NULL;
  FSettings = NULL;
  FOptions = 0;
  FSplitterLoaded = false;

  ui.setupUi(this);

  FMessenger = AMessenger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  FInfoWidget = FMessenger->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout);
  ui.wdtInfo->layout()->addWidget(FInfoWidget);
  ui.wdtInfo->layout()->setMargin(0);
  connect(FInfoWidget,SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
    SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));

  FViewWidget = FMessenger->newViewWidget(AStreamJid,AContactJid);
  FViewWidget->setShowKind(IViewWidget::ChatMessage);
  FViewWidget->document()->setDefaultFont(FMessenger->defaultChatFont());
  ui.wdtView->setLayout(new QVBoxLayout);
  ui.wdtView->layout()->addWidget(FViewWidget);
  ui.wdtView->layout()->setMargin(0);
  ui.wdtView->layout()->setSpacing(0);

  FEditWidget = FMessenger->newEditWidget(AStreamJid,AContactJid);
  FEditWidget->document()->setDefaultFont(FMessenger->defaultChatFont());
  ui.wdtEdit->setLayout(new QVBoxLayout);
  ui.wdtEdit->layout()->addWidget(FEditWidget);
  ui.wdtEdit->layout()->setMargin(0);
  ui.wdtEdit->layout()->setSpacing(0);
  connect(FEditWidget,SIGNAL(messageReady()),SLOT(onMessageReady()));

  FToolBarWidget = FMessenger->newToolBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
  ui.wdtView->layout()->addWidget(FToolBarWidget);

  ui.sprSplitter->setStretchFactor(0,20);
  ui.sprSplitter->setStretchFactor(1,1);

  connect(FMessenger->instance(),SIGNAL(messageReceived(const Message &)),SLOT(onMessageReceived(const Message &)));
  connect(FMessenger->instance(),SIGNAL(defaultChatFontChanged(const QFont &)), SLOT(onDefaultChatFontChanged(const QFont &)));

  initialize();
}

ChatWindow::~ChatWindow()
{
  emit windowDestroyed();
}

void ChatWindow::showWindow()
{
  if (isWindow())
  {
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
  }
  else
    emit windowShow();
}

void ChatWindow::closeWindow()
{
  if (isWindow())
    close();
  else
    emit windowClose();
}

void ChatWindow::initialize()
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
  loadActiveMessages();
}

void ChatWindow::saveWindowState()
{
  if (FSettings)
  {
    FSettingsValueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->setValueNS(SVN_GEOMETRY,FSettingsValueNS,saveGeometry());
    FSettings->setValueNS(SVN_SPLITTER,FSettingsValueNS,ui.sprSplitter->saveState());
  }
}

void ChatWindow::loadWindowState()
{
  if (FSettings)
  {
    FSettingsValueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->valueNS(SVN_GEOMETRY,FSettingsValueNS).toByteArray());
    if (!FSplitterLoaded)
    {
      ui.sprSplitter->restoreState(FSettings->valueNS(SVN_SPLITTER,FSettingsValueNS).toByteArray());
      FSplitterLoaded = true;
    }

  }
  FEditWidget->textEdit()->setFocus();
}

void ChatWindow::loadActiveMessages()
{
  QList<int> messagesId = FMessenger->messages(FStreamJid,FContactJid,Message::Chat);
  foreach(int messageId, messagesId)
  {
    Message message = FMessenger->messageById(messageId);
    onMessageReceived(message);
  }
}

void ChatWindow::removeActiveMessages()
{
  if (!FActiveMessages.isEmpty() && isVisible() && isActiveWindow())
  {
    foreach(int messageId, FActiveMessages)
      FMessenger->removeMessage(messageId);
    FActiveMessages.clear();
    updateWindow();
  }
}

void ChatWindow::updateWindow()
{
  if (!FActiveMessages.isEmpty())
  {
    SkinIconset *iconset = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
    setWindowIcon(iconset->iconByName(IN_CHAT_MESSAGE));
  }
  else if (FStatusIcons)
    setWindowIcon(FStatusIcons->iconByJid(FStreamJid,FContactJid));
  
  QString contactName = FInfoWidget->field(IInfoWidget::ContactName).toString();
  setWindowIconText(contactName);
  setWindowTitle(tr("%1 - Chat").arg(contactName));
  
  emit windowChanged();
}

bool ChatWindow::event(QEvent *AEvent)
{
  if (AEvent->type() == QEvent::WindowActivate)
    removeActiveMessages();
  return QMainWindow::event(AEvent);
}

void ChatWindow::showEvent(QShowEvent *AEvent)
{
  loadWindowState();
  removeActiveMessages();
  QMainWindow::showEvent(AEvent);
}

void ChatWindow::closeEvent(QCloseEvent *AEvent)
{
  saveWindowState();
  emit windowClosed();
  QMainWindow::closeEvent(AEvent);
}

void ChatWindow::onMessageReady()
{
  Message message;
  message.setFrom(FStreamJid.eFull()).setTo(FContactJid.eFull()).setType(Message::Chat);
  FMessenger->textToMessage(message,FEditWidget->document());
  if (!message.body().isEmpty() && FMessenger->sendMessage(message,FStreamJid))
  {
    FViewWidget->showMessage(message);
    FEditWidget->clearEditor();
  }
}

void ChatWindow::onMessageReceived(const Message &AMessage)
{
  if (AMessage.type() == Message::Chat)
  {
    Jid fromJid = AMessage.from();
    Jid toJid = AMessage.to();
    if (fromJid == FContactJid && toJid == FStreamJid)
    {
      FViewWidget->showMessage(AMessage);
      int messageId = AMessage.data(MDR_MESSAGEID).toInt();
      if (!isVisible() || !isActiveWindow())
      {
        FActiveMessages.append(messageId);
        updateWindow();
      }
      else
        FMessenger->removeMessage(messageId);
    }
  }
}

void ChatWindow::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
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

void ChatWindow::onPresenceItem(IPresenceItem *APresenceItem)
{
  if (FContactJid && APresenceItem->jid())
  {
    if (FContactJid.resource().isEmpty())
    {
      Jid befour = FContactJid;
      FContactJid = APresenceItem->jid();
      FInfoWidget->setContactJid(FContactJid);
      FViewWidget->setContactJid(FContactJid);
      FEditWidget->setContactJid(FContactJid);
      emit contactJidChanged(befour);
    }
    updateWindow();
  }
}

void ChatWindow::onStatusIconsChanged()
{
  updateWindow();
}

void ChatWindow::onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
  if (AField == IInfoWidget::ContactName) 
  {
    QString selfName = FStreamJid && FContactJid ? FStreamJid.resource() : FStreamJid.node();
    FViewWidget->setNickForJid(FStreamJid,selfName);
    FViewWidget->setNickForJid(FContactJid,AValue.toString());
    updateWindow();
  }
  else if (AField == IInfoWidget::ContactStatus)
  {
    if (FMessenger->checkOption(IMessenger::ShowStatus))
    {
      QString status = FInfoWidget->field(IInfoWidget::ContactStatus).toString();
      QString show = FInfoWidget->field(IInfoWidget::ContactShow).toString();
      if (FLastStatusShow != status+show)
      {
        QString dateTime;
        if (FMessenger->checkOption(IMessenger::ShowDateTime))
          dateTime = QString("[%1] ").arg(QDateTime::currentDateTime().toString("hh::mm"));
        QString nick = FViewWidget->nickForJid(FContactJid);
        QString html = QString("<span style='color:green;'>%1*** %2 [%3] %4</span>").arg(dateTime).arg(Qt::escape(nick)).arg(show).arg(status);
        FViewWidget->showCustomHtml(html);
        FLastStatusShow = status+show;
      }
    }
  }
}

void ChatWindow::onDefaultChatFontChanged(const QFont &AFont)
{
  FViewWidget->document()->setDefaultFont(AFont);
  FEditWidget->document()->setDefaultFont(AFont);
}
