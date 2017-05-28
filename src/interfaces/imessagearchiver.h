#ifndef IMESSAGEARCHIVER_H
#define IMESSAGEARCHIVER_H

#include <QRegExp>
#include <QMainWindow>
#include <QStandardItemModel>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idataforms.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/toolbarchanger.h>
#include <utils/xmpperror.h>
#include <utils/datetime.h>
#include <utils/message.h>
#include <utils/menu.h>
#include <utils/jid.h>

#define MESSAGEARCHIVER_UUID    "{66FEAE08-BE4D-4fd4-BCEA-494F3A70997A}"

#define ARCHIVE_SAVE_ROSTER     "roster"    // messages are archived only if the contact's bare JID is in the user's roster
#define ARCHIVE_SAVE_ALWAYS     "always"    // all messages are archived by default
#define ARCHIVE_SAVE_NEVER      "never"     // messages are never archived by default


struct IArchiveStreamPrefs
{
	IArchiveStreamPrefs() {
		defaults = ARCHIVE_SAVE_ROSTER;
	}
	QSet<Jid> never;
	QSet<Jid> always;
	QString defaults;
	bool operator==(const IArchiveStreamPrefs &AOther) const {
		return never==AOther.never && always==AOther.always && defaults==AOther.defaults;
	}
	bool operator!=(const IArchiveStreamPrefs &AOther) const {
		return !operator==(AOther);
	}
};

struct IArchiveHeader
{
	IArchiveHeader() { 
		version = 0;
	}
	Jid with;
	QDateTime start;
	QString subject;
	QString threadId;
	quint32 version;
	QUuid engineId;
	bool operator<(const IArchiveHeader &AOther) const {
		return start==AOther.start ? with<AOther.with : start<AOther.start;
	}
	bool operator==(const IArchiveHeader &AOther) const {
		return with==AOther.with && start==AOther.start;
	}
	bool operator!=(const IArchiveHeader &AOther) const {
		return !operator==(AOther);
	}
};

struct IArchiveCollectionLink
{
	Jid with;
	QDateTime start;
};

struct IArchiveCollectionBody 
{
	QList<Message> messages;
	QMultiMap<QDateTime,QString> notes;
};

struct IArchiveCollection
{
	IArchiveHeader header;
	IDataForm attributes;
	IArchiveCollectionBody body;
	IArchiveCollectionLink next;
	IArchiveCollectionLink previous;
	bool operator<(const IArchiveCollection &AOther) const {
		return header<AOther.header;
	}
	bool operator==(const IArchiveCollection &AOther) const {
		return header==AOther.header;
	}
};

struct IArchiveModification
{
	enum ModifyAction {
		Changed,
		Removed
	};
	ModifyAction action;
	IArchiveHeader header;
};

struct IArchiveModifications
{
	IArchiveModifications() {
		isValid = false;
	}
	bool isValid;
	QString next;
	QDateTime start;
	QList<IArchiveModification> items;
};

struct IArchiveRequest
{
	IArchiveRequest() {
		openOnly = false;
		exactmatch = false;
		maxItems = 0xFFFFFFFF;
		threadId = QString::null;
		order = Qt::AscendingOrder;
	}
	Jid with;
	QDateTime start;
	QDateTime end;
	QString text;
	quint32 maxItems;
	Qt::SortOrder order;

	// Old params
	bool openOnly;
	bool exactmatch;
	QString threadId;
};

struct IArchiveReply {
	QString nextRef;
	QList<Message> messages;
};

class IArchiveHandler
{
public:
	virtual QObject *instance() =0;
	virtual bool archiveMessageEdit(int AOrder, const Jid &AStreamJid, Message &AMessage, bool ADirectionIn) =0;
};

