#include "messagewidgets.h"

#define SVN_USE_TABWINDOW           "useTabWindow"
#define SVN_SHOW_STATUS             "showStatus"
#define SVN_SEND_MESSAGE_KEY        "sendMessageKey"
#define SVN_DEFAULT_TABWINDOW       "defaultTabWindow"
#define SVN_TABWINDOW               "tabWindow[]"
#define SVN_TABWINDOW_NAME          SVN_TABWINDOW":name"
#define SVN_TABWIDGET               "tabWidget[]"
#define SVN_TABWIDGET_WINDOWID      SVN_TABWIDGET":windowId"


MessageWidgets::MessageWidgets()
{
  FPluginManager = NULL;
  FXmppStreams = NULL;
  FSettingsPlugin = NULL;

  FOptions = 0;
  FSendKey = Qt::Key_Return;
}

MessageWidgets::~MessageWidgets()
{

}

void MessageWidgets::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Manager of the widgets for displaying messages");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Message Widgets"); 
  APluginInfo->uid = MESSAGEWIDGETS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->conflicts.append("{153A4638-B468-496f-B57C-9F30CEDFCC2E}");
}

bool MessageWidgets::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
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

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
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
  insertUrlHandler(this,UHO_MESSAGEWIDGETS_DEFAULT);
  return true;
}

QWidget *MessageWidgets::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MESSAGES)
  {
    AOrder = OWO_MESSAGES;
    MessengerOptions *widget = new MessengerOptions(this);
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(widget,SIGNAL(optionsApplied()),SIGNAL(optionsAccepted()));
    return widget;
  }
  return NULL;
}

bool MessageWidgets::executeUrl(IViewWidget * /*AWidget*/, const QUrl &AUrl, int /*AOrder*/)
{
  return QDesktopServices::openUrl(AUrl);
}

QKeySequence MessageWidgets::sendMessageKey() const
{
  return FSendKey;
}

void MessageWidgets::setSendMessageKey(const QKeySequence &AKey)
{
  if (FSendKey != AKey)
  {
    FSendKey = AKey;
    emit sendMessageKeyChanged(AKey);
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
    connect(window->instance(),SIGNAL(widgetAdded(ITabWidget *)),SLOT(onTabWindowWidgetAdded(ITabWidget *)));
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

void MessageWidgets::assignTabWindow(ITabWidget *AWidget)
{
  if (checkOption(IMessageWidgets::UseTabWindow))
  {
    QUuid windowId = FDefaultTabWindow;
    if (FSettingsPlugin)
    {
      ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
      windowId = settings->valueNS(SVN_TABWIDGET_WINDOWID,AWidget->tabWidgetId(),windowId.toString()).toString();
    }
    if (!FAvailTabWindows.contains(windowId))
      windowId = FDefaultTabWindow;
    ITabWindow *window = openTabWindow(windowId);
    window->addWidget(AWidget);
  }
}

bool MessageWidgets::checkOption(IMessageWidgets::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void MessageWidgets::setOption(IMessageWidgets::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    emit optionChanged(AOption,AValue);
  }
}

void MessageWidgets::insertUrlHandler(IUrlHandler *AHandler, int AOrder)
{
  if (!FUrlHandlers.values(AOrder).contains(AHandler))  
  {
    FUrlHandlers.insert(AOrder,AHandler);
    emit urlHandlerInserted(AHandler,AOrder);
  }
}

void MessageWidgets::removeUrlHandler(IUrlHandler *AHandler, int AOrder)
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
    for (QMap<int,IUrlHandler *>::const_iterator it = FUrlHandlers.constBegin(); it!=FUrlHandlers.constEnd(); it++)
      if (it.value()->executeUrl(widget,AUrl,it.key()))
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

void MessageWidgets::onTabWindowWidgetAdded(ITabWidget *AWidget)
{
  ITabWindow *window = qobject_cast<ITabWindow *>(sender());
  if (FSettingsPlugin && window)
  {
    ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
    if (window->windowId() != FDefaultTabWindow)
      settings->setValueNS(SVN_TABWIDGET_WINDOWID,AWidget->tabWidgetId(),window->windowId().toString());
    else
      settings->deleteNS(AWidget->tabWidgetId());
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
  if (!(AAfter && AXmppStream->jid()))
    deleteStreamWindows(AXmppStream->jid());
}

void MessageWidgets::onStreamRemoved(IXmppStream *AXmppStream)
{
  deleteStreamWindows(AXmppStream->jid());
}

void MessageWidgets::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(MESSAGEWIDGETS_UUID);
  setOption(UseTabWindow, settings->value(SVN_USE_TABWINDOW,true).toBool());
  setOption(ShowStatus, settings->value(SVN_SHOW_STATUS,true).toBool());
  setSendMessageKey(QKeySequence::fromString(settings->value(SVN_SEND_MESSAGE_KEY,FSendKey.toString()).toString()));

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
  settings->setValue(SVN_USE_TABWINDOW,checkOption(UseTabWindow));
  settings->setValue(SVN_SHOW_STATUS,checkOption(ShowStatus));
  settings->setValue(SVN_SEND_MESSAGE_KEY,FSendKey.toString());
  settings->setValue(SVN_DEFAULT_TABWINDOW,FDefaultTabWindow.toString());

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

Q_EXPORT_PLUGIN2(MessageWidgetsPlugin, MessageWidgets)
