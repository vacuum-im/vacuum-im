#ifndef DATASTREAMSPUBLISHER_H
#define DATASTREAMSPUBLISHER_H

#include <QObject>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idatastreamspublisher.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <utils/pluginhelper.h>

class DataStreamsPublisher :
	public QObject,
	public IPlugin,
	public IDataStreamsPublisher,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IDataStreamsPublisher IStanzaHandler IStanzaRequestOwner);
public:
	DataStreamsPublisher();
	~DataStreamsPublisher();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return DATASTREAMSPUBLISHER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings()  { return true; }
	virtual bool startPlugin() { return true; }
	// IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	// IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	// IDataStreamsPublisher
	virtual QList<QString> streams() const;
	virtual IPublicDataStream findStream(const QString &AStreamId) const;
	virtual bool publishStream(const IPublicDataStream &AStream);
	virtual void removeStream(const QString &AStreamId);
	// Published Streams
	virtual QList<IPublicDataStream> readStreams(const QDomElement &AParent) const;
	virtual bool writeStream(const QString &AStreamId, QDomElement &AParent) const;
	// Process Streams
	virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual QString startStream(const Jid &AStreamJid, const Jid &AContactJid, const QString &AStreamId);
	// Stream Handlers
	virtual QList<IPublicDataStreamHandler *> streamHandlers() const;
	virtual void insertStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler);
	virtual void removeStreamHandler(int AOrder, IPublicDataStreamHandler *AHandler);
signals:
	void streamPublished(const IPublicDataStream &AStream);
	void streamRemoved(const IPublicDataStream &AStream);
	void streamStartAccepted(const QString &ARequestId, const QString &ASessionId);
	void streamStartRejected(const QString &ARequestId, const XmppError &AError);
	void streamHandlerInserted(int AOrder, IPublicDataStreamHandler *AHandler);
	void streamHandlerRemoved(int AOrder, IPublicDataStreamHandler *AHandler);
private:
	PluginPointer<IServiceDiscovery> FDiscovery;
	PluginPointer<IStanzaProcessor> FStanzaProcessor;
private:
	int FSHIStreamStart;
	QMap<QString, QString> FStartRequest;
	QMap<QString, IPublicDataStream> FStreams;
	QMultiMap<int, IPublicDataStreamHandler *> FHandlers;
};

#endif // DATASTREAMSPUBLISHER_H
