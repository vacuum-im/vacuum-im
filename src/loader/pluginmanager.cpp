#include "pluginmanager.h"

#include <QtDebug>

#include <QTimer>
#include <QStack>
#include <QThread>
#include <QProcess>
#include <QLibrary>
#include <QFileInfo>
#include <QSettings>
#include <QLibraryInfo>

#define START_SHUTDOWN_TIMEOUT      100
#define DELAYED_SHUTDOWN_TIMEOUT    5000

#define ORGANIZATION_NAME           "JRuDevels"
#define APPLICATION_NAME            "VacuumIM"

#define FILE_PLUGINS_SETTINGS       "plugins.xml"

#define SVN_DATA_PATH               "DataPath"
#define SVN_LOCALE_NAME             "Locale"

#ifdef SVNINFO
#  include "svninfo.h"
#  define SVN_DATE                  ""
#else
#  define SVN_DATE                  ""
#  define SVN_REVISION              "0"
#endif

#if defined(Q_WS_WIN)
#  define ENV_APP_DATA              "APPDATA"
#  define DIR_APP_DATA              APPLICATION_NAME
#  define PATH_APP_DATA             ORGANIZATION_NAME"/"DIR_APP_DATA
#elif defined(Q_WS_X11)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              ".vacuum"
#  define PATH_APP_DATA             DIR_APP_DATA
#elif defined(Q_WS_MAC)
#  define ENV_APP_DATA              "HOME"
#  define DIR_APP_DATA              APPLICATION_NAME
#  define PATH_APP_DATA             "Library/Application Support/"DIR_APP_DATA
#elif defined(Q_WS_HAIKU)
#  define ENV_APP_DATA              "APPDATA"
#  define DIR_APP_DATA              APPLICATION_NAME
#  define PATH_APP_DATA             ORGANIZATION_NAME"/"DIR_APP_DATA
#endif

#if defined(Q_WS_WIN)
#  define LIB_PREFIX_SIZE           0
#else
#  define LIB_PREFIX_SIZE           3
#endif

PluginManager::PluginManager(QApplication *AParent) : QObject(AParent)
{
	FQuitReady = false;
	FQuitStarted = false;
	FCloseStarted = false;
	FShutdownKind = SK_WORK;
	FShutdownDelayCount = 0;

	FQtTranslator = new QTranslator(this);
	FUtilsTranslator = new QTranslator(this);
	FLoaderTranslator = new QTranslator(this);

	FShutdownTimer.setSingleShot(true);
	connect(&FShutdownTimer,SIGNAL(timeout()),SLOT(onShutdownTimerTimeout()));

	connect(AParent,SIGNAL(aboutToQuit()),SLOT(onApplicationAboutToQuit()));
	connect(AParent,SIGNAL(commitDataRequest(QSessionManager &)),SLOT(onApplicationCommitDataRequested(QSessionManager &)));
}

PluginManager::~PluginManager()
{

}

QString PluginManager::version() const
{
	return CLIENT_VERSION;
}

QString PluginManager::revision() const
{
	static const QString rev = QString(SVN_REVISION).contains(':') ? QString(SVN_REVISION).split(':').value(1) : QString(SVN_REVISION);
	return rev;
}

QDateTime PluginManager::revisionDate() const
{
	return QDateTime::fromString(SVN_DATE,"yyyy/MM/dd hh:mm:ss");
}

bool PluginManager::isShutingDown() const
{
	return FShutdownKind != SK_WORK;
}

QString PluginManager::homePath() const
{
	return FDataPath;
}

void PluginManager::setHomePath(const QString &APath)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);
	settings.setValue(SVN_DATA_PATH, APath);
}

void PluginManager::setLocale(QLocale::Language ALanguage, QLocale::Country ACountry)
{
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);
	if (ALanguage != QLocale::C)
		settings.setValue(SVN_LOCALE_NAME, QLocale(ALanguage, ACountry).name());
	else
		settings.remove(SVN_LOCALE_NAME);
}

IPlugin *PluginManager::pluginInstance(const QUuid &AUuid) const
{
	return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).plugin : NULL;
}

QList<IPlugin *> PluginManager::pluginInterface(const QString &AInterface) const
{
	QList<IPlugin *> plugins;
	if (!FPlugins.contains(AInterface))
	{
		foreach(const PluginItem &pluginItem, FPluginItems)
			if (AInterface.isEmpty() || pluginItem.plugin->instance()->inherits(AInterface.toLatin1().data()))
				FPlugins.insertMulti(AInterface,pluginItem.plugin);
	}
	return FPlugins.values(AInterface);
}

