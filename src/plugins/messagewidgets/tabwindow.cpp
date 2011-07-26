#include "tabwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>

#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,false);

	ui.twtTabs->widget(0)->deleteLater();
	ui.twtTabs->removeTab(0);
	ui.twtTabs->setMovable(true);
	ui.twtTabs->setDocumentMode(true);

	FWindowId = AWindowId;
	FMessageWidgets = AMessageWidgets;
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowAppended(const QUuid &, const QString &)),
		SLOT(onTabWindowAppended(const QUuid &, const QString &)));
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowNameChanged(const QUuid &, const QString &)),
		SLOT(onTabWindowNameChanged(const QUuid &, const QString &)));
	connect(FMessageWidgets->instance(),SIGNAL(tabWindowDeleted(const QUuid &)),SLOT(onTabWindowDeleted(const QUuid &)));

	QPushButton *menuButton = new QPushButton(ui.twtTabs);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(menuButton,MNI_MESSAGEWIDGETS_TAB_MENU);
	menuButton->setFlat(true);

	FWindowMenu = new Menu(menuButton);
	menuButton->setMenu(FWindowMenu);
	ui.twtTabs->setCornerWidget(menuButton);

	createActions();
	loadWindowStateAndGeometry();

	FOptionsNode = Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId);
	onOptionsChanged(FOptionsNode.node("tabs-closable"));
	onOptionsChanged(FOptionsNode.node("tabs-bottom"));
	onOptionsChanged(FOptionsNode.node("show-indices"));
	onOptionsChanged(FOptionsNode.node("remove-tabs-on-close"));
	onOptionsChanged(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
	connect(ui.twtTabs,SIGNAL(tabMoved(int,int)),SLOT(onTabMoved(int,int)));
	connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));
}

TabWindow::~TabWindow()
{
	clearTabs();
	saveWindowStateAndGeometry();
	emit windowDestroyed();
}

void TabWindow::showWindow()
{
	WidgetManager::showActivateRaiseWindow(this);
}

