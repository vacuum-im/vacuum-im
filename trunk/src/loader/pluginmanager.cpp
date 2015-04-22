#include "pluginmanager.h"

#include <QTimer>
#include <QStack>
#include <QThread>
#include <QProcess>
#include <QLibrary>
#include <QFileInfo>
#include <QSettings>
#include <QLibraryInfo>
#include <definitions/plugininitorders.h>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/actiongroups.h>
#include <definitions/version.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/commandline.h>
#include <definitions/statisticsparams.h>
#include <utils/widgetmanager.h>
#include <utils/systemmanager.h>
#include <utils/pluginhelper.h>
#include <utils/filestorage.h>
#include <utils/shortcuts.h>
#include <utils/action.h>
#include <utils/logger.h>

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

#define DIR_LOGS                    "logs"
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

	PluginHelper::setPluginManager(this);
}

PluginManager::~PluginManager()
{
	Logger::closeLog();
	PluginHelper::setPluginManager(NULL);
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

QSet<QUuid> PluginManager::pluginDependencesOn(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QSet<QUuid> plugins;
	for (QHash<QUuid, PluginItem>::const_iterator it = FPluginItems.constBegin(); it!=FPluginItems.constEnd(); ++it)
	{
		if (!deepStack.contains(it.key()) && it.value().info->dependences.contains(AUuid))
		{
			plugins += pluginDependencesOn(it.key());
			plugins += it.key();
		}
	}

	deepStack.pop();
	return plugins;
}

QSet<QUuid> PluginManager::pluginDependencesFor(const QUuid &AUuid) const
{
	static QStack<QUuid> deepStack;
	deepStack.push(AUuid);

	QSet<QUuid> plugins;
	if (FPluginItems.contains(AUuid))
	{
		foreach(const QUuid &depend, FPluginItems.value(AUuid).info->dependences)
		{
			if (!deepStack.contains(depend) && FPluginItems.contains(depend))
			{
				plugins += pluginDependencesFor(depend);
				plugins += depend;
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
		LOG_INFO("Application quit requested");
		FShutdownKind = SK_QUIT;
		startClose();
	}
}

void PluginManager::restart()
{
	if (!isShutingDown())
	{
		Logger::startTiming(STMP_APPLICATION_START);
		FShutdownKind = SK_RESTART;
		startClose();
	}
}

void PluginManager::delayShutdown()
{
	FShutdownDelayCount++;
	if (isShutingDown())
		FShutdownTimer.start(DELAYED_SHUTDOWN_TIMEOUT);
}

void PluginManager::continueShutdown()
{
	FShutdownDelayCount--;
	if (FShutdownDelayCount<=0 && isShutingDown())
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

	QDir logDir(FDataPath);
	if (logDir.exists() && (logDir.exists(DIR_LOGS) || logDir.mkpath(DIR_LOGS)) && logDir.cd(DIR_LOGS))
	{
#ifndef DEBUG_MODE
		quint32 logTypes = Logger::Fatal|Logger::Error|Logger::Warning|Logger::Info;
#else
		quint32 logTypes = Logger::Fatal|Logger::Error|Logger::Warning|Logger::Info|Logger::View|Logger::Event|Logger::Timing|Logger::Debug;
#endif
		if (args.contains(CLO_LOG_TYPES))
			logTypes = args.value(args.indexOf(CLO_LOG_TYPES)+1).toUInt();

		if (logTypes > 0)
		{
			Logger::setEnabledTypes(logTypes);
			Logger::openLog(logDir.absolutePath());
		}
	}
	LOG_INFO(QString("%1: %2.%3, Qt: %4/%5, OS: %6, Locale: %7").arg(CLIENT_NAME,CLIENT_VERSION).arg(revision(),QT_VERSION_STR,qVersion(),SystemManager::osVersion(),QLocale().name()));

	FPluginsSetup.clear();
	QDir homeDir(FDataPath);
	QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
	if (file.open(QFile::ReadOnly))
	{
		QString xmlError;
		if (!FPluginsSetup.setContent(&file,true,&xmlError))
		{
			REPORT_ERROR(QString("Failed to load plugins settings from file content: %1").arg(xmlError));
			file.remove();
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load plugin settings from file: %1").arg(file.errorString()));
	}

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
			file.write(FPluginsSetup.toByteArray());
			file.close();
		}
		else
		{
			REPORT_ERROR(QString("Failed to save plugins settings to file: %1").arg(file.errorString()));
		}
	}
}

bool PluginManager::loadPlugins()
{
	LOG_INFO("Loading plugins");

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
			if (!QLibrary::isLibrary(file))
			{
				LOG_WARNING(QString("Failed to load plugin %1: Invalid library format").arg(file));
			}
			else if (!isPluginEnabled(file))
			{
				LOG_DEBUG(QString("Skipping disabled plugin file %1").arg(file));
			}
			else
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
					LOG_DEBUG(QString("Failed to load translation for plugin %1").arg(file));
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
							LOG_DEBUG(QString("Loaded plugin from file=%1, version=%2, uuid=%3").arg(file,pluginItem.info->version,uid.toString()));
						}
						else
						{
							LOG_ERROR(QString("Failed to load plugin %1: Duplicate uuid=%2").arg(file,uid.toString()));
							savePluginError(file, tr("Duplicate plugin uuid"));
							delete loader;
						}
					}
					else
					{
						LOG_ERROR(QString("Failed to load plugin %1: Invalid interface").arg(file));
						savePluginError(file, tr("Wrong plugin interface"));
						delete loader;
					}
				}
				else
				{
					LOG_ERROR(QString("Failed to load plugin %1: %2").arg(file,loader->errorString()));
					savePluginError(file,loader->errorString());
					delete loader;
				}
			}
		}

		QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
		while (it != FPluginItems.constEnd())
		{
			QUuid puid = it.key();
			if (!checkDependences(puid))
			{
				LOG_WARNING(QString("Dependences not found for plugin, uuid=%1").arg(puid.toString()));
				unloadPlugin(puid, tr("Dependences not found"));
				it = FPluginItems.constBegin();
			}
			else if (!checkConflicts(puid))
			{
				LOG_WARNING(QString("Conflicts found for plugin, uuid=%1").arg(puid.toString()));
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
		REPORT_FATAL("Plugins directory not found");
	}
	return !FPluginItems.isEmpty();
}

