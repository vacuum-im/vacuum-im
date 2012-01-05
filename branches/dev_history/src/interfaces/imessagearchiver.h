#ifndef IMESSAGEARCHIVER_H
#define IMESSAGEARCHIVER_H

#include <QRegExp>
#include <QMainWindow>
#include <QStandardItemModel>
#include <interfaces/ipluginmanager.h>
#include <interfaces/idataforms.h>
#include <utils/toolbarchanger.h>
#include <utils/datetime.h>
#include <utils/message.h>
#include <utils/menu.h>
#include <utils/jid.h>

#define MESSAGEARCHIVER_UUID "{66FEAE08-BE4D-4fd4-BCEA-494F3A70997A}"

#define ARCHIVE_SCOPE_GLOBAL    "global"    //the setting will remain for next streams
#define ARCHIVE_SCOPE_STREAM    "stream"    //the setting is true only until the end of the stream. For next stream, server default value will be used

#define ARCHIVE_OTR_APPROVE     "approve"   //the user MUST explicitly approve off-the-record communication.
#define ARCHIVE_OTR_CONCEDE     "concede"   //communications MAY be off the record if requested by another user.
#define ARCHIVE_OTR_FORBID      "forbid"    //communications MUST NOT be off the record.
#define ARCHIVE_OTR_OPPOSE      "oppose"    //communications SHOULD NOT be off the record even if requested.
#define ARCHIVE_OTR_PREFER      "prefer"    //communications SHOULD be off the record if possible.
#define ARCHIVE_OTR_REQUIRE     "require"   //communications MUST be off the record.

#define ARCHIVE_SAVE_FALSE      "false"     //the saving entity MUST save nothing.
#define ARCHIVE_SAVE_BODY       "body"      //the saving entity SHOULD save only <body/> elements.
#define ARCHIVE_SAVE_MESSAGE    "message"   //the saving entity SHOULD save the full XML content of each <message/> element.
#define ARCHIVE_SAVE_STREAM     "stream"    //the saving entity SHOULD save every byte that passes over the stream in either direction.

#define ARCHIVE_METHOD_CONCEDE  "concede"   //this method MAY be used if no other methods are available.
#define ARCHIVE_METHOD_FORBID   "forbid"    //this method MUST NOT be used.
#define ARCHIVE_METHOD_PREFER   "prefer"    //this method SHOULD be used if available.

struct IArchiveItemPrefs
{
	IArchiveItemPrefs() {
		expire = 0;
		exactmatch = false;
	}
	QString save;
	QString otr;
	quint32 expire;
	bool exactmatch;
	bool operator==(const IArchiveItemPrefs &AOther) const {
		return save==AOther.save && otr==AOther.otr && expire==AOther.expire && exactmatch==AOther.exactmatch;
	}
	bool operator!=(const IArchiveItemPrefs &AOther) const {
		return !operator==(AOther);
	}
};

struct IArchiveStreamPrefs
{
	bool autoSave;
	QString autoScope;
	QString methodAuto;
	QString methodLocal;
	QString methodManual;
	IArchiveItemPrefs defaultPrefs;
	QMap<Jid,IArchiveItemPrefs> itemPrefs;
	QMap<QString,IArchiveItemPrefs> sessionPrefs;
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

struct IArchiveCollection
{
	IArchiveHeader header;
	IDataForm attributes;
	IArchiveCollectionLink next;
	IArchiveCollectionLink previous;
	QList<Message> messages;
	QMultiMap<QDateTime,QString> notes;
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
		Created,
		Modified,
		Removed
	};
	ModifyAction action;
	IArchiveHeader header;
};

struct IArchiveModifications
{
	DateTime startTime;
	DateTime endTime;
	QList<IArchiveModification> items;
};

struct IArchiveRequest
{
	IArchiveRequest() {
		count = -1;
		threadId = QString::null;
		order = Qt::AscendingOrder;
	}
	Jid with;
	qint32 count;
	QDateTime start;
	QDateTime end;
	QString threadId;
	Qt::SortOrder order;
};

