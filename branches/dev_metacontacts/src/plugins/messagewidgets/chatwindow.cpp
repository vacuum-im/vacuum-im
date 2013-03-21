#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);

	FMessageWidgets = AMessageWidgets;

	FShownDetached = false;
	FTabPageNotifier = NULL;

	FAddress = FMessageWidgets->newAddress(AStreamJid,AContactJid,this);

	ui.wdtInfo->setLayout(new QVBoxLayout);
	ui.wdtInfo->layout()->setMargin(0);
	FInfoWidget = FMessageWidgets->newInfoWidget(this,ui.wdtInfo);
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());

	ui.wdtView->setLayout(new QVBoxLayout);
	ui.wdtView->layout()->setMargin(0);
	FViewWidget = FMessageWidgets->newViewWidget(this,ui.wdtView);
	ui.wdtView->layout()->addWidget(FViewWidget->instance());

	ui.wdtEdit->setLayout(new QVBoxLayout);
	ui.wdtEdit->layout()->setMargin(0);
	FEditWidget = FMessageWidgets->newEditWidget(this,ui.wdtEdit);
	FEditWidget->setSendShortcut(SCT_MESSAGEWINDOWS_CHAT_SENDMESSAGE);
	ui.wdtEdit->layout()->addWidget(FEditWidget->instance());
	connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

	ui.wdtToolBar->setLayout(new QVBoxLayout);
	ui.wdtToolBar->layout()->setMargin(0);
	FToolBarWidget = FMessageWidgets->newToolBarWidget(this,ui.wdtToolBar);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());

	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(this,this);
	setMenuBar(FMenuBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(this,this);
	setStatusBar(FStatusBarWidget->instance());

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));
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

Jid ChatWindow::streamJid() const
{
	return FAddress->streamJid();
}

Jid ChatWindow::contactJid() const
{
	return FAddress->contactJid();
}

IMessageAddress *ChatWindow::address() const
{
	return FAddress;
}

IMessageInfoWidget *ChatWindow::infoWidget() const
{
	return FInfoWidget;
}

IMessageViewWidget *ChatWindow::viewWidget() const
{
	return FViewWidget;
}

IMessageEditWidget *ChatWindow::editWidget() const
{
	return FEditWidget;
}

IMessageMenuBarWidget *ChatWindow::menuBarWidget() const
{
	return FMenuBarWidget;
}

IMessageToolBarWidget *ChatWindow::toolBarWidget() const
{
	return FToolBarWidget;
}

IMessageStatusBarWidget *ChatWindow::statusBarWidget() const
{
	return FStatusBarWidget;
}

IMessageReceiversWidget *ChatWindow::receiversWidget() const
{
	return NULL;
}

QString ChatWindow::tabPageId() const
{
	return "ChatWindow|"+streamJid().pBare()+"|"+contactJid().pBare();
}

bool ChatWindow::isVisibleTabPage() const
{
	return window()->isVisible();
}

bool ChatWindow::isActiveTabPage() const
{
	return isVisible() && WidgetManager::isActiveWindow(this);
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

IMessageTabPageNotifier *ChatWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void ChatWindow::setTabPageNotifier(IMessageTabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
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

void ChatWindow::saveWindowGeometryAndState()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"messages.chatwindow.state",tabPageId());
		Options::setFileValue(saveGeometry(),"messages.chatwindow.geometry",tabPageId());
	}
}

void ChatWindow::loadWindowGeometryAndState()
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
			loadWindowGeometryAndState();
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
		saveWindowGeometryAndState();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void ChatWindow::onMessageReady()
{
	emit messageReady();
}

void ChatWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
	{
		closeTabPage();
	}
}
