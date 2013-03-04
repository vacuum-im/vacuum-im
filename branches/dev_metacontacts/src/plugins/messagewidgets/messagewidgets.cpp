#include "messagewidgets.h"

#include <QPair>
#include <QBuffer>
#include <QMimeData>
#include <QClipboard>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentWriter>

#define ADR_QUOTE_WINDOW        Action::DR_Parametr1
#define ADR_CONTEXT_DATA        Action::DR_Parametr1

MessageWidgets::MessageWidgets()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FMainWindow = NULL;
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

bool MessageWidgets::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		IMainWindowPlugin *mainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
		if (mainWindowPlugin)
			FMainWindow = mainWindowPlugin->mainWindow();
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	return true;
}

bool MessageWidgets::initObjects()
{
	Shortcuts::declareShortcut(SCT_MAINWINDOW_COMBINEWITHMESSAGES, tr("Combine/Split with message windows"), QKeySequence::UnknownKey);

	Shortcuts::declareGroup(SCTG_TABWINDOW, tr("Tab window"), SGO_TABWINDOW);
	Shortcuts::declareShortcut(SCT_TABWINDOW_CLOSETAB, tr("Close tab"), tr("Ctrl+W","Close tab"));
	Shortcuts::declareShortcut(SCT_TABWINDOW_CLOSEOTHERTABS, tr("Close other tabs"), tr("Ctrl+Shift+W","Close other tabs"));
	Shortcuts::declareShortcut(SCT_TABWINDOW_DETACHTAB, tr("Detach tab to separate window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_NEXTTAB, tr("Next tab"), QKeySequence::NextChild);
	Shortcuts::declareShortcut(SCT_TABWINDOW_PREVTAB, tr("Previous tab"), QKeySequence::PreviousChild);
	Shortcuts::declareShortcut(SCT_TABWINDOW_SHOWCLOSEBUTTTONS, tr("Set tabs closable"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_TABSBOTTOM, tr("Show tabs at bottom"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_TABSINDICES, tr("Show tabs indices"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_RENAMEWINDOW, tr("Rename tab window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_CLOSEWINDOW, tr("Close tab window"), tr("Esc","Close tab window"));
	Shortcuts::declareShortcut(SCT_TABWINDOW_DELETEWINDOW, tr("Delete tab window"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_TABWINDOW_SETASDEFAULT, tr("Use as default tab window"), QKeySequence::UnknownKey);

	for (int tabNumber=1; tabNumber<=10; tabNumber++)
		Shortcuts::declareShortcut(QString(SCT_TABWINDOW_QUICKTAB).arg(tabNumber), QString::null, tr("Alt+%1","Show tab").arg(tabNumber % 10));

	Shortcuts::declareGroup(SCTG_MESSAGEWINDOWS, tr("Message windows"), SGO_MESSAGEWINDOWS);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_QUOTE, tr("Quote selected text"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_CLOSEWINDOW, tr("Close message window"), tr("Esc","Close message window"));
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_EDITNEXTMESSAGE, tr("Edit next message"), tr("Ctrl+Down","Edit next message"), Shortcuts::WidgetShortcut);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_EDITPREVMESSAGE, tr("Edit previous message"), tr("Ctrl+Up","Edit previous message"), Shortcuts::WidgetShortcut);
	
	Shortcuts::declareGroup(SCTG_MESSAGEWINDOWS_CHAT, tr("Chat window"), SGO_MESSAGEWINDOWS_CHAT);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_CHAT_SENDMESSAGE, tr("Send message"), tr("Return","Send message"), Shortcuts::WidgetShortcut);

	Shortcuts::declareGroup(SCTG_MESSAGEWINDOWS_NORMAL, tr("Message window"), SGO_MESSAGEWINDOWS_NORMAL);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_NORMAL_SENDMESSAGE, tr("Send message"), tr("Ctrl+Return","Send message"), Shortcuts::WidgetShortcut);

	insertViewUrlHandler(VUHO_MESSAGEWIDGETS_DEFAULT,this);
	insertEditContentsHandler(ECHO_MESSAGEWIDGETS_COPY_INSERT,this);

	if (FMainWindow)
		Shortcuts::insertWidgetShortcut(SCT_MAINWINDOW_COMBINEWITHMESSAGES,FMainWindow->instance());

	return true;
}

bool MessageWidgets::initSettings()
{
	Options::setDefaultValue(OPV_MESSAGES_SHOWSTATUS,true);
	Options::setDefaultValue(OPV_MESSAGES_ARCHIVESTATUS,false);
	Options::setDefaultValue(OPV_MESSAGES_EDITORAUTORESIZE,true);
	Options::setDefaultValue(OPV_MESSAGES_EDITORMINIMUMLINES,1);
	Options::setDefaultValue(OPV_MESSAGES_CLEANCHATTIMEOUT,30);
	Options::setDefaultValue(OPV_MESSAGES_COMBINEWITHROSTER,false);
	Options::setDefaultValue(OPV_MESSAGES_SHOWTABSINCOMBINEDMODE,false);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOWS_ENABLE,true);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_NAME,tr("Tab Window"));
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_TABSCLOSABLE,true);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_TABSBOTTOM,false);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_SHOWINDICES,false);
	Options::setDefaultValue(OPV_MESSAGES_TABWINDOW_REMOVETABSONCLOSE,false);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_MESSAGES, OPN_MESSAGES, tr("Messages"), MNI_NORMAL_MHANDLER_MESSAGE };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

