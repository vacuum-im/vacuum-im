#include "messagewindow.h"

#include <QHeaderView>

#define BDI_MESSAGE_GEOMETRY        "MessageWindowGeometry"

MessageWindow::MessageWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FMessageWidgets = AMessageWidgets;
  FSettings = NULL;

  FNextCount = 0;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FMode = AMode;
  FCurrentThreadId = QUuid::createUuid().toString();

  FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout(ui.wdtInfo));
  ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
  ui.wdtInfo->layout()->setMargin(0);

  FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid);

  FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid);
  FEditWidget->setSendMessageKey(QKeySequence());

  FReceiversWidget = FMessageWidgets->newReceiversWidget(FStreamJid);
  connect(FReceiversWidget->instance(),SIGNAL(receiverAdded(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
  connect(FReceiversWidget->instance(),SIGNAL(receiverRemoved(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
  FReceiversWidget->addReceiver(FContactJid);

  FViewToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,NULL,NULL);
  FViewToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

  FEditToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,NULL,FEditWidget,NULL);
  FEditToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

  ui.wdtToolBar->setLayout(new QVBoxLayout(ui.wdtToolBar));
  ui.wdtToolBar->layout()->setMargin(0);

  ui.wdtMessage->setLayout(new QVBoxLayout(ui.wdtMessage));
  ui.wdtMessage->layout()->setMargin(0);

  connect(ui.pbtSend,SIGNAL(clicked()),SLOT(onSendButtonClicked()));
  connect(ui.pbtReply,SIGNAL(clicked()),SLOT(onReplyButtonClicked()));
  connect(ui.pbtForward,SIGNAL(clicked()),SLOT(onForwardButtonClicked()));
  connect(ui.pbtChat,SIGNAL(clicked()),SLOT(onChatButtonClicked()));
  connect(ui.pbtNext,SIGNAL(clicked()),SLOT(onNextButtonClicked()));

  initialize();
  setCurrentTabWidget(ui.tabMessage);
  setMode(FMode);
  setNextCount(FNextCount);
}

MessageWindow::~MessageWindow()
{
  emit windowDestroyed();
  delete FInfoWidget->instance();
  delete FViewWidget->instance();
  delete FEditWidget->instance();
  delete FReceiversWidget->instance();
  delete FViewToolBarWidget->instance();
  delete FEditToolBarWidget->instance();
}

void MessageWindow::showWindow()
{
  if (isWindow())
  {
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
    raise();
  }
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
  if (FMessageWidgets->findMessageWindow(FStreamJid,AContactJid) == NULL)
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
    ui.wdtMessage->layout()->removeWidget(FEditWidget->instance());
    ui.wdtMessage->layout()->addWidget(FViewWidget->instance());
    ui.wdtToolBar->layout()->removeWidget(FEditToolBarWidget->instance());
    ui.wdtToolBar->layout()->addWidget(FViewToolBarWidget->instance());
    FEditWidget->instance()->setParent(NULL);
    FEditToolBarWidget->instance()->setParent(NULL);
    removeTabWidget(FReceiversWidget->instance());
  }
  else
  {
    ui.wdtMessage->layout()->removeWidget(FViewWidget->instance());
    ui.wdtMessage->layout()->addWidget(FEditWidget->instance());
    ui.wdtToolBar->layout()->removeWidget(FViewToolBarWidget->instance());
    ui.wdtToolBar->layout()->addWidget(FEditToolBarWidget->instance());
    FViewWidget->instance()->setParent(NULL);
    FViewToolBarWidget->instance()->setParent(NULL);
    addTabWidget(FReceiversWidget->instance());
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
  //else
  //  FViewWidget->setMessage(FMessage);
}

void MessageWindow::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->getPlugins("IXmppStreams").value(0,NULL);
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

  plugin = FMessageWidgets->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
      FSettings = settingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
  }
}

void MessageWindow::saveWindowState()
{
  if (FSettings)
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->saveBinaryData(BDI_MESSAGE_GEOMETRY+dataId,saveGeometry());
  }
}

void MessageWindow::loadWindowState()
{
  if (FSettings)
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->loadBinaryData(BDI_MESSAGE_GEOMETRY+dataId));
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
  QTextCursor cursor(&doc);
  QTextCharFormat format;
  ErrorHandler err(AMessage.stanza().element());
  format.setFontWeight(QFont::Bold);
  cursor.insertText(tr("The message with a error code %1 is received.").arg(err.code()),format);
  cursor.insertBlock();
  cursor.insertBlock();
  format.setForeground(Qt::red);
  cursor.insertText(err.message(),format);
  cursor.insertBlock();
  format.setForeground(Qt::black);
  cursor.insertText("______________",format);
  cursor.insertBlock();
  format.setFontWeight(QFont::Normal);
  cursor.insertText(AMessage.body(),format);

  //FViewWidget->setHtml(doc.toHtml());
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
