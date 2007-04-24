#ifndef IPLUGINMANAGER_H
#define IPLUGINMANAGER_H

#include <QApplication>
#include <QtPlugin>
#include <QUuid>
#include <QUrl>
#include <QList>
#include <QIcon>

struct PluginInfo 
{
  QUuid uid;
  QString name;
  QIcon icon;
  QString description;
  QString version;
  QString author;
  QList<QUuid> implements;
  QList<QUuid> dependences;
  QList<QUuid> conflicts;
  QUrl homePage;
};

class IPluginManager;

class IPlugin  {
public:
  virtual QObject *instance()=0;
  virtual QUuid pluginUuid() const=0;
  virtual void pluginInfo(PluginInfo *APluginInfo)=0;
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder)=0;
  virtual bool initObjects()=0;
  virtual bool initSettings()=0;
  virtual bool startPlugin() =0;
};

typedef QList<IPlugin *> PluginList;

class IPluginManager  {
public:
  virtual QObject* instance() =0;
  virtual bool unloadPlugin(const QUuid &) =0;
  virtual QApplication *application() const =0;
  virtual IPlugin* getPlugin(const QUuid &) const=0;
  virtual PluginList getPlugins() const =0;
  virtual PluginList getPlugins(const QString &AInterface) const =0;
  virtual const PluginInfo *getPluginInfo(const QUuid &) const =0;
  virtual QList<QUuid> getDependencesOn(const QUuid &) const =0;
  virtual QList<QUuid> getDependencesFor(const QUuid &) const =0;
public slots:
  virtual void quit() =0;
signals:
  virtual void aboutToQuit() =0;
};

Q_DECLARE_INTERFACE(IPluginManager,"Vacuum.Core.IPluginManager/1.0")
Q_DECLARE_INTERFACE(IPlugin,"Vacuum.Core.IPlugin/1.0")

#endif
