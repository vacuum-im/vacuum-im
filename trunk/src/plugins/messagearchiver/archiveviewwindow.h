#ifndef ARCHIVEVIEWWINDOW_H
#define ARCHIVEVIEWWINDOW_H

#include <QDate>
#include <QTimer>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagestyles.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/iurlprocessor.h>
#include <utils/iconstorage.h>
#include <utils/textmanager.h>
#include <utils/widgetmanager.h>
#include "ui_archiveviewwindow.h"

enum RequestStatus {
	RequestFinished,
	RequestStarted,
	RequestError
};

struct ViewOptions {
	bool isGroupChat;
	bool isPrivateChat;
	QString selfName;
	QString contactName;
	QString lastSenderId;
	QDateTime lastTime;
	IMessageStyle *style;
};

class SortFilterProxyModel :
	public QSortFilterProxyModel
{
public:
	SortFilterProxyModel(QObject *AParent = NULL);
	void startInvalidate();
	void setVisibleInterval(const QDateTime &AStart, const QDateTime &AEnd);
protected:
	virtual bool filterAcceptsRow(int ARow, const QModelIndex &AParent) const;
	virtual bool lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const;
private:
	QDateTime FStart;
	QDateTime FEnd;
	QTimer FInvalidateTimer;
};

class ArchiveViewWindow : 
	public QMainWindow
{
	Q_OBJECT;
public:
	ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, IRoster *ARoster, QWidget *AParent = NULL);
	~ArchiveViewWindow();
	Jid streamJid() const;
	Jid contactJid() const;
	void setContactJid(const Jid &AContactJid);
	QString searchString() const;
	void setSearchString(const QString &AText);
protected:
	void initialize(IPluginManager *APluginManager);
	void reset();
protected:
	QString contactName(const Jid &AContactJid, bool AShowResource = false) const;
	QStandardItem *createContactItem(const Jid &AContactJid);
	QStandardItem *createHeaderItem(const IArchiveHeader &AHeader);
	IArchiveHeader modelIndexHeader(const QModelIndex &AIndex) const;
	bool isConferencePrivateChat(const Jid &AContactJid) const;
	bool isJidMatched(const Jid &ARequested, const Jid &AHeaderJid) const;
	QList<QStandardItem *> findHeaderItems(const IArchiveRequest &ARequest, QStandardItem *AParent = NULL) const;
protected:
	QDate currentPage() const;
	void setRequestStatus(RequestStatus AStatus, const QString &AMessage);
	void setPageStatus(RequestStatus AStatus, const QString &AMessage = QString::null);
	void setMessagesStatus(RequestStatus AStatus, const QString &AMessage = QString::null);
protected:
	void clearMessages();
	void processCollectionsLoad();
	IArchiveHeader currentLoadingHeader() const;
	bool updateHeaders(const IArchiveRequest &ARequest);
	void removeHeaderItems(const IArchiveRequest &ARequest);
	QString showCollectionInfo(const IArchiveCollection &ACollection);
	QString showNote(const QString &ANote, const IMessageContentOptions &AOptions);
	QString showMessage(const Message &AMessage, const IMessageContentOptions &AOptions);
	void showCollection(const IArchiveCollection &ACollection);
protected slots:
	void onHeadersUpdateButtonClicked();
	void onHeadersRequestTimerTimeout();
	void onCurrentPageChanged(int AYear, int AMonth);
protected slots:
	void onCollectionShowTimerTimeout();
	void onCollectionsRequestTimerTimeout();
	void onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &ABefore);
protected slots:
	void onArchiveSearchUpdate();
	void onArchiveSearchChanged(const QString &AText);
	void onTextHilightTimerTimeout();
	void onTextVerticalScrollBarChanged();
	void onTextSearchTimerTimeout();
	void onTextSearchNextClicked();
	void onTextSearchPreviousClicked();
	void onTextSearchCaseSensitivityChanged();
	void onTextSearchTextChanged(const QString &AText);
protected slots:
	void onRemoveCollectionsByAction();
	void onHeaderContextMenuRequested(const QPoint &APos);
protected slots:
	void onArchiveRequestFailed(const QString &AId, const QString &AError);
	void onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
	void onArchiveCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest);
private:
	Ui::ArchiveViewWindow ui;
private:
	IRoster *FRoster;
	IMessageArchiver *FArchiver;
	IStatusIcons *FStatusIcons;
	IUrlProcessor *FUrlProcessor;
	IMessageStyles *FMessageStyles;
	IMessageProcessor *FMessageProcessor;
private:
	QStandardItemModel *FModel;
	SortFilterProxyModel *FProxyModel;
private:
	Jid FContactJid;
	QMap<Jid,QStandardItem *> FContactModelItems;
	QMap<IArchiveHeader,IArchiveCollection> FCollections;
private:
	QString FSearchString;
	QTimer FTextSearchTimer;
	QTimer FTextHilightTimer;
	QMap<int,QTextEdit::ExtraSelection> FSearchResults;
private:
	QList<QDate> FLoadedPages;
	QTimer FHeadersRequestTimer;
	QMap<QString, QDate> FHeadersRequests;
	QMap<QString, IArchiveRequest> FRemoveRequests;
private:
	int FLoadHeaderIndex;
	ViewOptions FViewOptions;
	QTimer FCollectionShowTimer;
	QTimer FCollectionsRequestTimer;
	QList<IArchiveHeader> FCurrentHeaders;
	QMap<QString, IArchiveHeader> FCollectionsRequests;
};

#endif // ARCHIVEVIEWWINDOW_H