QMultiMap<int, IOptionsWidget *> MessageWidgets::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_MESSAGES)
	{
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE),tr("Enable tab windows"),AParent));
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_SHOWSTATUS),tr("Show status changes in chat windows"),AParent));
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_ARCHIVESTATUS),tr("Save status messages to history"),AParent));
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_EDITORAUTORESIZE),tr("Auto resize input field"),AParent));
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_COMBINEWITHROSTER),tr("Combine message windows with contact-list"),AParent));
		widgets.insertMulti(OWO_MESSAGES,FOptionsManager->optionsNodeWidget(Options::node(OPV_MESSAGES_SHOWTABSINCOMBINEDMODE),tr("Show tabs in combined message windows with contact-list mode"),AParent));
		widgets.insertMulti(OWO_MESSAGES,new MessengerOptions(this,AParent));
	}
	return widgets;
}

bool MessageWidgets::viewUrlOpen(int AOrder, IMessageViewWidget* APage, const QUrl &AUrl)
{
	Q_UNUSED(APage);
	Q_UNUSED(AOrder);
	return QDesktopServices::openUrl(AUrl);
}

bool MessageWidgets::editContentsCreate(int AOrder, IMessageEditWidget *AWidget, QMimeData *AData)
{
	if (AOrder == ECHO_MESSAGEWIDGETS_COPY_INSERT)
	{
		QTextDocumentFragment fragment = AWidget->textEdit()->textCursor().selection();
		if (!fragment.isEmpty())
		{
			if (AWidget->isRichTextEnabled())
			{
				QBuffer buffer;
				QTextDocumentWriter writer(&buffer, "ODF");
				writer.write(fragment);
				buffer.close();
				AData->setData("application/vnd.oasis.opendocument.text", buffer.data());
				AData->setData("text/html", fragment.toHtml("utf-8").toUtf8());
			}
			AData->setText(fragment.toPlainText());
		}
	}
	return false;
}

bool MessageWidgets::editContentsCanInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData)
{
	Q_UNUSED(AWidget);
	if (AOrder == ECHO_MESSAGEWIDGETS_COPY_INSERT)
	{
		return AData->hasText() || AData->hasHtml();
	}
	return false;
}

bool MessageWidgets::editContentsInsert(int AOrder, IMessageEditWidget *AWidget, const QMimeData *AData, QTextDocument *ADocument)
{
	if (AOrder == ECHO_MESSAGEWIDGETS_COPY_INSERT)
	{
		if (editContentsCanInsert(AOrder,AWidget,AData))
		{
			QTextDocumentFragment fragment;
			if (AData->hasHtml())
				fragment = QTextDocumentFragment::fromHtml(AData->html());
			else if (AData->hasText())
				fragment = QTextDocumentFragment::fromPlainText(AData->text());

			if (!fragment.isEmpty())
			{
				QTextCursor cursor(ADocument);
				cursor.insertFragment(fragment);
			}
		}
	}
	return false;
}

