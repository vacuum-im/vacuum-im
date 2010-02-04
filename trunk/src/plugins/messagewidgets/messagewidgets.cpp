#include "messagewidgets.h"

#define SVN_TAB_WINDOWS_ENABLED     "tabWindowsEnabled"
#define SVN_CHATWINDOW_SHOW_STATUS  "chatWindowShowStatus"
#define SVN_EDITOR_AUTO_RESIZE      "editorAutoResize"
#define SVN_SHOW_INFO_WIDGET        "showInfoWidget"
#define SVN_EDITOR_MINIMUM_LINES    "editorMinimumLines"
#define SVN_EDITOR_SEND_KEY         "editorSendKey"
#define SVN_DEFAULT_TABWINDOW       "defaultTabWindow"
#define SVN_TABWINDOW               "tabWindow[]"
#define SVN_TABWINDOW_NAME          SVN_TABWINDOW":name"
#define SVN_TABPAGE                 "tabPage[]"
#define SVN_TABPAGE_PAGEID          SVN_TABPAGE":pageId"


MessageWidgets::MessageWidgets()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FSettingsPlugin = NULL;

  FTabWindowsEnabled = true;
  FChatWindowShowStatus = true;
  FEditorAutoResize = true;
  FShowInfoWidgetInChatWindow = true;
  FEditorMinimumLines = 1;
  FEditorSendKey = Qt::Key_Return;
}

MessageWidgets::~MessageWidgets()
{

}

void MessageWidgets::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Message Widgets Manager"); 
  APluginInfo->description = tr("Allows other modules to use standard widgets for messaging");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MessageWidgets::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin) 
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    }
  }

  plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin) 
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(jidAboutToBeChanged(IXmppStream *, const Jid &)),
        SLOT(onStreamJidAboutToBeChanged(IXmppStream *, const Jid &)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
    }
  }

  return true;
}

bool MessageWidgets::initObjects()
{
  if (FSettingsPlugin)
  {
    FSettingsPlugin->openOptionsNode(ON_MESSAGES,tr("Messages"),tr("Message window options"),MNI_NORMAL_MHANDLER_MESSAGE,ONO_MESSAGES);
    FSettingsPlugin->insertOptionsHolder(this);
  }
  insertUrlHandler(this,WUHO_MESSAGEWIDGETS_DEFAULT);
  return true;
}

QWidget *MessageWidgets::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MESSAGES)
  {
    AOrder = OWO_MESSAGES;
    MessengerOptions *widget = new MessengerOptions(this);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

bool MessageWidgets::widgetUrlOpen(IViewWidget * /*APage*/, const QUrl &AUrl, int /*AOrder*/)
{
  return QDesktopServices::openUrl(AUrl);
}

IInfoWidget *MessageWidgets::newInfoWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IInfoWidget *widget = new InfoWidget(this,AStreamJid,AContactJid);
  FCleanupHandler.add(widget->instance());
  emit infoWidgetCreated(widget);
  return widget;
}

IViewWidget *MessageWidgets::newViewWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IViewWidget *widget = new ViewWidget(this,AStreamJid,AContactJid);
  connect(widget->instance(),SIGNAL(urlClicked(const QUrl &)),SLOT(onViewWidgetUrlClicked(const QUrl &)));
  FCleanupHandler.add(widget->instance());
  emit viewWidgetCreated(widget);
  return widget;
}

IEditWidget *MessageWidgets::newEditWidget(const Jid &AStreamJid, const Jid &AContactJid)
{
  IEditWidget *widget = new EditWidget(this,AStreamJid,AContactJid);
  FCleanupHandler.add(widget->instance());
  emit editWidgetCreated(widget);
  return widget;
}

IReceiversWidget *MessageWidgets::newReceiversWidget(const Jid &AStreamJid)
{
  IReceiversWidget *widget = new ReceiversWidget(this,AStreamJid);
  FCleanupHandler.add(widget->instance());
  emit receiversWidgetCreated(widget);
  return widget;
}

IMenuBarWidget *MessageWidgets::newMenuBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
  IMenuBarWidget *widget = new MenuBarWidget(AInfo,AView,AEdit,AReceivers);
  FCleanupHandler.add(widget->instance());
  emit menuBarWidgetCreated(widget);
  return widget;
}

IToolBarWidget *MessageWidgets::newToolBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
  IToolBarWidget *widget = new ToolBarWidget(AInfo,AView,AEdit,AReceivers);
  FCleanupHandler.add(widget->instance());
  emit toolBarWidgetCreated(widget);
  return widget;
}

