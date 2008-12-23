#ifndef IPLUGINMANAGER_H
#define IPLUGINMANAGER_H

#include <QUrl>
#include <QUuid>
#include <QList>
#include <QtPlugin>

struct IPluginInfo 
{
  QUuid uid;
  QString name;
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
  virtual QObject *instance() =0;
  virtual QUuid pluginUuid() const =0;
  virtual void pluginInfo(IPluginInfo *APluginInfo) =0;
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder) =0;
  virtual bool initObjects() =0;
  virtual bool initSettings()=0;
  virtual bool startPlugin() =0;
};

class IPluginManager  
{
public:
  virtual QObject* instance() =0;
  virtual void restart() =0;
  virtual IPlugin* getPlugin(const QUuid &AUuid) const =0;
  virtual QList<IPlugin *> getPlugins(const QString &AInterface = "") const =0;
  virtual const IPluginInfo *getPluginInfo(const QUuid &AUuid) const =0;
  virtual QList<QUuid> getDependencesOn(const QUuid &AUuid) const =0;
  virtual QList<QUuid> getDependencesFor(const QUuid &AUuid) const =0;
public slots:
  virtual void quit() =0;
signals:
  virtual void aboutToQuit() =0;
};

Q_DECLARE_INTERFACE(IPlugin,"Vacuum.Core.IPlugin/1.0")
Q_DECLARE_INTERFACE(IPluginManager,"Vacuum.Core.IPluginManager/1.0")

#endif
