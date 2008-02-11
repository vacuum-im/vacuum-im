#ifndef IXMPPSTREAMS_H
#define IXMPPSTREAMS_H

#include <QStringList>
#include <QByteArray>
#include "../../interfaces/iconnectionmanager.h"
#include "../../utils/jid.h"
#include "../../utils/stanza.h"

#define XMPPSTREAMS_UUID "{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"

class IXmppStream;

class IStreamFeature
{
public:
  enum Direction {
    DirectionIn,
    DirectionOut,
  };
public:
  virtual QObject *instance() =0;
  virtual QString featureNS() const =0;
  virtual IXmppStream *xmppStream() const =0;
  virtual bool start(const QDomElement &AFeatureElem) =0;
  virtual bool needHook(Direction ADirection) const =0;
  virtual bool hookData(QByteArray *AData, Direction ADirection) =0;
  virtual bool hookElement(QDomElement *AElem, Direction ADirection) =0;
signals:
  virtual void ready(bool ARestart) =0;
  virtual void error(const QString &AError) =0;
};

class IStreamFeaturePlugin
{
public:
  virtual QObject *instance() =0;
  virtual QList<QString> streamFeatures() const =0;
  virtual IStreamFeature *getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream) =0;
  virtual void destroyStreamFeature(IStreamFeature *AFeature) =0;
signals:
  virtual void featureCreated(IStreamFeature *AStreamFeature) =0;
  virtual void featureDestroyed(IStreamFeature *AStreamFeature) =0;
};

class IXmppStream
{
public:
  virtual QObject *instance() =0;
  virtual bool isOpen() const =0;
  virtual void open() =0;
  virtual void close() =0;
  virtual qint64 sendStanza(const Stanza &AStanza) =0;
  virtual const Jid &jid() const=0;
  virtual void setJid(const Jid &) =0;
  virtual const QString &streamId() const =0;
  virtual const QString &lastError() const =0;
  virtual const QString &password() const =0;
  virtual void setPassword(const QString &APassword) =0;
  virtual const QString &defaultLang() const =0;
  virtual void setDefaultLang(const QString &ADefLang) =0;
  virtual const QString &xmppVersion() const =0;
  virtual void setXmppVersion(const QString &AVersion) =0;
  virtual IConnection *connection() const =0;
  virtual void setConnection(IConnection *AConnection) =0;
  virtual void addFeature(IStreamFeature *AFeature) =0;
  virtual QList<IStreamFeature *> features() const =0;
  virtual void removeFeature(IStreamFeature *AFeature) =0;
signals:
  virtual void opened(IXmppStream *AXmppStream) =0;
  virtual void element(IXmppStream *AXmppStream, const QDomElement &AElem) =0;
  virtual void aboutToClose(IXmppStream *AXmppStream) =0;
  virtual void error(IXmppStream *AXmppStream, const QString &AMessage) =0;
  virtual void closed(IXmppStream *AXmppStream) =0;
  virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter) =0;
  virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour) =0;
  virtual void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void featureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void featureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void destroyed(IXmppStream *AXmppStream) =0;
};

class IXmppStreams 
{
public:
  virtual QObject *instance()=0;
  virtual IXmppStream *newStream(const Jid &AStreamJid) =0;
  virtual void addStream(IXmppStream *AXmppStream) =0;
  virtual bool isActive(IXmppStream *AXmppStream) const =0; 
  virtual IXmppStream *getStream(const Jid &AStreamJid) const =0;
  virtual QList<IXmppStream *> getStreams() const =0;
  virtual void removeStream(IXmppStream *AXmppStream) =0;
  virtual void destroyStream(const Jid &AStreamJid) =0;
  virtual IStreamFeaturePlugin *featurePlugin(const QString &AFeatureNS) const =0;
  virtual void registerFeature(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin) =0;
signals:
  virtual void created(IXmppStream *AXmppStream) =0;
  virtual void added(IXmppStream *AXmppStream) =0;
  virtual void opened(IXmppStream *AXmppStream) =0;
  virtual void element(IXmppStream *AXmppStream, const QDomElement &AElem) =0;
  virtual void aboutToClose(IXmppStream *AXmppStream) =0;
  virtual void error(IXmppStream *AXmppStream, const QString &AMessage) =0;
  virtual void closed(IXmppStream *AXmppStream) =0;
  virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter) =0;
  virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour) =0;
  virtual void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void featureRegistered(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin) =0;
  virtual void featureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void featureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void removed(IXmppStream *AXmppStream) =0;
  virtual void destroyed(IXmppStream *AXmppStream) =0;
};

Q_DECLARE_INTERFACE(IStreamFeature,"Vacuum.Plugin.IStreamFeature/1.0");
Q_DECLARE_INTERFACE(IStreamFeaturePlugin,"Vacuum.Plugin.IStreamFeaturePlugin/1.0");
Q_DECLARE_INTERFACE(IXmppStream, "Vacuum.Plugin.IXmppStream/1.0")
Q_DECLARE_INTERFACE(IXmppStreams,"Vacuum.Plugin.IXmppStreams/1.0")

#endif
