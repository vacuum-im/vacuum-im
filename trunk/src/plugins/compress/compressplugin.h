#ifndef COMPRESSPLUGIN_H
#define COMPRESSPLUGIN_H

#include <QObject>
#include <QObjectCleanupHandler>
#include <definations/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include "compression.h"

#define COMPRESS_UUID "{061D0687-B954-416d-B690-D1BA7D845D83}"

class CompressPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);
public:
  CompressPlugin();
  ~CompressPlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return COMPRESS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStreamFeaturePlugin
  virtual QList<QString> streamFeatures() const { return QList<QString>() << NS_FEATURE_COMPRESS; }
  virtual IStreamFeature *newStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
  virtual void destroyStreamFeature(IStreamFeature *AFeature);
signals:
  virtual void featureCreated(IStreamFeature *AStreamFeature);
  virtual void featureDestroyed(IStreamFeature *AStreamFeature);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<IXmppStream *, IStreamFeature *> FFeatures;
};

#endif // COMPRESSPLUGIN_H
