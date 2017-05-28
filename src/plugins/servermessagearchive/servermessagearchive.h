#ifndef SERVERMESSAGEARCHIVE_H
#define SERVERMESSAGEARCHIVE_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/iservermessagearchive.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/idataforms.h>

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

struct LoadMessagesRequest {
	IArchiveRequest request;
	IArchiveReply reply;
};

class ServerMessageArchive : 
	public QObject,
	public IPlugin,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IServerMesssageArchive
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaHandler IStanzaRequestOwner IArchiveEngine IServerMesssageArchive);
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
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IArchiveEngine
	virtual QUuid engineId() const;
	virtual QString engineName() const;
	virtual QString engineDescription() const;
	virtual IOptionsDialogWidget *engineSettingsWidget(QWidget *AParent);
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn);
	virtual QString loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef = QString::null);


	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection);
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader);
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest);
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef);
signals:
	//IArchiveEngine
	void capabilitiesChanged(const Jid &AStreamJid);
	void requestFailed(const QString &AId, const XmppError &AError);
	void messagesLoaded(const QString &AId, const IArchiveReply &AReply);
	
	void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void collectionSaved(const QString &AId, const IArchiveCollection &ACollection);
	void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
	void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifications);
protected:
	ResultSet readResultSetReply(const QDomElement &AElem) const;
	void insertResultSetRequest(QDomElement &AElem, const QString &ANextRef, quint32 ALimit, quint32 AMax=0xFFFFFFFF, Qt::SortOrder AOrder=Qt::AscendingOrder) const;
protected slots:
	void onArchivePrefsOpened(const Jid &AStreamJid);
	void onArchivePrefsClosed(const Jid &AStreamJid);
private:
	IDataForms *FDataForms;
	IMessageArchiver *FArchiver;
	IStanzaProcessor *FStanzaProcessor;
private:
	QMap<Jid,int> FSHIResult;
	QMap<Jid,QString> FNamespaces;
	QMap<QString,LoadMessagesRequest> FLoadMessagesRequests;
};

#endif // SERVERMESSAGEARCHIVE_H
