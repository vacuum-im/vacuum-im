#include <QtDebug>
#include "pluginmanager.h"

#include <QDir>
#include <QTimer>
#include <QStack>
#include <QLocale>
#include <QLibrary>
#include <QMultiMap>

#define DIR_PLUGINS         "plugins"
#define DIR_TRANSLATIONS    "translations"

//PluginManager
PluginManager::PluginManager(QApplication *AParent) : QObject(AParent)
{
  FQtTranslator = NULL;
  connect(AParent,SIGNAL(aboutToQuit()),SLOT(onAboutToQuit()));
}

PluginManager::~PluginManager()
{

}

void PluginManager::restart()
{
  onAboutToQuit();

  FPlugins.clear();
  FPluginItems.clear();

  loadPlugins();
  initPlugins();
  startPlugins();
}

QList<IPlugin *> PluginManager::getPlugins(const QString &AInterface) const
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

IPlugin *PluginManager::getPlugin(const QUuid &AUuid) const
{ 
  return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).plugin : NULL;
}

const IPluginInfo *PluginManager::getPluginInfo(const QUuid &AUuid) const
{
  return FPluginItems.contains(AUuid) ? FPluginItems.value(AUuid).info : NULL;
}

QList<QUuid> PluginManager::getDependencesOn(const QUuid &AUuid) const
{
  static QStack<QUuid> deepStack;
  deepStack.push(AUuid);

  QList<QUuid> plugins;
  foreach(PluginItem pluginItem, FPluginItems)
  {
    if (!deepStack.contains(pluginItem.uid) && pluginItem.info->dependences.contains(AUuid))
    {
      plugins += getDependencesOn(pluginItem.uid);
      plugins.append(pluginItem.uid);
    }
  }

  deepStack.pop(); 
  return plugins;
}

QList<QUuid> PluginManager::getDependencesFor(const QUuid &AUuid) const
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
        plugins += getDependencesFor(depend); 
      }
    }
  }

  deepStack.pop(); 
  return plugins;
}

void PluginManager::loadPlugins()
{
  QDir dir(QApplication::applicationDirPath());
  if (dir.cd(DIR_PLUGINS)) 
  {
    QStringList args = qApp->arguments();
    QString locale = args.contains(CLO_LOCALE) ? args.value(args.indexOf(CLO_LOCALE)+1) : QLocale::system().name();
    QString tsDir = QApplication::applicationDirPath()+"/"DIR_TRANSLATIONS"/"+locale;
    QString qtTranslator = tsDir+"/qtlib.qm";
    if (QFile::exists(qtTranslator))
    {
      if (!FQtTranslator)
      {
        FQtTranslator = new QTranslator(this);
        FQtTranslator->load(qtTranslator);
        qApp->installTranslator(FQtTranslator);
      }
      else
        FQtTranslator->load(qtTranslator);
    }
    else if (FQtTranslator)
    {
      qApp->removeTranslator(FQtTranslator);
      delete FQtTranslator;
      FQtTranslator = NULL;
    }

    QStringList files = dir.entryList(QDir::Files);
    foreach (QString file, files) 
    {
      if (QLibrary::isLibrary(file)) 
      {
        QPluginLoader *loader = new QPluginLoader(dir.filePath(file),this);
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
              pluginItem.uid = uid;
              pluginItem.plugin = plugin;
              pluginItem.loader = loader;
              pluginItem.info = new IPluginInfo;
              pluginItem.translator =  NULL;
              
              QString qmFile = tsDir+"/"+file.left(file.lastIndexOf('.'))+".qm";
              if (QFile::exists(qmFile))
              {
                QTranslator *translator = new QTranslator(loader);
                if (translator->load(qmFile))
                {
                  qApp->installTranslator(translator);
                  pluginItem.translator = translator;
                }
                else
                  delete translator;
              }

              plugin->pluginInfo(pluginItem.info);
              FPluginItems.insert(uid,pluginItem);
              qDebug() << "Plugin loaded:"  << pluginItem.info->name;
            } 
            else
            {
              qDebug() << "DUBLICATE UUID IN:" << loader->fileName();
              delete loader;
            }
          } 
          else
          {
            qDebug() << "WRONG PLUGIN INTERFACE IN:" << loader->fileName();
            delete loader;
          }
        } 
        else 
        {
          qDebug() << "WRONG LIBRARY BUILD CONTEXT" << loader->fileName();
          delete loader;
        }
      }
    }

    QHash<QUuid,PluginItem>::const_iterator it = FPluginItems.constBegin();
    while (it!=FPluginItems.constEnd())
    {
      if (!checkDependences(it.key()))
      {
        qDebug() << "UNLOADING PLUGIN: Dependences not exists" << it.value().info->name;
        unloadPlugin(it.key());
        it = FPluginItems.constBegin();
      }
      else if (!checkConflicts(it.key()))
      {
        foreach(QUuid uid, getConflicts(it.key()))
        {
          qDebug() << "UNLOADING PLUGIN: Conflicts with another plugin" << uid;
          unloadPlugin(uid); 
        }
        it = FPluginItems.constBegin();
      }
      else 
        it++;
    }
  }
  else
  {
    qDebug() << "CANT FIND PLUGINS DIRECTORY";
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
      qDebug() << "UNLOADING PLUGIN: Plugin returned false on init" << it.value().info->name;
      unloadPlugin(it.key());
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

void PluginManager::unloadPlugin(const QUuid &AUuid)
{
  if (FPluginItems.contains(AUuid))
  {
    QList<QUuid> unloadList = getDependencesOn(AUuid);
    unloadList.append(AUuid);
    foreach(QUuid uid, unloadList)
    {
      if (FPluginItems.contains(uid))
      {
        PluginItem pluginItem = FPluginItems.take(uid);
        qDebug() << "Unloading plugin:" << pluginItem.info->name;
        if (pluginItem.translator)
          qApp->removeTranslator(pluginItem.translator);
        delete pluginItem.translator;
        delete pluginItem.info;
        delete pluginItem.loader;
      }
    }
    FPlugins.clear();
  }
}

void PluginManager::quit()
{
  QTimer::singleShot(0,qApp,SLOT(quit()));
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

void PluginManager::onAboutToQuit()
{
  emit aboutToQuit();
  foreach(QUuid uid,FPluginItems.keys())
    unloadPlugin(uid);
}

