#ifndef ARCHIVEVIEWWINDOW_H
#define ARCHIVEVIEWWINDOW_H

#include <QDate>
#include <QTimer>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagestylemanager.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/ifilemessagearchive.h>
#include <interfaces/iurlprocessor.h>
#include "ui_archiveviewwindow.h"

enum RequestStatus {
	RequestFinished,
	RequestStarted,
	RequestError
};

struct ArchiveHeader : 
	public IArchiveHeader 
{
	Jid stream;
	bool operator<(const ArchiveHeader &AOther) const {
		return IArchiveHeader::operator!=(AOther) ? IArchiveHeader::operator<(AOther) : stream<AOther.stream;
	}
	bool operator==(const ArchiveHeader &AOther) const {
		return stream==AOther.stream && IArchiveHeader::operator==(AOther);
	}
	bool operator!=(const ArchiveHeader &AOther) const {
		return !operator==(AOther);
	}
};

struct ArchiveCollection : 
	public IArchiveCollection 
{
	ArchiveHeader header;
};

struct ViewOptions {
	bool isGroupChat;
	bool isPrivateChat;
	QString selfName;
	QString senderName;
	QString lastInfo;
	QDateTime lastTime;
	QString lastSubject;
	QString lastSenderId;
	IMessageStyle *style;
};

class SortFilterProxyModel :
	public QSortFilterProxyModel
{
public:
	SortFilterProxyModel(QObject *AParent = NULL);
protected:
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
};

class ArchiveViewWindow : 
	public QMainWindow
{
	Q_OBJECT;
public:
	ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, const QMultiMap<Jid,Jid> &AAddresses, QWidget *AParent = NULL);
	~ArchiveViewWindow();
	QMultiMap<Jid,Jid> addresses() const;
	void setAddresses(const QMultiMap<Jid,Jid> &AAddresses);
protected:
	void initialize(IPluginManager *APluginManager);
	void reset();
protected:
	Jid gatewayJid(const Jid &AContactJid) const;
	bool isConferencePrivateChat(const Jid &AWith) const;
	bool isJidMatched(const Jid &ARequestWith, const Jid &AHeaderWith) const;
	QString contactName(const Jid &AStreamJid, const Jid &AContactJid, bool AShowResource = false) const;
	QList<ArchiveHeader> convertHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders) const;
	ArchiveCollection convertCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection) const;
protected:
	void clearHeaders();
	ArchiveHeader itemHeader(const QStandardItem *AItem) const;
	QList<ArchiveHeader> itemHeaders(const QStandardItem *AItem) const;
	QMultiMap<Jid,Jid> itemAddresses(const QStandardItem *AItem) const;
	QList<ArchiveHeader> itemsHeaders(const QList<QStandardItem *> &AItems) const;
	QStandardItem *findItem(int AType, int ARole, const QVariant &AValue, QStandardItem *AParent) const;
protected:
	QList<QStandardItem *> selectedItems() const;
	QList<QStandardItem *> filterChildItems(const QList<QStandardItem *> &AItems) const;
	QList<QStandardItem *> findStreamItems(const Jid &AStreamJid, QStandardItem *AParent = NULL) const;
	QList<QStandardItem *> findRequestItems(const Jid &AStreamJid, const IArchiveRequest &ARequest, QStandardItem *AParent = NULL) const;
	void removeRequestItems(const Jid &AStreamJid, const IArchiveRequest &ARequest);
protected:
	void setRequestStatus(RequestStatus AStatus, const QString &AMessage);
	void setHeaderStatus(RequestStatus AStatus, const QString &AMessage = QString::null);
	void setMessageStatus(RequestStatus AStatus, const QString &AMessage = QString::null);
protected:
	QStandardItem *createHeaderItem(const ArchiveHeader &AHeader);
	QStandardItem *createParentItem(const ArchiveHeader &AHeader);
	QStandardItem *createDateGroupItem(const QDateTime &ADateTime, QStandardItem *AParent);
	QStandardItem *createContactItem(const Jid &AStreamJid, const Jid &AContactJid, QStandardItem *AParent);
	QStandardItem *createPrivateChatItem(const Jid &AStreamJid, const Jid &AContactJid, QStandardItem *AParent);
	QStandardItem *createMetacontactItem(const Jid &AStreamJid, const IMetaContact &AMeta, QStandardItem *AParent);
protected:
	void clearMessages();
	void processCollectionsLoad();
	ArchiveHeader loadingCollectionHeader() const;
	void showCollection(const ArchiveCollection &ACollection);
	QString showInfo(const ArchiveCollection &ACollection);
	QString showNote(const QString &ANote, const IMessageStyleContentOptions &AOptions);
	QString showMessage(const Message &AMessage, const IMessageStyleContentOptions &AOptions);
protected slots:
	void onArchiveSearchStart();
	void onTextHilightTimerTimeout();
	void onTextVisiblePositionBoundaryChanged();
	void onTextSearchStart();
	void onTextSearchNextClicked();
	void onTextSearchPrevClicked();
protected slots:
	void onSetContactJidByAction();
	void onRemoveCollectionsByAction();
	void onHeaderContextMenuRequested(const QPoint &APos);
	void onPrintConversationsByAction();
	void onExportConversationsByAction();
	void onExportLabelLinkActivated(const QString &ALink);
protected slots:
	void onHeadersRequestTimerTimeout();
	void onHeadersLoadMoreLinkClicked();
	void onCollectionsRequestTimerTimeout();
	void onCollectionsProcessTimerTimeout();
	void onCurrentSelectionChanged(const QItemSelection &ASelected, const QItemSelection &ADeselected);
protected slots:
	void onArchiveRequestFailed(const QString &AId, const XmppError &AError);
	void onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void onArchiveCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
protected slots:
	void onRosterRemoved(IRoster *ARoster);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
private:
	Ui::ArchiveViewWindowClass ui;
private:
	IMessageArchiver *FArchiver;
	IStatusIcons *FStatusIcons;
	IMetaContacts *FMetaContacts;
	IRosterPlugin *FRosterPlugin;
	IUrlProcessor *FUrlProcessor;
	IMessageProcessor *FMessageProcessor;
	IFileMessageArchive *FFileMessageArchive;
	IMessageStyleManager *FMessageStyleManager;
private:
	QLabel *FExportLabel;
	QLabel *FHeaderActionLabel;
	QLabel *FHeadersEmptyLabel;
	QLabel *FMessagesEmptyLabel;
	QStandardItemModel *FModel;
	SortFilterProxyModel *FProxyModel;
private:
	bool FGroupByContact;
	QMultiMap<Jid,Jid> FAddresses;
	QMap<ArchiveHeader,ArchiveCollection> FCollections;
private:
	int FHistoryTime;
	int FHeadersLoaded;
	QWidget *FFocusWidget;
	QTimer FHeadersRequestTimer;
	QMap<QString, Jid> FRemoveRequests;
	QMap<QString, Jid> FHeadersRequests;
private:
	int FSelectedHeaderIndex;
	ViewOptions FViewOptions;
	QTimer FCollectionsRequestTimer;
	QTimer FCollectionsProcessTimer;
	QList<ArchiveHeader> FSelectedHeaders;
	QMap<QString, ArchiveHeader> FCollectionsRequests;
private:
	QTimer FTextHilightTimer;
	bool FArchiveSearchEnabled;
	QMap<int,QTextEdit::ExtraSelection> FSearchResults;
};

#endif // ARCHIVEVIEWWINDOW_H
