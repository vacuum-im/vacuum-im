#include "tabwindow.h"

#include <QToolButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/shortcuts.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/logger.h>

#define BLINK_VISIBLE_TIME          750
#define BLINK_INVISIBLE_TIME        250

#define ADR_TAB_INDEX               Action::DR_Parametr1
#define ADR_TAB_MENU_ACTION         Action::DR_Parametr2
#define ADR_TABWINDOWID             Action::DR_Parametr3

enum TabMenuActions {
	CloseTabAction,
	CloseOtherTabsAction,
	DetachTabAction,
	JoinTabAction,
	NewTabWindowAction
};

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,false);

	ui.twtTabs->widget(0)->deleteLater();
	ui.twtTabs->removeTab(0);
	ui.twtTabs->setMovable(true);
	ui.twtTabs->setDocumentMode(true);
	ui.twtTabs->setUsesScrollButtons(true);

	FAutoClose = true;
	FShownDetached = false;
	FWindowId = AWindowId;
	FMessageWidgets = AMessageWidgets;
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowNameChanged(const QUuid &, const QString &)),SLOT(onTabWindowNameChanged(const QUuid &, const QString &)));

	FCornerBar = new QToolBar(ui.twtTabs);
	FCornerBar->setIconSize(QSize(16, 16));
	FCornerBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
#if !defined(Q_OS_MAC)
	FCornerBar->setStyleSheet(QLatin1String("QToolBar {margin: 0px; border: 0px;}"));
#endif

	FMenuButton = new QToolButton(FCornerBar);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(FMenuButton,MNI_MESSAGEWIDGETS_TAB_MENU);
	FMenuButton->setAutoRaise(true);
	FMenuButton->setPopupMode(QToolButton::InstantPopup);

	FWindowMenu = new Menu(FMenuButton);
	FMenuButton->setMenu(FWindowMenu);
	FCornerBar->addWidget(FMenuButton);
	ui.twtTabs->setCornerWidget(FCornerBar, Qt::BottomRightCorner);

	FBlinkVisible = true;
	FBlinkTimer.setSingleShot(true);
	connect(&FBlinkTimer,SIGNAL(timeout()),SLOT(onBlinkTabNotifyTimerTimeout()));
	FBlinkTimer.start(BLINK_VISIBLE_TIME);

	createActions();

	Shortcuts::insertWidgetShortcut(SCT_TABWINDOW_CLOSETAB,this);
	Shortcuts::insertWidgetShortcut(SCT_TABWINDOW_CLOSEOTHERTABS,this);
	Shortcuts::insertWidgetShortcut(SCT_TABWINDOW_DETACHTAB,this);
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	FOptionsNode = Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId);
	onOptionsChanged(FOptionsNode.node("tabs-closable"));
	onOptionsChanged(FOptionsNode.node("tabs-bottom"));
	onOptionsChanged(FOptionsNode.node("show-indices"));
	onOptionsChanged(FOptionsNode.node("remove-tabs-on-close"));
	onOptionsChanged(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT));
	onOptionsChanged(Options::node(OPV_MESSAGES_COMBINEWITHROSTER));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
	connect(ui.twtTabs,SIGNAL(tabMoved(int,int)),SLOT(onTabMoved(int,int)));
	connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));
	connect(ui.twtTabs,SIGNAL(tabMenuRequested(int)),SLOT(onTabMenuRequested(int)));
}

TabWindow::~TabWindow()
{
	clearTabs();
	FCornerBar->deleteLater();
	emit windowDestroyed();
	emit centralPageDestroyed();
}

void TabWindow::showCentralPage(bool AMinimized)
{
	if (!AMinimized)
		showWindow();
	else
		showMinimizedWindow();
}

QIcon TabWindow::centralPageIcon() const
{
	return windowIcon();
}

QString TabWindow::centralPageCaption() const
{
	IMessageTabPage *page = currentTabPage();
	if (page)
		return page->tabPageCaption();
	return QString::null;
}

