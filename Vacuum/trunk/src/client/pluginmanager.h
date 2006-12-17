#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QPluginLoader>
#include <interfaces/ipluginmanager.h>

class PluginItem :
  public QObject
{
  Q_OBJECT;

public:
  PluginItem(const QUuid &AUuid, QPluginLoader *ALoader, QObject *parent)
    : QObject(parent) 
  {
    FUuid = AUuid;
    FLoader = ALoader;
    FLoader->setParent(this);
    FInfo = new PluginInfo;
    FPlugin = qobject_cast<IPlugin *>(FLoader->instance());
    FPlugin->getPluginInfo(FInfo);
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
  PluginManager(QObject *parent);
  ~PluginManager();

  //IPluginManager 
  virtual QObject *instance() {return this;}
  virtual void loadPlugins();
  virtual void initPlugins();
  virtual void startPlugins();
  virtual bool unloadPlugin(const QUuid &AUuid);
  virtual QApplication *application();
  virtual IPlugin* getPlugin(const QUuid &uid);
  virtual QList<IPlugin *> getPlugins();
  virtual QList<IPlugin *> getPlugins(const QString &AClassName);
  virtual const PluginInfo *getPluginInfo(const QUuid &AUuid) const;
  virtual QVector<QUuid> getDependencesOn(const QUuid &AUuid);
  virtual QVector<QUuid> getDependencesFor(const QUuid &AUuid);
public slots:
  virtual void quit();
signals:
    virtual void aboutToQuit();
protected:
  PluginItem *getPluginItem(const QUuid &AUuid);
  bool checkDependences(PluginItem *APluginItem);
  bool checkConflicts(PluginItem *APluginItem);
  QList<QUuid> getConflicts(PluginItem *APluginItem);
private:
  QList<PluginItem *> FPluginItems;
};

#endif
