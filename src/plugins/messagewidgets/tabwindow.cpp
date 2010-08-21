#include "tabwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QSignalMapper>
#include <QShortcut>

#define ADR_TABWINDOWID             Action::DR_Parametr1

TabWindow::TabWindow(IMessageWidgets *AMessageWidgets, const QUuid &AWindowId)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);

	ui.twtTabs->widget(0)->deleteLater();
	ui.twtTabs->removeTab(0);
	ui.twtTabs->setMovable(true);

	QSignalMapper *tabMapper = new QSignalMapper(this);
	for (int digit=1; digit<=10; digit++)
	{
		QKeySequence key(QString("Alt+%1").arg(digit % 10));
		QShortcut *shortcut = new QShortcut(key, this);
		tabMapper->setMapping(shortcut, digit-1); // QTabWidget's indices are 0-based
		connect(shortcut, SIGNAL(activated()),
				tabMapper, SLOT(map()));
	}
	connect(tabMapper, SIGNAL(mapped(int)),
			ui.twtTabs, SLOT(setCurrentIndex(int)));

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

	initialize();
	loadWindowState();
	createActions();

	connect(ui.twtTabs,SIGNAL(currentChanged(int)),SLOT(onTabChanged(int)));
	connect(ui.twtTabs,SIGNAL(tabCloseRequested(int)),SLOT(onTabCloseRequested(int)));
	connect(ui.twtTabs,SIGNAL(tabMoved(int,int)),SLOT(onTabMoved(int,int)));
}

TabWindow::~TabWindow()
{
	clear();
	saveWindowState();
	emit windowDestroyed();
}

void TabWindow::showWindow()
{
	WidgetManager::showActivateRaiseWindow(this);
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

void TabWindow::addPage(ITabWindowPage *APage)
{
	if (ui.twtTabs->indexOf(APage->instance()) < 0)
	{
		int index = ui.twtTabs->addTab(APage->instance(),APage->instance()->windowTitle());
		connect(APage->instance(),SIGNAL(windowShow()),SLOT(onTabPageShow()));
		connect(APage->instance(),SIGNAL(windowClose()),SLOT(onTabPageClose()));
		connect(APage->instance(),SIGNAL(windowChanged()),SLOT(onTabPageChanged()));
		connect(APage->instance(),SIGNAL(windowDestroyed()),SLOT(onTabPageDestroyed()));
		updateTab(index);
		updateWindow();
		emit pageAdded(APage);
	}
}

bool TabWindow::hasPage(ITabWindowPage *APage) const
{
	return ui.twtTabs->indexOf(APage->instance()) >= 0;
}

ITabWindowPage *TabWindow::currentPage() const
{
	return qobject_cast<ITabWindowPage *>(ui.twtTabs->currentWidget());
}

void TabWindow::setCurrentPage(ITabWindowPage *APage)
{
	ui.twtTabs->setCurrentWidget(APage->instance());
}

void TabWindow::detachPage(ITabWindowPage *APage)
{
	removePage(APage);
	APage->instance()->show();
	if (APage->instance()->x()<=0 || APage->instance()->y()<0)
		APage->instance()->move(0,0);
	emit pageDetached(APage);
}

void TabWindow::removePage(ITabWindowPage *APage)
{
	int index = ui.twtTabs->indexOf(APage->instance());
	if (index >=0)
	{
		ui.twtTabs->removeTab(index);
		APage->instance()->close();
		APage->instance()->setParent(NULL);
		disconnect(APage->instance(),SIGNAL(windowShow()),this,SLOT(onTabPageShow()));
		disconnect(APage->instance(),SIGNAL(windowClose()),this,SLOT(onTabPageClose()));
		disconnect(APage->instance(),SIGNAL(windowChanged()),this,SLOT(onTabPageChanged()));
		disconnect(APage->instance(),SIGNAL(windowDestroyed()),this,SLOT(onTabPageDestroyed()));
		emit pageRemoved(APage);
		if (ui.twtTabs->count() == 0)
			close();
	}
}

void TabWindow::clear()
{
	while (ui.twtTabs->count() > 0)
	{
		ITabWindowPage *page = qobject_cast<ITabWindowPage *>(ui.twtTabs->widget(0));
		if (page)
			removePage(page);
		else
			ui.twtTabs->removeTab(0);
	}
}

void TabWindow::initialize()
{
	onOptionsChanged(Options::node(OPV_MESSAGES_TABWINDOWS_SHOW_INDICES));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),
			SLOT(onOptionsChanged(const OptionsNode &)));
}

