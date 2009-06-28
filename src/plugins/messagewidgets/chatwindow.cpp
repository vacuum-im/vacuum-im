#include "chatwindow.h"

#include <QTextDocumentFragment>

#define BDI_CHAT_GEOMETRY           "ChatWindowGeometry"

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FSettings = NULL;
  FStatusChanger = NULL;

  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FMessageWidgets = AMessageWidgets;

  FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout);
  ui.wdtInfo->layout()->setMargin(0);
  ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());

  FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid);
  ui.wdtView->setLayout(new QVBoxLayout);
  ui.wdtView->layout()->setMargin(0);
  ui.wdtView->layout()->addWidget(FViewWidget->instance());

  FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid);
  ui.wdtEdit->setLayout(new QVBoxLayout);
  ui.wdtEdit->layout()->setMargin(0);
  ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
  connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

  FMenuBarWidget = FMessageWidgets->newMenuBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
  setMenuBar(FMenuBarWidget->instance());

  FToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
  FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
  ui.wdtToolBar->setLayout(new QVBoxLayout);
  ui.wdtToolBar->layout()->setMargin(0);
  ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());

  FStatusBarWidget = FMessageWidgets->newStatusBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL);
  setStatusBar(FStatusBarWidget->instance());

  initialize();
}

ChatWindow::~ChatWindow()
{
  emit windowDestroyed();
  delete FInfoWidget->instance();
  delete FViewWidget->instance();
  delete FEditWidget->instance();
  delete FMenuBarWidget->instance();
  delete FToolBarWidget->instance();
  delete FStatusBarWidget->instance();
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
  if (FSettings && isWindow() && isVisible())
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    FSettings->saveBinaryData(BDI_CHAT_GEOMETRY + dataId,saveGeometry());
  }
}

void ChatWindow::loadWindowState()
{
  if (FSettings && isWindow())
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    restoreGeometry(FSettings->loadBinaryData(BDI_CHAT_GEOMETRY+dataId));
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
  {
    deleteLater();
  }
}