void TabWindow::showMinimizedWindow()
{
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

int TabWindow::tabPageCount() const
{
	return ui.twtTabs->count();
}

ITabPage *TabWindow::tabPage(int AIndex) const
{
	return qobject_cast<ITabPage *>(ui.twtTabs->widget(AIndex));
}

void TabWindow::addTabPage(ITabPage *APage)
{
	if (!hasTabPage(APage))
	{
		int index = ui.twtTabs->addTab(APage->instance(),APage->instance()->windowTitle());
		connect(APage->instance(),SIGNAL(tabPageShow()),SLOT(onTabPageShow()));
		connect(APage->instance(),SIGNAL(tabPageShowMinimized()),SLOT(onTabPageShowMinimized()));
		connect(APage->instance(),SIGNAL(tabPageClose()),SLOT(onTabPageClose()));
		connect(APage->instance(),SIGNAL(tabPageChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(tabPageDestroyed()),SLOT(onTabPageDestroyed()));
		updateTab(index);
		updateWindow();
		emit tabPageAdded(APage);
	}
}

bool TabWindow::hasTabPage(ITabPage *APage) const
{
	return APage!=NULL && ui.twtTabs->indexOf(APage->instance()) >= 0;
}

ITabPage *TabWindow::currentTabPage() const
{
	return qobject_cast<ITabPage *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentTabPage(ITabPage *APage)
{
	if (APage)
		ui.twtTabs->setCurrentWidget(APage->instance());
}

void TabWindow::detachTabPage(ITabPage *APage)
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

void TabWindow::removeTabPage(ITabPage *APage)
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
		updateTabs(index,ui.twtTabs->count()-1);
		emit tabPageRemoved(APage);

		if (ui.twtTabs->count() == 0)
			deleteLater();
	}
}

void TabWindow::createActions()
{
	QSignalMapper *tabMapper = new QSignalMapper(this);
	for (int tabNumber=1; tabNumber<=10; tabNumber++)
	{
		Action *action = new Action(this);
		action->setShortcutId(QString(SCT_TABWINDOW_QUICKTAB).arg(tabNumber));
		addAction(action);

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

	FCloseTab = new Action(FWindowMenu);
	FCloseTab->setText(tr("Close Tab"));
	FCloseTab->setShortcutId(SCT_TABWINDOW_CLOSETAB);
	FWindowMenu->addAction(FCloseTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FDetachWindow = new Action(FWindowMenu);
	FDetachWindow->setText(tr("Detach to Separate Window"));
	FDetachWindow->setShortcutId(SCT_TABWINDOW_DETACHTAB);
	FWindowMenu->addAction(FDetachWindow,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FDetachWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FJoinMenu = new Menu(FWindowMenu);
	FJoinMenu->setTitle(tr("Join to"));
	FWindowMenu->addAction(FJoinMenu->menuAction(),AG_MWTW_MWIDGETS_TAB_ACTIONS);

	foreach(QUuid id,FMessageWidgets->tabWindowList())
		if (id != FWindowId)
			onTabWindowAppended(id, FMessageWidgets->tabWindowName(id));

	FNewTab = new Action(FJoinMenu);
	FNewTab->setText(tr("New Tab Window"));
	FJoinMenu->addAction(FNewTab,AG_DEFAULT+1);
	connect(FNewTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FShowCloseButtons = new Action(FWindowMenu);
	FShowCloseButtons->setText(tr("Tabs Closable"));
	FShowCloseButtons->setCheckable(true);
	FShowCloseButtons->setChecked(ui.twtTabs->tabsClosable());
	FShowCloseButtons->setShortcutId(SCT_TABWINDOW_SHOWCLOSEBUTTTONS);
	FWindowMenu->addAction(FShowCloseButtons,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FShowCloseButtons,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FTabsBottom = new Action(FWindowMenu);
	FTabsBottom->setText(tr("Show Tabs at Bottom of the Window"));
	FTabsBottom->setCheckable(true);
	FTabsBottom->setChecked(ui.twtTabs->tabPosition() == QTabWidget::South);
	FTabsBottom->setShortcutId(SCT_TABWINDOW_TABSBOTTOM);
	FWindowMenu->addAction(FTabsBottom,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FTabsBottom,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FShowIndices = new Action(FWindowMenu);
	FShowIndices->setText(tr("Show Tabs Indices"));
	FShowIndices->setCheckable(true);
	FShowIndices->setShortcutId(SCT_TABWINDOW_TABSINDICES);
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
	FSetAsDefault->setShortcutId(SCT_TABWINDOW_SETASDEFAULT);
	FWindowMenu->addAction(FSetAsDefault,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FSetAsDefault,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FRenameWindow = new Action(FWindowMenu);
	FRenameWindow->setText(tr("Rename Tab Window"));
	FRenameWindow->setShortcutId(SCT_TABWINDOW_RENAMEWINDOW);
	FWindowMenu->addAction(FRenameWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FRenameWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FCloseWindow = new Action(FWindowMenu);
	FCloseWindow->setText(tr("Close Tab Window"));
	FCloseWindow->setShortcutId(SCT_TABWINDOW_CLOSEWINDOW);
	FWindowMenu->addAction(FCloseWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FCloseWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FDeleteWindow = new Action(FWindowMenu);
	FDeleteWindow->setText(tr("Delete Tab Window"));
	FDeleteWindow->setShortcutId(SCT_TABWINDOW_DELETEWINDOW);
	FWindowMenu->addAction(FDeleteWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FDeleteWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
}

void TabWindow::saveWindowStateAndGeometry()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		if (isWindow())
		{
			Options::setFileValue(saveState(),"messages.tabwindows.window.state",FWindowId.toString());
			Options::setFileValue(saveGeometry(),"messages.tabwindows.window.geometry",FWindowId.toString());
		}
	}
}

void TabWindow::loadWindowStateAndGeometry()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		if (isWindow())
		{
			if (!restoreGeometry(Options::fileValue("messages.tabwindows.window.geometry",FWindowId.toString()).toByteArray()))
				setGeometry(WidgetManager::alignGeometry(QSize(640,480),this));
			restoreState(Options::fileValue("messages.tabwindows.window.state",FWindowId.toString()).toByteArray());
		}
	}
}

void TabWindow::updateWindow()
{
	ITabPage *page = currentTabPage();
	if (page)
	{
		setWindowIcon(page->tabPageIcon());
		setWindowTitle(page->tabPageCaption() + " - " + windowName());
		emit windowChanged();
	}
}
void TabWindow::clearTabs()
{
	while (ui.twtTabs->count() > 0)
	{
		ITabPage *page = qobject_cast<ITabPage *>(ui.twtTabs->widget(0));
		if (page)
			removeTabPage(page);
		else
			ui.twtTabs->removeTab(0);
	}
}

void TabWindow::updateTab(int AIndex)
{
	ITabPage *page = tabPage(AIndex);
	if (page)
	{
		QString tabText;
		if (FShowIndices->isChecked() && AIndex<10)
			tabText = tr("%1) %2").arg(QString::number((AIndex+1) % 10)).arg(page->tabPageCaption());
		else
			tabText = page->tabPageCaption();
		ui.twtTabs->setTabText(AIndex,tabText);
		ui.twtTabs->setTabIcon(AIndex,page->tabPageIcon());
		ui.twtTabs->setTabToolTip(AIndex,page->tabPageToolTip());
	}
}

void TabWindow::updateTabs(int AFrom, int ATo)
{
	for (int tab=AFrom; tab<=ATo; tab++)
		updateTab(tab);
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
	removeTabPage(qobject_cast<ITabPage *>(ui.twtTabs->widget(AIndex)));
}

void TabWindow::onTabPageShow()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
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
	removeTabPage(qobject_cast<ITabPage *>(sender()));
}

void TabWindow::onTabPageChanged()
{
	ITabPage *page = qobject_cast<ITabPage *>(sender());
	if (page)
	{
		int index = ui.twtTabs->indexOf(page->instance());
		updateTab(index);
		if (index == ui.twtTabs->currentIndex())
			updateWindow();
	}
}

void TabWindow::onTabPageDestroyed()
{
	removeTabPage(qobject_cast<ITabPage *>(sender()));
}

void TabWindow::onTabWindowAppended(const QUuid &AWindowId, const QString &AName)
{
	Action *action = new Action(FJoinMenu);
	action->setText(AName);
	action->setData(ADR_TABWINDOWID,AWindowId.toString());
	FJoinMenu->addAction(action,AG_DEFAULT);
	connect(action,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
}

void TabWindow::onTabWindowNameChanged(const QUuid &AWindowId, const QString &AName)
{
	if (AWindowId == FWindowId)
		updateWindow();

	foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
		if (AWindowId == action->data(ADR_TABWINDOWID).toString())
			action->setText(AName);
}

void TabWindow::onTabWindowDeleted(const QUuid &AWindowId)
{
	foreach(Action *action, FJoinMenu->groupActions(AG_DEFAULT))
		if (AWindowId == action->data(ADR_TABWINDOWID).toString())
			FJoinMenu->removeAction(action);
}

void TabWindow::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MESSAGES_TABWINDOWS_DEFAULT)
	{
		FSetAsDefault->setChecked(FWindowId==ANode.value().toString());
		FDeleteWindow->setVisible(!FSetAsDefault->isChecked());
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
	if (action == FCloseTab)
	{
		removeTabPage(currentTabPage());
	}
	else if (action == FNextTab)
	{
		ui.twtTabs->setCurrentIndex((ui.twtTabs->currentIndex()+1) % ui.twtTabs->count());
	}
	else if (action == FPrevTab)
	{
		ui.twtTabs->setCurrentIndex(ui.twtTabs->currentIndex()>0 ? ui.twtTabs->currentIndex()-1 : ui.twtTabs->count()-1);
	}
	else if (action == FDetachWindow)
	{
		detachTabPage(currentTabPage());
	}
	else if (action == FNewTab)
	{
		QString name = QInputDialog::getText(this,tr("New Tab Window"),tr("Tab window name:"));
		if (!name.isEmpty())
		{
			ITabPage *page = currentTabPage();
			removeTabPage(page);
			ITabWindow *window = FMessageWidgets->openTabWindow(FMessageWidgets->appendTabWindow(name));
			window->addTabPage(page);
		}
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
	else if (FJoinMenu->groupActions(AG_DEFAULT).contains(action))
	{
		ITabPage *page = currentTabPage();
		removeTabPage(page);

		ITabWindow *window = FMessageWidgets->openTabWindow(action->data(ADR_TABWINDOWID).toString());
		window->addTabPage(page);
	}
}
