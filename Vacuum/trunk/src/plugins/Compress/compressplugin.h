#ifndef COMPRESSPLUGIN_H
#define COMPRESSPLUGIN_H

#include <QObject>
#include <QObjectCleanupHandler>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
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
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStreamFeaturePlugin
  virtual IStreamFeature *addFeature(IXmppStream *AXmppStream);
  virtual IStreamFeature *getFeature(const Jid &AStreamJid) const;
  virtual void removeFeature(IXmppStream *AXmppStream);
signals:
  virtual void featureAdded(IStreamFeature *);
  virtual void featureRemoved(IStreamFeature *);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onFeatureDestroyed(QObject *AObject);
private:
  QList<Compression *> FFeatures;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // COMPRESSPLUGIN_H
