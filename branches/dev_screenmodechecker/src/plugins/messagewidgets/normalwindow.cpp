#include "normalwindow.h"

#include <QLineEdit>
#include <QHeaderView>

NormalWindow::NormalWindow(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, Mode AMode)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	ui.bwtMessageBox->layout()->setSpacing(3);

	FMessageWidgets = AMessageWidgets;

	FShownDetached = false;
	FCurrentThreadId = QUuid::createUuid().toString();

	FTabPageNotifier = NULL;

	FAddress = FMessageWidgets->newAddress(AStreamJid,AContactJid,this);

	FSubjectWidget = new QLineEdit(ui.bwtMessageBox);
	FSubjectWidget->setPlaceholderText(tr("Subject"));
	ui.bwtMessageBox->insertWidget(MNWW_SUBJECTWIDGET,FSubjectWidget);

	FInfoWidget = FMessageWidgets->newInfoWidget(this,ui.bwtMessageBox);
	ui.bwtMessageBox->insertWidget(MNWW_INFOWIDGET,FInfoWidget->instance());

	FViewWidget = FMessageWidgets->newViewWidget(this,ui.bwtMessageBox);
	ui.bwtMessageBox->insertWidget(MNWW_VIEWWIDGET,FViewWidget->instance(),100);

	FEditWidget = FMessageWidgets->newEditWidget(this,ui.bwtMessageBox);
	FEditWidget->setEditToolBarVisible(false);
	FEditWidget->setSendShortcutId(SCT_MESSAGEWINDOWS_NORMAL_SENDMESSAGE);
	ui.bwtMessageBox->insertWidget(MNWW_EDITWIDGET,FEditWidget->instance(),100);

	FToolBarWidget = FMessageWidgets->newToolBarWidget(this,ui.bwtMessageBox);
	FToolBarWidget->toolBarChanger()->setSeparatorsVisible(false);
	ui.bwtMessageBox->insertWidget(MNWW_TOOLBARWIDGET,FToolBarWidget->instance());

	ui.wdtReceiversTree->setLayout(new QVBoxLayout(ui.wdtReceiversTree));
	ui.wdtReceiversTree->layout()->setMargin(0);
	FReceiversWidget = FMessageWidgets->newReceiversWidget(this,ui.wdtReceivers);
	connect(FReceiversWidget->instance(),SIGNAL(addressSelectionChanged()),SLOT(onReceiverslAddressSelectionChanged()));
	FReceiversWidget->setAddressSelection(AStreamJid,AContactJid,true);
	ui.wdtReceiversTree->layout()->addWidget(FReceiversWidget->instance());

	FMenuBarWidget = FMessageWidgets->newMenuBarWidget(this,this);
	setMenuBar(FMenuBarWidget->instance());

	FStatusBarWidget = FMessageWidgets->newStatusBarWidget(this,this);
	setStatusBar(FStatusBarWidget->instance());

	Menu *menu = new Menu(ui.tlbReceivers);
	ui.tlbReceivers->setMenu(menu);
	connect(menu,SIGNAL(aboutToShow()),SLOT(onSelectReceiversMenuAboutToShow()));

	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(ui.tlbReceivers,MNI_MESSAGEWIDGETS_SELECT);

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	setMode(AMode);
	onReceiverslAddressSelectionChanged();
}

NormalWindow::~NormalWindow()
{
	emit tabPageDestroyed();
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
	return "NormalWindow|"+streamJid().pBare()+"|"+contactJid().pBare();
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

IMessageNormalWindow::Mode NormalWindow::mode() const
{
	return FMode;
}

void NormalWindow::setMode(Mode AMode)
{
	FMode = AMode;
	if (AMode == ReadMode)
	{
		FViewWidget->instance()->setVisible(true);
		FEditWidget->instance()->setVisible(false);
	}
	else
	{
		FViewWidget->instance()->setVisible(false);
		FEditWidget->instance()->setVisible(true);
	}

	ui.wdtReceivers->setVisible(AMode == WriteMode);
	FInfoWidget->instance()->setVisible(AMode == ReadMode);
	FSubjectWidget->setVisible(AMode == WriteMode);

	QTimer::singleShot(0,this,SIGNAL(widgetLayoutChanged()));
	emit modeChanged(AMode);
}

QString NormalWindow::subject() const
{
	return FSubjectWidget->text();
}

void NormalWindow::setSubject(const QString &ASubject)
{
	FSubjectWidget->setText(ASubject);
}

QString NormalWindow::threadId() const
{
	return FCurrentThreadId;
}

void NormalWindow::setThreadId(const QString &AThreadId)
{
	FCurrentThreadId = AThreadId;
}

void NormalWindow::saveWindowGeometryAndState()
{
	if (isWindow())
	{
		Options::setFileValue(saveState(),"messages.messagewindow.state",tabPageId());
		Options::setFileValue(saveGeometry(),"messages.messagewindow.geometry",tabPageId());
	}
	Options::setFileValue(ui.sprReceivers->saveState(),"messages.messagewindow.splitter-receivers-state");
}

void NormalWindow::loadWindowGeometryAndState()
{
	if (isWindow())
	{
		if (!restoreGeometry(Options::fileValue("messages.messagewindow.geometry",tabPageId()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("messages.messagewindow.state",tabPageId()).toByteArray());
	}

	if (!ui.sprReceivers->restoreState(Options::fileValue("messages.messagewindow.splitter-receivers-state").toByteArray()))
		ui.sprReceivers->setSizes(QList<int>() << 700 << 300);
}

BoxWidget *NormalWindow::messageWidgetsBox() const
{
	return ui.bwtMessageBox;
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
	if (FMode == WriteMode)
		FEditWidget->textEdit()->setFocus();
	if (isActiveTabPage())
		emit tabPageActivated();
}

void NormalWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowGeometryAndState();
	QMainWindow::closeEvent(AEvent);
	emit tabPageClosed();
}

void NormalWindow::onSelectReceiversMenuAboutToShow()
{
	Menu *menu = qobject_cast<Menu *>(sender());
	if (menu)
	{
		menu->clear();
		FReceiversWidget->contextMenuForItems(QList<QStandardItem *>() << FReceiversWidget->receiversModel()->invisibleRootItem(),menu);
	}
}

void NormalWindow::onReceiverslAddressSelectionChanged()
{
	ui.lblReceivers->setText(tr("Selected %n contact(s)","",FReceiversWidget->selectedAddresses().count()));
}

void NormalWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AId==SCT_MESSAGEWINDOWS_CLOSEWINDOW && AWidget==this)
	{
		closeTabPage();
	}
}
