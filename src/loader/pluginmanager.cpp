#include "pluginmanager.h"

#include <QtDebug>

#include <QDir>
#include <QTimer>
#include <QStack>
#include <QLibrary>
#include <QFileInfo>
#include <QSettings>

#define DIR_HOME                    ".vacuum"
#define DIR_PLUGINS                 "plugins"
#define DIR_TRANSLATIONS            "translations"

#define FILE_PLUGINS_SETTINGS       "plugins.xml"

#define ORGANIZATION_NAME           "JRuDevels"
#define APPLICATION_NAME            "Vacuum IM"

#define SVN_HOME_PATH               "HomePath"
#define SVN_LOCALE_NAME             "Locale"

#if defined(Q_OS_WIN)
# define LIB_PREFIX_SIZE            0
#else
# define LIB_PREFIX_SIZE            3
#endif


PluginManager::PluginManager(QApplication *AParent) : QObject(AParent)
{
  FShowDialog = NULL;
  FQtTranslator = new QTranslator(this);
  FUtilsTranslator = new QTranslator(this);
  FLoaderTranslator = new QTranslator(this);
  connect(AParent,SIGNAL(aboutToQuit()),SLOT(onApplicationAboutToQuit()));
}

PluginManager::~PluginManager()
{

}

QString PluginManager::homePath() const
{
  return FHomePath;
}

void PluginManager::setHomePath(const QString &APath)
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, ORGANIZATION_NAME, APPLICATION_NAME);
  settings.setValue(SVN_HOME_PATH, APath);
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
    foreach(PluginItem pluginItem, FPluginItems)
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
    it++;
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
    foreach(QUuid depend, FPluginItems.value(AUuid).info->dependences)
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
  QTimer::singleShot(0,qApp,SLOT(quit()));
}

void PluginManager::restart()
{
  onApplicationAboutToQuit();
  loadSettings();
  loadPlugins();
  initPlugins();
  saveSettings();
  createMenuActions();
  startPlugins();
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

  FHomePath = QString::null;
  if (args.contains(CLO_HOME_DIR))
  {
    QDir dir(args.value(args.indexOf(CLO_HOME_DIR)+1));
    if (dir.exists() && (dir.exists(DIR_HOME) || dir.mkpath(DIR_HOME)) && dir.cd(DIR_HOME))
      FHomePath = dir.absolutePath();
  }
  if (FHomePath.isNull() && !settings.value(SVN_HOME_PATH).toString().isEmpty())
  {
    QDir dir(settings.value(SVN_HOME_PATH).toString());
    if (dir.exists() && (dir.exists(DIR_HOME) || dir.mkpath(DIR_HOME)) && dir.cd(DIR_HOME))
      FHomePath = dir.absolutePath();
  }
  if (FHomePath.isNull())
  {
    QDir dir = QDir::home();
    if (dir.exists() && (dir.exists(DIR_HOME) || dir.mkpath(DIR_HOME)) && dir.cd(DIR_HOME))
      FHomePath = dir.absolutePath();
  }

  FPluginsSetup.clear();
  QDir homeDir(FHomePath);
  QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
  if (file.exists() && file.open(QFile::ReadOnly))
  {
    FPluginsSetup.setContent(&file,true);
  }
  if (FPluginsSetup.isNull())
  {
    FPluginsSetup.appendChild(FPluginsSetup.createElement("plugins"));
  }
}

void PluginManager::saveSettings()
{
  if (!FPluginsSetup.isNull())
  {
    QDir homeDir(FHomePath);
    QFile file(homeDir.absoluteFilePath(FILE_PLUGINS_SETTINGS));
    if (file.open(QFile::WriteOnly|QFile::Truncate))
      file.write(FPluginsSetup.toString(3).toUtf8());
  }
}

void PluginManager::loadPlugins()
{
  QDir dir(QApplication::applicationDirPath());
  if (dir.cd(DIR_PLUGINS)) 
  {
    QString tsDir = QApplication::applicationDirPath()+ "/" DIR_TRANSLATIONS "/" + QLocale().name();
    loadCoreTranslations(tsDir);

    QStringList files = dir.entryList(QDir::Files);
    removePluginsInfo(files);

    foreach (QString file, files)
    {
      if (QLibrary::isLibrary(file) && isPluginEnabled(file))
      {
        QPluginLoader *loader = new QPluginLoader(dir.absoluteFilePath(file),this);
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
              pluginItem.translator =  NULL;

              QTranslator *translator = new QTranslator(loader);
              QString tsFile = file.mid(LIB_PREFIX_SIZE,file.lastIndexOf('.')-LIB_PREFIX_SIZE);
              if (translator->load(tsFile,tsDir))
              {
                qApp->installTranslator(translator);
                pluginItem.translator = translator;
              }
              else
                delete translator;

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
        foreach(QUuid uid, getConflicts(puid)) {
          unloadPlugin(uid, tr("Conflict with plugin %1").arg(puid.toString())); }
        it = FPluginItems.constBegin();
      }
      else 
      {
        it++;
      }
    }
  }
  else
  {
    qDebug() << tr("Plugins directory not fount");
    quit();
  }
}