bool MessageWidgets::editContentsChanged(int AOrder, IMessageEditWidget *AWidget, int &APosition, int &ARemoved, int &AAdded)
{
	Q_UNUSED(AOrder);
	Q_UNUSED(AWidget);
	Q_UNUSED(APosition);
	Q_UNUSED(ARemoved);
	Q_UNUSED(AAdded);
	return false;
}

IMessageAddress *MessageWidgets::newAddress(const Jid &AStreamJid, const Jid &AContactJid, QObject *AParent)
{
	IMessageAddress *address = new Address(this,AStreamJid,AContactJid,AParent);
	FCleanupHandler.add(address->instance());
	emit addressCreated(address);
	return address;
}

IMessageInfoWidget *MessageWidgets::newInfoWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageInfoWidget *widget = new InfoWidget(this,AWindow,AParent);
	FCleanupHandler.add(widget->instance());
	emit infoWidgetCreated(widget);
	return widget;
}

IMessageViewWidget *MessageWidgets::newViewWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageViewWidget *widget = new ViewWidget(this,AWindow,AParent);
	connect(widget->instance(),SIGNAL(contextMenuRequested(const QPoint &, const QTextDocumentFragment &, Menu *)),
		SLOT(onViewWidgetContextMenuRequested(const QPoint &, const QTextDocumentFragment &, Menu *)));
	connect(widget->instance(),SIGNAL(urlClicked(const QUrl &)),SLOT(onViewWidgetUrlClicked(const QUrl &)));
	FCleanupHandler.add(widget->instance());
	emit viewWidgetCreated(widget);
	return widget;
}

IMessageEditWidget *MessageWidgets::newEditWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageEditWidget *widget = new EditWidget(this,AWindow,AParent);
	connect(widget->instance(),SIGNAL(createDataRequest(QMimeData *)),
		SLOT(onEditWidgetCreateDataRequest(QMimeData *)));
	connect(widget->instance(),SIGNAL(canInsertDataRequest(const QMimeData *, bool &)),
		SLOT(onEditWidgetCanInsertDataRequest(const QMimeData *, bool &)));
	connect(widget->instance(),SIGNAL(insertDataRequest(const QMimeData *, QTextDocument *)),
		SLOT(onEditWidgetInsertDataRequest(const QMimeData *, QTextDocument *)));
	connect(widget->instance(),SIGNAL(contentsChanged(int, int, int)),SLOT(onEditWidgetContentsChanged(int, int, int)));
	FCleanupHandler.add(widget->instance());
	emit editWidgetCreated(widget);
	return widget;
}

IMessageReceiversWidget *MessageWidgets::newReceiversWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageReceiversWidget *widget = new ReceiversWidget(this,AWindow,AParent);
	FCleanupHandler.add(widget->instance());
	emit receiversWidgetCreated(widget);
	return widget;
}

IMessageMenuBarWidget *MessageWidgets::newMenuBarWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageMenuBarWidget *widget = new MenuBarWidget(AWindow,AParent);
	FCleanupHandler.add(widget->instance());
	emit menuBarWidgetCreated(widget);
	return widget;
}

IMessageToolBarWidget *MessageWidgets::newToolBarWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageToolBarWidget *widget = new ToolBarWidget(AWindow,AParent);
	FCleanupHandler.add(widget->instance());
	insertToolBarQuoteAction(widget);
	emit toolBarWidgetCreated(widget);
	return widget;
}

IMessageStatusBarWidget *MessageWidgets::newStatusBarWidget(IMessageWindow *AWindow, QWidget *AParent)
{
	IMessageStatusBarWidget *widget = new StatusBarWidget(AWindow,AParent);
	FCleanupHandler.add(widget->instance());
	emit statusBarWidgetCreated(widget);
	return widget;
}

IMessageTabPageNotifier *MessageWidgets::newTabPageNotifier(IMessageTabPage *ATabPage)
{
	IMessageTabPageNotifier *notifier = new TabPageNotifier(ATabPage);
	FCleanupHandler.add(notifier->instance());
	emit tabPageNotifierCreated(notifier);
	return notifier;
}