const IPluginInfo *PluginManager::pluginInfo(const QUuid &AUuid) const
{
	return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).info : NULL;
}

QList<QUuid> PluginManager::pluginDependencesOn(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QList<QUuid> plugins;
	QHash<QUuid, PluginItem>::const_iterator it = FPluginItems.constBegin();
	while (it != FPluginItems.constEnd())
	{
		if (!deepStack.contains(it.key()) && it.value().info->dependences.contains(AUuid))
		{
			plugins += pluginDependencesOn(it.key());
			plugins.append(it.key());
		}
		++it;
	}

	deepStack.pop();
	return plugins;
}

QList<QUuid> PluginManager::pluginDependencesFor(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QList<QUuid> plugins;
	if (FPluginItems.contains(AUuid))
	{
		QList<QUuid> dependences = FPluginItems.value(AUuid).info->dependences;
		foreach(const QUuid &depend, dependences)
		{
			if (!deepStack.contains(depend) && FPluginItems.contains(depend))
			{
				plugins.append(depend);
				plugins += pluginDependencesFor(depend);
			}
		}
	}

	deepStack.pop();
	return plugins;
}

void PluginManager::quit()
{
	if (!isShutingDown())
	{
		FShutdownKind = SK_QUIT;
		startClose();
	}
}

void PluginManager::restart()
{
	if (!isShutingDown())
	{
		FShutdownKind = SK_RESTART;
		startClose();
	}
}

void PluginManager::delayShutdown()
{
	if (isShutingDown())
		FShutdownTimer.start(DELAYED_SHUTDOWN_TIMEOUT);
	FShutdownDelayCount++;
}

void PluginManager::continueShutdown()
{
	if (--FShutdownDelayCount<=0 && isShutingDown())
		FShutdownTimer.start(START_SHUTDOWN_TIMEOUT);
}

void PluginManager::loadSettings()
{
	QStringList args = qApp->arguments();
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);

	QLocale locale(QLocale::C,  QLocale::AnyCountry);
	if (args.contains(CLO_LOCALE))
	{
		locale = QLocale(args.value(args.indexOf(CLO_LOCALE)+1));
	}
	if (locale.language()==QLocale::C && !settings.value(SVN_LOCALE_NAME).toString().isEmpty())
	{
		locale = QLocale(settings.value(SVN_LOCALE_NAME).toString());
	}
	if (locale.language() == QLocale::C)
	{
		locale = QLocale::system();
	}
	QLocale::setDefault(locale);

	FDataPath = QString::null;
	if (args.contains(CLO_APP_DATA_DIR))
	{
		QDir dir(args.value(args.indexOf(CLO_APP_DATA_DIR)+1));
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull())
	{
		QDir dir(qApp->applicationDirPath());
		if (dir.exists(DIR_APP_DATA) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull() && !settings.value(SVN_DATA_PATH).toString().isEmpty())
	{
		QDir dir(settings.value(SVN_DATA_PATH).toString());
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	if (FDataPath.isNull())
	{
		foreach(const QString &env, QProcess::systemEnvironment())
		{
			if (env.startsWith(ENV_APP_DATA"="))
			{
				QDir dir(env.split("=").value(1));
				if (dir.exists() && (dir.exists(PATH_APP_DATA) || dir.mkpath(PATH_APP_DATA)) && dir.cd(PATH_APP_DATA))
					FDataPath = dir.absolutePath();
			}
		}
	}
	if (FDataPath.isNull())
	{
		QDir dir(QDir::homePath());
		if (dir.exists() && (dir.exists(DIR_APP_DATA) || dir.mkpath(DIR_APP_DATA)) && dir.cd(DIR_APP_DATA))
			FDataPath = dir.absolutePath();
	}
	FileStorage::setResourcesDirs(FileStorage::resourcesDirs()
		<< (QDir::isAbsolutePath(RESOURCES_DIR) ? RESOURCES_DIR : qApp->applicationDirPath()+"/"+RESOURCES_DIR)
		<< FDataPath+"/resources");

	FPluginsSetup.clear();
	QDir homeDir(FDataPath);
	QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
	if (file.exists() && file.open(QFile::ReadOnly))
		FPluginsSetup.setContent(&file,true);
	file.close();

	if (FPluginsSetup.documentElement().tagName() != "plugins")
	{
		FPluginsSetup.clear();
		FPluginsSetup.appendChild(FPluginsSetup.createElement("plugins"));
	}
}

void PluginManager::saveSettings()
{
	if (!FPluginsSetup.documentElement().isNull())
	{
		QDir homeDir(FDataPath);
		QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			file.write(FPluginsSetup.toString(3).toUtf8());
			file.flush();
			file.close();
		}
	}
}

