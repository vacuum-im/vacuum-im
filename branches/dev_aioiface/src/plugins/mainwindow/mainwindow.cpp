#include "mainwindow.h"

#include <QResizeEvent>
#include <QApplication>
#include <QDesktopWidget>

#define ONE_WINDOW_MODE_OPTIONS_NS "one-window-mode"

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags) : QMainWindow(AParent,AFlags)
{
	setWindowRole("MainWindow");
	setAttribute(Qt::WA_DeleteOnClose,false);
	setIconSize(QSize(16,16));

	FAligned = false;
	FLeftWidgetWidth = 0;
	FCentralVisible = true;

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
	FSplitterHandleWidth = FSplitter->handleWidth();
	connect(FSplitter,SIGNAL(splitterMoved(int,int)),SLOT(onSplitterMoved(int,int)));
	setCentralWidget(FSplitter);

	FLeftWidget = new QFrame(this);
	FLeftWidget->setFrameShape(QFrame::StyledPanel);
	FSplitter->addWidget(FLeftWidget);
	FSplitter->setCollapsible(0,false);
	FSplitter->setStretchFactor(0,1);

	FCentralWidget = new MainCentralWidget(this,this);
	FCentralWidget->instance()->setFrameShape(QFrame::StyledPanel);
	connect(FCentralWidget->instance(),SIGNAL(currentCentralPageChanged()),SLOT(onCurrentCentralPageChanged()));

	FSplitter->addWidget(FCentralWidget->instance());
	FSplitter->setCollapsible(1,false);
	FSplitter->setStretchFactor(1,4);

	FLeftLayout = new QVBoxLayout(FLeftWidget);
	FLeftLayout->setMargin(0);
	FLeftLayout->setSpacing(0);

	FTabWidget = new MainTabWidget(FLeftWidget);
	FTabWidget->instance()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	insertWidget(MWW_TABPAGES_WIDGET,FTabWidget->instance(),100);

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

	updateWindow();
}

MainWindow::~MainWindow()
{
	delete FMainMenuBar->menuBar();
}

bool MainWindow::isActive() const
{
	return WidgetManager::isActiveWindow(this);
}

void MainWindow::showWindow(bool AMinimized)
{
	if (!Options::isNull())
	{
		if (!AMinimized)
			WidgetManager::showActivateRaiseWindow(this);
		else if (!isVisible())
			showMinimized();

		if (!FAligned)
		{
			FAligned = true;
			QString ns = isCentralWidgetVisible() ? ONE_WINDOW_MODE_OPTIONS_NS : "";
			WidgetManager::alignWindow(this,(Qt::Alignment)Options::fileValue("mainwindow.align",ns).toInt());
		}

		correctWindowPosition();
	}
}