QList<IMessageNormalWindow *> MessageWidgets::normalWindows() const
{
	return FNormalWindows;
}

IMessageNormalWindow *MessageWidgets::getNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, IMessageNormalWindow::Mode AMode)
{
	IMessageNormalWindow *window = findNormalWindow(AStreamJid,AContactJid,true);
	if (!window)
	{
		window = new NormalWindow(this,AStreamJid,AContactJid,AMode);
		FNormalWindows.append(window);
		WidgetManager::setWindowSticky(window->instance(),true);
		connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onNormalWindowDestroyed()));
		FCleanupHandler.add(window->instance());
		emit normalWindowCreated(window);
		return window;
	}
	return NULL;
}

IMessageNormalWindow *MessageWidgets::findNormalWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact) const
{
	foreach(IMessageNormalWindow *window,FNormalWindows)
	{
		if (AExact)
		{
			if (window->address()->availAddresses().contains(AStreamJid,AContactJid))
				return window;
		}
		else
		{
			if (window->address()->availAddresses(true).contains(AStreamJid,AContactJid.bare()))
				return window;
		}
	}
	return NULL;
}

QList<IMessageChatWindow *> MessageWidgets::chatWindows() const
{
	return FChatWindows;
}

IMessageChatWindow *MessageWidgets::getChatWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IMessageChatWindow *window = findChatWindow(AStreamJid,AContactJid,true);
	if (!window)
	{
		window = new ChatWindow(this,AStreamJid,AContactJid);
		FChatWindows.append(window);
		WidgetManager::setWindowSticky(window->instance(),true);
		connect(window->instance(),SIGNAL(tabPageDestroyed()),SLOT(onChatWindowDestroyed()));
		FCleanupHandler.add(window->instance());
		emit chatWindowCreated(window);
		return window;
	}
	return NULL;
}

IMessageChatWindow *MessageWidgets::findChatWindow(const Jid &AStreamJid, const Jid &AContactJid, bool AExact) const
{
	foreach(IMessageChatWindow *window, FChatWindows)
	{
		if (AExact)
		{
			if (window->address()->availAddresses().contains(AStreamJid,AContactJid))
				return window;
		}
		else
		{
			if (window->address()->availAddresses(true).contains(AStreamJid,AContactJid.bare()))
				return window;
		}
	}
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
		IMessageTabWindow *window = findTabWindow(AWindowId);
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

QList<IMessageTabWindow *> MessageWidgets::tabWindows() const
{
	return FTabWindows;
}

IMessageTabWindow *MessageWidgets::getTabWindow(const QUuid &AWindowId)
{
	IMessageTabWindow *window = findTabWindow(AWindowId);
	if (!window)
	{
		window = new TabWindow(this,AWindowId);
		FTabWindows.append(window);
		WidgetManager::setWindowSticky(window->instance(),true);
		connect(window->instance(),SIGNAL(tabPageAdded(IMessageTabPage *)),SLOT(onTabWindowPageAdded(IMessageTabPage *)));
		connect(window->instance(),SIGNAL(currentTabPageChanged(IMessageTabPage *)),SLOT(onTabWindowCurrentPageChanged(IMessageTabPage *)));
		connect(window->instance(),SIGNAL(windowDestroyed()),SLOT(onTabWindowDestroyed()));
		emit tabWindowCreated(window);
	}
	return window;
}

IMessageTabWindow *MessageWidgets::findTabWindow(const QUuid &AWindowId) const
{
	foreach(IMessageTabWindow *window,FTabWindows)
		if (window->windowId() == AWindowId)
			return window;
	return NULL;
}

void MessageWidgets::assignTabWindowPage(IMessageTabPage *APage)
{
	if (!FAssignedPages.contains(APage))
	{
		FAssignedPages.append(APage);
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onAssignedTabPageDestroyed()));
	}

	if (Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
	{
		IMessageTabWindow *window = getTabWindow(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString());
		window->addTabPage(APage);
	}
	else if (Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE).value().toBool())
	{
		QList<QUuid> availWindows = tabWindowList();

		QUuid windowId = FPageWindows.value(APage->tabPageId());
		if (!availWindows.contains(windowId))
			windowId = Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString();
		if (!availWindows.contains(windowId))
			windowId = availWindows.value(0);

		IMessageTabWindow *window = getTabWindow(windowId);
		window->addTabPage(APage);
	}
}