IStatusBarWidget *MessageWidgets::newStatusBarWidget(IInfoWidget *AInfo, IViewWidget *AView, IEditWidget *AEdit, IReceiversWidget *AReceivers)
{
  IStatusBarWidget *widget = new StatusBarWidget(AInfo,AView,AEdit,AReceivers);
  FCleanupHandler.add(widget->instance());
  emit statusBarWidgetCreated(widget);
  return widget;
}

QList<IMessageWindow *> MessageWidgets::messageWindows() const
{
  return FMessageWindows;
}

IMessageWindow *MessageWidgets::newMessageWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageWindow::Mode AMode)
{
  IMessageWindow *window = findMessageWindow(AStreamJid,AContactJid);
  if (!window)
  {
    window = new MessageWindow(this,AStreamJid,AContactJid,AMode);
    FMessageWindows.append(window);
    connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onMessageWindowDestroyed()));
    FCleanupHandler.add(window->instance());
    emit messageWindowCreated(window);
    return window;
  }
  return NULL;
}

IMessageWindow *MessageWidgets::findMessageWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
  foreach(IMessageWindow *window,FMessageWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

QList<IChatWindow *> MessageWidgets::chatWindows() const
{
  return FChatWindows;
}

IChatWindow *MessageWidgets::newChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
  IChatWindow *window = findChatWindow(AStreamJid,AContactJid);
  if (!window)
  {
    window = new ChatWindow(this,AStreamJid,AContactJid);
    FChatWindows.append(window);
    connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onChatWindowDestroyed()));
    FCleanupHandler.add(window->instance());
    emit chatWindowCreated(window);
    return window;
  }
  return NULL;
}

IChatWindow *MessageWidgets::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid) const
{
  foreach(IChatWindow *window,FChatWindows)
    if (window->streamJid() == AStreamJid && window->contactJid() == AContactJid)
      return window;
  return NULL;
}

QList<QUuid> MessageWidgets::tabWindowList() const
{
  return FAvailTabWindows.keys();
}

QUuid MessageWidgets::appendTabWindow(const QString &AName)
{
  QUuid id = QUuid::createUuid();
  QString name = AName;
  if (name.isEmpty())
  {
    int i = 0;
    QList<QString> names = FAvailTabWindows.values();
    do 
    {
      i++;
      name = tr("Tab Window %1").arg(i);
    } while (names.contains(name));
  }
  FAvailTabWindows.insert(id,name);
  emit tabWindowAppended(id,name);
  return id;
}

void MessageWidgets::deleteTabWindow(const QUuid &AWindowId)
{
  if (FDefaultTabWindow!=AWindowId && FAvailTabWindows.contains(AWindowId))
  {
    ITabWindow *window = findTabWindow(AWindowId);
    if (window)
      window->instance()->deleteLater();
    FAvailTabWindows.remove(AWindowId);
    emit tabWindowDeleted(AWindowId);
  }
}

QString MessageWidgets::tabWindowName(const QUuid &AWindowId) const
{
  return FAvailTabWindows.value(AWindowId);
}

void MessageWidgets::setTabWindowName(const QUuid &AWindowId, const QString &AName)
{
  if (!AName.isEmpty() && FAvailTabWindows.contains(AWindowId))
  {
    FAvailTabWindows.insert(AWindowId,AName);
    emit tabWindowNameChanged(AWindowId,AName);
  }
}

QList<ITabWindow *> MessageWidgets::tabWindows() const
{
  return FTabWindows;
}

ITabWindow *MessageWidgets::openTabWindow(const QUuid &AWindowId)
{
  ITabWindow *window = findTabWindow(AWindowId);
  if (!window)
  {
    window = new TabWindow(this,AWindowId);
    FTabWindows.append(window);
    connect(window->instance(),SIGNAL(pageAdded(ITabWindowPage *)),SLOT(onTabWindowPageAdded(ITabWindowPage *)));
    connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onTabWindowDestroyed()));
    emit tabWindowCreated(window);
  }
  window->showWindow();
  return window;
}

ITabWindow *MessageWidgets::findTabWindow(const QUuid &AWindowId) const
{
  foreach(ITabWindow *window,FTabWindows)
    if (window->windowId() == AWindowId)
      return window;
  return NULL;
}