bool PluginManager::initPlugins()
{
	LOG_INFO("Initializing plugins");

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
			LOG_WARNING(QString("Failed to initialize plugin, uuid=%1").arg(plugin->pluginUuid().toString()));
			FBlockedPlugins.append(QFileInfo(it.value().loader->fileName()).fileName());
			unloadPlugin(it.key(),tr("Initialization failed"));
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

bool PluginManager::startPlugins()
{
	LOG_INFO("Starting plugins");

	bool allStarted = true;
	foreach(const PluginItem &pluginItem, FPluginItems)
	{
		bool started = pluginItem.plugin->startPlugin();
		allStarted = allStarted && started;
	}
	return allStarted;
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
		LOG_INFO(QString("Starting quit stage 1, delay: %1").arg(FShutdownDelayCount));
		Logger::startTiming(STMP_APPLICATION_QUIT);

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
		LOG_INFO(QString("Finished quit stage 1, delay: %1").arg(FShutdownDelayCount));
		FCloseStarted = false;
		startQuit();
	}
}

void PluginManager::startQuit()
{
	if (!FQuitStarted)
	{
		LOG_INFO(QString("Starting quit stage 2, delay: %1").arg(FShutdownDelayCount));
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
		LOG_INFO(QString("Finished quit stage 2, delay: %1").arg(FShutdownDelayCount));

		FQuitStarted = false;
		unloadPlugins();
		
		if (FShutdownKind == SK_RESTART)
		{
			FShutdownKind = SK_WORK;
			FShutdownDelayCount = 0;

			loadSettings();
			if (!loadPlugins())
			{
				quit();
			}
			else if (!initPlugins())
			{
				QTimer::singleShot(0,this,SLOT(restart()));
			}
			else 
			{
				saveSettings();
				createMenuActions();
				declareShortcuts();
				startPlugins();
				FBlockedPlugins.clear();
				REPORT_TIMING(STMP_APPLICATION_START,Logger::finishTiming(STMP_APPLICATION_START));
				LOG_INFO("Application started");
			}
		}
		else if (FShutdownKind == SK_QUIT)
		{
			FQuitReady = true;
			QTimer::singleShot(0,qApp,SLOT(quit()));
			REPORT_TIMING(STMP_APPLICATION_QUIT,Logger::finishTiming(STMP_APPLICATION_QUIT));
		}
	}
}

