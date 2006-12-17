#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QCoreApplication>
#include <QtPlugin>
#include <QUuid>
#include <QUrl>
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
  virtual QUuid getPluginUuid() const=0;
  virtual void getPluginInfo(PluginInfo *info)=0;
  virtual bool initPlugin(IPluginManager *)=0;
  virtual bool startPlugin()=0;
};

Q_DECLARE_INTERFACE(IPlugin,"Vacuum.Core.IPlugin/0.1")

#endif