void PluginManager::initPlugins()
{
  QMultiMap<int,IPlugin *> pluginOrder;
  QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
  while (it!=FPluginItems.constEnd())
  {
    int initOrder = PIO_DEFAULT;
    IPlugin *plugin = it.value().plugin;
    if(plugin->initConnections(this,initOrder))
    {
      pluginOrder.insertMulti(initOrder,plugin);
      it++;
    }
    else
    {
      unloadPlugin(it.key(), tr("Initialization failed"));
      pluginOrder.clear();
      it = FPluginItems.constBegin();
    } 
  }

  foreach(IPlugin *plugin, pluginOrder)
    plugin->initObjects();

  foreach(IPlugin *plugin, pluginOrder)
    plugin->initSettings();
}

void PluginManager::startPlugins()
{
  foreach(PluginItem pluginItem, FPluginItems)
    pluginItem.plugin->startPlugin();
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
    foreach(QUuid uid, pluginDependencesOn(AUuid))
      removePluginItem(uid, AError);
    removePluginItem(AUuid, AError);
    FPlugins.clear();
  }
}

bool PluginManager::checkDependences(const QUuid AUuid) const
{ 
  if (FPluginItems.contains(AUuid))
  {
    foreach(QUuid depend, FPluginItems.value(AUuid).info->dependences)
    {
      if (!FPluginItems.contains(depend))
      {
        bool found = false;
        QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
        while (!found && it!=FPluginItems.constEnd())
        {
          found = it.value().info->implements.contains(depend);
          it++;
        }
        if (!found)
          return false;
      }
    }
  }
  return true;
}

bool PluginManager::checkConflicts(const QUuid AUuid) const
{ 
  if (FPluginItems.contains(AUuid))
  {
    foreach (QUuid conflict, FPluginItems.value(AUuid).info->conflicts)
    {
      if (!FPluginItems.contains(conflict))
      {
        foreach(PluginItem pluginItem, FPluginItems)
          if (pluginItem.info->implements.contains(conflict))
            return false;
      }
      else
        return false;
    }
  }
  return true;
}

QList<QUuid> PluginManager::getConflicts(const QUuid AUuid) const 
{
  QSet<QUuid> plugins;
  if (FPluginItems.contains(AUuid))
  {
    foreach (QUuid conflict, FPluginItems.value(AUuid).info->conflicts)
    {
      QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
      while (it!=FPluginItems.constEnd())
      {
        if (it.key()==conflict || it.value().info->implements.contains(conflict))
          plugins+=conflict;
        it++;
      }
    }
  }
  return plugins.toList(); 
}

void PluginManager::loadCoreTranslations(const QString &ADir)
{
  if (FLoaderTranslator->load("vacuum",ADir))
    qApp->installTranslator(FLoaderTranslator);

  if (FUtilsTranslator->load("utils",ADir))
    qApp->installTranslator(FUtilsTranslator);

  if (FQtTranslator->load("qt_"+QLocale().name(),ADir))
    qApp->installTranslator(FQtTranslator);

}

bool PluginManager::isPluginEnabled(const QString &AFile) const
{
  return FPluginsSetup.documentElement().firstChildElement(AFile).attribute("enabled","true") == "true";
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

  qDebug() << QString("%1: %2").arg(AFile).arg(AError);
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
      pluginElem.parentNode().removeChild(oldElem);
    }
    else
      pluginElem = pluginElem.nextSiblingElement();
  }
}

void PluginManager::createMenuActions()
{
  IPlugin *plugin = pluginInterface("IMainWindowPlugin").value(0);
  IMainWindowPlugin *mainWindowPligin = plugin!=NULL ? qobject_cast<IMainWindowPlugin *>(plugin->instance()) : NULL;

  plugin = pluginInterface("ITrayManager").value(0);
  ITrayManager *trayManager = plugin!=NULL ? qobject_cast<ITrayManager *>(plugin->instance()) : NULL;

  if (mainWindowPligin || trayManager)
  {
    if (FShowDialog == NULL)
    {
      FShowDialog = new Action(this);
      FShowDialog->setIcon(RSR_STORAGE_MENUICONS, MNI_PLUGINMANAGER_SETUP);
      connect(FShowDialog,SIGNAL(triggered(bool)),SLOT(onShowSetupPluginsDialog(bool)));
    }
    FShowDialog->setText(tr("Setup plugins"));

    if (mainWindowPligin)
      mainWindowPligin->mainWindow()->mainMenu()->addAction(FShowDialog,AG_MMENU_PLUGINMANAGER,true);
    if (trayManager)
      trayManager->addAction(FShowDialog,AG_TMTM_PLUGINMANAGER,true);
  }
  else
    onShowSetupPluginsDialog(false);
}

void PluginManager::onApplicationAboutToQuit()
{
  if (!FDialog.isNull())
    FDialog->reject();

  emit aboutToQuit();

  foreach(QUuid uid, FPluginItems.keys())
    unloadPlugin(uid);

  QCoreApplication::removeTranslator(FQtTranslator);
  QCoreApplication::removeTranslator(FUtilsTranslator);
  QCoreApplication::removeTranslator(FLoaderTranslator);

  saveSettings();
}

void PluginManager::onShowSetupPluginsDialog(bool)
{
  if (FDialog.isNull())
    FDialog = new SetupPluginsDialog(this,FPluginsSetup,NULL);
  FDialog->show();
  FDialog->raise();
  FDialog->activateWindow();
}
