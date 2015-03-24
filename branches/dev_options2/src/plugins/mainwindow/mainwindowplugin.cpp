#include "mainwindowplugin.h"

#include <QApplication>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <utils/widgetmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/action.h>

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FTrayManager = NULL;

	FStartShowLoopCount = 0;

	FActivationChanged = QTime::currentTime();
	FMainWindow = new MainWindow(NULL, Qt::Window|Qt::WindowTitleHint);
	FMainWindow->installEventFilter(this);
	WidgetManager::setWindowSticky(FMainWindow,true);
}

MainWindowPlugin::~MainWindowPlugin()
{
	delete FMainWindow;
}

void MainWindowPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Main Window");
	APluginInfo->description = tr("Allows other modules to place their widgets in the main window");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	FPluginManager = APluginManager;
	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onApplicationShutdownStarted()));

	IPlugin *plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));
	
	return true;
}

bool MainWindowPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_GLOBAL_SHOWROSTER,tr("Show roster"),QKeySequence::UnknownKey,Shortcuts::GlobalShortcut);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_CLOSEWINDOW,QString::null,tr("Esc","Close main window"));
	
	Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_CLOSEWINDOW,FMainWindow);

	Action *quitAction = new Action(this);
	quitAction->setText(tr("Quit"));
	quitAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	connect(quitAction,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FMainWindow->mainMenu()->addAction(quitAction,AG_MMENU_MAINWINDOW,true);

	if (FTrayManager)
	{
		Action *showRosterAction = new Action(this);
		showRosterAction->setText(tr("Show roster"));
		showRosterAction->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_SHOW_ROSTER);
		connect(showRosterAction,SIGNAL(triggered(bool)),SLOT(onShowMainWindowByAction(bool)));
		FTrayManager->contextMenu()->addAction(showRosterAction,AG_TMTM_MAINWINDOW,true);
	}

	return true;
}

bool MainWindowPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MAINWINDOW_SHOWONSTART,true);
	return true;
}

bool MainWindowPlugin::startPlugin()
{
	Shortcuts::setGlobalShortcut(SCT_GLOBAL_SHOWROSTER,true);
	return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

bool MainWindowPlugin::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched==FMainWindow && AEvent->type()==QEvent::ActivationChange)
		FActivationChanged = QTime::currentTime();
	return QObject::eventFilter(AWatched,AEvent);
}

void MainWindowPlugin::onOptionsOpened()
{
	FMainWindow->loadWindowGeometryAndState();
	QTimer::singleShot(0,this,SLOT(onShowMainWindowOnStart()));
}

void MainWindowPlugin::onOptionsClosed()
{
	FMainWindow->saveWindowGeometryAndState();
	FMainWindow->closeWindow();
}

void MainWindowPlugin::onApplicationShutdownStarted()
{
	Options::node(OPV_MAINWINDOW_SHOWONSTART).setValue(FMainWindow->isVisible());
}

void MainWindowPlugin::onShowMainWindowOnStart()
{
	if (FStartShowLoopCount >= 3)
	{
		if (Options::node(OPV_MAINWINDOW_SHOWONSTART).value().toBool())
			FMainWindow->showWindow();
		FStartShowLoopCount = 0;
	}
	else
	{
		FStartShowLoopCount++;
		QTimer::singleShot(0,this,SLOT(onShowMainWindowOnStart()));
	}
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	FMainWindow->showWindow();
}

void MainWindowPlugin::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AWidget==NULL && AId==SCT_GLOBAL_SHOWROSTER)
	{
		FMainWindow->showWindow();
	}
	else if (AWidget==FMainWindow && AId==SCT_ROSTERVIEW_CLOSEWINDOW)
	{
		FMainWindow->closeWindow();
	}
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId<=0 && AReason==QSystemTrayIcon::Trigger)
	{
		if (FMainWindow->isActive() || qAbs(FActivationChanged.msecsTo(QTime::currentTime())) < qApp->doubleClickInterval())
			FMainWindow->closeWindow();
		else
			FMainWindow->showWindow();
	}
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