void PluginManager::loadPlugins()
{
	QDir pluginsDir(QApplication::applicationDirPath());
	if (pluginsDir.cd(PLUGINS_DIR))
	{
		QString localeName = QLocale().name();
		QDir tsDir(QApplication::applicationDirPath());
		tsDir.cd(TRANSLATIONS_DIR);
		loadCoreTranslations(tsDir,localeName);

		QStringList files = pluginsDir.entryList(QDir::Files);
		removePluginsInfo(files);

		foreach (const QString &file, files)
		{
			if (QLibrary::isLibrary(file) && isPluginEnabled(file))
			{
				QPluginLoader *loader = new QPluginLoader(pluginsDir.absoluteFilePath(file),this);

				QTranslator *translator = new QTranslator(loader);
				QString tsFile = file.mid(LIB_PREFIX_SIZE,file.lastIndexOf('.')-LIB_PREFIX_SIZE);
				if (translator->load(tsFile,tsDir.absoluteFilePath(localeName)) || translator->load(tsFile,tsDir.absoluteFilePath(localeName.left(2))))
				{
					qApp->installTranslator(translator);
				}
				else
				{
					delete translator;
					translator = NULL;
				}

				if (loader->load())
				{
					IPlugin *plugin = qobject_cast<IPlugin *>(loader->instance());
					if (plugin)
					{
						plugin->instance()->setParent(loader);
						QUuid uid = plugin->pluginUuid();
						if (!FPluginItems.contains(uid))
						{
							PluginItem pluginItem;
							pluginItem.plugin = plugin;
							pluginItem.loader = loader;
							pluginItem.info = new IPluginInfo;
							pluginItem.translator =  translator;

							plugin->pluginInfo(pluginItem.info);
							savePluginInfo(file, pluginItem.info).setAttribute("uuid", uid.toString());

							FPluginItems.insert(uid,pluginItem);
						}
						else
						{
							savePluginError(file, tr("Duplicate plugin uuid"));
							delete loader;
						}
					}
					else
					{
						savePluginError(file, tr("Wrong plugin interface"));
						delete loader;
					}
				}
				else
				{
					savePluginError(file, loader->errorString());
					delete loader;
				}
			}
		}

		QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
		while (it!=FPluginItems.constEnd())
		{
			QUuid puid = it.key();
			if (!checkDependences(puid))
			{
				unloadPlugin(puid, tr("Dependences not found"));
				it = FPluginItems.constBegin();
			}
			else if (!checkConflicts(puid))
			{
				foreach(const QUuid &uid, getConflicts(puid))
					unloadPlugin(uid, tr("Conflict with plugin %1").arg(puid.toString()));
				it = FPluginItems.constBegin();
			}
			else
			{
				++it;
			}
		}
	}
	else
	{
		qDebug() << tr("Plugins directory not found");
		quit();
	}
}

bool PluginManager::initPlugins()
{
	bool initOk = true;
	QMultiMap<int,IPlugin *> pluginOrder;
	QHash<QUuid, PluginItem>::const_iterator it = FPluginItems.constBegin();
	while (initOk && it!=FPluginItems.constEnd())
	{
		int initOrder = PIO_DEFAULT;
		IPlugin *plugin = it.value().plugin;
		if (plugin->initConnections(this,initOrder))
		{
			pluginOrder.insertMulti(initOrder,plugin);
			++it;
		}
		else
		{
			initOk = false;
			FBlockedPlugins.append(QFileInfo(it.value().loader->fileName()).fileName());
			unloadPlugin(it.key(), tr("Initialization failed"));
		}
	}

	if (initOk)
	{
		foreach(IPlugin *plugin, pluginOrder)
			plugin->initObjects();

		foreach(IPlugin *plugin, pluginOrder)
			plugin->initSettings();
	}

	return initOk;
}

void PluginManager::startPlugins()
{
	foreach(const PluginItem &pluginItem, FPluginItems)
		pluginItem.plugin->startPlugin();
}

