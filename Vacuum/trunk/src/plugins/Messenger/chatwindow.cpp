#include "chatwindow.h"

#include <QTextDocumentFragment>

#define SVN_CHATWINDOWS             "chatWindows:window[]"
#define SVN_GEOMETRY                SVN_CHATWINDOWS ":geometry"
#define SVN_SPLITTER                SVN_CHATWINDOWS ":splitter"  

ChatWindow::ChatWindow(IMessenger *AMessenger, const Jid& AStreamJid, const Jid &AContactJid)
{
  ui.setupUi(this);

  FSettings = NULL;
  FStatusChanger = NULL;
  FSplitterLoaded = false;

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

  connect(FMessenger->instance(),SIGNAL(defaultChatFontChanged(const QFont &)), SLOT(onDefaultChatFontChanged(const QFont &)));

  initialize();
}

ChatWindow::~ChatWindow()
{
  emit windowDestroyed();
  delete FInfoWidget;
  delete FViewWidget;
  delete FEditWidget;
  delete FToolBarWidget;
}

void ChatWindow::setContactJid(const Jid &AContactJid)
{
  if (FMessenger->findChatWindow(FStreamJid,AContactJid) == NULL)
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
    {
      FSettings = settingsPlugin->settingsForPlugin(MESSENGER_UUID);
    }
  }

  plugin = FMessenger->pluginManager()->getPlugins("IStatusChanger").value(0,NULL);
  if (plugin)
  {
    FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
  }
}

void ChatWindow::saveWindowState()
{
  if (FSettings)
  {
    QString valueNameNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow() && isVisible())
      FSettings->setValueNS(SVN_GEOMETRY,valueNameNS,saveGeometry());
    FSettings->setValueNS(SVN_SPLITTER,valueNameNS,ui.sprSplitter->saveState());
  }
}

void ChatWindow::loadWindowState()
{
  if (FSettings)
  {
    QString valueNameNS = FStreamJid.pBare()+" | "+FContactJid.pBare();
    if (isWindow())
      restoreGeometry(FSettings->valueNS(SVN_GEOMETRY,valueNameNS).toByteArray());
    if (!FSplitterLoaded)
    {
      ui.sprSplitter->restoreState(FSettings->valueNS(SVN_SPLITTER,valueNameNS).toByteArray());
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
    if (FMessenger->checkOption(IMessenger::ShowStatus))
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