QList<IMessageViewDropHandler *> MessageWidgets::viewDropHandlers() const
{
	return FViewDropHandlers;
}

void MessageWidgets::insertViewDropHandler(IMessageViewDropHandler *AHandler)
{
	if (AHandler && !FViewDropHandlers.contains(AHandler))
		FViewDropHandlers.append(AHandler);
}

void MessageWidgets::removeViewDropHandler(IMessageViewDropHandler *AHandler)
{
	if (FViewDropHandlers.contains(AHandler))
		FViewDropHandlers.removeAll(AHandler);
}

QMultiMap<int, IMessageViewUrlHandler *> MessageWidgets::viewUrlHandlers() const
{
	return FViewUrlHandlers;
}

void MessageWidgets::insertViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler)
{
	if (AHandler && !FViewUrlHandlers.contains(AOrder,AHandler))
		FViewUrlHandlers.insertMulti(AOrder,AHandler);
}

void MessageWidgets::removeViewUrlHandler(int AOrder, IMessageViewUrlHandler *AHandler)
{
	if (FViewUrlHandlers.contains(AOrder,AHandler))
		FViewUrlHandlers.remove(AOrder,AHandler);
}

QMultiMap<int, IMessageEditContentsHandler *> MessageWidgets::editContentsHandlers() const
{
	return FEditContentsHandlers;
}

void MessageWidgets::insertEditContentsHandler(int AOrder, IMessageEditContentsHandler *AHandler)
{
	if (AHandler && !FEditContentsHandlers.contains(AOrder,AHandler))
		FEditContentsHandlers.insertMulti(AOrder,AHandler);
}

void MessageWidgets::removeEditContentsHandler(int AOrder, IMessageEditContentsHandler *AHandler)
{
	if (FEditContentsHandlers.contains(AOrder,AHandler))
		FEditContentsHandlers.remove(AOrder,AHandler);
}

void MessageWidgets::deleteTabWindows()
{
	foreach(IMessageTabWindow *window, tabWindows())
		delete window->instance();
}

void MessageWidgets::insertToolBarQuoteAction(IMessageToolBarWidget *AWidget)
{
	Action *quoteAction = createQuouteAction(AWidget->messageWindow(),AWidget->instance());
	if (quoteAction)
	{
		AWidget->toolBarChanger()->insertAction(quoteAction,TBG_MWTBW_MESSAGEWIDGETS_QUOTE);
		AWidget->toolBarChanger()->actionHandle(quoteAction)->setVisible(quoteAction->isVisible());
		connect(AWidget->messageWindow()->instance(),SIGNAL(widgetLayoutChanged()),SLOT(onMessageWindowWidgetLayoutChanged()));
	}
}

Action *MessageWidgets::createQuouteAction(IMessageWindow *AWindow, QObject *AParent)
{
	if (AWindow->viewWidget() && AWindow->editWidget())
	{
		Action *quoteAction = new Action(AParent);
		quoteAction->setData(ADR_QUOTE_WINDOW,(qint64)AWindow->instance());
		quoteAction->setText(tr("Quote Selected Text"));
		quoteAction->setToolTip(tr("Quote selected text"));
		quoteAction->setIcon(RSR_STORAGE_MENUICONS, MNI_MESSAGEWIDGETS_QUOTE);
		quoteAction->setShortcutId(SCT_MESSAGEWINDOWS_QUOTE);
		quoteAction->setVisible(AWindow->viewWidget()->isVisibleOnWindow() && AWindow->editWidget()->isVisibleOnWindow());
		connect(quoteAction,SIGNAL(triggered(bool)),SLOT(onQuoteActionTriggered(bool)));
		return quoteAction;
	}
	return NULL;
}