void MessageWidgets::assignTabWindowPage(ITabWindowPage *APage)
{
  if (tabWindowsEnabled())
  {
    QUuid windowId = FDefaultTabWindow;
    if (FSettingsPlugin)
    {
      ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
      windowId = settings->valueNS(SVN_TABPAGE_PAGEID,APage->tabPageId(),windowId.toString()).toString();
    }
    if (!FAvailTabWindows.contains(windowId))
      windowId = FDefaultTabWindow;
    ITabWindow *window = openTabWindow(windowId);
    window->addPage(APage);
  }
}

bool MessageWidgets::tabWindowsEnabled() const
{
  return FTabWindowsEnabled;
}

void MessageWidgets::setTabWindowsEnabled(bool AEnabled)
{
  if (FTabWindowsEnabled != AEnabled)
  {
    FTabWindowsEnabled = AEnabled;
    emit tabWindowsEnabledChanged(AEnabled);
  }
}

QUuid MessageWidgets::defaultTabWindow() const
{
  return FDefaultTabWindow;
}

void MessageWidgets::setDefaultTabWindow(const QUuid &AWindowId)
{
  if (FDefaultTabWindow!=AWindowId && FAvailTabWindows.contains(AWindowId))
  {
    FDefaultTabWindow = AWindowId;
    emit defaultTabWindowChanged(AWindowId);
  }
}

bool MessageWidgets::chatWindowShowStatus() const
{
  return FChatWindowShowStatus;
}

void MessageWidgets::setChatWindowShowStatus(bool AShow)
{
  if (FChatWindowShowStatus != AShow)
  {
    FChatWindowShowStatus = AShow;
    emit chatWindowShowStatusChanged(AShow);
  }
}

bool MessageWidgets::editorAutoResize() const
{
  return FEditorAutoResize;
}

void MessageWidgets::setEditorAutoResize(bool AResize)
{
  if (FEditorAutoResize != AResize)
  {
    FEditorAutoResize = AResize;
    emit editorAutoResizeChanged(AResize);
  }
}

bool MessageWidgets::showInfoWidgetInChatWindow() const
{
  return FShowInfoWidgetInChatWindow;
}

void MessageWidgets::setShowInfoWidgetInChatWindow(bool AShow)
{
  if (FShowInfoWidgetInChatWindow != AShow)
  {
    FShowInfoWidgetInChatWindow = AShow;
    emit showInfoWidgetInChatWindowChanged(AShow);
  }
}

int MessageWidgets::editorMinimumLines() const
{
  return FEditorMinimumLines;
}

void MessageWidgets::setEditorMinimumLines(int ALines)
{
  if (FEditorMinimumLines!=ALines && ALines>0)
  {
    FEditorMinimumLines = ALines;
    emit editorMinimumLinesChanged(ALines);
  }
}

QKeySequence MessageWidgets::editorSendKey() const
{
  return FEditorSendKey;
}

void MessageWidgets::setEditorSendKey(const QKeySequence &AKey)
{
  if (FEditorSendKey!=AKey && !AKey.isEmpty())
  {
    FEditorSendKey = AKey;
    emit editorSendKeyChanged(AKey);
  }
}

void MessageWidgets::insertUrlHandler(IWidgetUrlHandler *AHandler, int AOrder)
{
  if (!FUrlHandlers.values(AOrder).contains(AHandler))  
  {
    FUrlHandlers.insert(AOrder,AHandler);
    emit urlHandlerInserted(AHandler,AOrder);
  }
}

void MessageWidgets::removeUrlHandler(IWidgetUrlHandler *AHandler, int AOrder)
{
  if (FUrlHandlers.values(AOrder).contains(AHandler))  
  {
    FUrlHandlers.remove(AOrder,AHandler);
    emit urlHandlerRemoved(AHandler,AOrder);
  }
}

void MessageWidgets::deleteWindows()
{
  foreach(ITabWindow *window, tabWindows())
    delete window->instance();
}

void MessageWidgets::deleteStreamWindows(const Jid &AStreamJid)
{
  QList<IChatWindow *> chatWindows = FChatWindows;
  foreach(IChatWindow *window, chatWindows)
    if (window->streamJid() == AStreamJid)
      delete window->instance();

  QList<IMessageWindow *> messageWindows = FMessageWindows;
  foreach(IMessageWindow *window, messageWindows)
    if (window->streamJid() == AStreamJid)
      delete window->instance();
}