void TabWindow::showWindow()
{
	if (isWindow())
		WidgetManager::showActivateRaiseWindow(this);
	else
		emit centralPageShow(false);
}

void TabWindow::showMinimizedWindow()
{
	if (!isWindow())
		emit centralPageShow(true);
	else if (!isVisible())
		showMinimized();
}

QUuid TabWindow::windowId() const
{
	return FWindowId;
}

QString TabWindow::windowName() const
{
	return FMessageWidgets->tabWindowName(FWindowId);
}

Menu *TabWindow::windowMenu() const
{
	return FWindowMenu;
}

bool TabWindow::isTabBarVisible() const
{
	return ui.twtTabs->isTabBarVisible();
}

void TabWindow::setTabBarVisible(bool AVisible)
{
	if (isTabBarVisible() != AVisible)
	{
		ui.twtTabs->setCornerWidget(AVisible ? FCornerBar : NULL);
		FCornerBar->setParent(AVisible ? ui.twtTabs : NULL);
		FCornerBar->setVisible(AVisible);

		ui.twtTabs->setTabBarVisible(AVisible);
		emit windowChanged();
	}
}

bool TabWindow::isAutoCloseEnabled() const
{
	return FAutoClose;
}

void TabWindow::setAutoCloseEnabled(bool AEnabled)
{
	if (AEnabled != FAutoClose)
	{
		FAutoClose = AEnabled;
		if (AEnabled)
			QTimer::singleShot(0,this,SLOT(onCloseWindowIfEmpty()));
		emit windowChanged();
	}
}

int TabWindow::tabPageCount() const
{
	return ui.twtTabs->count();
}

IMessageTabPage *TabWindow::tabPage(int AIndex) const
{
	QWidget *page = ui.twtTabs->widget(AIndex);
	return qobject_cast<IMessageTabPage *>(page);
}

void TabWindow::addTabPage(IMessageTabPage *APage)
{
	if (!hasTabPage(APage))
	{
		int index = ui.twtTabs->addTab(APage->instance(),APage->tabPageIcon(),APage->tabPageCaption());
		connect(APage->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
		connect(APage->instance(),SIGNAL(tabPageShowMinimized()),SLOT(onTabPageShowMinimized()));
		connect(APage->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
		connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));

		if (APage->tabPageNotifier())
			connect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		connect(APage->instance(),SIGNAL(tabPageNotifierChanged()),SLOT(onTabPageNotifierChanged()));
		
		updateTab(index);
		emit tabPageAdded(APage);
	}
}

bool TabWindow::hasTabPage(IMessageTabPage *APage) const
{
	return APage!=NULL && ui.twtTabs->indexOf(APage->instance()) >= 0;
}

IMessageTabPage *TabWindow::currentTabPage() const
{
	return qobject_cast<IMessageTabPage *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentTabPage(IMessageTabPage *APage)
{
	if (APage)
		ui.twtTabs->setCurrentWidget(APage->instance());
}

void TabWindow::detachTabPage(IMessageTabPage *APage)
{
	if (hasTabPage(APage))
	{
		removeTabPage(APage);
		APage->instance()->show();
		if (APage->instance()->x()<=0 || APage->instance()->y()<0)
			APage->instance()->move(0,0);
		emit tabPageDetached(APage);
	}
}

void TabWindow::removeTabPage(IMessageTabPage *APage)
{
	int index = APage!=NULL ? ui.twtTabs->indexOf(APage->instance()) : -1;
	if (index >= 0)
	{
		ui.twtTabs->removeTab(index);
		APage->instance()->close();
		APage->instance()->setParent(NULL);
		disconnect(APage->instance(),SIGNAL(tabPageShow()),this,SLOT(onTabPageShow()));
		disconnect(APage->instance(),SIGNAL(tabPageClose()),this,SLOT(onTabPageClose()));
		disconnect(APage->instance(),SIGNAL(tabPageChanged()),this,SLOT(onTabPageChanged()));
		disconnect(APage->instance(),SIGNAL(tabPageDestroyed()),this,SLOT(onTabPageDestroyed()));

		if (APage->tabPageNotifier())
			disconnect(APage->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),this,SLOT(onTabPageNotifierActiveNotifyChanged(int)));
		disconnect(APage->instance(),SIGNAL(tabPageNotifierChanged()),this,SLOT(onTabPageNotifierChanged()));

		updateTabs(index,ui.twtTabs->count()-1);
		emit tabPageRemoved(APage);

		QTimer::singleShot(0,this,SLOT(onCloseWindowIfEmpty()));
	}
}

