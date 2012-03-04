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
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
#include "ui_archiveviewwindow.h"

enum HeadersStatus {
	HeadersReady,
	HeadersLoading,
	HeadersLoadError
};


class SortFilterProxyModel :
	public QSortFilterProxyModel
{
public:
	SortFilterProxyModel(QObject *AParent = NULL);
	void startInvalidate();
	void setVisibleInterval(QDateTime &AStart, QDateTime &AEnd);
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
protected:
	void initialize(IPluginManager *APluginManager);
	void reset();
protected:
	QDate currentPage() const;
	QString contactName(const Jid &AContactJid) const;
	QStandardItem *createContactItem(const Jid &AContactJid);
	QStandardItem *createHeaderItem(const IArchiveHeader &AHeader);
protected:
	void setHeadersStatus(HeadersStatus AStatus, const QString &AMessage = QString::null);
protected slots:
	void onPageRequestTimerTimeout();
	void onCurrentPageChanged(int AYear, int AMonth);
	void onArchiveRequestFailed(const QString &AId, const QString &AError);
	void onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders);
	void onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection);
private:
	Ui::ArchiveViewWindow ui;
private:
	IRoster *FRoster;
	IMessageArchiver *FArchiver;
	IStatusIcons *FStatusIcons;
	IMessageStyles *FMessageStyles;
	IMessageWidgets *FMessageWidgets;
private:
	IViewWidget *FViewWidget;
	QStandardItemModel *FModel;
	SortFilterProxyModel *FProxyModel;
private:
	QMap<QString, QDate> FHeaderRequests;
private:
	Jid FContactJid;
	QTimer FPageRequestTimer;
	QList<QDate> FLoadedPages;
	QMap<Jid,QStandardItem *> FContactModelItems;
	QMap<IArchiveHeader,IArchiveCollection> FCollections;
};

#endif // ARCHIVEVIEWWINDOW_H
