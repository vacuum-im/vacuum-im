#include "mainwindowplugin.h"

#include <QApplication>

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;

	FActivationChanged = QTime::currentTime();
	FMainWindow = new MainWindow(NULL,Qt::Window);
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

	IPlugin *plugin = FPluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
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
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	connect(FPluginManager->instance(),SIGNAL(shutdownStarted()),SLOT(onShutdownStarted()));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString, QWidget *)),SLOT(onShortcutActivated(const QString, QWidget *)));

	return true;
}

bool MainWindowPlugin::initObjects()
{
	Shortcuts::declareShortcut(SCT_GLOBAL_SHOWROSTER,tr("Show roster"),QKeySequence::UnknownKey,Shortcuts::GlobalShortcut);

	Shortcuts::declareGroup(SCTG_MAINWINDOW,tr("Main window"),SGO_MAINWINDOW);
	Shortcuts::declareShortcut(SCT_MAINWINDOW_CLOSEWINDOW,tr("Hide roster"),tr("Esc","Hide roster"));
	Shortcuts::declareShortcut(SCT_MAINWINDOW_CHANGECENTRALVISIBLE,tr("Combine/Split with message windows"),QKeySequence::UnknownKey);

	Shortcuts::insertWidgetShortcut(SCT_MAINWINDOW_CLOSEWINDOW,FMainWindow);
	Shortcuts::insertWidgetShortcut(SCT_MAINWINDOW_CHANGECENTRALVISIBLE,FMainWindow);

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
	Options::setDefaultValue(OPV_MAINWINDOW_CENTRALVISIBLE,true);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

bool MainWindowPlugin::startPlugin()
{
	Shortcuts::setGlobalShortcut(SCT_GLOBAL_SHOWROSTER,true);
	return true;
}

QMultiMap<int, IOptionsWidget *> MainWindowPlugin::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_ROSTER)
	{
		widgets.insertMulti(OWO_ROSTER_CENTRALVISIBLE, FOptionsManager->optionsNodeWidget(Options::node(OPV_MAINWINDOW_CENTRALVISIBLE),tr("Combine contact-list with message windows"),AParent));
	}
	return widgets;
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
	onOptionsChanged(Options::node(OPV_MAINWINDOW_CENTRALVISIBLE));
	if (Options::node(OPV_MAINWINDOW_SHOWONSTART).value().toBool())
		FMainWindow->showWindow();
}

void MainWindowPlugin::onOptionsClosed()
{
	FMainWindow->saveWindowGeometryAndState();
	FMainWindow->closeWindow();
}

void MainWindowPlugin::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MAINWINDOW_CENTRALVISIBLE)
	{
		FMainWindow->setCentralWidgetVisible(ANode.value().toBool());
	}
}

void MainWindowPlugin::onShutdownStarted()
{
	Options::node(OPV_MAINWINDOW_SHOWONSTART).setValue(FMainWindow->isVisible());
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	FMainWindow->showWindow();
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

void MainWindowPlugin::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (AWidget==NULL && AId==SCT_GLOBAL_SHOWROSTER)
	{
		FMainWindow->showWindow();
	}
	else if (AWidget==FMainWindow && AId==SCT_MAINWINDOW_CLOSEWINDOW)
	{
		FMainWindow->closeWindow();
	}
	else if (AWidget==FMainWindow && AId==SCT_MAINWINDOW_CHANGECENTRALVISIBLE)
	{
		FMainWindow->setCentralWidgetVisible(!FMainWindow->isCentralWidgetVisible());
	}
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
