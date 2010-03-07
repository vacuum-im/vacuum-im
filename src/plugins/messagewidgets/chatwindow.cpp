#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>

#define SVN_CHATWINDOW              "chatWindow[]"
#define SVN_CHAT_TABWINDOW_ID       SVN_CHATWINDOW":tabWindowId"
#define BDI_CHAT_GEOMETRY           "ChatWindowGeometry"

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FSettings = NULL;
  FStatusChanger = NULL;
  FMessageWidgets = AMessageWidgets;

  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FShownDetached = false;

  FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
  ui.wdtInfo->setLayout(new QVBoxLayout);
  ui.wdtInfo->layout()->setMargin(0);
  ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
  onShowInfoWidgetChanged(FMessageWidgets->showInfoWidgetInChatWindow());
  connect(FMessageWidgets->instance(),SIGNAL(showInfoWidgetInChatWindowChanged(bool)),SLOT(onShowInfoWidgetChanged(bool)));

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

QString ChatWindow::tabPageId() const
{
  return "ChatWindow|"+FStreamJid.pBare()+"|"+FContactJid.pBare();
}

void ChatWindow::showWindow()
{
  if (isWindow())
  {
    isVisible() ? (isMinimized() ? showNormal() : activateWindow()) : show();
    WidgetManager::raiseWidget(this);
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

void ChatWindow::updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle)
{
  setWindowIcon(AIcon);
  setWindowIconText(AIconText);
  setWindowTitle(ATitle);
  emit windowChanged();
}

void ChatWindow::initialize()
{
  IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      IXmppStream *xmppStream = xmppStreams->xmppStream(FStreamJid);
      if (xmppStream)
      {
        connect(xmppStream->instance(),SIGNAL(jidChanged(const Jid &)), SLOT(onStreamJidChanged(const Jid &)));
      }
    }
  }

  plugin = FMessageWidgets->pluginManager()->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    ISettingsPlugin *settingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (settingsPlugin)
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    }
  }

  plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
  if (plugin)
  {
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
  }
}

void ChatWindow::saveWindowGeometry()
{
  if (FSettings && isWindow())
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    FSettings->saveBinaryData(BDI_CHAT_GEOMETRY"|"+dataId,saveGeometry());
  }
}

void ChatWindow::loadWindowGeometry()
{
  if (FSettings && isWindow())
  {
    QString dataId = FStreamJid.pBare()+"|"+FContactJid.pBare();
    restoreGeometry(FSettings->loadBinaryData(BDI_CHAT_GEOMETRY"|"+dataId));
  }
}

bool ChatWindow::event(QEvent *AEvent)
{
  if (AEvent->type() == QEvent::KeyPress)
  {
    static QKeyEvent *sentEvent = NULL;
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(AEvent);
    if (sentEvent!=keyEvent && !keyEvent->text().isEmpty())
    {
      sentEvent = keyEvent;
      FEditWidget->textEdit()->setFocus();
      QCoreApplication::sendEvent(FEditWidget->textEdit(),AEvent);
      sentEvent = NULL;
      AEvent->accept();
      return true;
    }
  }
  else if (AEvent->type() == QEvent::WindowActivate)
  {
    emit windowActivated();
  }
  return QMainWindow::event(AEvent);
}

void ChatWindow::showEvent(QShowEvent *AEvent)
{
  if (!FShownDetached && isWindow())
    loadWindowGeometry();
  FShownDetached = isWindow();
  QMainWindow::showEvent(AEvent);
  FEditWidget->textEdit()->setFocus();
  emit windowActivated();
}

void ChatWindow::closeEvent(QCloseEvent *AEvent)
{
  if (FShownDetached)
    saveWindowGeometry();
  QMainWindow::closeEvent(AEvent);
  emit windowClosed();
}

void ChatWindow::onMessageReady()
{
  emit messageReady();
}

void ChatWindow::onStreamJidChanged(const Jid &ABefour)
{
  IXmppStream *xmppStream = qobject_cast<IXmppStream *>(sender());
  if (xmppStream)
  {
    if (FStreamJid && xmppStream->streamJid())
    {
      FStreamJid = xmppStream->streamJid();
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
}

void ChatWindow::onShowInfoWidgetChanged(bool AShow)
{
  FInfoWidget->instance()->setVisible(AShow);
}
