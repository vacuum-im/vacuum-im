#include "mainwindow.h"

#include <QTimer>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QStringList>
#include <definitions/version.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/mainwindowwidgets.h>
#include <utils/widgetmanager.h>
#include <utils/options.h>

#define ONE_WINDOW_MODE_OPTIONS_NS "one-window-mode"
#define DEFAULT_LEFT_FRAME_WIDTH 200

MainWindow::MainWindow(QWidget *AParent, Qt::WindowFlags AFlags) : QMainWindow(AParent,AFlags)
{
	setWindowRole("MainWindow");
	setAttribute(Qt::WA_DeleteOnClose,false);
	setIconSize(QSize(16,16));

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
	connect(FSplitter, &QSplitter::splitterMoved, this, &MainWindow::onSplitterMoved);
	setCentralWidget(FSplitter);

	FLeftWidget = new BoxWidget(this);
	FLeftWidget->layout()->setSpacing(0);

	FSplitter->addWidget(FLeftWidget);
	FSplitter->setCollapsible(0,false);
	FSplitter->setStretchFactor(0,1);

	MainCentralWidget *centralWidget = new MainCentralWidget(this,this);
	FCentralWidget = centralWidget;
	connect(centralWidget, &MainCentralWidget::currentCentralPageChanged, this, &MainWindow::onCurrentCentralPageChanged);
	connect(centralWidget, &MainCentralWidget::centralPageAppended, this, &MainWindow::onCentralPageAddedOrRemoved);
	connect(centralWidget, &MainCentralWidget::centralPageRemoved, this, &MainWindow::onCentralPageAddedOrRemoved);
	centralWidget->setFrameShape(QFrame::StyledPanel);

	FSplitter->addWidget(centralWidget);
	FSplitter->setCollapsible(1,false);
	FSplitter->setStretchFactor(1,4);

	FSplitter->setHandleWidth(0);
	centralWidget->setVisible(false);

	FTabWidget = new MainTabWidget(FLeftWidget);
	FTabWidget->instance()->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	FLeftWidget->insertWidget(MWW_TABPAGES_WIDGET,FTabWidget->instance(),100);

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
		// Restore geometry before the first show: the window manager must map
		// the window once with its final size, otherwise it may briefly appear
		// at the default size and occasionally keep it (X11 WM race).
		if (!FGeometryLoaded)
		{
			const bool shouldBeVisible = !FCentralWidget->centralPages().isEmpty();
			if (shouldBeVisible != FCentralVisible)
				setCentralWidgetVisible(shouldBeVisible);
			if (!FGeometryLoaded)
				loadWindowGeometryAndState();
		}

		if (!AMinimized)
			WidgetManager::showActivateRaiseWindow(this);
		else if (!isVisible())
			showMinimized();

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

BoxWidget *MainWindow::mainLeftWidget() const
{
	return FLeftWidget;
}

IMainTabWidget *MainWindow::mainTabWidget() const
{
	return FTabWidget;
}

bool MainWindow::isCentralWidgetVisible() const
{
	return FCentralVisible;
}

IMainCentralWidget * MainWindow::mainCentralWidget() const
{
	return FCentralWidget;
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
	return FToolBarOrders.key(AChanger,-1);
}

ToolBarChanger *MainWindow::toolBarChangerByOrder(int AOrderId) const
{
	return FToolBarOrders.value(AOrderId);
}

void MainWindow::insertToolBarChanger(int AOrderId, ToolBarChanger *AChanger)
{
	if (FLeftWidget->widgetByOrder(AOrderId)==NULL && !toolBarChangers().contains(AChanger))
	{
		AChanger->toolBar()->setIconSize(iconSize());
		FLeftWidget->insertWidget(AOrderId,AChanger->toolBar());
		FToolBarOrders.insert(AOrderId,AChanger);
		emit toolBarChangerInserted(AOrderId,AChanger);
	}
}

void MainWindow::removeToolBarChanger(ToolBarChanger *AChanger)
{
	if (toolBarChangers().contains(AChanger))
	{
		FLeftWidget->removeWidget(AChanger->toolBar());
		FToolBarOrders.remove(toolBarChangerOrder(AChanger));
		emit toolBarChangerRemoved(AChanger);
	}
}

void MainWindow::saveWindowGeometryAndState()
{
	// Never persist geometry of a window that has not had it loaded yet
	// (e.g. hidden in tray since startup): that would save the default size.
	if (!FGeometryLoaded)
		return;

	QString ns = geometryOptionsNamespace();
	if (isCentralWidgetVisible() && FLeftWidgetWidth>0)
		Options::setFileValue(FLeftWidgetWidth,"mainwindow.left-frame-width",ns);
	Options::setFileValue(saveGeometry(),"mainwindow.geometry",ns);
	Options::setFileValue((int)WidgetManager::windowAlignment(this),"mainwindow.align",ns);
}

void MainWindow::loadWindowGeometryAndState()
{
	FGeometryLoaded = true;
	FAligned = false;
	QString ns = geometryOptionsNamespace();
	if (!restoreGeometry(Options::fileValue("mainwindow.geometry",ns).toByteArray()))
	{
		if (isCentralWidgetVisible())
		{
			FLeftWidgetWidth = Options::fileValue("mainwindow.left-frame-width",ns).toInt();
			if (FLeftWidgetWidth <= 0)
				FLeftWidgetWidth = DEFAULT_LEFT_FRAME_WIDTH;
			Options::setFileValue(0,"mainwindow.align",ns);
			setGeometry(WidgetManager::alignGeometry(QSize(800,600),this,Qt::AlignCenter));
		}
		else
		{
			Options::setFileValue((int)(Qt::AlignRight|Qt::AlignBottom),"mainwindow.align",ns);
			setGeometry(WidgetManager::alignGeometry(QSize(200,600),this,Qt::AlignRight|Qt::AlignBottom));
		}
	}
	else if (isCentralWidgetVisible())
	{
		FLeftWidgetWidth = Options::fileValue("mainwindow.left-frame-width",ns).toInt();
		if (FLeftWidgetWidth <= 0)
			FLeftWidgetWidth = DEFAULT_LEFT_FRAME_WIDTH;
	}
}

void MainWindow::updateWindowFlags(bool AVisible)
{
	if (AVisible)
		setWindowFlags(Qt::Window);
	else
	{
		// Window Flags button hints works incorrectly under Gnome, especially under Gnome X11.
		// XDG_CURRENT_DESKTOP is a colon-separated list of DEs, e.g. "ubuntu:GNOME".
		QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP").split(QLatin1Char(':'), QString::SkipEmptyParts);
		bool isGnome = false;
		foreach (const QString &desktop, desktops)
			isGnome = isGnome || desktop.startsWith("GNOME", Qt::CaseInsensitive);
		if (!isGnome)
			setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
	}
}

void MainWindow::updateWindow()
{
	IMainCentralPage *page = isCentralWidgetVisible() ? mainCentralWidget()->currentCentralPage() : NULL;
	if (page && !page->centralPageCaption().isEmpty())
		setWindowTitle(QString(CLIENT_NAME" - %1").arg(page->centralPageCaption()));
	else
		setWindowTitle(CLIENT_NAME);
}

QMenu *MainWindow::createPopupMenu()
{
	return NULL;
}

QString MainWindow::geometryOptionsNamespace() const
{
	return isCentralWidgetVisible() ? QString(ONE_WINDOW_MODE_OPTIONS_NS) : QString();
}

void MainWindow::correctWindowPosition()
{
	if (!FGeometryLoaded)
		loadWindowGeometryAndState();

	if (!FAligned)
	{
		FAligned = true;
		QString ns = geometryOptionsNamespace();
		WidgetManager::alignWindow(this,(Qt::Alignment)Options::fileValue("mainwindow.align",ns).toInt());
	}

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

void MainWindow::restoreAcceptDrops(QWidget *AParent)
{
#ifdef Q_OS_WIN
	foreach(QObject *object, AParent->children())
	{
		if (object->isWidgetType())
		{
			QWidget *childWidget = qobject_cast<QWidget *>(object);
			if (childWidget->acceptDrops())
			{
				childWidget->setAcceptDrops(false);
				childWidget->setAcceptDrops(true);
			}
			restoreAcceptDrops(childWidget);
		}
	}
#else
	Q_UNUSED(AParent);
#endif
}

void MainWindow::setCentralWidgetVisible(bool AVisible)
{
	if (AVisible != FCentralVisible)
	{
		bool windowVisible = isVisible();
		if (FGeometryLoaded)
			saveWindowGeometryAndState();
		hide();

		FAligned = false;
		FGeometryLoaded = false;
		FCentralVisible = AVisible;
		if (AVisible)
		{
			FSplitter->setHandleWidth(FSplitterHandleWidth);
			FLeftWidget->setFrameShape(QFrame::StyledPanel);
			FCentralWidget->instance()->setVisible(true);
		}
		else
		{
			FSplitter->setHandleWidth(0);
			FLeftWidget->setFrameShape(QFrame::NoFrame);
			FCentralWidget->instance()->setVisible(false);
		}

		updateWindowFlags(AVisible);
		updateWindow();
		setMinimumSize(0,0);
		restoreAcceptDrops(this);

		if (windowVisible)
		{
			WidgetManager::showActivateRaiseWindow(this);
			correctWindowPosition();
		}

		emit centralWidgetVisibleChanged(AVisible);
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

void MainWindow::closeEvent(QCloseEvent *AEvent)
{
	if (!Options::isNull())
	{
		if (Options::node(OPV_ROSTER_MINIMIZEONCLOSE).value().toBool() && AEvent->spontaneous())
		{
			AEvent->ignore();
			setWindowState(windowState() | Qt::WindowMinimized);
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
			if (FLeftWidgetWidth>0 && leftIndex>=0 && rightIndex>=0 && resizeEvent->oldSize().width()>0)
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

void MainWindow::onUpdateCentralWidgetVisible()
{
	setCentralWidgetVisible(!FCentralWidget->centralPages().isEmpty());
}

void MainWindow::onCurrentCentralPageChanged()
{
	updateWindow();
}

void MainWindow::onCentralPageAddedOrRemoved(IMainCentralPage *APage)
{
	Q_UNUSED(APage);
	QTimer::singleShot(0,this,&MainWindow::onUpdateCentralWidgetVisible);
}

void MainWindow::onSplitterMoved(int APos, int AIndex)
{
	Q_UNUSED(APos); Q_UNUSED(AIndex);
	FLeftWidgetWidth = FSplitter->sizes().value(FSplitter->indexOf(FLeftWidget));
}
