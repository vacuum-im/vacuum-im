#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);

	FStatusChanger = NULL;
	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FShownDetached = false;

	ui.wdtInfo->setLayout(new QVBoxLayout);
	ui.wdtInfo->layout()->setMargin(0);
	FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid,ui.wdtInfo);
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
	onOptionsChanged(Options::node(OPV_MESSAGES_SHOWINFOWIDGET));

	ui.wdtView->setLayout(new QVBoxLayout);
	ui.wdtView->layout()->setMargin(0);
	FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid,ui.wdtView);
	ui.wdtView->layout()->addWidget(FViewWidget->instance());

	ui.wdtEdit->setLayout(new QVBoxLayout);
	ui.wdtEdit->layout()->setMargin(0);
	FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid,ui.wdtEdit);
	FEditWidget->setSendShortcut(SCT_MESSAGEWINDOWS_CHAT_SENDMESSAGE);
	ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
	connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

	ui.wdtToolBar->setLayout(new QVBoxLayout);
	ui.wdtToolBar->layout()->setMargin(0);
	FToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL,ui.wdtToolBar);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());

	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL,this);
	setMenuBar(FMenuBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(FInfoWidget,FViewWidget,FEditWidget,NULL,this);
	setStatusBar(FStatusBarWidget->instance());

	initialize();
}

ChatWindow::~ChatWindow()
{
	emit tabPageDestroyed();
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

bool ChatWindow::isActiveTabPage() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return isVisible() && widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

void ChatWindow::assignTabPage()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);
	else
		emit tabPageAssign();
}

void ChatWindow::showTabPage()
{
	assignTabPage();
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void ChatWindow::showMinimizedTabPage()
{
	assignTabPage();
	if (isWindow() && !isVisible())
		showMinimized();
	else
		emit tabPageShowMinimized();
}

void ChatWindow::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QIcon ChatWindow::tabPageIcon() const
{
	return windowIcon();
}

QString ChatWindow::tabPageCaption() const
{
	return windowIconText();
}

QString ChatWindow::tabPageToolTip() const
{
	return FTabPageToolTip;
}

void ChatWindow::setContactJid(const Jid &AContactJid)
{
	if (FMessageWidgets->findChatWindow(FStreamJid,AContactJid) == NULL)
	{
		Jid before = FContactJid;
		FContactJid = AContactJid;
		FInfoWidget->setContactJid(FContactJid);
		FViewWidget->setContactJid(FContactJid);
		FEditWidget->setContactJid(FContactJid);
		emit contactJidChanged(before);
	}
}

void ChatWindow::updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip)
{
	setWindowIcon(AIcon);
	setWindowIconText(ACaption);
	setWindowTitle(ATitle);
	FTabPageToolTip = AToolTip;
	emit tabPageChanged();
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

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));
}

void ChatWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"messages.chatwindow.state",tabPageId());
		Options::setFileValue(saveGeometry(),"messages.chatwindow.geometry",tabPageId());
	}
}

void ChatWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		if (!restoreGeometry(Options::fileValue("messages.chatwindow.geometry",tabPageId()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("messages.chatwindow.state",tabPageId()).toByteArray());
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
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QMainWindow::event(AEvent);
}

void ChatWindow::showEvent(QShowEvent *AEvent)
{
	if (isWindow())
	{
		if (!FShownDetached)
			loadWindowGeometry();
		FShownDetached = true;
		Shortcuts::insertWidgetShortcut(SCT_MESSAGEWINDOWS_CLOSEWINDOW,this);
	}
	else
	{
		FShownDetached = false;
		Shortcuts::removeWidgetShortcut(SCT_MESSAGEWINDOWS_CLOSEWINDOW,this);
	}

	QMainWindow::showEvent(AEvent);
	FEditWidget->textEdit()->setFocus();
	if (isActiveTabPage())
		emit tabPageActivated();
}

void ChatWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void ChatWindow::onMessageReady()
{
	emit messageReady();
}

void ChatWindow::onStreamJidChanged(const Jid &ABefore)
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
			emit streamJidChanged(ABefore);
		}
		else
		{
			deleteLater();
		}
	}
}

void ChatWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_SHOWINFOWIDGET)
	{
		FInfoWidget->instance()->setVisible(ANode.value().toBool());
	}
	else if (ANode.path() == OPV_MESSAGES_INFOWIDGETMAXSTATUSCHARS)
	{
		FInfoWidget->setField(IInfoWidget::ContactStatus,FInfoWidget->field(IInfoWidget::ContactStatus));
	}
}

void ChatWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
	{
		closeTabPage();
	}
}
