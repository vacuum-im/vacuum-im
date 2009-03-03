#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QHash>
#include <QTranslator>
#include <QApplication>
#include <QPluginLoader>
#include "../definations/plugininitorders.h"
#include "../definations/commandline.h"
#include "../interfaces/ipluginmanager.h"

struct PluginItem 
{
  QUuid uid;
  IPlugin *plugin;
  IPluginInfo *info;
  QPluginLoader *loader;
  QTranslator *translator;
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
  virtual QObject *instance() { return this; }
  virtual void restart();
  virtual IPlugin* getPlugin(const QUuid &AUuid) const;
  virtual QList<IPlugin *> getPlugins(const QString &AInterface = "") const;
  virtual const IPluginInfo *getPluginInfo(const QUuid &AUuid) const;
  virtual QList<QUuid> getDependencesOn(const QUuid &AUuid) const;
  virtual QList<QUuid> getDependencesFor(const QUuid &AUuid) const;
public:
  void loadPlugins();
  void initPlugins();
  void startPlugins();
  void unloadPlugin(const QUuid &AUuid);
public slots:
  virtual void quit();
signals:
  virtual void aboutToQuit();
protected:
  bool checkDependences(const QUuid AUuid) const;
  bool checkConflicts(const QUuid AUuid) const;
  QList<QUuid> getConflicts(const QUuid AUuid) const;
protected slots:
  void onAboutToQuit();
private:
  QTranslator *FQtTranslator;
  QHash<QUuid, PluginItem> FPluginItems;
  mutable QMultiHash<QString, IPlugin *> FPlugins;
};

#endif
