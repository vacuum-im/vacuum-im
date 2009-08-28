#ifndef SASLPLUGIN_H
#define SASLPLUGIN_H

#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "saslauth.h"
#include "saslbind.h"
#include "saslsession.h"

#define SASLAUTH_UUID "{E583F155-BE87-4919-8769-5C87088F0F57}"

class SASLPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);
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
  //IStreamFeaturePlugin
  virtual QList<QString> streamFeatures() const;
  virtual IStreamFeature *newStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
  virtual void destroyStreamFeature(IStreamFeature *AFeature);
signals:
  virtual void featureCreated(IStreamFeature *AStreamFeature);
  virtual void featureDestroyed(IStreamFeature *AStreamFeature);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<IXmppStream *, IStreamFeature *> FAuthFeatures;
  QHash<IXmppStream *, IStreamFeature *> FBindFeatures;
  QHash<IXmppStream *, IStreamFeature *> FSessionFeatures;
};

#endif // SASLPLUGIN_H
