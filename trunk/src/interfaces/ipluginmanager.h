#ifndef IPLUGINMANAGER_H
#define IPLUGINMANAGER_H

#include <QUrl>
#include <QUuid>
#include <QList>
#include <QLocale>
#include <QtPlugin>

class IPluginManager;

struct IPluginInfo 
{
  QString name;
  QString description;
  QString version;
  QString author;
  QUrl homePage;
  QList<QUuid> implements;
  QList<QUuid> dependences;
  QList<QUuid> conflicts;
};

class IPlugin
{
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
  virtual QObject *instance() =0;
  virtual QString homePath() const =0;
  virtual void setHomePath(const QString &APath) =0;
  virtual void setLocale(QLocale::Language ALanguage, QLocale::Country ACountry) =0;
  virtual IPlugin *pluginInstance(const QUuid &AUuid) const =0;
  virtual QList<IPlugin *> pluginInterface(const QString &AInterface = QString::null) const =0;
  virtual const IPluginInfo *pluginInfo(const QUuid &AUuid) const =0;
  virtual QList<QUuid> pluginDependencesOn(const QUuid &AUuid) const =0;
  virtual QList<QUuid> pluginDependencesFor(const QUuid &AUuid) const =0;
public slots:
  virtual void quit() =0;
  virtual void restart() =0;
signals:
  virtual void aboutToQuit() =0;
};

Q_DECLARE_INTERFACE(IPlugin,"Vacuum.Core.IPlugin/1.0")
Q_DECLARE_INTERFACE(IPluginManager,"Vacuum.Core.IPluginManager/1.0")

#endif