void MessageWidgets::onViewWidgetUrlClicked(const QUrl &AUrl)
{
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	if (widget)
	{
		for (QMap<int,IMessageViewUrlHandler *>::const_iterator it = FViewUrlHandlers.constBegin(); it!=FViewUrlHandlers.constEnd(); ++it)
			if (it.value()->viewUrlOpen(it.key(),widget,AUrl))
				break;
	}
}

void MessageWidgets::onViewWidgetContextMenuRequested(const QPoint &APosition, const QTextDocumentFragment &AText, Menu *AMenu)
{
	Q_UNUSED(APosition);
	IMessageViewWidget *widget = qobject_cast<IMessageViewWidget *>(sender());
	if (widget && !AText.isEmpty())
	{
		Action *copyAction = new Action(AMenu);
		copyAction->setText(tr("Copy"));
		copyAction->setShortcut(QKeySequence::Copy);
		copyAction->setData(ADR_CONTEXT_DATA,AText.toHtml());
		connect(copyAction,SIGNAL(triggered(bool)),SLOT(onViewContextCopyActionTriggered(bool)));
		AMenu->addAction(copyAction,AG_VWCM_MESSAGEWIDGETS_COPY,true);

		Action *quoteAction = createQuouteAction(widget->messageWindow(),AMenu);
		if (quoteAction)
			AMenu->addAction(quoteAction,AG_VWCM_MESSAGEWIDGETS_QUOTE,true);

		QUrl href = TextManager::getTextFragmentHref(AText);
		if (href.isValid())
		{
			bool isMailto = href.scheme()=="mailto";

			Action *urlAction = new Action(AMenu);
			urlAction->setText(isMailto ? tr("Send mail") : tr("Open link"));
			urlAction->setData(ADR_CONTEXT_DATA,href.toString());
			connect(urlAction,SIGNAL(triggered(bool)),SLOT(onViewContextUrlActionTriggered(bool)));
			AMenu->addAction(urlAction,AG_VWCM_MESSAGEWIDGETS_URL,true);
			AMenu->setDefaultAction(urlAction);

			Action *copyHrefAction = new Action(AMenu);
			copyHrefAction->setText(tr("Copy address"));
			copyHrefAction->setData(ADR_CONTEXT_DATA,isMailto ? href.path() : href.toString());
			connect(copyHrefAction,SIGNAL(triggered(bool)),SLOT(onViewContextCopyActionTriggered(bool)));
			AMenu->addAction(copyHrefAction,AG_VWCM_MESSAGEWIDGETS_COPY,true);
		}
		else
		{
			QString plainSelection = AText.toPlainText().trimmed();
			Action *searchAction = new Action(AMenu);
			searchAction->setText(tr("Search on Google '%1'").arg(TextManager::getElidedString(plainSelection,Qt::ElideRight,30)));
			searchAction->setData(ADR_CONTEXT_DATA, plainSelection);
			connect(searchAction,SIGNAL(triggered(bool)),SLOT(onViewContextSearchActionTriggered(bool)));
			AMenu->addAction(searchAction,AG_VWCM_MESSAGEWIDGETS_SEARCH,true);
		}
	}
}

void MessageWidgets::onViewContextCopyActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString html = action->data(ADR_CONTEXT_DATA).toString();
		QMimeData *data = new QMimeData;
		data->setHtml(html);
		data->setText(QTextDocumentFragment::fromHtml(html).toPlainText());
		QApplication::clipboard()->setMimeData(data);
	}
}

void MessageWidgets::onViewContextUrlActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QDesktopServices::openUrl(action->data(ADR_CONTEXT_DATA).toString());
	}
}

void MessageWidgets::onViewContextSearchActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString domain = tr("google.com","Your google domain");
		QUrl url = QString("http://www.%1/search").arg(domain);
		url.setQueryItems(QList<QPair<QString,QString> >() << qMakePair<QString,QString>(QString("q"),action->data(ADR_CONTEXT_DATA).toString()));
		QDesktopServices::openUrl(url);
	}
}