class IArchiveEngine
{
public:
	enum Capability {
		DirectArchiving     = 0x0001,
		ManualArchiving     = 0x0002,
		AutomaticArchiving  = 0x0004,
		ArchiveManagement   = 0x0008,
		ArchiveReplication  = 0x0010,
		FullTextSearch      = 0x0020
	};
public:
	virtual QObject *instance() =0;
	virtual QUuid engineId() const =0;
	virtual QString engineName() const =0;
	virtual QString engineDescription() const =0;
	virtual IOptionsDialogWidget *engineSettingsWidget(QWidget *AParent) = 0;
	virtual quint32 capabilities(const Jid &AStreamJid = Jid::null) const =0;
	virtual bool isCapable(const Jid &AStreamJid, quint32 ACapability) const =0;
	virtual int capabilityOrder(quint32 ACapability, const Jid &AStreamJid = Jid::null) const =0;
	//Direct Archiving
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn) =0;
	//Archive Management
	virtual QString loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &ANextRef = QString::null) =0;
	//Manual Archiving
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection) =0;
	//Archive Management
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) =0;
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest) =0;
	//Archive Replication
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef) =0;
protected:
	virtual void capabilitiesChanged(const Jid &AStreamJid) =0;
	virtual void requestFailed(const QString &AId, const XmppError &AError) =0;
	virtual void messagesLoaded(const QString &AId, const IArchiveReply &AReply) =0;


	virtual void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders) =0;
	virtual void collectionSaved(const QString &AId, const IArchiveCollection &ACollection) =0;
	virtual void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection) =0;
	virtual void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest) =0;
	virtual void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifications) =0;
};

class IMessageArchiver
{
public:
	virtual QObject *instance() =0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual bool isSupported(const Jid &AStreamJid) const =0;
	virtual QString supportedNamespace(const Jid &AStreamJid) const =0;
	virtual QString archiveDirPath(const Jid &AStreamJid = Jid::null) const =0;
	virtual QWidget *showArchiveWindow(const QMultiMap<Jid,Jid> &AAddresses) =0;
	//Archive Preferences
	virtual bool isPrefsSupported(const Jid &AStreamJid) const =0;
	virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const =0;
	virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs) =0;
	virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid) const =0;
	//Archive Management
	virtual QString loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest) =0;
	virtual QString loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) =0;
	virtual QString loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest) =0;
	//Archive Utilities
	virtual void elementToCollection(const Jid &AStreamJid, const QDomElement &AChatElem, IArchiveCollection &ACollection) const =0;
	virtual void collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem) const =0;
	//Archive Handlers
	virtual void insertArchiveHandler(int AOrder, IArchiveHandler *AHandler) =0;
	virtual void removeArchiveHandler(int AOrder, IArchiveHandler *AHandler) =0;
	//Archive Engines
	virtual quint32 totalCapabilities(const Jid &AStreamJid) const =0;
	virtual QList<IArchiveEngine *> archiveEngines() const =0;
	virtual IArchiveEngine *findArchiveEngine(const QUuid &AId) const =0;
	virtual bool isArchiveEngineEnabled(const QUuid &AId) const =0;
	virtual void setArchiveEngineEnabled(const QUuid &AId, bool AEnabled) =0;
	virtual void registerArchiveEngine(IArchiveEngine *AEngine) =0;
protected:
	//Common Requests
	virtual void requestCompleted(const QString &AId) =0;
	virtual void requestFailed(const QString &AId, const XmppError &AError) =0;
	//Archive Preferences
	virtual void archivePrefsOpened(const Jid &AStreamJid) =0;
	virtual void archivePrefsChanged(const Jid &AStreamJid) =0;
	virtual void archivePrefsClosed(const Jid &AStreamJid) =0;
	//Archive Management
	virtual void messagesLoaded(const QString &AId, const IArchiveCollectionBody &ABody) =0;
	virtual void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders) =0;
	virtual void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection) =0;
	virtual void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest) =0;
	//Engines
	virtual void totalCapabilitiesChanged(const Jid &AStreamJid) =0;
	virtual void archiveEngineRegistered(IArchiveEngine *AEngine) =0;
	virtual void archiveEngineEnableChanged(const QUuid &AId, bool AEnabled) =0;
};

Q_DECLARE_INTERFACE(IArchiveHandler,"Vacuum.Plugin.IArchiveHandler/1.1")
Q_DECLARE_INTERFACE(IArchiveEngine,"Vacuum.Plugin.IArchiveEngine/1.3")
Q_DECLARE_INTERFACE(IMessageArchiver,"Vacuum.Plugin.IMessageArchiver/1.4")

#endif // IMESSAGEARCHIVER_H