void PluginManager::closeAndQuit()
{
	if (!isShutingDown())
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
			savePluginError(QFileInfo(pluginItem.loader->fileName()).fileName(), AError);
		if (pluginItem.translator)
			qApp->removeTranslator(pluginItem.translator);
		delete pluginItem.translator;
		delete pluginItem.info;
		delete pluginItem.loader;
		LOG_DEBUG(QString("Plugin destroyed, uuid=%1, error=%2").arg(AUuid.toString(),AError));
	}
}

void PluginManager::unloadPlugin(const QUuid &AUuid, const QString &AError)
{
	if (FPluginItems.contains(AUuid))
	{
		foreach(const QUuid &uid, pluginDependencesOn(AUuid))
			removePluginItem(uid,AError);
		removePluginItem(AUuid,AError);
		FPlugins.clear();
	}
}

bool PluginManager::checkDependences(const QUuid &AUuid) const
{
	if (FPluginItems.contains(AUuid))
	{
		foreach(const QUuid &depend, FPluginItems.value(AUuid).info->dependences)
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
	else
		LOG_DEBUG("Translation for 'Qt' not found");

	if (FLoaderTranslator->load("vacuum",ADir.absoluteFilePath(ALocaleName)) || FLoaderTranslator->load("vacuum",ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FLoaderTranslator);
	else
		LOG_DEBUG("Translation for 'vacuum' not found");

	if (FUtilsTranslator->load("vacuumutils",ADir.absoluteFilePath(ALocaleName)) || FUtilsTranslator->load("vacuumutils",ADir.absoluteFilePath(ALocaleName.left(2))))
		qApp->installTranslator(FUtilsTranslator);
	else
		LOG_DEBUG("Translation for 'vacuumutils' not found");
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
	{
		nameElem.firstChild().toCharacterData().setData(AInfo->name);
	}

	QDomElement descElem = pluginElem.firstChildElement("desc");
	if (descElem.isNull())
	{
		descElem = pluginElem.appendChild(FPluginsSetup.createElement("desc")).toElement();
		descElem.appendChild(FPluginsSetup.createTextNode(AInfo->description));
	}
	else
	{
		descElem.firstChild().toCharacterData().setData(AInfo->description);
	}

	QDomElement versionElem = pluginElem.firstChildElement("version");
	if (versionElem.isNull())
	{
		versionElem = pluginElem.appendChild(FPluginsSetup.createElement("version")).toElement();
		versionElem.appendChild(FPluginsSetup.createTextNode(AInfo->version));
	}
	else
	{
		versionElem.firstChild().toCharacterData().setData(AInfo->version);
	}

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
	if (!AError.isEmpty())
	{
		if (errorElem.isNull())
		{
			errorElem = pluginElem.appendChild(FPluginsSetup.createElement("error")).toElement();
			errorElem.appendChild(FPluginsSetup.createTextNode(AError));
		}
		else
		{
			errorElem.firstChild().toCharacterData().setData(AError);
		}
	}
	else
	{
		pluginElem.removeChild(errorElem);
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
		connect(aboutQt,SIGNAL(triggered()),QApplication::instance(),SLOT(aboutQt()));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(aboutQt,AG_MMENU_PLUGINMANAGER_ABOUT);

		Action *about = new Action(mainWindowPlugin->mainWindow()->mainMenu());
		about->setText(tr("About the program"));
		about->setIcon(RSR_STORAGE_MENUICONS,MNI_PLUGINMANAGER_ABOUT);
		connect(about,SIGNAL(triggered()),SLOT(onShowAboutBoxDialog()));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(about,AG_MMENU_PLUGINMANAGER_ABOUT);

		Action *pluginsDialog = new Action(mainWindowPlugin->mainWindow()->mainMenu());
		pluginsDialog->setText(tr("Setup plugins"));
		pluginsDialog->setIcon(RSR_STORAGE_MENUICONS, MNI_PLUGINMANAGER_SETUP);
		connect(pluginsDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
		mainWindowPlugin->mainWindow()->mainMenu()->addAction(pluginsDialog,AG_MMENU_PLUGINMANAGER_SETUP,true);
	}
}

void PluginManager::declareShortcuts()
{
	Shortcuts::declareGroup(SCTG_GLOBAL, tr("Global shortcuts"), SGO_GLOBAL);
	Shortcuts::declareGroup(SCTG_APPLICATION, tr("Application shortcuts"), SGO_APPLICATION);
}

void PluginManager::onApplicationAboutToQuit()
{
	LOG_INFO("Application about to quit");
	closeAndQuit();
}

void PluginManager::onApplicationCommitDataRequested(QSessionManager &AManager)
{
	Q_UNUSED(AManager);
	LOG_INFO("Application session about to close");
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