void MessageWidgets::onViewWidgetUrlClicked(const QUrl &AUrl)
{
  IViewWidget *widget = qobject_cast<IViewWidget *>(sender());
  if (widget)
  {
    for (QMap<int,IWidgetUrlHandler *>::const_iterator it = FUrlHandlers.constBegin(); it!=FUrlHandlers.constEnd(); it++)
      if (it.value()->widgetUrlOpen(widget,AUrl,it.key()))
        break;
  }
}

void MessageWidgets::onMessageWindowDestroyed()
{
  IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
  if (window)
  {
    FMessageWindows.removeAt(FMessageWindows.indexOf(window));
    emit messageWindowDestroyed(window);
  }
}

void MessageWidgets::onChatWindowDestroyed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    FChatWindows.removeAt(FChatWindows.indexOf(window));
    emit chatWindowDestroyed(window);
  }
}

void MessageWidgets::onTabWindowPageAdded(ITabWindowPage *APage)
{
  ITabWindow *window = qobject_cast<ITabWindow *>(sender());
  if (FSettingsPlugin && window)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    if (window->windowId() != FDefaultTabWindow)
      settings->setValueNS(SVN_TABPAGE_PAGEID,APage->tabPageId(),window->windowId().toString());
    else
      settings->deleteNS(APage->tabPageId());
  }
}

void MessageWidgets::onTabWindowDestroyed()
{
  ITabWindow *window = qobject_cast<ITabWindow *>(sender());
  if (window)
  {
    FTabWindows.removeAt(FTabWindows.indexOf(window));
    emit tabWindowDestroyed(window);
  }
}

void MessageWidgets::onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter)
{
  if (!(AAfter && AXmppStream->streamJid()))
    deleteStreamWindows(AXmppStream->streamJid());
}

void MessageWidgets::onStreamRemoved(IXmppStream *AXmppStream)
{
  deleteStreamWindows(AXmppStream->streamJid());
}

void MessageWidgets::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
  setTabWindowsEnabled(settings->value(SVN_TAB_WINDOWS_ENABLED,true).toBool());
  setChatWindowShowStatus(settings->value(SVN_CHATWINDOW_SHOW_STATUS,true).toBool());
  setEditorAutoResize(settings->value(SVN_EDITOR_AUTO_RESIZE,true).toBool());
  setShowInfoWidgetInChatWindow(settings->value(SVN_SHOW_INFO_WIDGET,true).toBool());
  setEditorMinimumLines(settings->value(SVN_EDITOR_MINIMUM_LINES,1).toInt());
  setEditorSendKey(QKeySequence::fromString(settings->value(SVN_EDITOR_SEND_KEY,FEditorSendKey.toString()).toString()));

  QHash<QString, QVariant> windows = settings->values(SVN_TABWINDOW_NAME);
  for (QHash<QString, QVariant>::const_iterator it = windows.constBegin(); it!=windows.constEnd(); it++)
    FAvailTabWindows.insert(it.key(),it.value().toString());

  FDefaultTabWindow = settings->value(SVN_DEFAULT_TABWINDOW).toString();
  if (!FAvailTabWindows.contains(FDefaultTabWindow))
    FDefaultTabWindow = appendTabWindow(tr("Main Tab Window"));
}

void MessageWidgets::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
  settings->setValue(SVN_TAB_WINDOWS_ENABLED,tabWindowsEnabled());
  settings->setValue(SVN_DEFAULT_TABWINDOW,FDefaultTabWindow.toString());
  settings->setValue(SVN_CHATWINDOW_SHOW_STATUS,chatWindowShowStatus());
  settings->setValue(SVN_EDITOR_AUTO_RESIZE,editorAutoResize());
  settings->setValue(SVN_SHOW_INFO_WIDGET,showInfoWidgetInChatWindow());
  settings->setValue(SVN_EDITOR_MINIMUM_LINES,editorMinimumLines());
  settings->setValue(SVN_EDITOR_SEND_KEY,FEditorSendKey.toString());

  QSet<QString> oldTabWindows = settings->values(SVN_TABWINDOW_NAME).keys().toSet();
  for (QMap<QUuid, QString>::const_iterator it = FAvailTabWindows.constBegin(); it!=FAvailTabWindows.constEnd(); it++)
  {
    settings->setValueNS(SVN_TABWINDOW_NAME,it.key(),it.value());
    oldTabWindows -= it.key().toString();
  }
  foreach(QString windowId, oldTabWindows)
  {
    settings->deleteNS(windowId);
  }
  deleteWindows();
}

Q_EXPORT_PLUGIN2(plg_messagewidgets, MessageWidgets)
