#ifndef IQAUTH_H
#define IQAUTH_H

#include <QObject>
#include <QDomDocument>
#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"


#define IQAUTH_UUID "{1E3645BC-313F-49e9-BD00-4CC062EE76A7}"
#define NS_FEATURE_IQAUTH "http://jabber.org/features/iq-auth"
#define NS_JABBER_IQ_AUTH "jabber:iq:auth"

class IqAuth : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);

public:
  IqAuth(IXmppStream *AXmppStream);
  ~IqAuth();

  //IStreamFeature
  virtual QObject *instance() { return this; }
  virtual QString name() const { return "auth"; }
  virtual QString nsURI() const { return NS_FEATURE_IQAUTH; }
  virtual IXmppStream *stream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *,Direction) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void finished(bool); 
  virtual void error(const QString &);
protected slots:
  virtual void onStreamClosed(IXmppStream *);
private:
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
};


//IqAuthPlugin
class IqAuthPlugin :
  public QObject,
  public IPlugin,
  public IStreamFeaturePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStreamFeaturePlugin);

public:
  IqAuthPlugin();
  ~IqAuthPlugin();

  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return IQAUTH_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStreamFeaturePlugin
  virtual IStreamFeature *newInstance(IXmppStream *AXmppStream);
protected slots:
  virtual void onStreamAdded(IXmppStream *AXmppStream);
private:
  QObjectCleanupHandler FCleanupHandler;
};

#endif // IQAUTH_H
