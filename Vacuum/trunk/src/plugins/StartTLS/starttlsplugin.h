#ifndef STARTTLSPLUGIN_H
#define STARTTLSPLUGIN_H

#include <QObject>
#include <QObjectCleanupHandler>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/idefaultconnection.h"
#include "starttls.h"

#define STARTTLS_UUID "{F554544C-0851-4e2a-9158-99191911E468}"

class StartTLSPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);

public:
  StartTLSPlugin();
  ~StartTLSPlugin();

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return STARTTLS_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStreamFeaturePlugin
  virtual IStreamFeature *addFeature(IXmppStream *AXmppStream);
  virtual IStreamFeature *getFeature(const Jid &AStreamJid) const;
  virtual void removeFeature(IXmppStream *AXmppStream);
signals:
  virtual void featureAdded(IStreamFeature *AStreamFeature);
  virtual void featureRemoved(IStreamFeature *AStreamFeature);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onFeatureDestroyed(QObject *AObject);
private:
  IXmppStreams *FXmppStreams;
private:
  QList<StartTLS *> FFeatures;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // STARTTLSPLUGIN_H