void MainWindow::closeWindow()
{
	close();
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

IMainTabWidget *MainWindow::mainTabWidget() const
{
	return FTabWidget;
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

IMainCentralWidget * MainWindow::mainCentralWidget() const
{
	return FCentralWidget;
}

bool MainWindow::isCentralWidgetVisible() const
{
	return FCentralVisible;
}

void MainWindow::setCentralWidgetVisible(bool AEnabled)
{
	if (AEnabled != FCentralVisible)
	{
		bool wasVisible = isVisible();
		saveWindowGeometryAndState();
		
		FCentralVisible = AEnabled;
		if (AEnabled)
		{
			FSplitter->setHandleWidth(FSplitterHandleWidth);
			FLeftWidget->setFrameShape(QFrame::StyledPanel);
			FCentralWidget->instance()->setVisible(true);
			setWindowFlags(Qt::Window);
		}
		else
		{
			FSplitter->setHandleWidth(0);
			FLeftWidget->setFrameShape(QFrame::NoFrame);
			FCentralWidget->instance()->setVisible(false);
			while (FCentralWidget->currentCentralPage()!=NULL)
				FCentralWidget->removeCentralPage(FCentralWidget->currentCentralPage());
			setWindowFlags(Qt::Window | Qt::WindowTitleHint);
		}

		updateWindow();
		loadWindowGeometryAndState();
		if (wasVisible)
			showWindow();
		Options::node(OPV_MAINWINDOW_CENTRALVISIBLE).setValue(AEnabled);

		emit centralWidgetVisibleChanged(AEnabled);
	}
}

void MainWindow::saveWindowGeometryAndState()
{
	QString ns = isCentralWidgetVisible() ? ONE_WINDOW_MODE_OPTIONS_NS : "";
	if (isCentralWidgetVisible() && FLeftWidgetWidth>0)
		Options::setFileValue(FLeftWidgetWidth,"mainwindow.left-frame-width",ns);
	Options::setFileValue(saveGeometry(),"mainwindow.geometry",ns);
	Options::setFileValue((int)WidgetManager::windowAlignment(this),"mainwindow.align",ns);
}

void MainWindow::loadWindowGeometryAndState()
{
	FAligned = false;
	QString ns = isCentralWidgetVisible() ? ONE_WINDOW_MODE_OPTIONS_NS : "";
	if (!restoreGeometry(Options::fileValue("mainwindow.geometry",ns).toByteArray()))
	{
		if (isCentralWidgetVisible())
		{
			FLeftWidgetWidth = 200;
			Options::setFileValue(0,"mainwindow.align",ns);
			setGeometry(WidgetManager::alignGeometry(QSize(640,480),this,Qt::AlignCenter));
		}
		else
		{
			Options::setFileValue((int)(Qt::AlignRight|Qt::AlignBottom),"mainwindow.align",ns);
			setGeometry(WidgetManager::alignGeometry(QSize(200,500),this,Qt::AlignRight|Qt::AlignBottom));
		}
	}
	else if (isCentralWidgetVisible())
	{
		FLeftWidgetWidth = Options::fileValue("mainwindow.left-frame-width",ns).toInt();
	}
}

void MainWindow::updateWindow()
{
	IMainCentralPage *page = isCentralWidgetVisible() ? mainCentralWidget()->currentCentralPage() : NULL;
	if (page)
		setWindowTitle(QString(CLIENT_NAME" - %1").arg(page->centralPageCaption()));
	else
		setWindowTitle(CLIENT_NAME);
}

QMenu *MainWindow::createPopupMenu()
{
	return NULL;
}

void MainWindow::correctWindowPosition()
{
	QRect windowRect = geometry();
	QRect screenRect = qApp->desktop()->availableGeometry(this);
	if (!screenRect.isEmpty() && !windowRect.isEmpty())
	{
		Qt::Alignment align = 0;
		if (windowRect.right() <= screenRect.left())
			align |= Qt::AlignLeft;
		else if (windowRect.left() >= screenRect.right())
			align |= Qt::AlignRight;
		if (windowRect.top() >= screenRect.bottom())
			align |= Qt::AlignBottom;
		else if (windowRect.bottom() <= screenRect.top())
			align |= Qt::AlignTop;
		WidgetManager::alignWindow(this,align);
	}
}

void MainWindow::showEvent(QShowEvent *AEvent)
{
	QMainWindow::showEvent(AEvent);
	if (isCentralWidgetVisible())
	{
		QList<int> splitterSizes = FSplitter->sizes();
		int leftIndex = FSplitter->indexOf(FLeftWidget);
		int rightIndex = FSplitter->indexOf(FCentralWidget->instance());
		if (FLeftWidgetWidth>0 && leftIndex>=0 && rightIndex>=0 && splitterSizes.value(leftIndex)!=FLeftWidgetWidth)
		{
			splitterSizes[rightIndex] += splitterSizes.value(leftIndex) - FLeftWidgetWidth;
			splitterSizes[leftIndex] = FLeftWidgetWidth;
			FSplitter->setSizes(splitterSizes);
		}
	}
}

bool MainWindow::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AObject == FSplitter)
	{
		if (isCentralWidgetVisible() && AEvent->type()==QEvent::Resize)
		{
			int leftIndex = FSplitter->indexOf(FLeftWidget);
			int rightIndex = FSplitter->indexOf(FCentralWidget->instance());
			QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(AEvent);
			if (resizeEvent && FLeftWidgetWidth>0 && leftIndex>=0 && rightIndex>=0 && resizeEvent->oldSize().width()>0)
			{
				double k = (double)resizeEvent->size().width() / resizeEvent->oldSize().width();
				QList<int> splitterSizes = FSplitter->sizes();
				for (int i=0; i<splitterSizes.count(); i++)
					splitterSizes[i] = qRound(splitterSizes[i]*k);
				if (splitterSizes.value(leftIndex) != FLeftWidgetWidth)
				{
					splitterSizes[rightIndex] += splitterSizes.value(leftIndex) - FLeftWidgetWidth;
					splitterSizes[leftIndex] = FLeftWidgetWidth;
					FSplitter->setSizes(splitterSizes);
				}
			}
		}
	}
	return QMainWindow::eventFilter(AObject,AEvent);
}

void MainWindow::onCurrentCentralPageChanged()
{
	updateWindow();
}

void MainWindow::onSplitterMoved(int APos, int AIndex)
{
	Q_UNUSED(APos); Q_UNUSED(AIndex);
	FLeftWidgetWidth = FSplitter->sizes().value(FSplitter->indexOf(FLeftWidget));
}