void TabWindow::createActions()
{
	FNextTab = new Action(FWindowMenu);
	FNextTab->setText(tr("Next Tab"));
	FNextTab->setShortcut(tr("Ctrl+Tab"));
	FWindowMenu->addAction(FNextTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FNextTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FPrevTab = new Action(FWindowMenu);
	FPrevTab->setText(tr("Prev. Tab"));
	FPrevTab->setShortcut(tr("Ctrl+Shift+Tab"));
	FWindowMenu->addAction(FPrevTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FPrevTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FCloseTab = new Action(FWindowMenu);
	FCloseTab->setText(tr("Close Tab"));
	FCloseTab->setShortcut(tr("Esc"));
	FWindowMenu->addAction(FCloseTab,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FCloseTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FDetachWindow = new Action(FWindowMenu);
	FDetachWindow->setText(tr("Detach to Separate Window"));
	FWindowMenu->addAction(FDetachWindow,AG_MWTW_MWIDGETS_TAB_ACTIONS);
	connect(FDetachWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FJoinMenu = new Menu(FWindowMenu);
	FJoinMenu->setTitle(tr("Join to"));
	FWindowMenu->addAction(FJoinMenu->menuAction(),AG_MWTW_MWIDGETS_TAB_ACTIONS);
	foreach(QUuid windowId,FMessageWidgets->tabWindowList())
		if (windowId!=FWindowId)
			onTabWindowAppended(windowId, FMessageWidgets->tabWindowName(windowId));

	FNewTab = new Action(FJoinMenu);
	FNewTab->setText(tr("New Tab Window"));
	FJoinMenu->addAction(FNewTab,AG_DEFAULT+1);
	connect(FNewTab,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FShowCloseButtons = new Action(FWindowMenu);
	FShowCloseButtons->setText(tr("Tabs Closable"));
	FShowCloseButtons->setCheckable(true);
	FShowCloseButtons->setChecked(ui.twtTabs->tabsClosable());
	FWindowMenu->addAction(FShowCloseButtons,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FShowCloseButtons,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FTabsBottom = new Action(FWindowMenu);
	FTabsBottom->setText(tr("Show Tabs at Bootom of the Window"));
	FTabsBottom->setCheckable(true);
	FTabsBottom->setChecked(ui.twtTabs->tabPosition() == QTabWidget::South);
	FWindowMenu->addAction(FTabsBottom,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FTabsBottom,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FSetAsDefault = new Action(FWindowMenu);
	FSetAsDefault->setText(tr("Use as Default Tab Window"));
	FSetAsDefault->setCheckable(true);
	FWindowMenu->addAction(FSetAsDefault,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FSetAsDefault,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FRenameWindow = new Action(FWindowMenu);
	FRenameWindow->setText(tr("Rename Tab Window"));
	FWindowMenu->addAction(FRenameWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FRenameWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	FDeleteWindow = new Action(FWindowMenu);
	FDeleteWindow->setText(tr("Delete Tab Window"));
	FWindowMenu->addAction(FDeleteWindow,AG_MWTW_MWIDGETS_WINDOW_OPTIONS);
	connect(FDeleteWindow,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));

	onOptionsChanged(Options::node(OPV_MESSAGES_TABWINDOWS_DEFAULT));
}

void TabWindow::saveWindowState()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		Options::setFileValue(saveGeometry(),"messages.tabwindows.window.geometry",FWindowId.toString());
		Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).setValue(ui.twtTabs->tabsClosable(),"tabs-closable");
		Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).setValue(ui.twtTabs->tabPosition()==QTabWidget::South,"tabs-bottom");
	}
}

void TabWindow::loadWindowState()
{
	if (FMessageWidgets->tabWindowList().contains(FWindowId))
	{
		restoreGeometry(Options::fileValue("messages.tabwindows.window.geometry",FWindowId.toString()).toByteArray());
		ui.twtTabs->setTabsClosable(Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).value("tabs-closable").toBool());
		ui.twtTabs->setTabPosition(Options::node(OPV_MESSAGES_TABWINDOW_ITEM,FWindowId.toString()).value("tabs-bottom").toBool() ? QTabWidget::South : QTabWidget::North);
	}
}

void TabWindow::updateWindow()
{
	QWidget *widget = ui.twtTabs->currentWidget();
	if (widget)
	{
		setWindowIcon(widget->windowIcon());
		setWindowTitle(widget->windowTitle()+" - "+windowName());
		emit windowChanged();
	}
}

void TabWindow::updateTab(int AIndex)
{
	QWidget *widget = ui.twtTabs->widget(AIndex);
	if (widget)
	{
		ui.twtTabs->setTabIcon(AIndex,widget->windowIcon());
		QString tabText;
		if (FShowTabIndices)
			tabText = tr("%1. %2", "First is tab index, second is tab name")
					  .arg(QString::number(AIndex+1)).arg(widget->windowIconText());
		else
			tabText = widget->windowIconText();
		ui.twtTabs->setTabText(AIndex,tabText);
	}
}

void TabWindow::onTabMoved(int from, int to)
{
	if (!FShowTabIndices)
		return;

	int first = qMin(from, to);
	int last = qMax(from, to);
	for (int tab=first; tab<=last; tab++)
		updateTab(tab);
}

void TabWindow::onTabChanged(int /*AIndex*/)
{
	updateWindow();
	emit currentPageChanged(currentPage());
}

void TabWindow::onTabCloseRequested(int AIndex)
{
	ITabWindowPage *page = qobject_cast<ITabWindowPage *>(ui.twtTabs->widget(AIndex));
	if (page)
		removePage(page);
}

void TabWindow::onTabPageShow()
{
	ITabWindowPage *page = qobject_cast<ITabWindowPage *>(sender());
	if (page)
	{
		setCurrentPage(page);
		showWindow();
	}
}

void TabWindow::onTabPageClose()
{
	ITabWindowPage *page = qobject_cast<ITabWindowPage *>(sender());
	if (page)
		removePage(page);
}

void TabWindow::onTabPageChanged()
{
	ITabWindowPage *page = qobject_cast<ITabWindowPage *>(sender());
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
	ITabWindowPage *page = qobject_cast<ITabWindowPage *>(sender());
	if (page)
		removePage(page);
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
	else if (ANode.path() == OPV_MESSAGES_TABWINDOWS_SHOW_INDICES)
	{
		FShowTabIndices = ANode.value().toBool();
		for (int tab=0; tab<ui.twtTabs->count(); tab++)
			updateTab(tab);
	}
}

void TabWindow::onActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action == FCloseTab)
	{
		removePage(currentPage());
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
		detachPage(currentPage());
	}
	else if (action == FNewTab)
	{
		QString name = QInputDialog::getText(this,tr("New Tab Window"),tr("Tab window name:"));
		if (!name.isEmpty())
		{
			ITabWindowPage *page = currentPage();
			removePage(page);
			ITabWindow *window = FMessageWidgets->openTabWindow(FMessageWidgets->appendTabWindow(name));
			window->addPage(page);
		}
	}
	else if (action == FShowCloseButtons)
	{
		ui.twtTabs->setTabsClosable(action->isChecked());
	}
	else if (action == FTabsBottom)
	{
		ui.twtTabs->setTabPosition(action->isChecked() ? QTabWidget::South : QTabWidget::North);
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
		ITabWindowPage *page = currentPage();
		removePage(page);

		ITabWindow *window = FMessageWidgets->openTabWindow(action->data(ADR_TABWINDOWID).toString());
		window->addPage(page);
	}
}
