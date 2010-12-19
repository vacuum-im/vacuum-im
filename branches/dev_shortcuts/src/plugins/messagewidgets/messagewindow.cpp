#include "messagewindow.h"

#include <QHeaderView>

MessageWindow::MessageWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	FMessageWidgets = AMessageWidgets;

	FMode = AMode;
	FNextCount = 0;
	FShownDetached = false;
	FStreamJid = AStreamJid;
	FContactJid = AContactJid;
	FCurrentThreadId = QUuid::createUuid().toString();

	FInfoWidget = FMessageWidgets->newInfoWidget(AStreamJid,AContactJid);
	ui.wdtInfo->setLayout(new QVBoxLayout(ui.wdtInfo));
	ui.wdtInfo->layout()->addWidget(FInfoWidget->instance());
	ui.wdtInfo->layout()->setMargin(0);

	FViewWidget = FMessageWidgets->newViewWidget(AStreamJid,AContactJid);

	FEditWidget = FMessageWidgets->newEditWidget(AStreamJid,AContactJid);
	FEditWidget->setSendShortcut(SCT_MESSAGEWINDOWS_NORMAL_SENDMESSAGE);
	FEditWidget->setSendButtonVisible(false);
	connect(FEditWidget->instance(),SIGNAL(messageReady()),SLOT(onMessageReady()));

	FReceiversWidget = FMessageWidgets->newReceiversWidget(FStreamJid);
	connect(FReceiversWidget->instance(),SIGNAL(receiverAdded(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
	connect(FReceiversWidget->instance(),SIGNAL(receiverRemoved(const Jid &)),SLOT(onReceiversChanged(const Jid &)));
	FReceiversWidget->addReceiver(FContactJid);

	FViewToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,FViewWidget,NULL,NULL);
	FViewToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

	FEditToolBarWidget = FMessageWidgets->newToolBarWidget(FInfoWidget,NULL,FEditWidget,NULL);
	FEditToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);

	ui.wdtToolBar->setLayout(new QVBoxLayout(ui.wdtToolBar));
	ui.wdtToolBar->layout()->setMargin(0);

	ui.wdtMessage->setLayout(new QVBoxLayout(ui.wdtMessage));
	ui.wdtMessage->layout()->setMargin(0);

	connect(ui.pbtSend,SIGNAL(clicked()),SLOT(onSendButtonClicked()));
	connect(ui.pbtReply,SIGNAL(clicked()),SLOT(onReplyButtonClicked()));
	connect(ui.pbtForward,SIGNAL(clicked()),SLOT(onForwardButtonClicked()));
	connect(ui.pbtChat,SIGNAL(clicked()),SLOT(onChatButtonClicked()));
	connect(ui.pbtNext,SIGNAL(clicked()),SLOT(onNextButtonClicked()));

	initialize();

	setCurrentTabWidget(ui.tabMessage);
	setMode(FMode);
	setNextCount(FNextCount);
}

MessageWindow::~MessageWindow()
{
	emit windowDestroyed();
	delete FInfoWidget->instance();
	delete FViewWidget->instance();
	delete FEditWidget->instance();
	delete FReceiversWidget->instance();
	delete FViewToolBarWidget->instance();
	delete FEditToolBarWidget->instance();
}

QString MessageWindow::tabPageId() const
{
	return "MessageWindow|"+FStreamJid.pBare()+"|"+FContactJid.pBare();
}

void MessageWindow::showWindow()
{
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit windowShow();
}

void MessageWindow::closeWindow()
{
	if (isWindow())
		close();
	else
		emit windowClose();
}

void MessageWindow::setContactJid(const Jid &AContactJid)
{
	if (FMessageWidgets->findMessageWindow(FStreamJid,AContactJid) == NULL)
	{
		Jid before = FContactJid;
		FContactJid = AContactJid;
		FInfoWidget->setContactJid(FContactJid);
		FViewWidget->setContactJid(FContactJid);
		FEditWidget->setContactJid(FContactJid);
		emit contactJidChanged(before);
	}
}

void MessageWindow::addTabWidget(QWidget *AWidget)
{
	ui.wdtTabs->addTab(AWidget,AWidget->windowIconText());
}

void MessageWindow::setCurrentTabWidget(QWidget *AWidget)
{
	if (AWidget)
		ui.wdtTabs->setCurrentWidget(AWidget);
	else
		ui.wdtTabs->setCurrentWidget(ui.wdtMessage);
}

