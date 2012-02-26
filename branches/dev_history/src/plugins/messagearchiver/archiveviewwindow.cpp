#include "archiveviewwindow.h"

#include <QLocale>

#define PAGE_LOAD_TIMEOUT    500

enum HistoryItemType {
	HIT_CONTACT,
	HIT_HEADER
};

enum HistoryDataRoles {
	HDR_TYPE                = Qt::UserRole+1,
	HDR_CONTACT_JID,
	HDR_HEADER_WITH,
	HDR_HEADER_START,
	HDR_HEADER_SUBJECT,
	HDR_HEADER_THREAD,
	HDR_HEADER_VERSION
};


SortFilterProxyModel::SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FInvalidateTimer.setSingleShot(true);
	connect(&FInvalidateTimer,SIGNAL(timeout()),SLOT(invalidate()));
}

void SortFilterProxyModel::startInvalidate()
{
	FInvalidateTimer.start(0);
}

void SortFilterProxyModel::setVisibleInterval(QDateTime &AStart, QDateTime &AEnd)
{
	if (AStart!=FStart || AEnd!=FEnd)
	{
		FStart = AStart;
		FEnd = AEnd;
		startInvalidate();
	}
}

bool SortFilterProxyModel::filterAcceptsRow(int ARow, const QModelIndex &AParent) const
{
	QModelIndex index = sourceModel()->index(ARow,0,AParent);
	int indexType = index.data(HDR_TYPE).toInt();
	if (indexType == HIT_CONTACT)
	{
		for (int i=0; i<sourceModel()->rowCount(index);i++)
			if (filterAcceptsRow(i,index))
				return true;
		return false;
	}
	else if (indexType == HIT_HEADER)
	{
		if (FStart.isValid() && FEnd.isValid())
		{
			QDateTime date = index.data(HDR_HEADER_START).toDateTime();
			return FStart<=date && date<=FEnd;
		}
	}
	return QSortFilterProxyModel::filterAcceptsRow(ARow,AParent);
}

bool SortFilterProxyModel::lessThan(const QModelIndex &ALeft, const QModelIndex &ARight) const
{
	int leftType = ALeft.data(HDR_TYPE).toInt();
	int rightType = ARight.data(HDR_TYPE).toInt();
	if (leftType == rightType)
	{
		if (leftType == HIT_CONTACT)
		{
			QString leftDisplay = ALeft.data(Qt::DisplayRole).toString();
			QString rightDisplay = ARight.data(Qt::DisplayRole).toString();
			if (sortCaseSensitivity() == Qt::CaseInsensitive)
			{
				leftDisplay = leftDisplay.toLower();
				rightDisplay = rightDisplay.toLower();
			}
			return QString::localeAwareCompare(leftDisplay,rightDisplay) < 0;
		}
		else if (leftType == HIT_HEADER)
		{
			QDateTime leftDate = ALeft.data(HDR_HEADER_START).toDateTime();
			QDateTime rightDate = ARight.data(HDR_HEADER_START).toDateTime();
			return rightDate < leftDate;
		}
		return QSortFilterProxyModel::lessThan(ALeft,ARight);
	}
	return leftType < rightType;
}

ArchiveViewWindow::ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, IRoster *ARoster, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("View History - %1").arg(ARoster->streamJid().bare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_HISTORY_VIEW,0,0,"windowIcon");

	FRoster = ARoster;
	FArchiver = AArchiver;

	FStatusIcons = NULL;
	FMessageStyles = NULL;
	FMessageWidgets = NULL;
	initialize(APluginManager);

	FModel = new QStandardItemModel(this);
	FProxyModel = new SortFilterProxyModel(FModel);
	FProxyModel->setSourceModel(FModel);
	FProxyModel->setDynamicSortFilter(true);
	FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	ui.trvCollections->setModel(FProxyModel);
	ui.trvCollections->header()->setSortIndicator(0,Qt::AscendingOrder);

	FViewWidget = FMessageWidgets!=NULL ? FMessageWidgets->newViewWidget(streamJid(),Jid::null,ui.wdtMessages) : NULL;
	if (FViewWidget)
	{
		ui.wdtMessages->setLayout(new QVBoxLayout);
		ui.wdtMessages->layout()->setMargin(0);
		ui.wdtMessages->layout()->addWidget(FViewWidget->instance());
	}

	FPageRequestTimer.setSingleShot(true);
	connect(&FPageRequestTimer,SIGNAL(timeout()),SLOT(onPageRequestTimerTimeout()));

	ui.clrCalendar->setSelectedDate(QDate::currentDate());
	connect(ui.clrCalendar,SIGNAL(selectionChanged()),SLOT(onCalendarSelectionChanged()));
	connect(ui.clrCalendar,SIGNAL(currentPageChanged(int, int)),SLOT(onCalendarCurrentPageChanged(int, int)));

	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
		SLOT(onArchiveRequestFailed(const QString &, const QString &)));
	connect(FArchiver->instance(),SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
		SLOT(onArchiveHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
	connect(FArchiver->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
		SLOT(onArchiveCollectionLoaded(const QString &, const IArchiveCollection &)));

	if (!restoreGeometry(Options::fileValue("history.archiveview.geometry",streamJid().pBare()).toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(640,640),this));
	restoreState(Options::fileValue("history.archiveview.state",streamJid().pBare()).toByteArray());

	onCalendarCurrentPageChanged(ui.clrCalendar->yearShown(),ui.clrCalendar->monthShown());
	FPageRequestTimer.start(0);
}

ArchiveViewWindow::~ArchiveViewWindow()
{
	Options::setFileValue(saveState(),"history.archiveview.state",streamJid().pBare());
	Options::setFileValue(saveGeometry(),"history.archiveview.geometry",streamJid().pBare());
}

Jid ArchiveViewWindow::streamJid() const
{
	return FRoster->streamJid();
}

Jid ArchiveViewWindow::contactJid() const
{
	return Jid::null;
}

void ArchiveViewWindow::setContactJid(const Jid &AContactJid)
{

}

void ArchiveViewWindow::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
	if (plugin)
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
}

