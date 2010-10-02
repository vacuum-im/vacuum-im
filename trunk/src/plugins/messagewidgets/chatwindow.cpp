#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);

	FStatusChanger = NULL;
	FMessageWidgets = AMessageWidgets;

	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FShownDetached = false;

	FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
	ui.wdtInfo->setLayout(new QVBoxLayout);
	ui.wdtInfo->layout()->setMargin(0);
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
	onOptionsChanged(Options::node(OPV_MESSAGES_SHOWINFOWIDGET));

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

bool ChatWindow::isActive() const
{
	const QWidget *widget = this;
	while (widget->parentWidget())
		widget = widget->parentWidget();
	return widget->isActiveWindow() && !widget->isMinimized() && widget->isVisible();
}

void ChatWindow::showWindow()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);

	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
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
		Jid before = FContactJid;
		FContactJid = AContactJid;
		FInfoWidget->setContactJid(FContactJid);
		FViewWidget->setContactJid(FContactJid);
		FEditWidget->setContactJid(FContactJid);
		emit contactJidChanged(before);
	}
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

	plugin = FMessageWidgets->pluginManager()->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
}

void ChatWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveGeometry(),"messages.chatwindow.geometry",tabPageId());
	}
}

void ChatWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		restoreGeometry(Options::fileValue("messages.chatwindow.geometry",tabPageId()).toByteArray());
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
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit windowDeactivated();
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
	emit windowDeactivated();
	emit windowClosed();
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
}
