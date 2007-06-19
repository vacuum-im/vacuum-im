#ifndef SASLSESSION_H
#define SASLSESSION_H

#include <QObject>
#include <QDomDocument>
#include <QObjectCleanupHandler>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"

#define SASLSESSION_UUID "{625B644B-E940-42b7-9DBF-C5B16B4B0616}"

class SASLSession : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);

public:
  SASLSession(IXmppStream *AXmppStream);
  ~SASLSession();

  //IStreamFeature
  virtual QObject *instance() { return this; }
  virtual QString name() const { return "session"; }
  virtual QString nsURI() const { return NS_FEATURE_SESSION; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *,Direction) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void finished(bool); 
  virtual void error(const QString &);
protected slots:
  void onStreamClosed(IXmppStream *);
private:
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
};


class SASLSessionPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);

public:
  SASLSessionPlugin();
  ~SASLSessionPlugin();

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SASLSESSION_UUID; }
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
  virtual void featureAdded(IStreamFeature *);
  virtual void featureRemoved(IStreamFeature *);
protected slots:
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onSASLSessionDestroyed(QObject *AObject);
private:
  QList<SASLSession *> FFeatures;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // SASLSession_H
