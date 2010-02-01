#ifndef SASLPLUGIN_H
#define SASLPLUGIN_H

#include <definations/namespaces.h>
#include <definations/xmppfeatureorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include "saslauth.h"
#include "saslbind.h"
#include "saslsession.h"

#define SASLAUTH_UUID "{E583F155-BE87-4919-8769-5C87088F0F57}"

class SASLPlugin : 
  public QObject,
  public IPlugin,
  public IXmppFeaturesPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
  SASLPlugin();
  ~SASLPlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SASLAUTH_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IXmppFeaturesPlugin
  virtual QList<QString> xmppFeatures() const;
  virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
  void featureCreated(IXmppFeature *AFeature);
  void featureDestroyed(IXmppFeature *AFeature);
protected slots:
  void onFeatureDestroyed();
private:
  IXmppStreams *FXmppStreams;
};

#endif // SASLPLUGIN_H
