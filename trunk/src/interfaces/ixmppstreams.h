#ifndef IXMPPSTREAMS_H
#define IXMPPSTREAMS_H

#include <QByteArray>
#include <QStringList>
#include "../../utils/jid.h"
#include "../../utils/stanza.h"
class IConnection;

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
  virtual bool hookData(QByteArray &AData, Direction ADirection) =0;
  virtual bool hookElement(QDomElement &AElem, Direction ADirection) =0;
signals:
  virtual void ready(bool ARestart) =0;
  virtual void error(const QString &AError) =0;
};

class IStreamFeaturePlugin
{
public:
  virtual QObject *instance() =0;
  virtual QList<QString> streamFeatures() const =0;
  virtual IStreamFeature *newStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream) =0;
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
  virtual void abort(const QString &AError) =0;
  virtual QString streamId() const =0;
  virtual QString errorString() const =0;
  virtual Jid streamJid() const=0;
  virtual void setStreamJid(const Jid &AStreamJid) =0;
  virtual QString password() const =0;
  virtual void setPassword(const QString &APassword) =0;
  virtual QString defaultLang() const =0;
  virtual void setDefaultLang(const QString &ADefLang) =0;
  virtual IConnection *connection() const =0;
  virtual void setConnection(IConnection *AConnection) =0;
  virtual QList<IStreamFeature *> features() const =0;
  virtual void insertFeature(IStreamFeature *AFeature) =0;
  virtual void removeFeature(IStreamFeature *AFeature) =0;
  virtual qint64 sendStanza(const Stanza &AStanza) =0;
signals:
  virtual void opened() =0;
  virtual void element(const QDomElement &AElem) =0;
  virtual void consoleElement(const QDomElement &AElem, bool ADirectionOut) =0;
  virtual void aboutToClose() =0;
  virtual void closed() =0;
  virtual void error(const QString &AError) =0;
  virtual void jidAboutToBeChanged(const Jid &AAfter) =0;
  virtual void jidChanged(const Jid &ABefour) =0;
  virtual void connectionAdded(IConnection *AConnection) =0;
  virtual void connectionRemoved(IConnection *AConnection) =0;
  virtual void featureAdded(IStreamFeature *AFeature) =0;
  virtual void featureRemoved(IStreamFeature *AFeature) =0;
  virtual void destroyed() =0;
};

class IXmppStreams 
{
public:
  virtual QObject *instance()=0;
  virtual QList<IXmppStream *> xmppStreams() const =0;
  virtual IXmppStream *xmppStream(const Jid &AStreamJid) const =0;
  virtual IXmppStream *newXmppStream(const Jid &AStreamJid) =0;
  virtual bool isActive(IXmppStream *AXmppStream) const =0; 
  virtual void addXmppStream(IXmppStream *AXmppStream) =0;
  virtual void removeXmppStream(IXmppStream *AXmppStream) =0;
  virtual void destroyXmppStream(const Jid &AStreamJid) =0;
  virtual IStreamFeaturePlugin *featurePlugin(const QString &AFeatureNS) const =0;
  virtual void registerFeature(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin) =0;
signals:
  virtual void created(IXmppStream *AXmppStream) =0;
  virtual void added(IXmppStream *AXmppStream) =0;
  virtual void opened(IXmppStream *AXmppStream) =0;
  virtual void element(IXmppStream *AXmppStream, const QDomElement &AElem) =0;
  virtual void consoleElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended) =0;
  virtual void aboutToClose(IXmppStream *AXmppStream) =0;
  virtual void closed(IXmppStream *AXmppStream) =0;
  virtual void error(IXmppStream *AXmppStream, const QString &AError) =0;
  virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter) =0;
  virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefour) =0;
  virtual void connectionAdded(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void connectionRemoved(IXmppStream *AXmppStream, IConnection *AConnection) =0;
  virtual void featureAdded(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void featureRemoved(IXmppStream *AXmppStream, IStreamFeature *AFeature) =0;
  virtual void removed(IXmppStream *AXmppStream) =0;
  virtual void destroyed(IXmppStream *AXmppStream) =0;
  virtual void featureRegistered(const QString &AFeatureNS, IStreamFeaturePlugin *AFeaturePlugin) =0;
};

Q_DECLARE_INTERFACE(IStreamFeature,"Vacuum.Plugin.IStreamFeature/1.0");
Q_DECLARE_INTERFACE(IStreamFeaturePlugin,"Vacuum.Plugin.IStreamFeaturePlugin/1.0");
Q_DECLARE_INTERFACE(IXmppStream, "Vacuum.Plugin.IXmppStream/1.0")
Q_DECLARE_INTERFACE(IXmppStreams,"Vacuum.Plugin.IXmppStreams/1.0")

#endif
