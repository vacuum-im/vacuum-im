#ifndef SERVERMESSAGEARCHIVE_H
#define SERVERMESSAGEARCHIVE_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/iservermessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/istanzaprocessor.h>

struct ResultSet {
	ResultSet() {
		index = 0;
		count = 0;
		hasCount = false;
	}
	bool hasCount;
	quint32 index;
	quint32 count;
	QString first;
	QString last;
};

struct ServerCollectionRequest {
	QString nextRef;
	IArchiveCollection collection;
};

struct ServerModificationsRequest {
	QDateTime start;
	quint32 count;
};

struct LocalHeadersRequest {
	QString id;
	Jid streamJid;
	IArchiveRequest request;
	QList<IArchiveHeader> headers;
};

struct LocalCollectionRequest {
	QString id;
	Jid streamJid;
	IArchiveCollection collection;
};

struct LocalModificationsRequest {
	QString id;
	Jid streamJid;
	quint32 count;
	QDateTime start;
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
	virtual bool initSettings();
	virtual bool startPlugin()  { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual IOptionsWidget *engineSettingsWidget(QWidget *AParent);
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef);
	//IServerMesssageArchive
	virtual QString loadServerHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef = QString::null);
	virtual QString saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ANextRef = QString::null);
	virtual QString loadServerCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader, const QString &ANextRef = QString::null);
	virtual QString loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef = QString::null);
signals:
	//IArchiveEngine
	void capabilitiesChanged(const Jid &AStreamJid);
	void requestFailed(const QString &AId, const XmppError &AError);
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionSaved(const QString &AId, const IArchiveCollection &ACollection);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifications);
	//IServerMesssageArchive
	void serverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const QString &ANextRef);
	void serverCollectionSaved(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef);
	void serverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef);
	void serverModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const QString &ANextRef);
protected:
	ResultSet readResultSetAnswer(const QDomElement &AElem) const;
	void insertResultSetRequest(QDomElement &AElem, const QString &ANextRef, quint32 ALimit, quint32 AMax=0xFFFFFFFF, Qt::SortOrder AOrder=Qt::AscendingOrder) const;
	QString getNextRef(const ResultSet &AResultSet, quint32 ACount, quint32 ALimit, quint32 AMax=0xFFFFFFFF, Qt::SortOrder AOrder=Qt::AscendingOrder) const;
protected slots:
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
protected slots:
	void onServerRequestFailed(const QString &AId, const XmppError &AError);
	void onServerHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const QString &ANextRef);
	void onServerCollectionSaved(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef);
	void onServerCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const QString &ANextRef);
	void onServerModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const QString &ANextRef);
private:
	IMessageArchiver *FArchiver;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid, QString> FNamespaces;
private:
	QMap<QString,IArchiveRequest> FServerLoadHeadersRequests;
	QMap<QString,IArchiveHeader> FServerLoadCollectionRequests;
	QMap<QString,IArchiveRequest> FServerRemoveCollectionsRequests;
	QMap<QString,ServerCollectionRequest> FServerSaveCollectionRequests;
	QMap<QString,ServerModificationsRequest> FServerLoadModificationsRequests;
private:
	QMap<QString,LocalHeadersRequest> FLocalLoadHeadersRequests;
	QMap<QString,LocalCollectionRequest> FLocalSaveCollectionRequests;
	QMap<QString,LocalCollectionRequest> FLocalLoadCollectionRequests;
	QMap<QString,LocalModificationsRequest> FLocalLoadModificationsRequests;
};

#endif // SERVERMESSAGEARCHIVE_H
