#ifndef STARTTLSPLUGIN_H
#define STARTTLSPLUGIN_H

#include <definations/namespaces.h>
#include <definations/xmppfeatureorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/idefaultconnection.h>
#include "starttls.h"

#define STARTTLS_UUID "{F554544C-0851-4e2a-9158-99191911E468}"

class StartTLSPlugin : 
  public QObject,
  public IPlugin,
  public IXmppFeaturesPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
  StartTLSPlugin();
  ~StartTLSPlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return STARTTLS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IXmppFeaturesPlugin
  virtual QList<QString> xmppFeatures() const { return QList<QString>() << NS_FEATURE_STARTTLS; }
  virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
  void featureCreated(IXmppFeature *AFeature);
  void featureDestroyed(IXmppFeature *AFeature);
protected slots:
  void onFeatureDestroyed();
private:
  IXmppStreams *FXmppStreams;
};

#endif // STARTTLSPLUGIN_H
