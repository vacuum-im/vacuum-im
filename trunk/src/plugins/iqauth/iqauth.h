#ifndef IQAUTH_H
#define IQAUTH_H

#include <QDomDocument>
#include <definations/namespaces.h>
#include <definations/xmppfeatureorders.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/jid.h>

#define IQAUTH_UUID "{1E3645BC-313F-49e9-BD00-4CC062EE76A7}"

class IqAuth : 
  public QObject,
  public IXmppFeature,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppElementHadler);
public:
  IqAuth(IXmppStream *AXmppStream);
  ~IqAuth();
  virtual QObject *instance() { return this; }
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppFeature
  virtual QString featureNS() const { return NS_FEATURE_IQAUTH; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart); 
  void error(const QString &AError);
  void featureDestroyed();
private:
  IXmppStream *FXmppStream;
};

class IqAuthPlugin :
  public QObject,
  public IPlugin,
  public IXmppFeaturesPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IXmppFeaturesPlugin);
public:
  IqAuthPlugin();
  ~IqAuthPlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return IQAUTH_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IXmppFeaturesPlugin
  virtual QList<QString> xmppFeatures() const { return QList<QString>() << NS_FEATURE_IQAUTH; }
  virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
signals:
  virtual void featureCreated(IXmppFeature *AStreamFeature);
  virtual void featureDestroyed(IXmppFeature *AStreamFeature);
protected slots:
  void onFeatureDestroyed();
private:
  IXmppStreams *FXmppStreams;
};

#endif // IQAUTH_H
