#ifndef IXMPPSTREAMMANAGER_H
#define IXMPPSTREAMMANAGER_H

#include <QByteArray>
#include <QStringList>
#include <utils/jid.h>
#include <utils/stanza.h>
#include <utils/xmpperror.h>

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
	virtual void error(const XmppError &AError) =0;
	virtual void featureDestroyed() =0;
};

class IXmppFeatureFactory
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
	virtual void abort(const XmppError &AError) =0;
	virtual bool isOpen() const =0;
	virtual bool isConnected() const =0;
	virtual QString streamId() const =0;
	virtual XmppError error() const =0;
	virtual Jid streamJid() const =0;
	virtual void setStreamJid(const Jid &AStreamJid) =0;
	virtual bool requestPassword() =0;
	virtual QString password() const =0;
	virtual void setPassword(const QString &APassword) =0;
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
	virtual void closed() =0;
	virtual void aboutToClose() =0;
	virtual void error(const XmppError &AError) =0;
	virtual void jidAboutToBeChanged(const Jid &AAfter) =0;
	virtual void jidChanged(const Jid &ABefore) =0;
	virtual void passwordRequested(bool &AWait) =0;
	virtual void passwordProvided(const QString &APassword) =0;
	virtual void connectionChanged(IConnection *AConnection) =0;
	virtual void dataHandlerInserted(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void dataHandlerRemoved(int AOrder, IXmppDataHandler *AHandler) =0;
	virtual void stanzaHandlerInserted(int AOrder, IXmppStanzaHadler *AHandler) =0;
	virtual void stanzaHandlerRemoved(int AOrder, IXmppStanzaHadler *AHandler) =0;
	virtual void streamDestroyed() =0;
};

class IXmppStreamManager
{
public:
	virtual QObject *instance()=0;
	// XmppStreams
	virtual QList<IXmppStream *> xmppStreams() const =0;
	virtual IXmppStream *findXmppStream(const Jid &AStreamJid) const =0;
	virtual IXmppStream *createXmppStream(const Jid &AStreamJid) =0;
	virtual void destroyXmppStream(IXmppStream *AXmppStream) =0;
	virtual bool isXmppStreamActive(IXmppStream *AXmppStream) const =0;
	virtual void setXmppStreamActive(IXmppStream *AXmppStream, bool AActive) =0;
	// XmppFeatures
	virtual QList<QString> xmppFeatures() const = 0;
	virtual void registerXmppFeature(int AOrder, const QString &AFeatureNS) =0;
	virtual QList<IXmppFeatureFactory *> xmppFeatureFactories(const QString &AFeatureNS) const =0;
	virtual void registerXmppFeatureFactory(int AOrder, const QString &AFeatureNS, IXmppFeatureFactory *AFactory) =0;
protected:
	// XmppStreams
	virtual void streamCreated(IXmppStream *AXmppStream) =0;
	virtual void streamOpened(IXmppStream *AXmppStream) =0;
	virtual void streamClosed(IXmppStream *AXmppStream) =0;
	virtual void streamAboutToClose(IXmppStream *AXmppStream) =0;
	virtual void streamError(IXmppStream *AXmppStream, const XmppError &AError) =0;
	virtual void streamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter) =0;
	virtual void streamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore) =0;
	virtual void streamConnectionChanged(IXmppStream *AXmppStream, IConnection *AConnection) =0;
	virtual void streamActiveChanged(IXmppStream *AXmppStream, bool AActive) =0;
	virtual void streamDestroyed(IXmppStream *AXmppStream) =0;
	// XmppFeatures
	virtual void xmppFeatureRegistered(int AOrder, const QString &AFeatureNS) =0;
	virtual void xmppFeatureFactoryRegistered(int AOrder, const QString &AFeatureNS, IXmppFeatureFactory *AFactory) =0;
};

Q_DECLARE_INTERFACE(IXmppDataHandler,"Vacuum.Plugin.IXmppDataHandler/1.0");
Q_DECLARE_INTERFACE(IXmppStanzaHadler,"Vacuum.Plugin.IXmppStanzaHadler/1.0");
Q_DECLARE_INTERFACE(IXmppFeature,"Vacuum.Plugin.IXmppFeature/1.1");
Q_DECLARE_INTERFACE(IXmppFeatureFactory,"Vacuum.Plugin.IXmppFeatureFactory/1.1");
Q_DECLARE_INTERFACE(IXmppStream, "Vacuum.Plugin.IXmppStream/1.4")
Q_DECLARE_INTERFACE(IXmppStreamManager,"Vacuum.Plugin.IXmppStreamManager/1.4")

#endif // IXMPPSTREAMMANAGER_H
