#ifndef SASLBIND_H
#define SASLBIND_H

#include <QObject>
#include <QDomDocument>
#include <QObjectCleanupHandler>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"

#define SASLBIND_UUID "{DED996DA-342A-4da8-B8F3-0CA4EB08D8AF}"

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
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *, Direction) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
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
  virtual QUuid pluginUuid() const { return SASLBIND_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStreamFeaturePlugin
  IStreamFeature *newInstance(IXmppStream *AStream);
protected slots:
  void onStreamAdded(IXmppStream *);
private:
  QObjectCleanupHandler FCleanupHandler;
};

#endif // SASLBIND_H
