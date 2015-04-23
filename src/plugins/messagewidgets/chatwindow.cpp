#include "chatwindow.h"

#include <QKeyEvent>
#include <QCoreApplication>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/optionvalues.h>
#include <definitions/messagedataroles.h>
#include <definitions/messagechatwindowwidgets.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/logger.h>

ChatWindow::ChatWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, false);
	ui.spwMessageBox->setSpacing(3);

	FMessageWidgets = AMessageWidgets;

	FShownDetached = false;
	FTabPageNotifier = NULL;

	FAddress = FMessageWidgets->newAddress(AStreamJid,AContactJid,this);
	
	FInfoWidget = FMessageWidgets->newInfoWidget(this,ui.spwMessageBox);
	ui.spwMessageBox->insertWidget(MCWW_INFOWIDGET,FInfoWidget->instance());

	FViewWidget = FMessageWidgets->newViewWidget(this,ui.spwMessageBox);
	ui.spwMessageBox->insertWidget(MCWW_VIEWWIDGET,FViewWidget->instance(),100);

	FEditWidget = FMessageWidgets->newEditWidget(this,ui.spwMessageBox);
	FEditWidget->setSendShortcutId(SCT_MESSAGEWINDOWS_SENDCHATMESSAGE);
	ui.spwMessageBox->insertWidget(MCWW_EDITWIDGET,FEditWidget->instance());
	
	FToolBarWidget = FMessageWidgets->newToolBarWidget(this,ui.spwMessageBox);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	ui.spwMessageBox->insertWidget(MCWW_TOOLBARWIDGET,FToolBarWidget->instance());
	
	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(this,this);
	setMenuBar(FMenuBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(this,this);
	setStatusBar(FStatusBarWidget->instance());

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));
}

ChatWindow::~ChatWindow()
{
	emit tabPageDestroyed();
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

SplitterWidget *ChatWindow::messageWidgetsBox() const
{
	return ui.spwMessageBox;
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

void ChatWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
		closeTabPage();
}