struct IArchiveResultSet 
{
	quint32 count;
	quint32 index;
	QString first;
	QString last;
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
	enum Capabilities {
		DirectArchiving     = 0x0001,
		ManualArchiving     = 0x0002,
		AutomaticArchiving  = 0x0004,
		ArchiveManagement   = 0x0008,
		Replication         = 0x0010,
		TextSearch          = 0x0020
	};
public:
	virtual QObject *instance() =0;
	virtual QUuid engineId() const =0;
	virtual QString engineName() const =0;
	virtual QString engineDescription() const =0;
	virtual uint capabilities(const Jid &AStreamJid = Jid::null) const =0;
	virtual bool isCapable(const Jid &AStreamJid, uint ACapability) const =0;
	//DirectArchiving
	virtual bool saveNote(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn) =0;
	virtual bool saveMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn) =0;
	//ManualArchiving
	virtual QString saveCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection) =0;
	virtual QString removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened = false) =0;
	//ArchiveManagement
	virtual QString loadHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter = QString::null) =0;
	virtual QString loadCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter = QString::null) =0;
	//Replication
	virtual QString loadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter = QString::null) =0;
protected:
	virtual void capabilitiesChanged(const Jid &AStreamJid) =0;
	virtual void requestFailed(const QString &AId, const QString &AError) =0;
	virtual void collectionSaved(const QString &AId, const IArchiveHeader &AHeader) =0;
	virtual void collectionsRemoved(const QString &AId, const IArchiveRequest &ARequest) =0;
	virtual void headersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult) =0;
	virtual void collectionLoaded(const QString &AId, const IArchiveCollection &ACollection, const IArchiveResultSet &AResult) =0;
	virtual void modificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult) =0;
};

class IMessageArchiver
{
public:
	virtual QObject *instance() =0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual bool isArchivePrefsEnabled(const Jid &AStreamJid) const =0;
	virtual bool isSupported(const Jid &AStreamJid, const QString &AFeatureNS) const =0;
	virtual bool isAutoArchiving(const Jid &AStreamJid) const =0;
	virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId) const =0;
	virtual QString expireName(int AExpire) const =0;
	virtual QString methodName(const QString &AMethod) const =0;
	virtual QString otrModeName(const QString &AOTRMode) const =0;
	virtual QString saveModeName(const QString &ASaveMode) const =0;
	virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const =0;
	virtual IArchiveItemPrefs archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId = QString::null) const =0;
	virtual QString setArchiveAutoSave(const Jid &AStreamJid, bool AAuto) =0;
	virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs) =0;
	virtual QString removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) =0;
	virtual QString removeArchiveSessionPrefs(const Jid &AStreamJid, const QString &AThreadId) =0;
	//Direct Archiving
	virtual bool saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage) =0;
	virtual bool saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId = QString::null) =0;
	//Archive Utilities
	virtual void elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const =0;
	virtual void collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const =0;
	//Archive Handlers
	virtual void insertArchiveHandler(int AOrder, IArchiveHandler *AHandler) =0;
	virtual void removeArchiveHandler(int AOrder, IArchiveHandler *AHandler) =0;
	//Archive Engines
	virtual QList<IArchiveEngine *> archiveEngines() const =0;
	virtual IArchiveEngine *findArchiveEngine(const QUuid &AId) const =0;
	virtual void registerArchiveEngine(IArchiveEngine *AEngine) =0;
protected:
	virtual void archivePrefsOpened(const Jid &AStreamJid) =0;
	virtual void archivePrefsChanged(const Jid &AStreamJid) =0;
	virtual void archivePrefsClosed(const Jid &AStreamJid) =0;
	virtual void requestCompleted(const QString &AId) =0;
	virtual void requestFailed(const QString &AId, const QString &AError) =0;
};

Q_DECLARE_INTERFACE(IArchiveHandler,"Vacuum.Plugin.IArchiveHandler/1.1")
Q_DECLARE_INTERFACE(IArchiveEngine,"Vacuum.Plugin.IArchiveEngine/1.0")
Q_DECLARE_INTERFACE(IMessageArchiver,"Vacuum.Plugin.IMessageArchiver/2.0")

#endif // IMESSAGEARCHIVER_H
