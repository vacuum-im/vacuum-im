#ifndef STARTTLSPLUGIN_H
#define STARTTLSPLUGIN_H

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
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStreamFeaturePlugin
  virtual QList<QString> streamFeatures() const { return QList<QString>() << NS_FEATURE_STARTTLS; }
  virtual IStreamFeature *getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
  virtual void destroyStreamFeature(IStreamFeature *AFeature);
signals:
  virtual void featureCreated(IStreamFeature *AStreamFeature);
  virtual void featureDestroyed(IStreamFeature *AStreamFeature);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<IXmppStream *, IStreamFeature *> FFeatures;
};

#endif // STARTTLSPLUGIN_H
