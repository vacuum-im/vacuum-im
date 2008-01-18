#include "messagewindow.h"

#include <QHeaderView>

#define IN_NORMAL_MESSAGE           "psi/sendMessage"

#define SVN_MESSAGEWINDOWS          "messageWindows:window[]"
#define SVN_GEOMETRY                SVN_MESSAGEWINDOWS ":geometry"

MessageWindow::MessageWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FMessenger = AMessenger;
  FSettings = NULL;

  FNextCount = 0;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FMode = AMode;
  FCurrentThreadId = QUuid::createUuid().toString();

  FInfoWidget = FMessenger->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout(ui.wdtInfo));
  ui.wdtInfo->layout()->addWidget(FInfoWidget);
  ui.wdtInfo->layout()->setMargin(0);

  FViewWidget = FMessenger->newViewWidget(AStreamJid,AContactJid);
  FViewWidget->setShowKind(IViewWidget::NormalMessage);
  FViewWidget->document()->setDefaultFont(FMessenger->defaultMessageFont());

  FEditWidget = FMessenger->newEditWidget(AStreamJid,AContactJid);
  FEditWidget->setSendMessageKey(-1);
  FEditWidget->document()->setDefaultFont(FMessenger->defaultMessageFont());

  FReceiversWidget = FMessenger->newReceiversWidget(FStreamJid);
  connect(FReceiversWidget,SIGNAL(receiverAdded(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
  connect(FReceiversWidget,SIGNAL(receiverRemoved(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
  FReceiversWidget->addReceiver(FContactJid);

  FViewToolBarWidget = FMessenger->newToolBarWidget(FInfoWidget,FViewWidget,NULL,NULL);
  FEditToolBarWidget = FMessenger->newToolBarWidget(FInfoWidget,NULL,FEditWidget,NULL);
  ui.wdtToolBar->setLayout(new QVBoxLayout(ui.wdtToolBar));
  ui.wdtToolBar->layout()->setMargin(0);

  ui.wdtMessage->setLayout(new QVBoxLayout(ui.wdtMessage));
  ui.wdtMessage->layout()->setMargin(0);

  connect(ui.pbtSend,SIGNAL(clicked()),SLOT(onSendButtonClicked()));
  connect(ui.pbtReply,SIGNAL(clicked()),SLOT(onReplyButtonClicked()));
  connect(ui.pbtForward,SIGNAL(clicked()),SLOT(onForwardButtonClicked()));
  connect(ui.pbtChat,SIGNAL(clicked()),SLOT(onChatButtonClicked()));
  connect(ui.pbtNext,SIGNAL(clicked()),SLOT(onNextButtonClicked()));
  connect(FMessenger->instance(),SIGNAL(defaultMessageFontChanged(const QFont &)), SLOT(onDefaultMessageFontChanged(const QFont &)));

  initialize();
  setCurrentTabWidget(ui.tabMessage);
  setMode(FMode);
  setNextCount(FNextCount);
}

MessageWindow::~MessageWindow()
{
  emit windowDestroyed();
  delete FInfoWidget;
  delete FViewWidget;
  delete FEditWidget;
  delete FReceiversWidget;
  delete FViewToolBarWidget;
  delete FEditToolBarWidget;
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

void MessageWindow::setContactJid(const Jid &AContactJid)
{
  if (FMessenger->findMessageWindow(FStreamJid,AContactJid) == NULL)
  {
    Jid befour = FContactJid;
    FContactJid = AContactJid;
    FInfoWidget->setContactJid(FContactJid);
    FViewWidget->setContactJid(FContactJid);
    FEditWidget->setContactJid(FContactJid);
    emit contactJidChanged(befour);
  }
}

void MessageWindow::addTabWidget(QWidget *AWidget)
{
  ui.wdtTabs->addTab(AWidget,AWidget->windowIconText());
}

void MessageWindow::setCurrentTabWidget(QWidget *AWidget)
{
  if (AWidget)
    ui.wdtTabs->setCurrentWidget(AWidget);
  else
    ui.wdtTabs->setCurrentWidget(ui.wdtMessage);
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
    FEditWidget->setParent(NULL);
    FEditToolBarWidget->setParent(NULL);
    removeTabWidget(FReceiversWidget);
  }
  else
  {
    ui.wdtMessage->layout()->removeWidget(FViewWidget);
    ui.wdtMessage->layout()->addWidget(FEditWidget);
    ui.wdtToolBar->layout()->removeWidget(FViewToolBarWidget);
    ui.wdtToolBar->layout()->addWidget(FEditToolBarWidget);
    FViewWidget->setParent(NULL);
    FViewToolBarWidget->setParent(NULL);
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

void MessageWindow::setSubject(const QString &ASubject)
{
  ui.lneSubject->setText(ASubject);
}

void MessageWindow::setThreadId( const QString &AThreadId )
{
  FCurrentThreadId = AThreadId;
}

void MessageWindow::setNextCount(int ACount)
{
  if (ACount > 0)
    ui.pbtNext->setText(tr("Next - %1").arg(ACount));
  else
    ui.pbtNext->setText(tr("Close"));
  FNextCount = ACount;
}

void MessageWindow::showMessage(const Message &AMessage)
{
  setMode(IMessageWindow::ReadMode);
  FMessage = AMessage;
  setSubject(FMessage.subject());
  setThreadId(FMessage.threadId());
  if (QDateTime::currentDateTime().date() == FMessage.dateTime().date())
    ui.lblDateTime->setText(FMessage.dateTime().toString(tr("hh:mm:ss")));
  else
    ui.lblDateTime->setText(FMessage.dateTime().toString(tr("dd MMM hh:mm")));
  if (FMessage.type() == Message::Error)
    showErrorMessage(FMessage);
  else
    FViewWidget->showMessage(FMessage);
}

void MessageWindow::initialize()
{
  IPlugin *plugin = FMessenger->pluginManager()->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      IXmppStream *xmppStream = xmppStreams->getStream(FStreamJid);
      if (xmppStream)
      {
        connect(xmppStream->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),
          SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
      }
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
      FSettings = settingsPlugin->settingsForPlugin(MESSENGER_UUID);
  }
}

void MessageWindow::saveWindowState()
{
  if (FSettings)
  {
    QString valueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->setValueNS(SVN_GEOMETRY,valueNS,saveGeometry());
  }
}

void MessageWindow::loadWindowState()
{
  if (FSettings)
  {
    QString valueNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->valueNS(SVN_GEOMETRY,valueNS).toByteArray());
  }
  if (FMode == WriteMode)
    FEditWidget->textEdit()->setFocus();
}

void MessageWindow::updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle)
{
  setWindowIcon(AIcon);
  setWindowIconText(AIconText);
  setWindowTitle(ATitle);
  emit windowChanged();
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

void MessageWindow::onSendButtonClicked()
{
  emit messageReady();
}

void MessageWindow::onNextButtonClicked()
{
  if (FNextCount > 0)
    emit showNextMessage();
  else
    close();
}

void MessageWindow::onReplyButtonClicked()
{
  emit replyMessage();
}

void MessageWindow::onForwardButtonClicked()
{
  emit forwardMessage();
}

void MessageWindow::onChatButtonClicked()
{
  emit showChatWindow();
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

