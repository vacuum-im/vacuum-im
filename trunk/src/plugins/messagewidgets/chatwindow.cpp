#include "chatwindow.h"

#include <QTextDocumentFragment>

#define BDI_CHAT_GEOMETRY           "ChatWindowGeometry"
#define BDI_CHAT_SPLITTER           "ChatWindowSplitter"  

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FSettings = NULL;
  FStatusChanger = NULL;
  FSplitterLoaded = false;

  FMessageWidgets = AMessageWidgets;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout);
  ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
  ui.wdtInfo->layout()->setMargin(0);
  connect(FInfoWidget->instance(),SIGNAL(fieldChanged(IInfoWidget::InfoField, const QVariant &)),
    SLOT(onInfoFieldChanged(IInfoWidget::InfoField, const QVariant &)));

  FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid);
  FViewWidget->setShowKind(IViewWidget::ChatMessage);
  FViewWidget->document()->setDefaultFont(FMessageWidgets->defaultChatFont());
  ui.wdtView->setLayout(new QVBoxLayout);
  ui.wdtView->layout()->addWidget(FViewWidget->instance());
  ui.wdtView->layout()->setMargin(0);
  ui.wdtView->layout()->setSpacing(0);

  FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid);
  FEditWidget->document()->setDefaultFont(FMessageWidgets->defaultChatFont());
  ui.wdtEdit->setLayout(new QVBoxLayout);
  ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
  ui.wdtEdit->layout()->setMargin(0);
  ui.wdtEdit->layout()->setSpacing(0);
  connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

  FToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
  ui.wdtView->layout()->addWidget(FToolBarWidget->instance());

  ui.sprSplitter->setStretchFactor(0,20);
  ui.sprSplitter->setStretchFactor(1,1);

  connect(FMessageWidgets->instance(),SIGNAL(defaultChatFontChanged(const QFont &)), SLOT(onDefaultChatFontChanged(const QFont &)));

  initialize();
}

ChatWindow::~ChatWindow()
{
  emit windowDestroyed();
  delete FInfoWidget->instance();
  delete FViewWidget->instance();
  delete FEditWidget->instance();
  delete FToolBarWidget->instance();
}

void ChatWindow::setContactJid(const Jid &AContactJid)
{
  if (FMessageWidgets->findChatWindow(FStreamJid,AContactJid) == NULL)
  {
    Jid befour = FContactJid;
    FContactJid = AContactJid;
    FInfoWidget->setContactJid(FContactJid);
    FViewWidget->setContactJid(FContactJid);
    FEditWidget->setContactJid(FContactJid);
    emit contactJidChanged(befour);
  }
}

bool ChatWindow::isActive() const
{
  return isVisible() && isActiveWindow();
}

void ChatWindow::showMessage(const Message &AMessage)
{
  FViewWidget->showMessage(AMessage);
}

void ChatWindow::showWindow()
{
  if (isWindow())
  {
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
    raise();
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

void ChatWindow::updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle)
{
  setWindowIcon(AIcon);
  setWindowIconText(AIconText);
  setWindowTitle(ATitle);
  emit windowChanged();
}

void ChatWindow::initialize()
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
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    }
  }

  plugin = FMessageWidgets->pluginManager()->getPlugins("IStatusChanger").value(0,NULL);
  if (plugin)
  {
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
  }
}

void ChatWindow::saveWindowState()
{
  if (FSettings)
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->saveBinaryData(BDI_CHAT_GEOMETRY + dataId,saveGeometry());
    FSettings->saveBinaryData(BDI_CHAT_SPLITTER + dataId,ui.sprSplitter->saveState());
  }
}

void ChatWindow::loadWindowState()
{
  if (FSettings)
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->loadBinaryData(BDI_CHAT_GEOMETRY+dataId));
    if (!FSplitterLoaded)
    {
      ui.sprSplitter->restoreState(FSettings->loadBinaryData(BDI_CHAT_SPLITTER+dataId));
      FSplitterLoaded = true;
    }
  }
  FEditWidget->textEdit()->setFocus();
}

bool ChatWindow::event(QEvent *AEvent)
{
  if (AEvent->type() == QEvent::WindowActivate)
    emit windowActivated();
  return QMainWindow::event(AEvent);
}

void ChatWindow::showEvent(QShowEvent *AEvent)
{
  loadWindowState();
  emit windowActivated();
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
  emit messageReady();
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

void ChatWindow::onInfoFieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue)
{
  if (AField == IInfoWidget::ContactStatus)
  {
    if (FMessageWidgets->checkOption(IMessageWidgets::ShowStatus))
    {
      QString status = AValue.toString();
      QString show = FStatusChanger ? FStatusChanger->nameByShow(FInfoWidget->field(IInfoWidget::ContactShow).toInt()) : "";
      if (FLastStatusShow != status+show)
      {
        QString nick = FViewWidget->nickForJid(FContactJid);
        QString html = QString("<span style='color:green;'>*** %1 [%2] %3</span>").arg(Qt::escape(nick)).arg(Qt::escape(show))
                                                                                  .arg(Qt::escape(status));
        FViewWidget->showCustomMessage(html,QDateTime::currentDateTime());
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

