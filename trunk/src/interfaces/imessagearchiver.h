#ifndef IMESSAGEARCHIVER_H
#define IMESSAGEARCHIVER_H

#include <QRegExp>
#include <QMainWindow>
#include <QStandardItemModel>
#include <interfaces/ipluginmanager.h>
#include <utils/toolbarchanger.h>
#include <utils/datetime.h>
#include <utils/message.h>
#include <utils/menu.h>
#include <utils/jid.h>

#define MESSAGEARCHIVER_UUID    "{66FEAE08-BE4D-4fd4-BCEA-494F3A70997A}"

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
	QString save;
	QString otr;
	int expire;
	bool operator==(const IArchiveItemPrefs &AOther) const {
		return save==AOther.save && otr==AOther.otr && expire==AOther.expire;
	}
	bool operator!=(const IArchiveItemPrefs &AOther) const {
		return !operator==(AOther);
	}
};

struct IArchiveStreamPrefs
{
	bool autoSave;
	QString methodAuto;
	QString methodLocal;
	QString methodManual;
	IArchiveItemPrefs defaultPrefs;
	QHash<Jid,IArchiveItemPrefs> itemPrefs;
};

struct IArchiveHeader
{
	IArchiveHeader() { version = 0; }
	Jid with;
	QDateTime start;
	QString subject;
	QString threadId;
	int version;
	bool operator<(const IArchiveHeader &AOther) const {
		return start<AOther.start;
	}
	bool operator==(const IArchiveHeader &AOther) const {
		return with==AOther.with && start==AOther.start;
	}
	bool operator!=(const IArchiveHeader &AOther) const {
		return !operator==(AOther);
	}
};

struct IArchiveCollection
{
	IArchiveHeader header;
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
	enum Action {
		Created,
		Modified,
		Removed
	} action;
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
		threadId = QString::null;
		count = 0x7FFFFFFF;
		order = Qt::AscendingOrder;
	}
	Jid with;
	QDateTime start;
	QDateTime end;
	QString threadId;
	int count;
	Qt::SortOrder order;
};

struct IArchiveFilter
{
	Jid with;
	QDateTime start;
	QDateTime end;
	QString threadId;
	QRegExp body;
};

struct IArchiveResultSet {
	int count;
	int index;
	QString first;
	QString last;
};

class IArchiveHandler
{
public:
	virtual QObject *instance() =0;
	virtual bool archiveMessage(int AOrder, const Jid &AStreamJid, Message &AMessage, bool ADirectionIn) =0;
};

class IArchiveWindow
{
public:
	enum GroupKind {
		GK_NO_GROUPS,
		GK_DATE,
		GK_CONTACT,
		GK_DATE_CONTACT,
		GK_CONTACT_DATE
	};
	enum ArchiveSource {
		AS_LOCAL_ARCHIVE,
		AS_SERVER_ARCHIVE,
		AS_AUTO
	};
public :
	virtual QMainWindow *instance() =0;
	virtual const Jid &streamJid() const =0;
	virtual ToolBarChanger *collectionTools() const =0;
	virtual ToolBarChanger *messagesTools() const =0;
	virtual bool isHeaderAccepted(const IArchiveHeader &AHeader) const =0;
	virtual QList<IArchiveHeader> currentHeaders() const =0;
	virtual QStandardItem *findHeaderItem(const IArchiveHeader &AHeader, QStandardItem *AParent = NULL) const =0;
	virtual int groupKind() const =0;
	virtual void setGroupKind(int AGroupKind) =0;
	virtual int archiveSource() const =0;
	virtual void setArchiveSource(int ASource) =0;
	virtual const IArchiveFilter &filter() const =0;
	virtual void setFilter(const IArchiveFilter &AFilter) =0;
	virtual void reload() =0;
protected:
	virtual void groupKindChanged(int AGroupKind) =0;
	virtual void archiveSourceChanged(int ASource) =0;
	virtual void filterChanged(const IArchiveFilter &AFilter) =0;
	virtual void itemCreated(QStandardItem *AItem) =0;
	virtual void itemContextMenu(QStandardItem *AItem, Menu *AMenu) =0;
	virtual void currentItemChanged(QStandardItem *ACurrent, QStandardItem *ABefour) =0;
	virtual void itemDestroyed(QStandardItem *AItem) =0;
	virtual void windowDestroyed(IArchiveWindow *AWindow) =0;
};

