#ifndef IXMPPSTREAMS_H
#define IXMPPSTREAMS_H

#include <QByteArray>
#include <QStringList>
#include <utils/jid.h>
#include <utils/stanza.h>

#define XMPPSTREAMS_UUID "{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"

class IXmppStream;
class IConnection;

class IXmppDataHandler
{
public:
	virtual bool xmppDataIn(IXmppStream *AXmppStream, QByteArray &AData, int AOrder) =0;
	virtual bool xmppDataOut(IXmppStream *AXmppStream, QByteArray &AData, int AOrder) =0;
};

class IXmppStanzaHadler
{
public:
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder) =0;
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder) =0;
};

class IXmppFeature
{
public:
	virtual QObject *instance() =0;
	virtual QString featureNS() const =0;
	virtual IXmppStream *xmppStream() const =0;
	virtual bool start(const QDomElement &AFeatureElem) =0;
protected:
	virtual void finished(bool ARestart) =0;
	virtual void error(const QString &AError) =0;
	virtual void featureDestroyed() =0;
};

class IXmppFeaturesPlugin
{
public:
	virtual QObject *instance() =0;
	virtual QList<QString> xmppFeatures() const =0;
	virtual IXmppFeature *newXmppFeature(const QString &AFeatureNS, IXmppStream *AXmppStream) =0;
protected:
	virtual void featureCreated(IXmppFeature *AFeature) =0;
	virtual void featureDestroyed(IXmppFeature *AFeature) =0;
};

class IXmppStream
{
public:
	virtual QObject *instance() =0;
	virtual bool open() =0;
	virtual void close() =0;
	virtual void abort(const QString &AError) =0;
	virtual bool isOpen() const =0;
	virtual bool isConnected() const =0;
	virtual QString streamId() const =0;
	virtual QString errorString() const =0;
	virtual Jid streamJid() const=0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual QString password() const =0;
	virtual void setPassword(const QString &APassword) =0;
	virtual QString getSessionPassword(bool AAskIfNeed = true) =0;
	virtual QString defaultLang() const =0;
	virtual void setDefaultLang(const QString &ADefLang) =0;
	virtual bool isEncryptionRequired() const =0;
	virtual void setEncryptionRequired(bool ARequire) =0;
	virtual IConnection *connection() const =0;
	virtual void setConnection(IConnection *AConnection) =0;
	virtual bool isKeepAliveTimerActive() const =0;
	virtual void setKeepAliveTimerActive(bool AActive) =0;
	virtual qint64 sendStanza(Stanza &AStanza) =0;
	virtual void insertXmppDataHandler(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void removeXmppDataHandler(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void insertXmppStanzaHandler(int AOrder, IXmppStanzaHadler *AHandler) =0;
	virtual void removeXmppStanzaHandler(int AOrder, IXmppStanzaHadler *AHandler) =0;
protected:
	virtual void opened() =0;
	virtual void aboutToClose() =0;
	virtual void closed() =0;
	virtual void error(const QString &AError) =0;
	virtual void jidAboutToBeChanged(const Jid &AAfter) =0;
	virtual void jidChanged(const Jid &ABefore) =0;
	virtual void connectionChanged(IConnection *AConnection) =0;
	virtual void dataHandlerInserted(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void dataHandlerRemoved(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void stanzaHandlerInserted(int AOrder, IXmppStanzaHadler *AHandler) =0;
	virtual void stanzaHandlerRemoved(int AOrder, IXmppStanzaHadler *AHandler) =0;
	virtual void streamDestroyed() =0;
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
	virtual QList<QString> xmppFeatures() const = 0;
	virtual void registerXmppFeature(int AOrder, const QString &AFeatureNS) =0;
	virtual QList<IXmppFeaturesPlugin *> xmppFeaturePlugins(const QString &AFeatureNS) const =0;
	virtual void registerXmppFeaturePlugin(int AOrder, const QString &AFeatureNS, IXmppFeaturesPlugin *AFeaturePlugin) =0;
protected:
	virtual void created(IXmppStream *AXmppStream) =0;
	virtual void added(IXmppStream *AXmppStream) =0;
	virtual void opened(IXmppStream *AXmppStream) =0;
	virtual void aboutToClose(IXmppStream *AXmppStream) =0;
	virtual void closed(IXmppStream *AXmppStream) =0;
	virtual void error(IXmppStream *AXmppStream, const QString &AError) =0;
	virtual void jidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter) =0;
	virtual void jidChanged(IXmppStream *AXmppStream, const Jid &ABefore) =0;
	virtual void connectionChanged(IXmppStream *AXmppStream, IConnection *AConnection) =0;
	virtual void removed(IXmppStream *AXmppStream) =0;
	virtual void streamDestroyed(IXmppStream *AXmppStream) =0;
	virtual void xmppFeatureRegistered(int AOrder, const QString &AFeatureNS) =0;
	virtual void xmppFeaturePluginRegistered(int AOrder, const QString &AFeatureNS, IXmppFeaturesPlugin *AFeaturePlugin) =0;
};

Q_DECLARE_INTERFACE(IXmppDataHandler,"Vacuum.Plugin.IXmppDataHandler/1.0");
Q_DECLARE_INTERFACE(IXmppStanzaHadler,"Vacuum.Plugin.IXmppStanzaHadler/1.0");
Q_DECLARE_INTERFACE(IXmppFeature,"Vacuum.Plugin.IXmppFeature/1.0");
Q_DECLARE_INTERFACE(IXmppFeaturesPlugin,"Vacuum.Plugin.IXmppFeaturesPlugin/1.0");
Q_DECLARE_INTERFACE(IXmppStream, "Vacuum.Plugin.IXmppStream/1.2")
Q_DECLARE_INTERFACE(IXmppStreams,"Vacuum.Plugin.IXmppStreams/1.2")

#endif
