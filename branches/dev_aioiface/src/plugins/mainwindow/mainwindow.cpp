#include "mainwindow.h"

#include <QResizeEvent>
#include <QApplication>

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags) : QMainWindow(AParent,AFlags)
{
	setWindowRole("MainWindow");
	setAttribute(Qt::WA_DeleteOnClose,false);
	setIconSize(QSize(16,16));

	FLeftFrameWidth = 0;

	QIcon icon;
	IconStorage *iconStorage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO16), QSize(16,16));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO24), QSize(24,24));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO32), QSize(32,32));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO48), QSize(48,48));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO64), QSize(64,64));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO96), QSize(96,96));
	icon.addFile(iconStorage->fileFullName(MNI_MAINWINDOW_LOGO128), QSize(128,128));
	setWindowIcon(icon);

	FSplitter = new QSplitter(this);
	FSplitter->installEventFilter(this);
	FSplitter->setOrientation(Qt::Horizontal);
	connect(FSplitter,SIGNAL(splitterMoved(int,int)),SLOT(onSplitterMoved(int,int)));
	setCentralWidget(FSplitter);

	FLeftFrame = new QFrame(this);
	FSplitter->addWidget(FLeftFrame);
	FSplitter->setCollapsible(0,false);
	FSplitter->setStretchFactor(0,1);

	FRightFrame = new QFrame(this);
	FSplitter->addWidget(FRightFrame);
	FSplitter->setCollapsible(1,false);
	FSplitter->setStretchFactor(1,4);

	FLeftLayout = new QVBoxLayout(FLeftFrame);
	FLeftLayout->setMargin(0);
	FLeftLayout->setSpacing(0);

	FTabWidget = new TabWidget(FLeftFrame);
	FTabWidget->setMovable(false);
	FTabWidget->setDocumentMode(true);
	FTabWidget->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	insertWidget(MWW_TABPAGES_WIDGET,FTabWidget,10);

	QToolBar *topToolbar = new QToolBar(this);
	topToolbar->setFloatable(false);
	topToolbar->setMovable(false);
	ToolBarChanger *topChanger = new ToolBarChanger(topToolbar);
	topChanger->setSeparatorsVisible(false);
	insertToolBarChanger(MWW_TOP_TOOLBAR,topChanger);

	QToolBar *bottomToolbar =  new QToolBar(this);
	bottomToolbar->setFloatable(false);
	bottomToolbar->setMovable(false);
	ToolBarChanger *bottomChanger = new ToolBarChanger(bottomToolbar);
	bottomChanger->setSeparatorsVisible(false);
	insertToolBarChanger(MWW_BOTTOM_TOOLBAR,bottomChanger);

	FMainMenu = new Menu(this);
	FMainMenu->setTitle(tr("Menu"));
	FMainMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_MENU);
	QToolButton *button = bottomToolBarChanger()->insertAction(FMainMenu->menuAction());
	button->setPopupMode(QToolButton::InstantPopup);

	FMainMenuBar = new MenuBarChanger(new QMenuBar());
	setMenuBar(FMainMenuBar->menuBar());
}

MainWindow::~MainWindow()
{
	delete FMainMenuBar->menuBar();
}

bool MainWindow::isActive() const
{
	return isVisible() && WidgetManager::isActiveWindow(this);
}

Menu *MainWindow::mainMenu() const
{
	return FMainMenu;
}

MenuBarChanger *MainWindow::mainMenuBar() const
{
	return FMainMenuBar;
}

QList<QWidget *> MainWindow::widgets() const
{
	return FWidgetOrders.values();
}

int MainWindow::widgetOrder(QWidget *AWidget) const
{
	return FWidgetOrders.key(AWidget);
}

QWidget *MainWindow::widgetByOrder(int AOrderId) const
{
	return FWidgetOrders.value(AOrderId);
}

void MainWindow::insertWidget(int AOrderId, QWidget *AWidget, int AStretch)
{
	if (!FWidgetOrders.contains(AOrderId))
	{
		removeWidget(AWidget);
		QMap<int, QWidget *>::const_iterator it = FWidgetOrders.lowerBound(AOrderId);
		int index = it!=FWidgetOrders.constEnd() ? FLeftLayout->indexOf(it.value()) : -1;
		FLeftLayout->insertWidget(index,AWidget,AStretch);
		FWidgetOrders.insert(AOrderId,AWidget);
		emit widgetInserted(AOrderId,AWidget);
	}
}

void MainWindow::removeWidget(QWidget *AWidget)
{
	if (widgets().contains(AWidget))
	{
		FLeftLayout->removeWidget(AWidget);
		FWidgetOrders.remove(widgetOrder(AWidget));
		emit widgetRemoved(AWidget);
	}
}

QList<QWidget *> MainWindow::tabPages() const
{
	return FTabPageOrders.values();
}

int MainWindow::tabPageOrder(QWidget *APage) const
{
	return FTabPageOrders.key(APage);
}

QWidget *MainWindow::tabPageByOrder(int AOrderId) const
{
	return FTabPageOrders.value(AOrderId);
}

QWidget *MainWindow::currentTabPage() const
{
	return FTabWidget->currentWidget();
}

void MainWindow::setCurrentTabPage(QWidget *APage)
{
	FTabWidget->setCurrentWidget(APage);
}