void MessageWidgets::onEditWidgetCreateDataRequest(QMimeData *AData)
{
	IMessageEditWidget *widget = qobject_cast<IMessageEditWidget *>(sender());
	if (widget)
	{
		for (QMap<int,IMessageEditContentsHandler *>::const_iterator it = FEditContentsHandlers.constBegin(); it!=FEditContentsHandlers.constEnd(); ++it)
			if (it.value()->editContentsCreate(it.key(),widget,AData))
				break;
	}
}

void MessageWidgets::onEditWidgetCanInsertDataRequest(const QMimeData *AData, bool &ACanInsert)
{
	IMessageEditWidget *widget = qobject_cast<IMessageEditWidget *>(sender());
	if (widget)
	{
		for (QMap<int,IMessageEditContentsHandler *>::const_iterator it = FEditContentsHandlers.constBegin(); !ACanInsert && it!=FEditContentsHandlers.constEnd(); ++it)
			ACanInsert = it.value()->editContentsCanInsert(it.key(),widget,AData);
	}
}

void MessageWidgets::onEditWidgetInsertDataRequest(const QMimeData *AData, QTextDocument *ADocument)
{
	IMessageEditWidget *widget = qobject_cast<IMessageEditWidget *>(sender());
	if (widget)
	{
		for (QMap<int,IMessageEditContentsHandler *>::const_iterator it = FEditContentsHandlers.constBegin(); it!=FEditContentsHandlers.constEnd(); ++it)
			if (it.value()->editContentsInsert(it.key(),widget,AData,ADocument))
				break;
	}
}

void MessageWidgets::onEditWidgetContentsChanged(int APosition, int ARemoved, int AAdded)
{
	IMessageEditWidget *widget = qobject_cast<IMessageEditWidget *>(sender());
	if (widget)
	{
		widget->document()->blockSignals(true);
		for (QMap<int,IMessageEditContentsHandler *>::const_iterator it = FEditContentsHandlers.constBegin(); it!=FEditContentsHandlers.constEnd(); ++it)
			if (it.value()->editContentsChanged(it.key(),widget,APosition,ARemoved,AAdded))
				break;
		widget->document()->blockSignals(false);
	}
}

void MessageWidgets::onMessageWindowWidgetLayoutChanged()
{
	IMessageWindow *window = qobject_cast<IMessageWindow *>(sender());
	if (window && window->toolBarWidget())
	{
		QAction *quoteActionHandle = window->toolBarWidget()->toolBarChanger()->groupItems(TBG_MWTBW_MESSAGEWIDGETS_QUOTE).value(0);
		if (quoteActionHandle)
			quoteActionHandle->setVisible(window->viewWidget()->isVisibleOnWindow() && window->editWidget()->isVisibleOnWindow());
	}
}

void MessageWidgets::onQuoteActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	IMessageWindow *window = action!=NULL ? qobject_cast<IMessageWindow *>((QWidget *)action->data(ADR_QUOTE_WINDOW).toLongLong()) : NULL;
	if (window && window->viewWidget() && window->viewWidget()->messageStyle() && window->editWidget())
	{
		QTextDocumentFragment fragment = window->viewWidget()->messageStyle()->selection(window->viewWidget()->styleWidget());
		fragment = TextManager::getTrimmedTextFragment(window->editWidget()->prepareTextFragment(fragment),!window->editWidget()->isRichTextEnabled());
		TextManager::insertQuotedFragment(window->editWidget()->textEdit()->textCursor(),fragment);
		window->editWidget()->textEdit()->setFocus();
	}
}

void MessageWidgets::onAssignedTabPageDestroyed()
{
	FAssignedPages.removeAll(qobject_cast<IMessageTabPage *>(sender()));
}

void MessageWidgets::onNormalWindowDestroyed()
{
	IMessageNormalWindow *window = qobject_cast<IMessageNormalWindow *>(sender());
	if (window)
	{
		FNormalWindows.removeAt(FNormalWindows.indexOf(window));
		emit normalWindowDestroyed(window);
	}
}

void MessageWidgets::onChatWindowDestroyed()
{
	IMessageChatWindow *window = qobject_cast<IMessageChatWindow *>(sender());
	if (window)
	{
		FChatWindows.removeAt(FChatWindows.indexOf(window));
		emit chatWindowDestroyed(window);
	}
}