void ArchiveViewWindow::setStatusMessage(const QString &AMessage)
{

}

QString ArchiveViewWindow::contactName(const Jid &AContactJid) const
{
	IRosterItem ritem = FRoster->rosterItem(AContactJid);
	return !ritem.name.isEmpty() ? ritem.name : AContactJid.bare();
}

QStandardItem *ArchiveViewWindow::createContactItem(const Jid &AContactJid)
{
	QStandardItem *item = FContactModelItems.value(AContactJid);
	if (item == NULL)
	{
		item = new QStandardItem(contactName(AContactJid));
		item->setData(HIT_CONTACT,HDR_TYPE);
		item->setData(AContactJid.pBare(),HDR_CONTACT_JID);
		item->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AContactJid,IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
		FContactModelItems.insert(AContactJid,item);
		FModel->appendRow(item);
	}
	return item;
}

QStandardItem *ArchiveViewWindow::createHeaderItem(const IArchiveHeader &AHeader)
{
	QStandardItem *item = new QStandardItem(AHeader.start.toString());
	item->setData(HIT_HEADER,HDR_TYPE);
	item->setData(AHeader.with.pBare(),HDR_HEADER_WITH);
	item->setData(AHeader.start,HDR_HEADER_START);
	item->setData(AHeader.subject,HDR_HEADER_SUBJECT);
	item->setData(AHeader.threadId,HDR_HEADER_THREAD);
	item->setData(AHeader.version,HDR_HEADER_VERSION);
	item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_DATE));

	QString itemToolTip = Qt::escape(AHeader.with.full());
	if (!AHeader.subject.isEmpty())
		itemToolTip += "<hr>" + Qt::escape(AHeader.subject);
	item->setToolTip(itemToolTip);

	QStandardItem *parentItem = createContactItem(AHeader.with.bare());
	parentItem->appendRow(item);

	FProxyModel->startInvalidate();

	return item;
}

void ArchiveViewWindow::onPageRequestTimerTimeout()
{
	QDate start(ui.clrCalendar->yearShown(),ui.clrCalendar->monthShown(),1);
	QDate end = start.addMonths(1);
	if (!FLoadedPages.contains(start))
	{
		IArchiveRequest request;
		request.start = QDateTime(start);
		request.end = QDateTime(end);
		QString requestId = FArchiver->loadHeaders(streamJid(),request);
		if (!requestId.isEmpty())
		{
			FLoadedPages.append(start);
			FHeaderRequests.insert(requestId,start);
		}
		else
		{
			setStatusMessage(tr("Failed to request archive headers"));
		}
	}
}

void ArchiveViewWindow::onCalendarSelectionChanged()
{

}

void ArchiveViewWindow::onCalendarCurrentPageChanged(int AYear, int AMonth)
{
	QDate start(AYear,AMonth,1);
	FProxyModel->setVisibleInterval(QDateTime(start),QDateTime(start.addMonths(1)));
	FPageRequestTimer.start(PAGE_LOAD_TIMEOUT);
}

void ArchiveViewWindow::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FHeaderRequests.contains(AId))
	{
		QDate start = FHeaderRequests.take(AId);
		FLoadedPages.removeAll(start);
		setStatusMessage(tr("Failed to request archive headers"));
	}
}

void ArchiveViewWindow::onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FHeaderRequests.contains(AId))
	{
		FHeaderRequests.remove(AId);
		for (QList<IArchiveHeader>::const_iterator it = AHeaders.constBegin(); it!=AHeaders.constEnd(); it++)
		{
			if (!FCollections.contains(*it) && it->with.isValid() && it->start.isValid())
			{
				IArchiveCollection collection;
				collection.header = *it;
				FCollections.insert(collection.header,collection);
				createHeaderItem(collection.header);
				//insertContact(collection.header.with);
			}
		}
	}
}

void ArchiveViewWindow::onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{

}
