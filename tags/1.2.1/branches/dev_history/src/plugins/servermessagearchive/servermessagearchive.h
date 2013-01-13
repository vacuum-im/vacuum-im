#ifndef SERVERMESSAGEARCHIVE_H
#define SERVERMESSAGEARCHIVE_H

#include <definitions/namespaces.h>
#include <definitions/archivecapabilityorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iservermessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/istanzaprocessor.h>

struct HeadersRequest {
	QString id;
	Jid streamJid;
	IArchiveRequest request;
	QList<IArchiveHeader> headers;
};

struct CollectionRequest {
	QString id;
	Jid streamJid;
	IArchiveHeader header;
	IArchiveCollection collection;
};

struct ModificationsRequest {
	QString id;
	Jid streamJid;
	QDateTime start;
	int count;
	IArchiveModifications modifications;
};

class ServerMessageArchive : 
	public QObject,
	public IPlugin,
	public IStanzaRequestOwner,
	public IServerMesssageArchive
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaRequestOwner IArchiveEngine IServerMesssageArchive);
public:
	ServerMessageArchive();
	~ServerMessageArchive();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SERVERMESSAGEARCHIVE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings()  { return true; }
	virtual bool startPlugin()  { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount);
	//IServerMesssageArchive
	virtual QString loadServerHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const IArchiveResultSet &AResult = IArchiveResultSet());
	virtual QString loadServerCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const IArchiveResultSet &AResult = IArchiveResultSet());
	virtual QString loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const IArchiveResultSet &AResult = IArchiveResultSet());
signals:
	//IArchiveEngine
	void capabilitiesChanged(const Jid &AStreamJid);
	void requestFailed(const QString &AId, const QString &AError);
	void collectionSaved(const QString &AId, const IArchiveHeader &AHeader);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifs);
	//IServerMesssageArchive
	void serverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult);
	void serverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult);
	void serverModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult);
protected:
	IArchiveResultSet readResultSetAnswer(const QDomElement &AElem) const;
	void insertResultSetRequest(QDomElement &AElem, const IArchiveResultSet &AResult, Qt::SortOrder AOrder, int AMax = 0) const;
protected slots:
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
protected slots:
	void onServerRequestFailed(const QString &AId, const QString &AError);
	void onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult);
	void onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult);
	void onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult);
private:
	IMessageArchiver *FArchiver;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<QString,IArchiveHeader> FSaveRequests;
	QMap<QString,IArchiveRequest> FRemoveRequests;
	QMap<QString,HeadersRequest> FHeadersRequests;
	QMap<QString,CollectionRequest> FCollectionRequests;
	QMap<QString,ModificationsRequest> FModificationsRequests;
private:
	QMap<QString,IArchiveRequest> FServerHeadersRequests;
	QMap<QString,IArchiveHeader> FServerCollectionRequests;
	QMap<QString,QDateTime> FServerModificationsRequests;
private:
	QMap<Jid, QString> FNamespaces;
};

#endif // SERVERMESSAGEARCHIVE_H
