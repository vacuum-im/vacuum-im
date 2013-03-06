#include "normalwindow.h"

#include <QHeaderView>

NormalWindow::NormalWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FMessageWidgets = AMessageWidgets;

	FNextCount = 0;
	FShownDetached = false;
	FCurrentThreadId = QUuid::createUuid().toString();

	FTabPageNotifier = NULL;
	ui.wdtTabs->setDocumentMode(true);

	FAddress = FMessageWidgets->newAddress(AStreamJid,AContactJid,this);

	ui.wdtInfo->setLayout(new QVBoxLayout(ui.wdtInfo));
	ui.wdtInfo->layout()->setMargin(0);
	FInfoWidget = FMessageWidgets->newInfoWidget(this,ui.wdtInfo);
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());

	ui.wdtMessage->setLayout(new QVBoxLayout(ui.wdtMessage));
	ui.wdtMessage->layout()->setMargin(0);
	FViewWidget = FMessageWidgets->newViewWidget(this,ui.wdtMessage);
	FEditWidget = FMessageWidgets->newEditWidget(this,ui.wdtMessage);
	FEditWidget->setSendShortcut(SCT_MESSAGEWINDOWS_NORMAL_SENDMESSAGE);
	FEditWidget->setEditToolBarVisible(false);
	connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

	ui.wdtToolBar->setLayout(new QVBoxLayout(ui.wdtToolBar));
	ui.wdtToolBar->layout()->setMargin(0);
	FToolBarWidget = FMessageWidgets->newToolBarWidget(this,ui.wdtToolBar);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	ui.wdtToolBar->layout()->addWidget(FToolBarWidget->instance());

	FReceiversWidget = FMessageWidgets->newReceiversWidget(this,ui.wdtTabs);
	connect(FReceiversWidget->instance(),SIGNAL(receiverAdded(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
	connect(FReceiversWidget->instance(),SIGNAL(receiverRemoved(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
	ui.wdtTabs->addTab(FReceiversWidget->instance(),FReceiversWidget->instance()->windowIconText());
	FReceiversWidget->addReceiver(AContactJid);

	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(this,this);
	setMenuBar(FMenuBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(this,this);
	setStatusBar(FStatusBarWidget->instance());

	connect(ui.pbtSend,SIGNAL(clicked()),SLOT(onSendButtonClicked()));
	connect(ui.pbtReply,SIGNAL(clicked()),SLOT(onReplyButtonClicked()));
	connect(ui.pbtForward,SIGNAL(clicked()),SLOT(onForwardButtonClicked()));
	connect(ui.pbtChat,SIGNAL(clicked()),SLOT(onChatButtonClicked()));
	connect(ui.pbtNext,SIGNAL(clicked()),SLOT(onNextButtonClicked()));

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	setMode(AMode);
	setNextCount(FNextCount);
	setCurrentTabWidget(ui.tabMessage);
}

NormalWindow::~NormalWindow()
{
	emit tabPageDestroyed();
	delete FInfoWidget->instance();
	delete FViewWidget->instance();
	delete FEditWidget->instance();
	delete FReceiversWidget->instance();
	delete FMenuBarWidget->instance();
	delete FToolBarWidget->instance();
	delete FStatusBarWidget->instance();
}

Jid NormalWindow::streamJid() const
{
	return FAddress->streamJid();
}

Jid NormalWindow::contactJid() const
{
	return FAddress->contactJid();
}

IMessageAddress *NormalWindow::address() const
{
	return FAddress;
}

IMessageInfoWidget *NormalWindow::infoWidget() const
{
	return FInfoWidget;
}

IMessageViewWidget *NormalWindow::viewWidget() const
{
	return FViewWidget;
}

IMessageEditWidget *NormalWindow::editWidget() const
{
	return FEditWidget;
}

IMessageMenuBarWidget *NormalWindow::menuBarWidget() const
{
	return FMenuBarWidget;
}

IMessageToolBarWidget *NormalWindow::toolBarWidget() const
{
	return FToolBarWidget;
}

IMessageStatusBarWidget *NormalWindow::statusBarWidget() const
{
	return FStatusBarWidget;
}

IMessageReceiversWidget *NormalWindow::receiversWidget() const
{
	return FReceiversWidget;
}

QString NormalWindow::tabPageId() const
{
	return "NormalWindow|"+FAddress->streamJid().pBare()+"|"+FAddress->contactJid().pBare();
}

bool NormalWindow::isVisibleTabPage() const
{
	return window()->isVisible();
}

bool NormalWindow::isActiveTabPage() const
{
	return isVisible() && WidgetManager::isActiveWindow(this);
}

void NormalWindow::assignTabPage()
{
	if (isWindow() && !isVisible())
		FMessageWidgets->assignTabWindowPage(this);
	else
		emit tabPageAssign();
}

void NormalWindow::showTabPage()
{
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit tabPageShow();
}

void NormalWindow::showMinimizedTabPage()
{
	if (isWindow() && !isVisible())
		showMinimized();
	else
		emit tabPageShowMinimized();
}

void NormalWindow::closeTabPage()
{
	if (isWindow())
		close();
	else
		emit tabPageClose();
}

QIcon NormalWindow::tabPageIcon() const
{
	return windowIcon();
}

QString NormalWindow::tabPageCaption() const
{
	return windowIconText();
}

QString NormalWindow::tabPageToolTip() const
{
	return FTabPageToolTip;
}

IMessageTabPageNotifier *NormalWindow::tabPageNotifier() const
{
	return FTabPageNotifier;
}

void NormalWindow::setTabPageNotifier(IMessageTabPageNotifier *ANotifier)
{
	if (FTabPageNotifier != ANotifier)
	{
		if (FTabPageNotifier)
			delete FTabPageNotifier->instance();
		FTabPageNotifier = ANotifier;
		emit tabPageNotifierChanged();
	}
}

void NormalWindow::addTabWidget(QWidget *AWidget)
{
	ui.wdtTabs->addTab(AWidget,AWidget->windowIconText());
}

void NormalWindow::setCurrentTabWidget(QWidget *AWidget)
{
	if (AWidget)
		ui.wdtTabs->setCurrentWidget(AWidget);
	else
		ui.wdtTabs->setCurrentWidget(ui.wdtMessage);
}

void NormalWindow::removeTabWidget(QWidget *AWidget)
{
	ui.wdtTabs->removeTab(ui.wdtTabs->indexOf(AWidget));
}

IMessageNormalWindow::Mode NormalWindow::mode() const
{
	return FMode;
}

void NormalWindow::setMode(Mode AMode)
{
	FMode = AMode;
	if (AMode == ReadMode)
	{
		ui.wdtMessage->layout()->addWidget(FViewWidget->instance());
		ui.wdtMessage->layout()->removeWidget(FEditWidget->instance());
		FEditWidget->instance()->setParent(NULL);
		removeTabWidget(FReceiversWidget->instance());
	}
	else
	{
		ui.wdtMessage->layout()->addWidget(FEditWidget->instance());
		ui.wdtMessage->layout()->removeWidget(FViewWidget->instance());
		FViewWidget->instance()->setParent(NULL);
		addTabWidget(FReceiversWidget->instance());
	}

	ui.wdtReceivers->setVisible(FMode == WriteMode);
	ui.wdtInfo->setVisible(FMode == ReadMode);
	ui.wdtSubject->setVisible(FMode == WriteMode);
	ui.pbtSend->setVisible(FMode == WriteMode);
	ui.pbtReply->setVisible(FMode == ReadMode);
	ui.pbtForward->setVisible(FMode == ReadMode);
	ui.pbtChat->setVisible(FMode == ReadMode);

	QTimer::singleShot(0,this,SIGNAL(widgetLayoutChanged()));
}

QString NormalWindow::subject() const
{
	return ui.lneSubject->text();
}

void NormalWindow::setSubject(const QString &ASubject)
{
	ui.lneSubject->setText(ASubject);
}

QString NormalWindow::threadId() const
{
	return FCurrentThreadId;
}

void NormalWindow::setThreadId(const QString &AThreadId)
{
	FCurrentThreadId = AThreadId;
}

int NormalWindow::nextCount() const
{
	return FNextCount;
}

void NormalWindow::setNextCount(int ACount)
{
	if (ACount > 0)
		ui.pbtNext->setText(tr("Next - %1").arg(ACount));
	else
		ui.pbtNext->setText(tr("Close"));
	FNextCount = ACount;
}

void NormalWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"messages.messagewindow.state",tabPageId());
		Options::setFileValue(saveGeometry(),"messages.messagewindow.geometry",tabPageId());
	}
}

void NormalWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		if (!restoreGeometry(Options::fileValue("messages.messagewindow.geometry",tabPageId()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("messages.messagewindow.state",tabPageId()).toByteArray());
	}
}

void NormalWindow::updateWindow(const QIcon &AIcon, const QString &ACaption, const QString &ATitle, const QString &AToolTip)
{
	setWindowIcon(AIcon);
	setWindowIconText(ACaption);
	setWindowTitle(ATitle);
	FTabPageToolTip = AToolTip;
	emit tabPageChanged();
}

bool NormalWindow::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		emit tabPageActivated();
	}
	else if (AEvent->type() == QEvent::WindowDeactivate)
	{
		emit tabPageDeactivated();
	}
	return QMainWindow::event(AEvent);
}

void NormalWindow::showEvent(QShowEvent *AEvent)
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
	if (FMode == WriteMode)
		FEditWidget->textEdit()->setFocus();
	if (isActiveTabPage())
		emit tabPageActivated();
}

void NormalWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void NormalWindow::onMessageReady()
{
	emit messageReady();
}

void NormalWindow::onSendButtonClicked()
{
	emit messageReady();
}

void NormalWindow::onNextButtonClicked()
{
	if (FNextCount > 0)
		emit showNextMessage();
	else
		close();
}

void NormalWindow::onReplyButtonClicked()
{
	emit replyMessage();
}

void NormalWindow::onForwardButtonClicked()
{
	emit forwardMessage();
}

void NormalWindow::onChatButtonClicked()
{
	emit showChatWindow();
}

void NormalWindow::onReceiversChanged(const Jid &AReceiver)
{
	Q_UNUSED(AReceiver);
	QString receiversStr;
	foreach(Jid contactJid,FReceiversWidget->receivers())
		receiversStr += QString("%1; ").arg(FReceiversWidget->receiverName(contactJid));
	ui.lblReceivers->setText(receiversStr);
}

void NormalWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
	{
		closeTabPage();
	}
}