class IMessageArchiver
{
public:
	virtual QObject *instance() =0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual bool isArchivePrefsEnabled(const Jid &AStreamJid) const =0;
	virtual bool isSupported(const Jid &AStreamJid, const QString &AFeatureNS) const =0;
	virtual bool isAutoArchiving(const Jid &AStreamJid) const =0;
	virtual bool isManualArchiving(const Jid &AStreamJid) const =0;
	virtual bool isLocalArchiving(const Jid &AStreamJid) const =0;
	virtual bool isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, int AMessageType) const =0;
	virtual QString methodName(const QString &AMethod) const =0;
	virtual QString otrModeName(const QString &AOTRMode) const =0;
	virtual QString saveModeName(const QString &ASaveMode) const =0;
	virtual QString expireName(int AExpire) const =0;
	virtual IArchiveStreamPrefs archivePrefs(const Jid &AStreamJid) const =0;
	virtual IArchiveItemPrefs archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) const =0;
	virtual QString setArchiveAutoSave(const Jid &AStreamJid, bool AAuto) =0;
	virtual QString setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs) =0;
	virtual QString removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid) =0;
	virtual IArchiveWindow *showArchiveWindow(const Jid &AStreamJid, const IArchiveFilter &AFilter, int AGroupKind, QWidget *AParent = NULL) =0;
	//Archive Handlers
	virtual void insertArchiveHandler(IArchiveHandler *AHandler, int AOrder) =0;
	virtual void removeArchiveHandler(IArchiveHandler *AHandler, int AOrder) =0;
	//Direct Archiving
	virtual bool saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage) =0;
	virtual bool saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId = "") =0;
	//Local Archive
	virtual Jid gateJid(const Jid &AContactJid) const =0;
	virtual QString gateNick(const Jid &AStreamJid, const Jid &AContactJid) const =0;
	virtual QList<Message> findLocalMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest) const =0;
	virtual bool hasLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const =0;
	virtual bool saveLocalCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection, bool AAppend = true) =0;
	virtual IArchiveHeader loadLocalHeader(const Jid &AStreamJid, const Jid &AWith, const QDateTime &AStart) const =0;
	virtual QList<IArchiveHeader> loadLocalHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest) const =0;
	virtual IArchiveCollection loadLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) const =0;
	virtual bool removeLocalCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	virtual IArchiveModifications loadLocalModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount) const =0;
	//Server Archive
	virtual QDateTime replicationPoint(const Jid &AStreamJid) const =0;
	virtual bool isReplicationEnabled(const Jid &AStreamJid) const =0;
	virtual void setReplicationEnabled(const Jid &AStreamJid, bool AEnabled) =0;
	virtual QString saveServerCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection) =0;
	virtual QString loadServerHeaders(const Jid AStreamJid, const IArchiveRequest &ARequest, const QString &AAfter = "") =0;
	virtual QString loadServerCollection(const Jid AStreamJid, const IArchiveHeader &AHeader, const QString &AAfter = "") =0;
	virtual QString removeServerCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest, bool AOpened = false) =0;
	virtual QString loadServerModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &AAfter = "") =0;
protected:
	virtual void archiveAutoSaveChanged(const Jid &AStreamJid, bool AAuto) =0;
	virtual void archivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs) =0;
	virtual void archiveItemPrefsChanged(const Jid &AStreamJid, const Jid &AItemJid, const IArchiveItemPrefs &APrefs) =0;
	virtual void archiveItemPrefsRemoved(const Jid &AStreamJid, const Jid &AItemJid) =0;
	virtual void requestCompleted(const QString &AId) =0;
	virtual void requestFailed(const QString &AId, const QString &AError) =0;
	//Local Archive
	virtual void localCollectionOpened(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	virtual void localCollectionSaved(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	virtual void localCollectionRemoved(const Jid &AStreamJid, const IArchiveHeader &AHeader) =0;
	//Server Archive
	virtual void serverCollectionSaved(const QString &AId, const IArchiveHeader &Aheader) =0;
	virtual void serverHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders, const IArchiveResultSet &AResult) =0;
	virtual void serverCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection,  const IArchiveResultSet &AResult) =0;
	virtual void serverCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest) =0;
	virtual void serverModificationsLoaded(const QString &AId, const IArchiveModifications &AModifs, const IArchiveResultSet &AResult) =0;
	//ArchiveWindow
	virtual void archiveWindowCreated(IArchiveWindow *AWindow) =0;
	virtual void archiveWindowDestroyed(IArchiveWindow *AWindow) =0;
};

Q_DECLARE_INTERFACE(IArchiveHandler,"Vacuum.Plugin.IArchiveHandler/1.0")
Q_DECLARE_INTERFACE(IArchiveWindow,"Vacuum.Plugin.IArchiveWindow/1.0")
Q_DECLARE_INTERFACE(IMessageArchiver,"Vacuum.Plugin.IMessageArchiver/1.0")

#endif