void PluginManager::unloadPlugins()
{
	foreach(const QUuid &uid, FPluginItems.keys())
		unloadPlugin(uid);

	QCoreApplication::removeTranslator(FQtTranslator);
	QCoreApplication::removeTranslator(FUtilsTranslator);
	QCoreApplication::removeTranslator(FLoaderTranslator);
}

void PluginManager::startClose()
{
	if (!FCloseStarted)
	{
		FCloseStarted = true;
		delayShutdown();
		emit shutdownStarted();
		closeTopLevelWidgets();
		continueShutdown();
	}
}

void PluginManager::finishClose()
{
	FShutdownTimer.stop();
	if (FCloseStarted)
	{
		FCloseStarted = false;
		startQuit();
	}
}

void PluginManager::startQuit()
{
	if (!FQuitStarted)
	{
		FQuitStarted = true;
		delayShutdown();
		emit aboutToQuit();
		continueShutdown();
	}
}

void PluginManager::finishQuit()
{
	FShutdownTimer.stop();
	if (FQuitStarted)
	{
		FQuitStarted = false;
		unloadPlugins();
		
		if (FShutdownKind == SK_RESTART)
		{
			FShutdownKind = SK_WORK;
			FShutdownDelayCount = 0;

			loadSettings();
			loadPlugins();
			if (initPlugins())
			{
				saveSettings();
				createMenuActions();
				declareShortcuts();
				startPlugins();
				FBlockedPlugins.clear();
			}
			else
			{
				QTimer::singleShot(0,this,SLOT(restart()));
			}
		}
		else if (FShutdownKind == SK_QUIT)
		{
			FQuitReady = true;
			QTimer::singleShot(0,qApp,SLOT(quit()));
		}
	}
}

void PluginManager::closeAndQuit()
{
	if (!FQuitReady)
	{
		FShutdownKind = SK_QUIT;
		startClose();

		QDateTime closeTimeout = QDateTime::currentDateTime().addMSecs(DELAYED_SHUTDOWN_TIMEOUT);
		while (closeTimeout>QDateTime::currentDateTime() && FShutdownDelayCount>0)
			QApplication::processEvents();
		finishClose();

		QDateTime quitTimeout = QDateTime::currentDateTime().addMSecs(DELAYED_SHUTDOWN_TIMEOUT);
		while (quitTimeout>QDateTime::currentDateTime() && FShutdownDelayCount>0)
			QApplication::processEvents();
		finishQuit();
	}
}

void PluginManager::closeTopLevelWidgets()
{
	if (!FPluginsDialog.isNull())
		FPluginsDialog->reject();

	if (!FAboutDialog.isNull())
		FAboutDialog->reject();

	foreach(QWidget *widget, QApplication::topLevelWidgets())
		widget->close();
}

void PluginManager::removePluginItem(const QUuid &AUuid, const QString &AError)
{
	if (FPluginItems.contains(AUuid))
	{
		PluginItem pluginItem = FPluginItems.take(AUuid);
		if (!AError.isEmpty())
		{
			savePluginError(QFileInfo(pluginItem.loader->fileName()).fileName(), AError);
		}
		if (pluginItem.translator)
		{
			qApp->removeTranslator(pluginItem.translator);
		}
		delete pluginItem.translator;
		delete pluginItem.info;
		delete pluginItem.loader;
	}
}

void PluginManager::unloadPlugin(const QUuid &AUuid, const QString &AError)
{
	if (FPluginItems.contains(AUuid))
	{
		foreach(const QUuid &uid, pluginDependencesOn(AUuid))
			removePluginItem(uid, AError);
		removePluginItem(AUuid, AError);
		FPlugins.clear();
	}
}

bool PluginManager::checkDependences(const QUuid &AUuid) const
{
	if (FPluginItems.contains(AUuid))
	{
		QList<QUuid> dependences = FPluginItems.value(AUuid).info->dependences;
		foreach(const QUuid &depend, dependences)
		{
			if (!FPluginItems.contains(depend))
			{
				bool found = false;
				QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
				while (!found && it!=FPluginItems.constEnd())
				{
					found = it.value().info->implements.contains(depend);
					++it;
				}
				if (!found)
					return false;
			}
		}
	}
	return true;
}

