#ifndef SASLSESSION_H
#define SASLSESSION_H

#include <QObject>
#include <QDomDocument>
#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"

#define SASLSESSION_UUID "{625B644B-E940-42b7-9DBF-C5B16B4B0616}"
#define NS_FEATURE_SESSION "urn:ietf:params:xml:ns:xmpp-session"

class SASLSession : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);

public:
  SASLSession(IXmppStream *AStream);
  ~SASLSession();

  //IStreamFeature
  virtual QObject *instance() { return this; }
  virtual QString name() const { return "session"; }
  virtual QString nsURI() const { return NS_FEATURE_SESSION; }
  virtual IXmppStream *stream() const { return FStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *,Direction) { return false; }
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
  virtual QUuid getPluginUuid() const { return SASLSESSION_UUID; }
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

#endif // SASLSession_H
