#include "messagewidgets.h"

MessageWidgets::MessageWidgets()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FOptionsManager = NULL;
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

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
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

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool MessageWidgets::initObjects()
{
	insertViewUrlHandler(this,VUHO_MESSAGEWIDGETS_DEFAULT);
	return true;
}

bool MessageWidgets::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MESSAGES_EDITORAUTORESIZE,true);
	Options::setDefaultValue(OPV_MESSAGES_SHOWINFOWIDGET,true);
	Options::setDefaultValue(OPV_MESSAGES_EDITORMINIMUMLINES,1);
	Options::setDefaultValue(OPV_MESSAGES_EDITORSENDKEY,QKeySequence(Qt::Key_Return));
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOWS_ENABLE,true);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_NAME,tr("Tab Window"));
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_TABSCLOSABLE,true);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_MESSAGES, OPN_MESSAGES, tr("Messages"), tr("Message window options"), MNI_NORMAL_MHANDLER_MESSAGE };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

IOptionsWidget *MessageWidgets::optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent)
{
	if (ANodeId == OPN_MESSAGES)
	{
		AOrder = OWO_MESSAGES;

		IOptionsContainer *container = FOptionsManager->optionsContainer(AParent);
		container->appendChild(Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE),tr("Enable tab windows"));
		container->appendChild(Options::node(OPV_MESSAGES_SHOWSTATUS),tr("Show status changes in chat windows"));
		container->appendChild(Options::node(OPV_MESSAGES_EDITORAUTORESIZE),tr("Auto resize input field"));
		container->appendChild(Options::node(OPV_MESSAGES_SHOWINFOWIDGET),tr("Show contact information in chat windows"));

		IOptionsWidget *widget = new MessengerOptions(this,container->instance());
		container->instance()->layout()->addWidget(widget->instance());
		container->registerChild(widget);

		return container;
	}
	return NULL;
}

bool MessageWidgets::viewUrlOpen(IViewWidget* APage, const QUrl &AUrl, int AOrder)
{
	Q_UNUSED(APage);
	Q_UNUSED(AOrder);
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
	insertQuoteAction(widget);
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
	QList<QUuid> list;
	foreach(QString tabWindowId, Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).childNSpaces("window"))
		list.append(tabWindowId);
	return list;
}

QUuid MessageWidgets::appendTabWindow(const QString &AName)
{
	QUuid id = QUuid::createUuid();
	QString name = AName;
	if (name.isEmpty())
	{
		QList<QString> names;
		foreach(QString tabWindowId, Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).childNSpaces("window"))
			names.append(Options::node(OPV_MESSAGES_TABWINDOW_ITEM,tabWindowId).value().toString());

		int i = 0;
		do
		{
			i++;
			name = tr("Tab Window %1").arg(i);
		} while (names.contains(name));
	}
	Options::node(OPV_MESSAGES_TABWINDOW_ITEM,id.toString()).setValue(name,"name");
	emit tabWindowAppended(id,name);
	return id;
}

void MessageWidgets::deleteTabWindow(const QUuid &AWindowId)
{
	if (AWindowId!=Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString() && tabWindowList().contains(AWindowId))
	{
		ITabWindow *window = findTabWindow(AWindowId);
		if (window)
			window->instance()->deleteLater();
		Options::node(OPV_MESSAGES_TABWINDOWS_ROOT).removeChilds("window",AWindowId.toString());
		emit tabWindowDeleted(AWindowId);
	}
}

QString MessageWidgets::tabWindowName(const QUuid &AWindowId) const
{
	if (tabWindowList().contains(AWindowId))
		return Options::node(OPV_MESSAGES_TABWINDOW_ITEM,AWindowId.toString()).value("name").toString();
	return Options::defaultValue(OPV_MESSAGES_TABWINDOW_NAME).toString();
}

void MessageWidgets::setTabWindowName(const QUuid &AWindowId, const QString &AName)
{
	if (!AName.isEmpty() && tabWindowList().contains(AWindowId))
	{
		Options::node(OPV_MESSAGES_TABWINDOW_ITEM,AWindowId.toString()).setValue(AName,"name");
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
	if (Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE).value().toBool())
	{
		QList<QUuid> availWindows = tabWindowList();
		QUuid windowId = FPageWindows.value(APage->tabPageId());
		if (!availWindows.contains(windowId))
			windowId = Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString();
		if (!availWindows.contains(windowId))
			windowId = availWindows.value(0);
		ITabWindow *window = openTabWindow(windowId);
		window->addPage(APage);
	}
}