void TabWindow::createActions()
{
	QSignalMapper *tabMapper = new QSignalMapper(this);
	for (int tabNumber=1; tabNumber<=10; tabNumber++)
	{
		Action *action = new Action(this);
		action->setShortcutId(QString(SCT_TABWINDOW_QUICKTAB).arg(tabNumber));
		FMenuButton->addAction(action);

		tabMapper->setMapping(action, tabNumber-1); // QTabWidget's indices are 0-based
		connect(action, SIGNAL(triggered()), tabMapper, SLOT(map()));
	}
	connect(tabMapper, SIGNAL(mapped(int)), ui.twtTabs, SLOT(setCurrentIndex(int)));

	FNextTab = new Action(FWindowMenu);
	FNextTab->setText(tr("Next Tab"));
	FNextTab->setShortcutId(SCT_TABWINDOW_NEXTTAB);
	FWindowMenu->addAction(FNextTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FNextTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FPrevTab = new Action(FWindowMenu);
	FPrevTab->setText(tr("Prev. Tab"));
	FPrevTab->setShortcutId(SCT_TABWINDOW_PREVTAB);
	FWindowMenu->addAction(FPrevTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FPrevTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FShowCloseButtons = new Action(FWindowMenu);
	FShowCloseButtons->setText(tr("Tabs Closable"));
	FShowCloseButtons->setCheckable(true);
	FShowCloseButtons->setChecked(ui.twtTabs->tabsClosable());
	FWindowMenu->addAction(FShowCloseButtons,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FShowCloseButtons,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FTabsBottom = new Action(FWindowMenu);
	FTabsBottom->setText(tr("Show Tabs at Bottom of the Window"));
	FTabsBottom->setCheckable(true);
	FTabsBottom->setChecked(ui.twtTabs->tabPosition() == QTabWidget::South);
	FWindowMenu->addAction(FTabsBottom,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FTabsBottom,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FShowIndices = new Action(FWindowMenu);
	FShowIndices->setText(tr("Show Tabs Indices"));
	FShowIndices->setCheckable(true);
	FWindowMenu->addAction(FShowIndices,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FShowIndices,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FRemoveTabsOnClose = new Action(FWindowMenu);
	FRemoveTabsOnClose->setText(tr("Remove all tabs on window close"));
	FRemoveTabsOnClose->setCheckable(true);
	FWindowMenu->addAction(FRemoveTabsOnClose,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FRemoveTabsOnClose,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FSetAsDefault = new Action(FWindowMenu);
	FSetAsDefault->setText(tr("Use as Default Tab Window"));
	FSetAsDefault->setCheckable(true);
	FWindowMenu->addAction(FSetAsDefault,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FSetAsDefault,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FRenameWindow = new Action(FWindowMenu);
	FRenameWindow->setText(tr("Rename Tab Window"));
	FWindowMenu->addAction(FRenameWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FRenameWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FCloseWindow = new Action(FWindowMenu);
	FCloseWindow->setText(tr("Close Tab Window"));
	FCloseWindow->setShortcutId(SCT_TABWINDOW_CLOSEWINDOW);
	FWindowMenu->addAction(FCloseWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FCloseWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FDeleteWindow = new Action(FWindowMenu);
	FDeleteWindow->setText(tr("Delete Tab Window"));
	FWindowMenu->addAction(FDeleteWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FDeleteWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
}

void TabWindow::saveWindowStateAndGeometry()
{
	if (isWindow() && FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		Options::setFileValue(saveState(),"messages.tabwindows.window.state",FWindowId.toString());
		Options::setFileValue(saveGeometry(),"messages.tabwindows.window.geometry",FWindowId.toString());
	}
}

void TabWindow::loadWindowStateAndGeometry()
{
	if (isWindow() && FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		if (!restoreGeometry(Options::fileValue("messages.tabwindows.window.geometry",FWindowId.toString()).toByteArray()))
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
		restoreState(Options::fileValue("messages.tabwindows.window.state",FWindowId.toString()).toByteArray());
	}
}

void TabWindow::updateWindow()
{
	IMessageTabPage *page = currentTabPage();
	if (page)
	{
		setWindowIcon(page->tabPageIcon());
		setWindowTitle(page->tabPageCaption() + " - " + windowName());
		emit windowChanged();
		emit centralPageChanged();
	}
}

void TabWindow::clearTabs()
{
	while (ui.twtTabs->count() > 0)
	{
		IMessageTabPage *page = qobject_cast<IMessageTabPage *>(ui.twtTabs->widget(0));
		if (page)
			removeTabPage(page);
		else
			ui.twtTabs->removeTab(0);
	}
}

void TabWindow::updateTab(int AIndex)
{
	IMessageTabPage *page = tabPage(AIndex);
	if (page)
	{
		QIcon tabIcon = page->tabPageIcon();
		QString tabCaption = page->tabPageCaption();
		QString tabToolTip = page->tabPageToolTip();

		if (page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
		{
			static QIcon emptyIcon;
			if (emptyIcon.isNull())
			{
				QPixmap pixmap(ui.twtTabs->iconSize());
				pixmap.fill(QColor(0,0,0,0));
				emptyIcon.addPixmap(pixmap);
			}

			IMessageTabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
			if (!notify.icon.isNull())
				tabIcon = notify.icon;
			if (notify.blink && !FBlinkVisible)
				tabIcon = emptyIcon;
			if (!notify.caption.isNull())
				tabCaption = notify.caption;
			if (!notify.toolTip.isNull())
				tabToolTip = notify.toolTip;
		}

		if (FShowIndices->isChecked() && AIndex<10)
			tabCaption = tr("%1) %2").arg(QString::number((AIndex+1) % 10)).arg(tabCaption);
		tabCaption = TextManager::getElidedString(tabCaption,Qt::ElideRight,20);

		ui.twtTabs->setTabIcon(AIndex,tabIcon);
		ui.twtTabs->setTabText(AIndex,tabCaption);
		ui.twtTabs->setTabToolTip(AIndex,tabToolTip);

		if (AIndex == ui.twtTabs->currentIndex())
			updateWindow();
	}
}

void TabWindow::updateTabs(int AFrom, int ATo)
{
	for (int tab=AFrom; tab<=ATo; tab++)
		updateTab(tab);
}

void TabWindow::showEvent(QShowEvent *AEvent)
{
	if (isWindow())
	{
		if (!FShownDetached)
			loadWindowStateAndGeometry();
		FShownDetached = true;
	}
	else
	{
		FShownDetached = false;
	}
	QMainWindow::showEvent(AEvent);
}

void TabWindow::closeEvent(QCloseEvent *AEvent)
{
	if (FShownDetached)
		saveWindowStateAndGeometry();
	QMainWindow::closeEvent(AEvent);
}

void TabWindow::onTabMoved(int AFrom, int ATo)
{
	if (FShowIndices->isChecked())
		updateTabs(qMin(AFrom,ATo),qMax(AFrom,ATo));
}

void TabWindow::onTabChanged(int AIndex)
{
	Q_UNUSED(AIndex);
	updateWindow();
	emit currentTabPageChanged(currentTabPage());
}

void TabWindow::onTabCloseRequested(int AIndex)
{
	removeTabPage(tabPage(AIndex));
}

void TabWindow::onTabMenuRequested(int AIndex)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose, true);

	bool isCombined = Options::node(OPV_MESSAGES_COMBINEWITHROSTER).value().toBool();

	if (AIndex >= 0)
	{
		Action *tabClose = new Action(menu);
		tabClose->setText(tr("Close Tab"));
		tabClose->setData(ADR_TAB_INDEX, AIndex);
		tabClose->setData(ADR_TAB_MENU_ACTION, CloseTabAction);
		tabClose->setShortcutId(SCT_TABWINDOW_CLOSETAB);
		connect(tabClose,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
		menu->addAction(tabClose,AG_MWTWTM_MWIDGETS_TAB_ACTIONS);

		Action *otherClose = new Action(menu);
		otherClose->setText(tr("Close Other Tabs"));
		otherClose->setData(ADR_TAB_INDEX, AIndex);
		otherClose->setData(ADR_TAB_MENU_ACTION, CloseOtherTabsAction);
		otherClose->setShortcutId(SCT_TABWINDOW_CLOSEOTHERTABS);
		otherClose->setEnabled(ui.twtTabs->count()>1);
		connect(otherClose,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
		menu->addAction(otherClose,AG_MWTWTM_MWIDGETS_TAB_ACTIONS);

		if (!isCombined)
		{
			Action *detachTab = new Action(menu);
			detachTab->setText(tr("Detach to Separate Window"));
			detachTab->setData(ADR_TAB_INDEX, AIndex);
			detachTab->setData(ADR_TAB_MENU_ACTION, DetachTabAction);
			detachTab->setShortcutId(SCT_TABWINDOW_DETACHTAB);
			menu->addAction(detachTab,AG_MWTWTM_MWIDGETS_TAB_ACTIONS);
			connect(detachTab,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));

			Menu *joinTab = new Menu(menu);
			joinTab->setTitle(tr("Join to"));
			menu->addAction(joinTab->menuAction(),AG_MWTWTM_MWIDGETS_TAB_ACTIONS);

			foreach(const QUuid &id, FMessageWidgets->tabWindowList())
			{
				if (id != FWindowId)
				{
					Action *action = new Action(joinTab);
					action->setText(FMessageWidgets->tabWindowName(id));
					action->setData(ADR_TAB_INDEX, AIndex);
					action->setData(ADR_TABWINDOWID,id.toString());
					action->setData(ADR_TAB_MENU_ACTION, JoinTabAction);
					joinTab->addAction(action);
					connect(action,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
				}
			}

			Action *newWindow = new Action(joinTab);
			newWindow->setText(tr("New Tab Window"));
			newWindow->setData(ADR_TAB_INDEX, AIndex);
			newWindow->setData(ADR_TAB_MENU_ACTION, NewTabWindowAction);
			joinTab->addAction(newWindow,AG_DEFAULT+1);
			connect(newWindow,SIGNAL(triggered(bool)),SLOT(onTabMenuActionTriggered(bool)));
		}
	}
	else if (!isCombined)
	{
		Action *windowClose = new Action(menu);
		windowClose->setText(tr("Close Tab Window"));
		windowClose->setShortcutId(SCT_TABWINDOW_CLOSEWINDOW);
		connect(windowClose,SIGNAL(triggered()),SLOT(close()));
		menu->addAction(windowClose,AG_MWTWTM_MWIDGETS_TAB_ACTIONS);
	}

	emit tabPageMenuRequested(tabPage(AIndex),menu);

	if (!menu->isEmpty())
		menu->popup(QCursor::pos());
	else
		delete menu;
}

void TabWindow::onTabPageShow()
{
	IMessageTabPage *page = qobject_cast<IMessageTabPage *>(sender());
	if (page)
	{
		setCurrentTabPage(page);
		showWindow();
	}
}

void TabWindow::onTabPageShowMinimized()
{
	showMinimizedWindow();
}

void TabWindow::onTabPageClose()
{
	removeTabPage(qobject_cast<IMessageTabPage *>(sender()));
}

void TabWindow::onTabPageChanged()
{
	IMessageTabPage *page = qobject_cast<IMessageTabPage *>(sender());
	if (page)
		updateTab(ui.twtTabs->indexOf(page->instance()));
}

void TabWindow::onTabPageDestroyed()
{
	removeTabPage(qobject_cast<IMessageTabPage *>(sender()));
}

void TabWindow::onTabPageNotifierChanged()
{
	IMessageTabPage *page = qobject_cast<IMessageTabPage *>(sender());
	if (page && page->tabPageNotifier()!=NULL)
		connect(page->tabPageNotifier()->instance(),SIGNAL(activeNotifyChanged(int)),SLOT(onTabPageNotifierActiveNotifyChanged(int)));
}

void TabWindow::onTabPageNotifierActiveNotifyChanged(int ANotifyId)
{
	Q_UNUSED(ANotifyId);
	IMessageTabPageNotifier *notifier = qobject_cast<IMessageTabPageNotifier *>(sender());
	if (notifier)
		updateTab(ui.twtTabs->indexOf(notifier->tabPage()->instance()));
}

void TabWindow::onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName)
{
	Q_UNUSED(AName);
	if (AWindowId == FWindowId)
		updateWindow();
}

void TabWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_TABWINDOWS_DEFAULT)
	{
		FSetAsDefault->setChecked(FWindowId==ANode.value().toString());
		FDeleteWindow->setVisible(!FSetAsDefault->isChecked());
	}
	else if (ANode.path() == OPV_MESSAGES_COMBINEWITHROSTER)
	{
		bool isCombined = ANode.value().toBool();
		FRemoveTabsOnClose->setVisible(!isCombined);
		FSetAsDefault->setVisible(!isCombined);
		FRenameWindow->setVisible(!isCombined);
		FCloseWindow->setVisible(!isCombined);
		FDeleteWindow->setVisible(!isCombined);
	}
	else if (FOptionsNode.childPath(ANode) == "tabs-closable")
	{
		FShowCloseButtons->setChecked(ANode.value().toBool());
		ui.twtTabs->setTabsClosable(ANode.value().toBool());
	}
	else if (FOptionsNode.childPath(ANode) == "tabs-bottom")
	{
		FTabsBottom->setChecked(ANode.value().toBool());
		ui.twtTabs->setTabPosition(ANode.value().toBool() ? QTabWidget::South : QTabWidget::North);
	}
	else if (FOptionsNode.childPath(ANode) == "show-indices")
	{
		FShowIndices->setChecked(ANode.value().toBool());
		updateTabs(0,ui.twtTabs->count()-1);
	}
	else if (FOptionsNode.childPath(ANode) == "remove-tabs-on-close")
	{
		FRemoveTabsOnClose->setChecked(ANode.value().toBool());
		setAttribute(Qt::WA_DeleteOnClose,ANode.value().toBool());
	}
}

void TabWindow::onActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FNextTab)
	{
		ui.twtTabs->setCurrentIndex((ui.twtTabs->currentIndex()+1) % ui.twtTabs->count());
	}
	else if (action == FPrevTab)
	{
		ui.twtTabs->setCurrentIndex(ui.twtTabs->currentIndex()>0 ? ui.twtTabs->currentIndex()-1 : ui.twtTabs->count()-1);
	}
	else if (action == FShowCloseButtons)
	{
		FOptionsNode.node("tabs-closable").setValue(action->isChecked());
	}
	else if (action == FTabsBottom)
	{
		FOptionsNode.node("tabs-bottom").setValue(action->isChecked());
	}
	else if (action == FShowIndices)
	{
		FOptionsNode.node("show-indices").setValue(action->isChecked());
	}
	else if (action == FRemoveTabsOnClose)
	{
		FOptionsNode.node("remove-tabs-on-close").setValue(action->isChecked());
	}
	else if (action == FSetAsDefault)
	{
		Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT).setValue(true);
	}
	else if (action == FRenameWindow)
	{
		QString name = QInputDialog::getText(this,tr("Rename Tab Window"),tr("Tab window name:"),QLineEdit::Normal,FMessageWidgets->tabWindowName(FWindowId));
		if (!name.isEmpty())
			FMessageWidgets->setTabWindowName(FWindowId,name);
	}
	else if (action == FCloseWindow)
	{
		close();
	}
	else if (action == FDeleteWindow)
	{
		if (QMessageBox::question(this,tr("Delete Tab Window"),tr("Are you sure you want to delete this tab window?"),
			QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok)
		{
			FMessageWidgets->deleteTabWindow(FWindowId);
		}
	}
}

void TabWindow::onTabMenuActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMessageTabPage *page = tabPage(action->data(ADR_TAB_INDEX).toInt());
		int tabAction = action->data(ADR_TAB_MENU_ACTION).toInt();
		if (tabAction == CloseTabAction)
		{
			removeTabPage(page);
		}
		else if (tabAction == CloseOtherTabsAction)
		{
			int index = action->data(ADR_TAB_INDEX).toInt();
			while (index+1 < ui.twtTabs->count())
				onTabCloseRequested(index+1);
			for (int i=0; i<index; i++)
				onTabCloseRequested(0);
		}
		else if (tabAction == DetachTabAction)
		{
			detachTabPage(page);
		}
		else if (tabAction == NewTabWindowAction)
		{
			QString name = QInputDialog::getText(this,tr("New Tab Window"),tr("Tab window name:"));
			if (!name.isEmpty())
			{
				IMessageTabWindow *window = FMessageWidgets->getTabWindow(FMessageWidgets->appendTabWindow(name));
				removeTabPage(page);
				window->addTabPage(page);
				window->showWindow();
			}
		}
		else if (tabAction == JoinTabAction)
		{
			IMessageTabWindow *window = FMessageWidgets->getTabWindow(action->data(ADR_TABWINDOWID).toString());
			removeTabPage(page);
			window->addTabPage(page);
			window->showWindow();
		}
	}
}

void TabWindow::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AWidget==this && isTabBarVisible())
	{
		if (AId == SCT_TABWINDOW_CLOSETAB)
		{
			removeTabPage(currentTabPage());
		}
		else if (AId == SCT_TABWINDOW_CLOSEOTHERTABS)
		{
			int index = ui.twtTabs->currentIndex();
			while (index+1 < ui.twtTabs->count())
				onTabCloseRequested(index+1);
			for (int i=0; i<index; i++)
				onTabCloseRequested(0);
		}
		else if (AId == SCT_TABWINDOW_DETACHTAB)
		{
			detachTabPage(currentTabPage());
		}
	}
}

void TabWindow::onBlinkTabNotifyTimerTimeout()
{
	if (!FBlinkVisible)
	{
		FBlinkVisible = true;
		FBlinkTimer.start(BLINK_VISIBLE_TIME);
	}
	else
	{
		FBlinkVisible = false;
		FBlinkTimer.start(BLINK_INVISIBLE_TIME);
	}

	for (int index=0; index<tabPageCount(); index++)
	{
		IMessageTabPage *page = tabPage(index);
		if (page && page->tabPageNotifier() && page->tabPageNotifier()->activeNotify()>0)
		{
			IMessageTabPageNotify notify = page->tabPageNotifier()->notifyById(page->tabPageNotifier()->activeNotify());
			if (notify.blink && !notify.icon.isNull())
				updateTab(index);
		}
	}
}

void TabWindow::onCloseWindowIfEmpty()
{
	if (isAutoCloseEnabled() && tabPageCount()==0)
	{
		deleteLater();
		close();
	}
}