void MessageWidgets::onTabWindowPageAdded(IMessageTabPage *APage)
{
	if (!Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
	{
		IMessageTabWindow *window = qobject_cast<IMessageTabWindow *>(sender());
		if (window)
		{
			if (window->windowId() != Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString())
				FPageWindows.insert(APage->tabPageId(), window->windowId());
			else
				FPageWindows.remove(APage->tabPageId());
		}
	}
}

void MessageWidgets::onTabWindowCurrentPageChanged(IMessageTabPage *APage)
{
	if (Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool() && !Options::node(OPV_MESSAGES_SHOWTABSINCOMBINEDMODE).value().toBool())
	{
		IMessageTabWindow *window = qobject_cast<IMessageTabWindow *>(sender());
		if (window && window->windowId()==Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString())
		{
			for (int index=0; index<window->tabPageCount(); index++)
			{
				IMessageTabPage *page = window->tabPage(index);
				if (page != APage)
				{
					index--;
					page->closeTabPage();
				}
			}
		}
	}
}

void MessageWidgets::onTabWindowDestroyed()
{
	IMessageTabWindow *window = qobject_cast<IMessageTabWindow *>(sender());
	if (window)
	{
		FTabWindows.removeAt(FTabWindows.indexOf(window));
		emit tabWindowDestroyed(window);
	}
}

void MessageWidgets::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MAINWINDOW_COMBINEWITHMESSAGES && FMainWindow && AWidget==FMainWindow->instance())
	{
		Options::node(OPV_MESSAGES_COMBINEWITHROSTER).setValue(!Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool());
	}
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

	onOptionsChanged(Options::node(OPV_MESSAGES_COMBINEWITHROSTER));
	onOptionsChanged(Options::node(OPV_MESSAGES_SHOWTABSINCOMBINEDMODE));
}

void MessageWidgets::onOptionsClosed()
{
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	stream << FPageWindows;
	Options::setFileValue(data,"messages.tab-window-pages");

	deleteTabWindows();
}

void MessageWidgets::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_TABWINDOWS_ENABLE)
	{
		if (ANode.value().toBool())
		{
			foreach(IMessageTabPage *page, FAssignedPages)
				assignTabWindowPage(page);

			foreach(IMessageTabWindow *window, tabWindows())
				window->showWindow();
		}
		else if (!Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		{
			foreach(IMessageTabWindow *window, tabWindows())
				while(window->currentTabPage())
					window->detachTabPage(window->currentTabPage());
		}
	}
	else if (FMainWindow && ANode.path()==OPV_MESSAGES_COMBINEWITHROSTER)
	{
		foreach(IMessageTabPage *page, FAssignedPages)
			assignTabWindowPage(page);

		IMessageTabWindow *window = findTabWindow(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString()); 
		if (ANode.value().toBool())
		{
			if (!window)
				window = getTabWindow(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString()); 
			window->setTabBarVisible(Options::node(OPV_MESSAGES_SHOWTABSINCOMBINEDMODE).value().toBool());
			window->setAutoCloseEnabled(false);
			FMainWindow->mainCentralWidget()->appendCentralPage(window);
		}
		else if (window && Options::node(OPV_MESSAGES_TABWINDOWS_ENABLE).value().toBool())
		{
			window->setTabBarVisible(true);
			window->setAutoCloseEnabled(true);
			FMainWindow->mainCentralWidget()->removeCentralPage(window);
			if (window->tabPageCount() > 0)
				window->showWindow();
			else
				window->instance()->deleteLater();
		}
		else if (window)
		{
			while(window->currentTabPage())
				window->detachTabPage(window->currentTabPage());
			window->instance()->deleteLater();
		}
	}
	else if (ANode.path()==OPV_MESSAGES_SHOWTABSINCOMBINEDMODE)
	{
		if (Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool())
		{
			IMessageTabWindow *window = findTabWindow(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).value().toString());
			if (window)
				window->setTabBarVisible(ANode.value().toBool());
		}
	}
}

Q_EXPORT_PLUGIN2(plg_messagewidgets, MessageWidgets)