void MessageWindow::removeTabWidget(QWidget *AWidget)
{
	ui.wdtTabs->removeTab(ui.wdtTabs->indexOf(AWidget));
}

void MessageWindow::setMode(Mode AMode)
{
	FMode = AMode;
	if (AMode == ReadMode)
	{
		ui.wdtMessage->layout()->removeWidget(FEditWidget->instance());
		ui.wdtMessage->layout()->addWidget(FViewWidget->instance());
		ui.wdtToolBar->layout()->removeWidget(FEditToolBarWidget->instance());
		ui.wdtToolBar->layout()->addWidget(FViewToolBarWidget->instance());
		FEditWidget->instance()->setParent(NULL);
		FEditToolBarWidget->instance()->setParent(NULL);
		removeTabWidget(FReceiversWidget->instance());
	}
	else
	{
		ui.wdtMessage->layout()->removeWidget(FViewWidget->instance());
		ui.wdtMessage->layout()->addWidget(FEditWidget->instance());
		ui.wdtToolBar->layout()->removeWidget(FViewToolBarWidget->instance());
		ui.wdtToolBar->layout()->addWidget(FEditToolBarWidget->instance());
		FViewWidget->instance()->setParent(NULL);
		FViewToolBarWidget->instance()->setParent(NULL);
		addTabWidget(FReceiversWidget->instance());
	}
	ui.wdtReceivers->setVisible(FMode == WriteMode);
	ui.wdtInfo->setVisible(FMode == ReadMode);
	ui.wdtSubject->setVisible(FMode == WriteMode);
	ui.pbtSend->setVisible(FMode == WriteMode);
	ui.pbtReply->setVisible(FMode == ReadMode);
	ui.pbtForward->setVisible(FMode == ReadMode);
	ui.pbtChat->setVisible(FMode == ReadMode);
}

void MessageWindow::setSubject(const QString &ASubject)
{
	ui.lneSubject->setText(ASubject);
}

void MessageWindow::setThreadId( const QString &AThreadId )
{
	FCurrentThreadId = AThreadId;
}

void MessageWindow::setNextCount(int ACount)
{
	if (ACount > 0)
		ui.pbtNext->setText(tr("Next - %1").arg(ACount));
	else
		ui.pbtNext->setText(tr("Close"));
	FNextCount = ACount;
}

void MessageWindow::initialize()
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
}

void MessageWindow::saveWindowGeometry()
{
	if (isWindow())
	{
		Options::setFileValue(saveGeometry(),"messages.messagewindow.geometry",tabPageId());
	}
}

void MessageWindow::loadWindowGeometry()
{
	if (isWindow())
	{
		restoreGeometry(Options::fileValue("messages.messagewindow.geometry",tabPageId()).toByteArray());
	}
}

void MessageWindow::updateWindow(const QIcon &AIcon, const QString &AIconText, const QString &ATitle)
{
	setWindowIcon(AIcon);
	setWindowIconText(AIconText);
	setWindowTitle(ATitle);
	emit windowChanged();
}

bool MessageWindow::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
		emit windowActivated();
	return QMainWindow::event(AEvent);
}

void MessageWindow::showEvent(QShowEvent *AEvent)
{
	if (!FShownDetached && isWindow())
		loadWindowGeometry();
	FShownDetached = isWindow();
	QMainWindow::showEvent(AEvent);
	if (FMode == WriteMode)
		FEditWidget->textEdit()->setFocus();
	emit windowActivated();
}

void MessageWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometry();
	QMainWindow::closeEvent(AEvent);
	emit windowClosed();
}

void MessageWindow::onStreamJidChanged(const Jid &ABefore)
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

void MessageWindow::onMessageReady()
{
	emit messageReady();
}

void MessageWindow::onSendButtonClicked()
{
	emit messageReady();
}

void MessageWindow::onNextButtonClicked()
{
	if (FNextCount > 0)
		emit showNextMessage();
	else
		close();
}

void MessageWindow::onReplyButtonClicked()
{
	emit replyMessage();
}

void MessageWindow::onForwardButtonClicked()
{
	emit forwardMessage();
}

void MessageWindow::onChatButtonClicked()
{
	emit showChatWindow();
}

void MessageWindow::onReceiversChanged(const Jid &AReceiver)
{
	Q_UNUSED(AReceiver);
	QString receiversStr;
	foreach(Jid contactJid,FReceiversWidget->receivers())
		receiversStr += QString("%1; ").arg(FReceiversWidget->receiverName(contactJid));
	ui.lblReceivers->setText(receiversStr);
}