bool PluginManager::checkConflicts(const QUuid &AUuid) const
{
	if (FPluginItems.contains(AUuid))
	{
		QList<QUuid> conflicts = FPluginItems.value(AUuid).info->conflicts;
		foreach (const QUuid &conflict, conflicts)
		{
			if (!FPluginItems.contains(conflict))
			{
				foreach(const PluginItem &pluginItem, FPluginItems)
					if (pluginItem.info->implements.contains(conflict))
						return false;
			}
			else
			{
				return false;
			}
		}
	}
	return true;
}

QList<QUuid> PluginManager::getConflicts(const QUuid &AUuid) const
{
	QSet<QUuid> plugins;
	if (FPluginItems.contains(AUuid))
	{
		QList<QUuid> conflicts = FPluginItems.value(AUuid).info->conflicts;
		foreach (const QUuid &conflict, conflicts)
		{
			QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
			while (it!=FPluginItems.constEnd())
			{
				if (it.key()==conflict || it.value().info->implements.contains(conflict))
					plugins+=conflict;
				++it;
			}
		}
	}
	return plugins.toList();
}

void PluginManager::loadCoreTranslations(const QDir &ADir, const QString &ALocaleName)
{
	if (FQtTranslator->load("qt_"+ALocaleName,ADir.absoluteFilePath(ALocaleName)) || FQtTranslator->load("qt_"+ALocaleName,ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FQtTranslator);
	else if (FQtTranslator->load("qt_"+QLocale().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
		qApp->installTranslator(FQtTranslator);

	if (FLoaderTranslator->load("vacuum",ADir.absoluteFilePath(ALocaleName)) || FLoaderTranslator->load("vacuum",ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FLoaderTranslator);

	if (FUtilsTranslator->load("vacuumutils",ADir.absoluteFilePath(ALocaleName)) || FUtilsTranslator->load("vacuumutils",ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FUtilsTranslator);
}

bool PluginManager::isPluginEnabled(const QString &AFile) const
{
	return !FBlockedPlugins.contains(AFile) && FPluginsSetup.documentElement().firstChildElement(AFile).attribute("enabled","true") == "true";
}

QDomElement PluginManager::savePluginInfo(const QString &AFile, const IPluginInfo *AInfo)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement(AFile);
	if (pluginElem.isNull())
		pluginElem = FPluginsSetup.firstChildElement("plugins").appendChild(FPluginsSetup.createElement(AFile)).toElement();

	QDomElement nameElem = pluginElem.firstChildElement("name");
	if (nameElem.isNull())
	{
		nameElem = pluginElem.appendChild(FPluginsSetup.createElement("name")).toElement();
		nameElem.appendChild(FPluginsSetup.createTextNode(AInfo->name));
	}
	else
		nameElem.firstChild().toCharacterData().setData(AInfo->name);

	QDomElement descElem = pluginElem.firstChildElement("desc");
	if (descElem.isNull())
	{
		descElem = pluginElem.appendChild(FPluginsSetup.createElement("desc")).toElement();
		descElem.appendChild(FPluginsSetup.createTextNode(AInfo->description));
	}
	else
		descElem.firstChild().toCharacterData().setData(AInfo->description);

	QDomElement versionElem = pluginElem.firstChildElement("version");
	if (versionElem.isNull())
	{
		versionElem = pluginElem.appendChild(FPluginsSetup.createElement("version")).toElement();
		versionElem.appendChild(FPluginsSetup.createTextNode(AInfo->version));
	}
	else
		versionElem.firstChild().toCharacterData().setData(AInfo->version);

	pluginElem.removeChild(pluginElem.firstChildElement("depends"));
	if (!AInfo->dependences.isEmpty())
	{
		QDomElement dependsElem = pluginElem.appendChild(FPluginsSetup.createElement("depends")).toElement();
		foreach(const QUuid &uid, AInfo->dependences)
			dependsElem.appendChild(FPluginsSetup.createElement("uuid")).appendChild(FPluginsSetup.createTextNode(uid.toString()));
	}

	pluginElem.removeChild(pluginElem.firstChildElement("error"));

	return pluginElem;
}

void PluginManager::savePluginError(const QString &AFile, const QString &AError)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement(AFile);
	if (pluginElem.isNull())
		pluginElem = FPluginsSetup.firstChildElement("plugins").appendChild(FPluginsSetup.createElement(AFile)).toElement();

	QDomElement errorElem = pluginElem.firstChildElement("error");
	if (AError.isEmpty())
	{
		pluginElem.removeChild(errorElem);
	}
	else if (errorElem.isNull())
	{
		errorElem = pluginElem.appendChild(FPluginsSetup.createElement("error")).toElement();
		errorElem.appendChild(FPluginsSetup.createTextNode(AError));
	}
	else
	{
		errorElem.firstChild().toCharacterData().setData(AError);
	}
}

void PluginManager::removePluginsInfo(const QStringList &ACurFiles)
{
	QDomElement pluginElem = FPluginsSetup.documentElement().firstChildElement();
	while (!pluginElem.isNull())
	{
		if (!ACurFiles.contains(pluginElem.tagName()))
		{
			QDomElement oldElem = pluginElem;
			pluginElem = pluginElem.nextSiblingElement();
			oldElem.parentNode().removeChild(oldElem);
		}
		else
		{
			pluginElem = pluginElem.nextSiblingElement();
		}
	}
}

void PluginManager::createMenuActions()
{
	IPlugin *plugin = pluginInterface("IMainWindowPlugin").value(0);
	IMainWindowPlugin *mainWindowPlugin = plugin!=NULL ? qobject_cast<IMainWindowPlugin *>(plugin->instance()) : NULL;

	if (mainWindowPlugin)
	{
		Action *aboutQt = new Action(mainWindowPlugin->mainWindow()->mainMenu());
		aboutQt->setText(tr("About Qt"));
		aboutQt->setIcon(RSR_STORAGE_MENUICONS,MNI_PLUGINMANAGER_ABOUT_QT);
		aboutQt->setShortcutId(SCT_APP_ABOUTQT);
		connect(aboutQt,SIGNAL(triggered()),QApplication::instance(),SLOT(aboutQt()));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(aboutQt,AG_MMENU_PLUGINMANAGER_ABOUT);

		Action *about = new Action(mainWindowPlugin->mainWindow()->mainMenu());
		about->setText(tr("About the program"));
		about->setIcon(RSR_STORAGE_MENUICONS,MNI_PLUGINMANAGER_ABOUT);
		about->setShortcutId(SCT_APP_ABOUTPROGRAM);
		connect(about,SIGNAL(triggered()),SLOT(onShowAboutBoxDialog()));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(about,AG_MMENU_PLUGINMANAGER_ABOUT);

		Action *pluginsDialog = new Action(mainWindowPlugin->mainWindow()->mainMenu());
		pluginsDialog->setText(tr("Setup plugins"));
		pluginsDialog->setIcon(RSR_STORAGE_MENUICONS, MNI_PLUGINMANAGER_SETUP);
		pluginsDialog->setShortcutId(SCT_APP_SETUPPLUGINS);
		connect(pluginsDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(pluginsDialog,AG_MMENU_PLUGINMANAGER_SETUP,true);
	}
	else
	{
		onShowSetupPluginsDialog(false);
	}
}

void PluginManager::declareShortcuts()
{
	Shortcuts::declareGroup(SCTG_GLOBAL, tr("Global"), SGO_GLOBAL);

	Shortcuts::declareGroup(SCTG_APPLICATION, tr("Application"), SGO_APPLICATION);
	Shortcuts::declareShortcut(SCT_APP_ABOUTQT, tr("Show information about Qt"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_ABOUTPROGRAM, tr("Show information about client"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
	Shortcuts::declareShortcut(SCT_APP_SETUPPLUGINS, tr("Show setup plugins dialog"), QKeySequence::UnknownKey, Shortcuts::ApplicationShortcut);
}

void PluginManager::onApplicationAboutToQuit()
{
	closeAndQuit();
}

void PluginManager::onApplicationCommitDataRequested(QSessionManager &AManager)
{
	Q_UNUSED(AManager);
	closeAndQuit();
}

void PluginManager::onShowSetupPluginsDialog(bool)
{
	if (FPluginsDialog.isNull())
	{
		FPluginsDialog = new SetupPluginsDialog(this,FPluginsSetup,NULL);
		connect(FPluginsDialog, SIGNAL(accepted()),SLOT(onSetupPluginsDialogAccepted()));
	}
	WidgetManager::showActivateRaiseWindow(FPluginsDialog);
}

void PluginManager::onSetupPluginsDialogAccepted()
{
	saveSettings();
}

void PluginManager::onShowAboutBoxDialog()
{
	if (FAboutDialog.isNull())
		FAboutDialog = new AboutBox(this);
	WidgetManager::showActivateRaiseWindow(FAboutDialog);
}

void PluginManager::onShutdownTimerTimeout()
{
	if (FCloseStarted)
		finishClose();
	else if (FQuitStarted)
		finishQuit();
}
