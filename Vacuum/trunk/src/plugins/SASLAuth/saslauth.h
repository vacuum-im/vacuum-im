#ifndef SASLAUTH_H
#define SASLAUTH_H

#include <QObject>
#include <QObjectCleanupHandler>
#include "interfaces/ipluginmanager.h"
#include "interfaces/ixmppstreams.h"

#define SASLAUTH_UUID "{E583F155-BE87-4919-8769-5C87088F0F57}"
#define NS_FEATURE_SASL "urn:ietf:params:xml:ns:xmpp-sasl"

class SASLAuth :
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);

public:
  SASLAuth(IXmppStream *AStream);
  ~SASLAuth();

  virtual QObject *instance() { return this; }
  virtual QString name() const { return "mechanisms"; }
  virtual QString nsURI() const { return NS_FEATURE_SASL; }
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
  QString FMechanism;
  qint8 chlNumber;
  QString realm, nonce, cnonce, qop, uri;
};


class SASLAuthPlugin : 
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT
  Q_INTERFACES(IPlugin IStreamFeaturePlugin)

public:
  SASLAuthPlugin();
  ~SASLAuthPlugin();

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid getPluginUuid() const { return SASLAUTH_UUID; }
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

#endif // SASLAUTH_H
