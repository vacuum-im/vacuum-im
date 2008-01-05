#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QHash>
#include <QApplication>
#include <QPluginLoader>
#include <../../interfaces/ipluginmanager.h>

//PluginItem
class PluginItem :
  public QObject
{
  Q_OBJECT;
public:
  PluginItem(const QUuid &AUuid, QPluginLoader *ALoader, QObject *AParent) : QObject(AParent) 
  {
    FUuid = AUuid;
    FLoader = ALoader;
    FLoader->setParent(this);
    FInfo = new PluginInfo;
    FPlugin = qobject_cast<IPlugin *>(FLoader->instance());
    FPlugin->pluginInfo(FInfo);
    FPlugin->instance()->setParent(FLoader);  
  }
  ~PluginItem()
  {
    delete FInfo;
  }
  const QUuid &uuid() { return FUuid; } 
  IPlugin *plugin() { return FPlugin; }
  PluginInfo *info() { return FInfo; }
private:
  QUuid FUuid;
  QPluginLoader *FLoader;
  IPlugin *FPlugin;
  PluginInfo *FInfo;
};


class PluginManager : 
  public QObject, 
  public IPluginManager
{
  Q_OBJECT;
  Q_INTERFACES(IPluginManager);
public:
  PluginManager(QApplication *AParent);
  ~PluginManager();
  //IPluginManager 
  virtual QObject *instance() {return this;}
  virtual IPlugin* getPlugin(const QUuid &uid) const;
  virtual PluginList getPlugins() const;
  virtual PluginList getPlugins(const QString &AInterface) const;
  virtual const PluginInfo *getPluginInfo(const QUuid &AUuid) const;
  virtual QList<QUuid> getDependencesOn(const QUuid &AUuid) const;
  virtual QList<QUuid> getDependencesFor(const QUuid &AUuid) const;
public:
  void loadPlugins();
  void initPlugins();
  void startPlugins();
  bool unloadPlugin(const QUuid &AUuid);
public slots:
  virtual void quit();
signals:
  virtual void aboutToQuit();
protected:
  PluginItem *getPluginItem(const QUuid &AUuid) const;
  bool checkDependences(PluginItem *APluginItem) const;
  bool checkConflicts(PluginItem *APluginItem) const;
  QList<QUuid> getConflicts(PluginItem *APluginItem) const;
protected slots:
  void onAboutToQuit();
private:
  QList<PluginItem *> FPluginItems;
  mutable QHash<QString,PluginList > FPlugins;
};

#endif
