#ifndef IQAUTH_H
#define IQAUTH_H

#include <QDomDocument>
#include "../../definations/namespaces.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"
#include "../../utils/jid.h"

#define IQAUTH_UUID "{1E3645BC-313F-49e9-BD00-4CC062EE76A7}"

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
  virtual QString featureNS() const { return NS_FEATURE_IQAUTH; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction ADirection) const;
  virtual bool hookData(QByteArray *,Direction) { return false; }
  virtual bool hookElement(QDomElement *AElem, Direction ADirection);
signals:
  virtual void ready(bool ARestart); 
  virtual void error(const QString &AError);
protected slots:
  virtual void onStreamClosed(IXmppStream *AXmppStream);
private:
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
};

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
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStreamFeaturePlugin
  virtual QList<QString> streamFeatures() const { return QList<QString>() << NS_FEATURE_IQAUTH; }
  virtual IStreamFeature *getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream);
  virtual void destroyStreamFeature(IStreamFeature *AFeature);
signals:
  virtual void featureCreated(IStreamFeature *AStreamFeature);
  virtual void featureDestroyed(IStreamFeature *AStreamFeature);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<IXmppStream *, IStreamFeature *> FFeatures;
};

#endif // IQAUTH_H
