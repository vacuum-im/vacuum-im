#ifndef XMPPSTREAMS_H
#define XMPPSTREAMS_H

#include <definations/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <utils/errorhandler.h>
#include "xmppstream.h"


class XmppStreams :
  public QObject,
  public IPlugin,
  public IXmppStreams
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IXmppStreams);
public:
  XmppStreams();
  ~XmppStreams();
  virtual QObject *instance() {return this;}
  //IPlugin
  virtual QUuid pluginUuid() const { return XMPPSTREAMS_UUID;}
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager * /*APluginManager*/, int &/*AInitOrder*/) { return true; }
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IXmppStreams
  virtual QList<IXmppStream *> xmppStreams() const;
  virtual IXmppStream *xmppStream(const Jid &AStreamJid) const;
  virtual IXmppStream *newXmppStream(const Jid &AStreamJid);
  virtual bool isActive(IXmppStream *AXmppStream) const;
  virtual void addXmppStream(IXmppStream *AXmppStream);
  virtual void removeXmppStream(IXmppStream *AXmppStream);
  virtual void destroyXmppStream(const Jid &AJid);
  virtual IStreamFeaturePlugin *featurePlugin(const QString &AFeatureNS) const;
  virtual void registerFeature(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin);
signals:
  void created(IXmppStream *AXmppStream);
  void added(IXmppStream *AXmppStream);
  void opened(IXmppStream *AXmppStream);
  void element(IXmppStream *AXmppStream, const QDomElement &AElem);
  void consoleElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended);
  void aboutToClose(IXmppStream *AXmppStream);
  void closed(IXmppStream *AXmppStream);
  void error(IXmppStream *AXmppStream, const QString &AError);
  void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
  void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection);
  void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection);
  void featureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature);
  void featureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature);
  void removed(IXmppStream *AXmppStream);
  void streamDestroyed(IXmppStream *AXmppStream);
  void featureRegistered(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin);
protected slots:
  void onStreamOpened();
  void onStreamElement(const QDomElement &AElem);
  void onStreamConsoleElement(const QDomElement &AElem, bool ASended);
  void onStreamAboutToClose();
  void onStreamClosed();
  void onStreamError(const QString &AError);
  void onStreamJidAboutToBeChanged(const Jid &AAfter);
  void onStreamJidChanged(const Jid &ABefour);
  void onStreamConnectionAdded(IConnection *AConnection);
  void onStreamConnectionRemoved(IConnection *AConnection);
  void onStreamFeatureAdded(IStreamFeature *AFeature);
  void onStreamFeatureRemoved(IStreamFeature *AFeature);
  void onStreamDestroyed();
private:
  QList<IXmppStream *> FStreams;
  QList<IXmppStream *> FActiveStreams;
  QHash<QString,IStreamFeaturePlugin *> FFeatures;
};

#endif // XMPPSTREAMS_H