void MainWindow::insertTabPage(int AOrderId, QWidget *APage)
{
	if (!FTabPageOrders.contains(AOrderId))
	{
		removeTabPage(APage);
		QMap<int, QWidget *>::const_iterator it = FTabPageOrders.lowerBound(AOrderId);
		int index = it!=FTabPageOrders.constEnd() ? FTabWidget->indexOf(it.value()) : -1;
		FTabWidget->insertTab(index,APage,APage->windowIcon(),APage->windowIconText());
		FTabPageOrders.insert(AOrderId,APage);
		updateTabPage(APage);
		emit tabPageInserted(AOrderId,APage);
	}
}

void MainWindow::updateTabPage(QWidget *APage)
{
	int index = FTabWidget->indexOf(APage);
	if (index >= 0)
	{
		FTabWidget->setTabIcon(index,APage->windowIcon());
		FTabWidget->setTabText(index,APage->windowIconText());
	}
}

void MainWindow::removeTabPage(QWidget *APage)
{
	if (tabPages().contains(APage))
	{
		FTabWidget->removeTab(FTabWidget->indexOf(APage));
		FTabPageOrders.remove(tabPageOrder(APage));
		emit tabPageRemoved(APage);
	}
}

ToolBarChanger *MainWindow::topToolBarChanger() const
{
	return toolBarChangerByOrder(MWW_TOP_TOOLBAR);
}

ToolBarChanger *MainWindow::bottomToolBarChanger() const
{
	return toolBarChangerByOrder(MWW_BOTTOM_TOOLBAR);
}

QList<ToolBarChanger *> MainWindow::toolBarChangers() const
{
	return FToolBarOrders.values();
}

int MainWindow::toolBarChangerOrder(ToolBarChanger *AChanger) const
{
	return FToolBarOrders.key(AChanger);
}

ToolBarChanger *MainWindow::toolBarChangerByOrder(int AOrderId) const
{
	return FToolBarOrders.value(AOrderId);
}

void MainWindow::insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger)
{
	if (!FWidgetOrders.contains(AOrderId))
	{
		AChanger->toolBar()->setIconSize(iconSize());
		insertWidget(AOrderId,AChanger->toolBar());
		FToolBarOrders.insert(AOrderId,AChanger);
		emit toolBarChangerInserted(AOrderId,AChanger);
	}
}

void MainWindow::removeToolBarChanger(ToolBarChanger *AChanger)
{
	if (toolBarChangers().contains(AChanger))
	{
		removeWidget(AChanger->toolBar());
		FToolBarOrders.remove(toolBarChangerOrder(AChanger));
		emit toolBarChangerRemoved(AChanger);
	}
}

void MainWindow::saveWindowState()
{
	if (FLeftFrameWidth > 0)
		Options::setFileValue(FLeftFrameWidth,"mainwindow.left-frame-width");
}

void MainWindow::loadWindowState()
{
	FLeftFrameWidth = Options::fileValue("mainwindow.left-frame-width").toInt();
}

QMenu *MainWindow::createPopupMenu()
{
	return NULL;
}

void MainWindow::showEvent(QShowEvent *AEvent)
{
	QMainWindow::showEvent(AEvent);

	QList<int> splitterSizes = FSplitter->sizes();
	int leftIndex = FSplitter->indexOf(FLeftFrame);
	int rightIndex = FSplitter->indexOf(FRightFrame);
	if (FLeftFrameWidth>0 && leftIndex>=0 && rightIndex>=0 && splitterSizes.value(leftIndex)!=FLeftFrameWidth)
	{
		splitterSizes[rightIndex] += splitterSizes.value(leftIndex) - FLeftFrameWidth;
		splitterSizes[leftIndex] = FLeftFrameWidth;
		FSplitter->setSizes(splitterSizes);
	}
}

bool MainWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject == FSplitter)
	{
		if (AEvent->type() == QEvent::Resize)
		{
			int leftIndex = FSplitter->indexOf(FLeftFrame);
			int rightIndex = FSplitter->indexOf(FRightFrame);
			QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(AEvent);
			if (resizeEvent && FLeftFrameWidth>0 && leftIndex>=0 && rightIndex>=0 && resizeEvent->oldSize().width()>0)
			{
				double k = (double)resizeEvent->size().width() / resizeEvent->oldSize().width();
				QList<int> splitterSizes = FSplitter->sizes();
				for (int i=0; i<splitterSizes.count(); i++)
					splitterSizes[i] = qRound(splitterSizes[i]*k);
				if (splitterSizes.value(leftIndex) != FLeftFrameWidth)
				{
					splitterSizes[rightIndex] += splitterSizes.value(leftIndex) - FLeftFrameWidth;
					splitterSizes[leftIndex] = FLeftFrameWidth;
					FSplitter->setSizes(splitterSizes);
				}
			}
		}
	}
	return QMainWindow::eventFilter(AObject,AEvent);
}

void MainWindow::onSplitterMoved(int APos, int AIndex)
{
	Q_UNUSED(APos);
	Q_UNUSED(AIndex);
	FLeftFrameWidth = FSplitter->sizes().value(FSplitter->indexOf(FLeftFrame));
}