QList<IViewDropHandler *> MessageWidgets::viewDropHandlers() const
{
	return FViewDropHandlers;
}

void MessageWidgets::insertViewDropHandler(IViewDropHandler *AHandler)
{
	if (!FViewDropHandlers.contains(AHandler))
	{
		FViewDropHandlers.append(AHandler);
		emit viewDropHandlerInserted(AHandler);
	}
}

void MessageWidgets::removeViewDropHandler(IViewDropHandler *AHandler)
{
	if (FViewDropHandlers.contains(AHandler))
	{
		FViewDropHandlers.removeAll(AHandler);
		emit viewDropHandlerRemoved(AHandler);
	}
}

QMultiMap<int, IViewUrlHandler *> MessageWidgets::viewUrlHandlers() const
{
	return FViewUrlHandlers;
}

void MessageWidgets::insertViewUrlHandler(IViewUrlHandler *AHandler, int AOrder)
{
	if (!FViewUrlHandlers.values(AOrder).contains(AHandler))
	{
		FViewUrlHandlers.insertMulti(AOrder,AHandler);
		emit viewUrlHandlerInserted(AHandler,AOrder);
	}
}

void MessageWidgets::removeViewUrlHandler(IViewUrlHandler *AHandler, int AOrder)
{
	if (FViewUrlHandlers.values(AOrder).contains(AHandler))
	{
		FViewUrlHandlers.remove(AOrder,AHandler);
		emit viewUrlHandlerRemoved(AHandler,AOrder);
	}
}

void MessageWidgets::insertQuoteAction(IToolBarWidget *AWidget)
{
	if (AWidget->viewWidget() && AWidget->editWidget())
	{
		Action *action = new Action(AWidget->instance());
		action->setToolTip(tr("Quote selected text"));
		action->setIcon(RSR_STORAGE_MENUICONS, MNI_MESSAGEWIDGETS_QUOTE);
		connect(action,SIGNAL(triggered(bool)),SLOT(onQuoteActionTriggered(bool)));
		AWidget->toolBarChanger()->insertAction(action,TBG_MWTBW_MESSAGEWIDGETS_QUOTE);
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
		for (QMap<int,IViewUrlHandler *>::const_iterator it = FViewUrlHandlers.constBegin(); it!=FViewUrlHandlers.constEnd(); it++)
			if (it.value()->viewUrlOpen(widget,AUrl,it.key()))
				break;
	}
}

void MessageWidgets::onQuoteActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IToolBarWidget *widget = action!=NULL ? qobject_cast<IToolBarWidget *>(action->parent()) : NULL;
	if (widget && widget->viewWidget() && widget->viewWidget()->messageStyle() && widget->editWidget())
	{
		QTextDocumentFragment fragment = widget->viewWidget()->messageStyle()->selection(widget->viewWidget()->styleWidget());
		if (!fragment.toPlainText().trimmed().isEmpty())
		{
			QTextEdit *editor = widget->editWidget()->textEdit();
			editor->textCursor().beginEditBlock();
			if (!editor->textCursor().atBlockStart())
				editor->textCursor().insertText("\n");
			editor->textCursor().insertText(">----\n");
			editor->textCursor().insertFragment(fragment);
			editor->textCursor().insertText("\n----<\n");
			editor->textCursor().endEditBlock();
			editor->setFocus();
		}
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
	if (window)
	{
		if (window->windowId() != Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString())
			FPageWindows.insert(APage->tabPageId(), window->windowId());
		else
			FPageWindows.remove(APage->tabPageId());
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

void MessageWidgets::onOptionsOpened()
{
	if (tabWindowList().isEmpty())
		appendTabWindow(tr("Main Tab Window"));

	if (!tabWindowList().contains(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString()))
		Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).setValue(tabWindowList().value(0).toString());

	QByteArray data = Options::fileValue("messages.tab-window-pages").toByteArray();
	QDataStream stream(data);
	stream >> FPageWindows;
}

void MessageWidgets::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FPageWindows;
	Options::setFileValue(data,"messages.tab-window-pages");

	deleteWindows();
}

Q_EXPORT_PLUGIN2(plg_messagewidgets, MessageWidgets)
