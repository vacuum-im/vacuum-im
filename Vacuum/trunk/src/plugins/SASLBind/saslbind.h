#ifndef SASLBIND_H
#define SASLBIND_H

#include <QObject>
#include <QDomDocument>
#include <QObjectCleanupHandler>
#include "interfaces/ipluginmanager.h"
#include "interfaces/ixmppstreams.h"

#define SASLBIND_UUID "{DED996DA-342A-4da8-B8F3-0CA4EB08D8AF}"
#define NS_FEATURE_BIND "urn:ietf:params:xml:ns:xmpp-bind"

class SASLBind : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT ;
  Q_INTERFACES(IStreamFeature);

public:
  SASLBind(IXmppStream *AStream);
  ~SASLBind();

  //IStreamFeature
  virtual QObject *instance() { return this; }
  virtual QString name() const { return "bind"; }
  virtual QString nsURI() const { return NS_FEATURE_BIND; }
  virtual IXmppStream *stream() const { return FStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook() const { return FNeedHook; }
  virtual bool hookData(QByteArray *) { return false; }
  virtual bool hookElement(QDomElement *AElem);
signals:
  virtual void finished(bool); 
  virtual void error(const QString &);
protected slots:
  void onStreamClosed(IXmppStream *);
private: //interfaces
  IXmppStream *FStream;
private:
  bool FNeedHook;
};


class SASLBindPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);

public:
  SASLBindPlugin();
  virtual ~SASLBindPlugin();

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid getPluginUuid() const { return SASLBIND_UUID; }
  virtual void getPluginInfo(PluginInfo *info);
  virtual bool initPlugin(IPluginManager *);
  virtual bool startPlugin();

  //IStreamFeaturePlugin
  IStreamFeature *newInstance(IXmppStream *AStream);
protected slots:
  void onStreamAdded(IXmppStream *);
private:
  QObjectCleanupHandler FCleanupHandler;
};

#endif // SASLBIND_H
