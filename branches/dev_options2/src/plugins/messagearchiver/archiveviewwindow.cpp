#include "archiveviewwindow.h"

#include <QLocale>
#include <QCursor>
#include <QPrinter>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrintDialog>
#include <QItemSelectionModel>
#include <QNetworkAccessManager>
#include <definitions/menuicons.h>
#include <definitions/resources.h>
#include <definitions/optionvalues.h>
#include <definitions/messagedataroles.h>
#include <utils/widgetmanager.h>
#include <utils/textmanager.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

#define ADR_HEADER_STREAM            Action::DR_StreamJid
#define ADR_HEADER_WITH              Action::DR_Parametr1
#define ADR_HEADER_START             Action::DR_Parametr2
#define ADR_HEADER_END               Action::DR_Parametr3

#define ADR_EXPORT_AS_HTML           Action::DR_Parametr1

#define MIN_LOAD_HEADERS             50
#define MAX_HILIGHT_ITEMS            10
#define LOAD_COLLECTION_TIMEOUT      100

enum HistoryItemType {
	HIT_CONTACT,
	HIT_DATEGROUP,
	HIT_HEADER
};

enum HistoryDataRoles {
	HDR_TYPE                = Qt::UserRole+1,
	HDR_CONTACT_JID,
	HDR_METACONTACT_ID,
	HDR_DATEGROUP_DATE,
	HDR_HEADER_WITH,
	HDR_HEADER_STREAM,
	HDR_HEADER_START,
	HDR_HEADER_SUBJECT,
	HDR_HEADER_THREAD,
	HDR_HEADER_VERSION,
	HDR_HEADER_ENGINE
};

static const int HistoryTimeCount = 8;
static const int HistoryTime[HistoryTimeCount] = { -1, -3, -6, -12, -24, -36, -48, -60 };

SortFilterProxyModel::SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent)
{

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
		else if (leftType == HIT_DATEGROUP)
		{
			QDate leftDate = ALeft.data(HDR_DATEGROUP_DATE).toDate();
			QDate rightDate = ARight.data(HDR_DATEGROUP_DATE).toDate();
			return leftDate >= rightDate;
		}
		else if (leftType == HIT_HEADER)
		{
			QDateTime leftDate = ALeft.data(HDR_HEADER_START).toDateTime();
			QDateTime rightDate = ARight.data(HDR_HEADER_START).toDateTime();
			return leftDate < rightDate;
		}
		return QSortFilterProxyModel::lessThan(ALeft,ARight);
	}
	return leftType < rightType;
}

ArchiveViewWindow::ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, const QMultiMap<Jid,Jid> &AAddresses, QWidget *AParent) : QMainWindow(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_HISTORY,0,0,"windowIcon");

	FFocusWidget = NULL;
	FArchiver = AArchiver;

	FStatusIcons = NULL;
	FUrlProcessor = NULL;
	FRosterManager = NULL;
	FMetaContacts = NULL;
	FMessageStyleManager = NULL;
	FMessageProcessor = NULL;
	FFileMessageArchive = NULL;
	initialize(APluginManager);

	FHeaderActionLabel = new QLabel(ui.trvHeaders);
	QPalette pal = FHeaderActionLabel->palette();
	pal.setColor(QPalette::Active,QPalette::Window,pal.color(QPalette::Active,QPalette::Base));
	pal.setColor(QPalette::Disabled,QPalette::Window,pal.color(QPalette::Disabled,QPalette::Base));
	FHeaderActionLabel->setAlignment(Qt::AlignCenter);
	FHeaderActionLabel->setAutoFillBackground(true);
	FHeaderActionLabel->setPalette(pal);
	FHeaderActionLabel->setMargin(3);

	FHeadersEmptyLabel = new QLabel(tr("Conversations are not found"),ui.trvHeaders);
	FHeadersEmptyLabel->setAlignment(Qt::AlignCenter);
	FHeadersEmptyLabel->setEnabled(false);
	FHeadersEmptyLabel->setMargin(3);

	QVBoxLayout *headersLayout = new QVBoxLayout(ui.trvHeaders);
	headersLayout->setMargin(0);
	headersLayout->addStretch();
	headersLayout->addWidget(FHeadersEmptyLabel);
	headersLayout->addStretch();
	headersLayout->addWidget(FHeaderActionLabel);

	FMessagesEmptyLabel = new QLabel(tr("Conversation is not selected"),ui.tbrMessages);
	FMessagesEmptyLabel->setAlignment(Qt::AlignCenter);
	FMessagesEmptyLabel->setEnabled(false);
	FMessagesEmptyLabel->setMargin(3);

	QVBoxLayout *messagesLayout = new QVBoxLayout(ui.tbrMessages);
	messagesLayout->setMargin(0);
	messagesLayout->addStretch();
	messagesLayout->addWidget(FMessagesEmptyLabel);
	messagesLayout->addStretch();

	FModel = new QStandardItemModel(this);
	FProxyModel = new SortFilterProxyModel(FModel);
	FProxyModel->setSourceModel(FModel);
	FProxyModel->setDynamicSortFilter(true);
	FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	QFont messagesFont = ui.tbrMessages->font();
	messagesFont.setPointSize(Options::node(OPV_HISTORY_ARCHIVEVIEW_FONTPOINTSIZE).value().toInt());
	ui.tbrMessages->setFont(messagesFont);

	QPalette palette = ui.tbrMessages->palette();
	palette.setColor(QPalette::Inactive,QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
	palette.setColor(QPalette::Inactive,QPalette::HighlightedText,palette.color(QPalette::Active,QPalette::HighlightedText));
	ui.tbrMessages->setPalette(palette);

	ui.tbrMessages->setNetworkAccessManager(FUrlProcessor!=NULL ? FUrlProcessor->networkAccessManager() : new QNetworkAccessManager(ui.tbrMessages));

	ui.trvHeaders->setModel(FProxyModel);
	ui.trvHeaders->setBottomWidget(FHeaderActionLabel);
	ui.trvHeaders->header()->setSortIndicator(0,Qt::AscendingOrder);
	connect(ui.trvHeaders->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
		SLOT(onCurrentSelectionChanged(const QItemSelection &, const QItemSelection &)));
	connect(ui.trvHeaders,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onHeaderContextMenuRequested(const QPoint &)));
	
	FExportLabel = new QLabel(ui.stbStatusBar);
	FExportLabel->setTextFormat(Qt::RichText);
	FExportLabel->setText(QString("<a href='export'>%1</a>").arg(tr("Export")));
	connect(FExportLabel,SIGNAL(linkActivated(const QString &)),SLOT(onExportLabelLinkActivated(const QString &)));
	ui.stbStatusBar->addPermanentWidget(FExportLabel);

	FHeadersRequestTimer.setSingleShot(true);
	connect(&FHeadersRequestTimer,SIGNAL(timeout()),SLOT(onHeadersRequestTimerTimeout()));

	FCollectionsProcessTimer.setSingleShot(true);
	connect(&FCollectionsProcessTimer,SIGNAL(timeout()),SLOT(onCollectionsProcessTimerTimeout()));

	FCollectionsRequestTimer.setSingleShot(true);
	connect(&FCollectionsRequestTimer,SIGNAL(timeout()),SLOT(onCollectionsRequestTimerTimeout()));

	FTextHilightTimer.setSingleShot(true);
	connect(&FTextHilightTimer,SIGNAL(timeout()),SLOT(onTextHilightTimerTimeout()));
	connect(ui.tbrMessages,SIGNAL(visiblePositionBoundaryChanged()),SLOT(onTextVisiblePositionBoundaryChanged()));

	ui.lneArchiveSearch->setStartSearchTimeout(-1);
	ui.lneArchiveSearch->setSelectTextOnFocusEnabled(false);
	connect(ui.lneArchiveSearch,SIGNAL(searchStart()),SLOT(onArchiveSearchStart()));

	ui.lneTextSearch->setPlaceholderText(tr("Search in text"));
	connect(ui.tlbTextSearchPrev,SIGNAL(clicked()),SLOT(onTextSearchPrevClicked()));
	connect(ui.tlbTextSearchNext,SIGNAL(clicked()),SLOT(onTextSearchNextClicked()));
	connect(ui.lneTextSearch,SIGNAL(searchStart()),SLOT(onTextSearchStart()));
	connect(ui.lneTextSearch,SIGNAL(searchNext()),SLOT(onTextSearchNextClicked()));

	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
		SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));
	connect(FArchiver->instance(),SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
		SLOT(onArchiveHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
	connect(FArchiver->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
		SLOT(onArchiveCollectionLoaded(const QString &, const IArchiveCollection &)));
	connect(FArchiver->instance(),SIGNAL(collectionsRemoved(const QString &, const IArchiveRequest &)),
		SLOT(onArchiveCollectionsRemoved(const QString &, const IArchiveRequest &)));

	if (!restoreGeometry(Options::fileValue("history.archiveview.geometry").toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(960,640),this));
	if (!ui.sprSplitter->restoreState(Options::fileValue("history.archiveview.splitter-state").toByteArray()))
		ui.sprSplitter->setSizes(QList<int>() << 50 << 150);
	restoreState(Options::fileValue("history.archiveview.state").toByteArray());
	
	setAddresses(AAddresses);
}

ArchiveViewWindow::~ArchiveViewWindow()
{
	Options::setFileValue(saveState(),"history.archiveview.state");
	Options::setFileValue(saveGeometry(),"history.archiveview.geometry");
	Options::setFileValue(ui.sprSplitter->saveState(),"history.archiveview.splitter-state");
	Options::node(OPV_HISTORY_ARCHIVEVIEW_FONTPOINTSIZE).setValue(ui.tbrMessages->font().pointSize());
}

QMultiMap<Jid,Jid> ArchiveViewWindow::addresses() const
{
	return FAddresses;
}

void ArchiveViewWindow::setAddresses(const QMultiMap<Jid,Jid> &AAddresses)
{
	if (FAddresses != AAddresses)
	{
		FAddresses = AAddresses;

		QStringList namesList;
		for (QMultiMap<Jid,Jid>::const_iterator it=FAddresses.constBegin(); it!=FAddresses.constEnd(); ++it)
		{
			if (!it.value().isEmpty())
				namesList.append(contactName(it.key(),it.value(),isConferencePrivateChat(it.value())));
		}
		namesList = namesList.toSet().toList(); qSort(namesList);
		setWindowTitle(tr("Conversation History") + (!namesList.isEmpty() ? " - " + namesList.join(", ") : QString::null));

		FArchiveSearchEnabled = false;
		foreach(const Jid &streamJid, FAddresses.uniqueKeys())
		{
			if ((FArchiver->totalCapabilities(streamJid) & IArchiveEngine::FullTextSearch) > 0)
			{
				FArchiveSearchEnabled = true;
				break;
			}
		}

		if (!FArchiveSearchEnabled)
		{
			ui.lneArchiveSearch->clear();
			ui.lneArchiveSearch->setPlaceholderText(tr("Search is not supported"));
		}
		else
		{
			ui.lneArchiveSearch->setPlaceholderText(tr("Search in history"));
		}

		reset();
	}
}

void ArchiveViewWindow::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterManager").value(0);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
		if (FRosterManager)
		{
			connect(FRosterManager->instance(),SIGNAL(rosterActiveChanged(IRoster *, bool)),SLOT(onRosterActiveChanged(IRoster *, bool)));
			connect(FRosterManager->instance(),SIGNAL(rosterStreamJidChanged(IRoster *, const Jid &)),SLOT(onRosterStreamJidChanged(IRoster *, const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IMetaContacts").value(0);
	if (plugin)
		FMetaContacts = qobject_cast<IMetaContacts *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	
	plugin = APluginManager->pluginInterface("IMessageStyleManager").value(0,NULL);
	if (plugin)
		FMessageStyleManager = qobject_cast<IMessageStyleManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IUrlProcessor").value(0);
	if (plugin)
		FUrlProcessor = qobject_cast<IUrlProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IFileMessageArchive").value(0);
	if (plugin)
		FFileMessageArchive = qobject_cast<IFileMessageArchive *>(plugin->instance());
}

void ArchiveViewWindow::reset()
{
	clearHeaders();
	clearMessages();

	FHistoryTime = 0;
	FHeadersLoaded = 0;
	FGroupByContact = FAddresses.values().contains(Jid::null);

	FHeadersRequestTimer.start(0);
}

Jid ArchiveViewWindow::gatewayJid(const Jid &AContactJid) const
{
	if (FFileMessageArchive!=NULL && !AContactJid.node().isEmpty())
	{
		QString gateType = FFileMessageArchive->contactGateType(AContactJid);
		if (!gateType.isEmpty())
		{
			Jid gateJid = AContactJid;
			gateJid.setDomain(QString("%1.gateway").arg(gateType));
			return gateJid;
		}
	}
	return AContactJid;
}

bool ArchiveViewWindow::isConferencePrivateChat(const Jid &AWith) const
{
	return !AWith.resource().isEmpty() && AWith.pDomain().startsWith("conference.");
}

bool ArchiveViewWindow::isJidMatched(const Jid &ARequestWith, const Jid &AHeaderWith) const
{
	if (ARequestWith.pBare() != AHeaderWith.pBare())
		return false;
	else if (!ARequestWith.resource().isEmpty() && ARequestWith.pResource()!=AHeaderWith.pResource())
		return false;
	return true;
}

QString ArchiveViewWindow::contactName(const Jid &AStreamJid, const Jid &AContactJid, bool AShowResource) const
{
	IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
	IRosterItem ritem = roster!=NULL ? roster->findItem(AContactJid) : IRosterItem();
	QString name = !ritem.name.isEmpty() ? ritem.name : AContactJid.uBare();
	if (AShowResource && !AContactJid.resource().isEmpty())
		name = name + "/" +AContactJid.resource();
	return name;
}

QList<ArchiveHeader> ArchiveViewWindow::convertHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders) const
{
	QList<ArchiveHeader> headers;
	for (QList<IArchiveHeader>::const_iterator it = AHeaders.constBegin(); it!=AHeaders.constEnd(); ++it)
	{
		ArchiveHeader header;
		header.stream = AStreamJid;
		header.with = it->with;
		header.start = it->start;
		header.subject = it->subject;
		header.threadId = it->threadId;
		header.engineId = it->engineId;
		headers.append(header);
	}
	return headers;
}

ArchiveCollection ArchiveViewWindow::convertCollection(const Jid &AStreamJid, const IArchiveCollection &ACollection) const
{
	ArchiveCollection collection;
	collection.header = convertHeaders(AStreamJid,QList<IArchiveHeader>()<<ACollection.header).value(0);
	collection.body = ACollection.body;
	collection.next = ACollection.next;
	collection.previous = ACollection.previous;
	collection.attributes = ACollection.attributes;
	return collection;
}

void ArchiveViewWindow::clearHeaders()
{
	FModel->clear();
	FCollections.clear();
	FHeadersRequests.clear();
	FCollectionsRequests.clear();
}

ArchiveHeader ArchiveViewWindow::itemHeader(const QStandardItem *AItem) const
{
	ArchiveHeader header;
	if (AItem->data(HDR_TYPE).toInt() == HIT_HEADER)
	{
		header.stream = AItem->data(HDR_HEADER_STREAM).toString();
		header.with = AItem->data(HDR_HEADER_WITH).toString();
		header.start = AItem->data(HDR_HEADER_START).toDateTime();
		header.subject = AItem->data(HDR_HEADER_SUBJECT).toString();
		header.threadId = AItem->data(HDR_HEADER_THREAD).toString();
		header.version = AItem->data(HDR_HEADER_VERSION).toInt();
		header.engineId = AItem->data(HDR_HEADER_ENGINE).toString();
	}
	return header;
}

QList<ArchiveHeader> ArchiveViewWindow::itemHeaders(const QStandardItem *AItem) const
{
	QList<ArchiveHeader> headers;
	if (AItem->data(HDR_TYPE) == HIT_HEADER)
		headers.append(itemHeader(AItem));
	else for (int row=0; row<AItem->rowCount(); row++)
		headers += itemHeaders(AItem->child(row));
	return headers;
}

QMultiMap<Jid,Jid> ArchiveViewWindow::itemAddresses(const QStandardItem *AItem) const
{
	QMultiMap<Jid,Jid> address;
	if (AItem->data(HDR_TYPE).toInt() != HIT_HEADER)
	{
		for (int row=0; row<AItem->rowCount(); row++)
		{
			QMultiMap<Jid,Jid> itemAddress = itemAddresses(AItem->child(row));
			for (QMultiMap<Jid,Jid>::const_iterator it=itemAddress.constBegin(); it!=itemAddress.constEnd(); ++it)
			{
				if (!address.contains(it.key(),it.value()))
					address.insertMulti(it.key(),it.value());
			}
		}
	}
	else
	{
		Jid streamJid = AItem->data(HDR_HEADER_STREAM).toString();
		Jid contactJid = AItem->data(HDR_HEADER_WITH).toString();
		address.insertMulti(streamJid,isConferencePrivateChat(contactJid) ? contactJid : contactJid.bare());
	}
	return address;
}

QList<ArchiveHeader> ArchiveViewWindow::itemsHeaders(const QList<QStandardItem *> &AItems) const
{
	QList<ArchiveHeader> headers;
	foreach(QStandardItem *item, filterChildItems(AItems))
		headers.append(itemHeaders(item));
	return headers;
}

QStandardItem *ArchiveViewWindow::findItem(int AType, int ARole, const QVariant &AValue, QStandardItem *AParent) const
{
	QStandardItem *parent = AParent!=NULL ? AParent : FModel->invisibleRootItem();
	for (int row=0; row<parent->rowCount(); row++)
	{
		QStandardItem *item = parent->child(row);
		if (item->data(HDR_TYPE)==AType && item->data(ARole)==AValue)
			return item;
	}
	return NULL;
}

QList<QStandardItem *> ArchiveViewWindow::selectedItems() const
{
	QList<QStandardItem *> items;
	foreach(const QModelIndex &proxyIndex, ui.trvHeaders->selectionModel()->selectedIndexes())
	{
		QModelIndex sourceIndex = FProxyModel->mapToSource(proxyIndex);
		if (sourceIndex.isValid())
			items.append(FModel->itemFromIndex(sourceIndex));
	}
	return items;
}

QList<QStandardItem *> ArchiveViewWindow::filterChildItems(const QList<QStandardItem *> &AItems) const
{
	QList<QStandardItem *> items;

	QSet<QStandardItem *> acceptParents;
	QSet<QStandardItem *> declineParents;
	foreach(QStandardItem *item, AItems)
	{
		QStandardItem *parentItem = item->parent();
		if (parentItem == NULL)
		{
			items.append(item);
		}
		else if (acceptParents.contains(parentItem))
		{
			items.append(item);
		}
		else if (!declineParents.contains(parentItem))
		{
			QSet<QStandardItem *> parents;

			bool isChild = false;
			while (!isChild && parentItem!=NULL)
			{
				parents += parentItem;
				isChild = AItems.contains(parentItem);
				parentItem = parentItem->parent();
			}

			if (!isChild)
			{
				acceptParents += parents;
				items.append(item);
			}
			else
			{
				declineParents += parents;
			}
		}
	}

	return items;
}

QList<QStandardItem *> ArchiveViewWindow::findStreamItems(const Jid &AStreamJid, QStandardItem *AParent) const
{
	QList<QStandardItem *> items;
	QStandardItem *parent = AParent!=NULL ? AParent : FModel->invisibleRootItem();
	for (int row=0; row<parent->rowCount(); row++)
	{
		QStandardItem *item = parent->child(row);
		if (item->data(HDR_TYPE) == HIT_HEADER)
		{
			if (AStreamJid == item->data(HDR_HEADER_STREAM).toString())
				items.append(item);
		}
		else
		{
			items += findStreamItems(AStreamJid,item);
		}
	}
	return items;
}

QList<QStandardItem *> ArchiveViewWindow::findRequestItems(const Jid &AStreamJid, const IArchiveRequest &ARequest, QStandardItem *AParent) const
{
	QList<QStandardItem *> items;
	QStandardItem *parent = AParent!=NULL ? AParent : FModel->invisibleRootItem();
	for (int row=0; row<parent->rowCount(); row++)
	{
		QStandardItem *item = parent->child(row);
		if (item->data(HDR_TYPE) == HIT_HEADER)
		{
			bool checked = true;
			if (AStreamJid != item->data(HDR_HEADER_STREAM).toString())
				checked = false;
			else if (ARequest.with.isValid() && !isJidMatched(ARequest.with,item->data(HDR_HEADER_WITH).toString()))
				checked = false;
			else if (ARequest.start.isValid() && ARequest.start>item->data(HDR_HEADER_START).toDateTime())
				checked = false;
			else if (ARequest.end.isValid() && ARequest.end<item->data(HDR_HEADER_START).toDateTime())
				checked = false;
			else if (ARequest.start.isValid() && !ARequest.end.isValid() && ARequest.start!=item->data(HDR_HEADER_START).toDateTime())
				checked = false;
			else if (!ARequest.threadId.isEmpty() && ARequest.threadId!=item->data(HDR_HEADER_THREAD).toString())
				checked = false;
			if (checked)
				items.append(item);
		}
		else
		{
			items += findRequestItems(AStreamJid,ARequest,item);
		}
	}
	return items;
}

void ArchiveViewWindow::removeRequestItems(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	foreach(QStandardItem *item, findRequestItems(AStreamJid, ARequest))
	{
		FCollections.remove(itemHeader(item));

		QStandardItem *parentItem = item->parent();
		while (parentItem != NULL)
		{
			if (parentItem->rowCount() > 1)
			{
				parentItem->removeRow(item->row());
				break;
			}
			else
			{
				item = parentItem;
				parentItem = parentItem->parent();
			}
		}

		if (parentItem == NULL)
			qDeleteAll(FModel->takeRow(item->row()));
	}
}

void ArchiveViewWindow::setRequestStatus(RequestStatus AStatus, const QString &AMessage)
{
	Q_UNUSED(AStatus);
	ui.stbStatusBar->showMessage(AMessage);
}

void ArchiveViewWindow::setHeaderStatus(RequestStatus AStatus, const QString &AMessage)
{
	if (AStatus == RequestStarted)
		FFocusWidget = focusWidget();
	else
		FHeadersLoaded = 0;

	ui.trvHeaders->setEnabled(AStatus != RequestStarted);
	ui.wdtArchiveSearch->setEnabled(AStatus!=RequestStarted && FArchiveSearchEnabled);

	FHeaderActionLabel->disconnect(this);
	FHeaderActionLabel->setEnabled(AStatus != RequestStarted);

	FHeadersEmptyLabel->setVisible(AStatus!=RequestStarted && FCollections.isEmpty());

	if (AStatus == RequestFinished)
	{
		if (FFocusWidget)
			FFocusWidget->setFocus(Qt::MouseFocusReason);
		
		if (FHistoryTime < HistoryTimeCount)
			FHeaderActionLabel->setText(QString("<a href='link'>%1</a>").arg(tr("Load more conversations")));
		else
			FHeaderActionLabel->setText(tr("All conversations loaded"));
		connect(FHeaderActionLabel,SIGNAL(linkActivated(QString)),SLOT(onHeadersLoadMoreLinkClicked()));

		if (!FCollections.isEmpty())
			ui.stbStatusBar->showMessage(tr("%n conversation header(s) found",0,FCollections.count()));
		else
			ui.stbStatusBar->showMessage(tr("Conversation headers are not found"));

		ui.trvHeaders->selectionModel()->clearSelection();
		ui.trvHeaders->setCurrentIndex(QModelIndex());
	}
	else if(AStatus == RequestStarted)
	{
		ui.stbStatusBar->showMessage(tr("Loading conversation headers..."));
	}
	else if (AStatus == RequestError)
	{
		if (FFocusWidget)
			FFocusWidget->setFocus(Qt::MouseFocusReason);

		FHeaderActionLabel->setText(QString("<a href='link'>%1</a>").arg(tr("Retry")));
		connect(FHeaderActionLabel,SIGNAL(linkActivated(QString)),SLOT(onHeadersRequestTimerTimeout()));

		ui.stbStatusBar->showMessage(tr("Failed to load conversation headers: %1").arg(AMessage));
	}
}

void ArchiveViewWindow::setMessageStatus(RequestStatus AStatus, const QString &AMessage)
{
	ui.wdtTextSearch->setEnabled(AStatus!=RequestStarted && !ui.tbrMessages->document()->isEmpty());
	FMessagesEmptyLabel->setVisible(AStatus!=RequestStarted && ui.tbrMessages->document()->isEmpty());
	FExportLabel->setVisible(AStatus!=RequestStarted && !ui.tbrMessages->document()->isEmpty());

	if (AStatus == RequestFinished)
	{
		if (FSelectedHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Select conversation to show"));
		else
			ui.stbStatusBar->showMessage(tr("%n conversation(s) shown",0,FSelectedHeaders.count()));
		onTextSearchStart();
	}
	else if(AStatus == RequestStarted)
	{
		if (FSelectedHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Loading conversations..."));
		else
			ui.stbStatusBar->showMessage(tr("Shown %1 of %2 conversations...").arg(FSelectedHeaderIndex+1).arg(FSelectedHeaders.count()));
	}
	else if (AStatus == RequestError)
	{
		ui.stbStatusBar->showMessage(tr("Failed to load conversations: %1").arg(AMessage));
	}
}

QStandardItem *ArchiveViewWindow::createHeaderItem(const ArchiveHeader &AHeader)
{
	QStandardItem *item = new QStandardItem(AHeader.start.toString("dd MMM, ddd"));
	
	item->setData(HIT_HEADER,HDR_TYPE);
	item->setData(AHeader.stream.pFull(),HDR_HEADER_STREAM);
	item->setData(AHeader.with.pFull(),HDR_HEADER_WITH);
	item->setData(AHeader.start,HDR_HEADER_START);
	item->setData(AHeader.subject,HDR_HEADER_SUBJECT);
	item->setData(AHeader.threadId,HDR_HEADER_THREAD);
	item->setData(AHeader.version,HDR_HEADER_VERSION);
	item->setData(AHeader.engineId.toString(),HDR_HEADER_ENGINE);
	item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_DATE));

	QString itemToolTip = Qt::escape(AHeader.with.uFull());
	if (!AHeader.subject.isEmpty())
		itemToolTip += "<hr>" + Qt::escape(AHeader.subject);
	item->setToolTip(itemToolTip);

	createParentItem(AHeader)->appendRow(item);

	return item;
}

QStandardItem *ArchiveViewWindow::createParentItem(const ArchiveHeader &AHeader)
{
	QStandardItem *item = FModel->invisibleRootItem();

	if (FGroupByContact)
	{
		IMetaContact meta = FMetaContacts!=NULL ? FMetaContacts->findMetaContact(AHeader.stream,AHeader.with) : IMetaContact();
		if (!meta.isNull())
			item = createMetacontactItem(AHeader.stream,meta,item);
		else
			item = createContactItem(AHeader.stream,AHeader.with,item);
	}

	if (!FAddresses.contains(AHeader.stream,AHeader.with) && isConferencePrivateChat(AHeader.with))
		item = createPrivateChatItem(AHeader.stream,AHeader.with,item);

	item = createDateGroupItem(AHeader.start,item);

	return item;
}

QStandardItem *ArchiveViewWindow::createDateGroupItem(const QDateTime &ADateTime, QStandardItem *AParent)
{
	QDate date(ADateTime.date().year(),ADateTime.date().month(),1);
	QStandardItem *item = findItem(HIT_DATEGROUP,HDR_DATEGROUP_DATE,date,AParent);
	if (item == NULL)
	{
		item = new QStandardItem(date.toString("MMMM yyyy"));
		item->setData(HIT_DATEGROUP,HDR_TYPE);
		item->setData(date,HDR_DATEGROUP_DATE);
		item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_DATE));
		AParent->appendRow(item);
	}
	return item;
}

QStandardItem *ArchiveViewWindow::createContactItem(const Jid &AStreamJid, const Jid &AContactJid, QStandardItem *AParent)
{
	Jid gateJid = gatewayJid(AContactJid);
	QStandardItem *item = findItem(HIT_CONTACT,HDR_CONTACT_JID,gateJid.pBare(),AParent);
	if (item == NULL)
	{
		item = new QStandardItem(contactName(AStreamJid,AContactJid));
		item->setData(HIT_CONTACT,HDR_TYPE);
		item->setData(gateJid.pBare(),HDR_CONTACT_JID);
		item->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AContactJid,IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
		AParent->appendRow(item);
	}
	return item;
}

QStandardItem * ArchiveViewWindow::createPrivateChatItem(const Jid &AStreamJid, const Jid &AContactJid, QStandardItem *AParent)
{
	Q_UNUSED(AStreamJid);
	QStandardItem *item = findItem(HIT_CONTACT,HDR_CONTACT_JID,AContactJid.pFull(),AParent);
	if (item == NULL)
	{
		item = new QStandardItem(AContactJid.resource());
		item->setData(HIT_CONTACT,HDR_TYPE);
		item->setData(AContactJid.pFull(),HDR_CONTACT_JID);
		item->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AContactJid,IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
		AParent->appendRow(item);
	}
	return item;
}

QStandardItem * ArchiveViewWindow::createMetacontactItem(const Jid &AStreamJid, const IMetaContact &AMeta, QStandardItem *AParent)
{
	Q_UNUSED(AStreamJid);
	QStandardItem *item = findItem(HIT_CONTACT,HDR_METACONTACT_ID,AMeta.id.toString(),AParent);
	if (item == NULL)
	{
		item = new QStandardItem(AMeta.name);
		item->setData(HIT_CONTACT,HDR_TYPE);
		item->setData(AMeta.id.toString(),HDR_METACONTACT_ID);
		item->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AMeta.items.value(0),IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
		AParent->appendRow(item);
	}
	return item;
}

void ArchiveViewWindow::clearMessages()
{
	FSearchResults.clear();
	ui.tbrMessages->clear();
	FSelectedHeaders.clear();
	FSelectedHeaderIndex = 0;
	FCollectionsProcessTimer.stop();
	setMessageStatus(RequestFinished);
}

void ArchiveViewWindow::processCollectionsLoad()
{
	if (FSelectedHeaderIndex < FSelectedHeaders.count())
	{
		ArchiveHeader header = loadingCollectionHeader();
		ArchiveCollection collection = FCollections.value(header);
		if (collection.body.messages.isEmpty() && collection.body.notes.isEmpty())
		{
			QString reqId = FArchiver->loadCollection(header.stream,header);
			if (!reqId.isEmpty())
				FCollectionsRequests.insert(reqId,header);
			else
				setMessageStatus(RequestError,tr("Archive is not accessible"));
		}
		else
		{
			showCollection(collection);
			FCollectionsProcessTimer.start(0);
		}
	}
	else
	{
		setMessageStatus(RequestFinished);
	}
}

ArchiveHeader ArchiveViewWindow::loadingCollectionHeader() const
{
	return FSelectedHeaders.value(FSelectedHeaderIndex);
}

void ArchiveViewWindow::showCollection(const ArchiveCollection &ACollection)
{
	if (FSelectedHeaderIndex == 0)
	{
		ui.tbrMessages->clear();

		FViewOptions.isPrivateChat = isConferencePrivateChat(ACollection.header.with);

		FViewOptions.isGroupChat = false;
		if (!FViewOptions.isPrivateChat)
			for (int i=0; !FViewOptions.isGroupChat && i<ACollection.body.messages.count(); i++)
				FViewOptions.isGroupChat = ACollection.body.messages.at(i).type()==Message::GroupChat;

		if (FMessageStyleManager)
		{
			IMessageStyleOptions soptions = FMessageStyleManager->styleOptions(FViewOptions.isGroupChat ? Message::GroupChat : Message::Chat);
			FViewOptions.style = FViewOptions.isGroupChat ? FMessageStyleManager->styleForOptions(soptions) : NULL;
		}
		else
		{
			FViewOptions.style = NULL;
		}

		FViewOptions.lastInfo = QString::null;
		FViewOptions.lastSubject = QString::null;
	}

	FViewOptions.lastTime = QDateTime();
	FViewOptions.lastSenderId = QString::null;

	if (!FViewOptions.isPrivateChat)
		FViewOptions.senderName = Qt::escape(FMessageStyleManager!=NULL ? FMessageStyleManager->contactName(ACollection.header.stream,ACollection.header.with) : contactName(ACollection.header.stream,ACollection.header.with));
	else
		FViewOptions.senderName = Qt::escape(ACollection.header.with.resource());
	FViewOptions.selfName = Qt::escape(FMessageStyleManager!=NULL ? FMessageStyleManager->contactName(ACollection.header.stream) : ACollection.header.stream.uBare());

	QString html = showInfo(ACollection);

	IMessageStyleContentOptions options;
	QList<Message>::const_iterator messageIt = ACollection.body.messages.constBegin();
	QMultiMap<QDateTime,QString>::const_iterator noteIt = ACollection.body.notes.constBegin();
	while (noteIt!=ACollection.body.notes.constEnd() || messageIt!=ACollection.body.messages.constEnd())
	{
		if (messageIt!=ACollection.body.messages.constEnd() && (noteIt==ACollection.body.notes.constEnd() || messageIt->dateTime()<noteIt.key()))
		{
			int direction = messageIt->data(MDR_MESSAGE_DIRECTION).toInt();
			Jid senderJid = direction==IMessageProcessor::DirectionIn ? messageIt->from() : ACollection.header.stream;

			options.type = IMessageStyleContentOptions::TypeEmpty;
			options.kind = IMessageStyleContentOptions::KindMessage;
			options.senderId = senderJid.full();
			options.time = messageIt->dateTime();
			options.timeFormat = FMessageStyleManager!=NULL ? FMessageStyleManager->timeFormat(options.time,ACollection.header.start) : QString::null;

			if (FViewOptions.isGroupChat)
			{
				options.type |= IMessageStyleContentOptions::TypeGroupchat;
				options.direction = IMessageStyleContentOptions::DirectionIn;
				options.senderName = Qt::escape(!senderJid.resource().isEmpty() ? senderJid.resource() : senderJid.uNode());
				options.senderColor = FViewOptions.style!=NULL ? FViewOptions.style->senderColor(options.senderName) : "blue";
			}
			else if (direction == IMessageProcessor::DirectionIn)
			{
				options.direction = IMessageStyleContentOptions::DirectionIn;
				options.senderName = FViewOptions.senderName;
				options.senderColor = "blue";
			}
			else
			{
				options.direction = IMessageStyleContentOptions::DirectionOut;
				options.senderName = FViewOptions.selfName;
				options.senderColor = "red";
			}

			html += showMessage(*messageIt,options);
			++messageIt;
		}
		else if (noteIt != ACollection.body.notes.constEnd())
		{
			options.kind = IMessageStyleContentOptions::KindStatus;
			options.type = IMessageStyleContentOptions::TypeEmpty;
			options.senderId = QString::null;
			options.senderName = QString::null;
			options.time = noteIt.key();
			options.timeFormat = FMessageStyleManager!=NULL ? FMessageStyleManager->timeFormat(options.time,ACollection.header.start) : QString::null;

			html += showNote(*noteIt,options);
			++noteIt;
		}
	}

	QTextCursor cursor(ui.tbrMessages->document());
	cursor.movePosition(QTextCursor::End);
	cursor.insertHtml(html);

	FSelectedHeaderIndex++;
	setMessageStatus(RequestStarted);
}

QString ArchiveViewWindow::showInfo(const ArchiveCollection &ACollection)
{
	static const QString infoTmpl =
		"<table width='100%' cellpadding='0' cellspacing='0' style='margin-top:10px;'>"
		"  <tr bgcolor='%bgcolor%'>"
		"    <td style='padding-top:5px; padding-bottom:5px; padding-left:15px; padding-right:15px;'><span style='color:darkCyan;'>%info%</span>%subject%</td>"
		"  </tr>"
		"</table>";

	
	QString startDate;
	startDate = ACollection.header.start.toString(QString("dd MMM yyyy hh:mm"));

	QString info;
	QString infoHash = ACollection.header.start.date().toString(Qt::ISODate);
	if (FViewOptions.isPrivateChat)
	{
		QString withName = Qt::escape(ACollection.header.with.resource());
		QString confName = Qt::escape(ACollection.header.with.uBare());
		info = tr("<b>%1</b> with %2 in %3").arg(startDate,withName,confName);
		infoHash += "~"+withName+"~"+confName;
	}
	else if (FViewOptions.isGroupChat)
	{
		QString confName = Qt::escape(ACollection.header.with.uBare());
		info = tr("<b>%1</b> in %2").arg(startDate,confName);
		infoHash += "~"+confName;
	}
	else
	{
		QString withName = Qt::escape(contactName(ACollection.header.stream,ACollection.header.with,true));
		info = tr("<b>%1</b> with %2").arg(startDate,withName);
		infoHash += "~"+withName;
	}

	QString subject;
	if (!ACollection.header.subject.isEmpty() && FViewOptions.lastSubject!=ACollection.header.subject)
	{
		subject += "<br>";
		if (FMessageProcessor)
		{
			Message message;
			message.setBody(ACollection.header.subject);
			QTextDocument doc;
			FMessageProcessor->messageToText(&doc,message);
			subject += TextManager::getDocumentBody(doc);
		}
		else
		{
			subject += Qt::escape(ACollection.header.subject);
		}
		FViewOptions.lastSubject = ACollection.header.subject;
	}
	infoHash += "~"+subject;

	QString html;
	if (FViewOptions.lastInfo != infoHash)
	{
		html = infoTmpl;
		html.replace("%bgcolor%",ui.tbrMessages->palette().color(QPalette::AlternateBase).name());
		html.replace("%info%",info);
		html.replace("%subject%",subject);
		FViewOptions.lastInfo = infoHash;
	}

	return html;
}

QString ArchiveViewWindow::showNote(const QString &ANote, const IMessageStyleContentOptions &AOptions)
{
	static const QString statusTmpl =
		"<table width='100%' cellpadding='0' cellspacing='0' style='margin-top:5px;'>"
		"  <tr>"
		"    <td width='3%'>***&nbsp;</td>"
		"    <td style='white-space:pre-wrap; color:darkgreen;'>%message%</td>"
		"    <td width='5%' align='right' style='white-space:nowrap; font-size:small; color:gray;'>[%time%]</td>"
		"  </tr>"
		"</table>";

	FViewOptions.lastTime = AOptions.time;
	FViewOptions.lastSenderId = AOptions.senderId;

	QString html = statusTmpl;
	html.replace("%time%",AOptions.time.toString(AOptions.timeFormat));
	html.replace("%message%",Qt::escape(ANote));

	return html;
}

QString ArchiveViewWindow::showMessage(const Message &AMessage, const IMessageStyleContentOptions &AOptions)
{
	QString html;
	bool meMessage = false;
	if (!AOptions.senderName.isEmpty() && AMessage.body().startsWith("/me "))
	{
		static const QString meMessageTmpl =
			"<table width='100%' cellpadding='0' cellspacing='0' style='margin-top:5px;'>"
			"  <tr>"
			"    <td style='padding-left:10px; white-space:pre-wrap;'><b><i>*&nbsp;<span style='color:%senderColor%;'>%sender%</span></i></b>&nbsp;%message%</td>"
			"    <td width='5%' align='right' style='white-space:nowrap; font-size:small; color:gray;'>[%time%]</td>"
			"  </tr>"
			"</table>";
		meMessage = true;
		html = meMessageTmpl;
	}
	else if (AOptions.senderId.isEmpty() || AOptions.senderId!=FViewOptions.lastSenderId || qAbs(FViewOptions.lastTime.secsTo(AOptions.time))>2*60)
	{
		static const QString firstMessageTmpl =
			"<table width='100%' cellpadding='0' cellspacing='0' style='margin-top:5px;'>"
			"  <tr>"
			"    <td style='color:%senderColor%; white-space:nowrap; font-weight:bold;'>%sender%</td>"
			"    <td width='5%' align='right' style='white-space:nowrap; font-size:small; color:gray;'>[%time%]</td>"
			"  </tr>"
			"  <tr>"
			"    <td colspan='2' style='padding-left:10px; white-space:pre-wrap;'>%message%</td>"
			"  </tr>"
			"</table>";
		html = firstMessageTmpl;
	}
	else
	{
		static const QString nextMessageTmpl =
			"<table width='100%' cellpadding='0' cellspacing='0'>"
			"  <tr>"
			"    <td style='padding-left:10px; white-space:pre-wrap;'>%message%</td>"
			"  </tr>"
			"</table>";
		html = nextMessageTmpl;
	}

	FViewOptions.lastTime = AOptions.time;
	FViewOptions.lastSenderId = AOptions.senderId;

	html.replace("%sender%",AOptions.senderName);
	html.replace("%senderColor%",AOptions.senderColor);
	html.replace("%time%",AOptions.time.toString(AOptions.timeFormat));

	QTextDocument doc;
	if (FMessageProcessor)
		FMessageProcessor->messageToText(&doc,AMessage);
	else
		doc.setPlainText(AMessage.body());

	if (meMessage)
	{
		QTextCursor cursor(&doc);
		cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::KeepAnchor,4);
		if (cursor.selectedText() == "/me ")
			cursor.removeSelectedText();
	}

	html.replace("%message%",TextManager::getDocumentBody(doc));

	return html;
}

void ArchiveViewWindow::onArchiveSearchStart()
{
	ui.lneTextSearch->setText(ui.lneArchiveSearch->text());
	reset();
}

void ArchiveViewWindow::onTextHilightTimerTimeout()
{
	if (FSearchResults.count() > MAX_HILIGHT_ITEMS)
	{
		QList<QTextEdit::ExtraSelection> selections;
		QPair<int,int> boundary = ui.tbrMessages->visiblePositionBoundary();
		for (QMap<int, QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.lowerBound(boundary.first); it!=FSearchResults.constEnd() && it.key()<boundary.second; ++it)
			selections.append(it.value());
		ui.tbrMessages->setExtraSelections(selections);
	}
	else
	{
		ui.tbrMessages->setExtraSelections(FSearchResults.values());
	}
}

void ArchiveViewWindow::onTextVisiblePositionBoundaryChanged()
{
	FTextHilightTimer.start(0);
}

void ArchiveViewWindow::onTextSearchStart()
{
	FSearchResults.clear();
	if (!ui.lneTextSearch->text().isEmpty())
	{
		QTextDocument::FindFlags options = (QTextDocument::FindFlag)0;
		QTextCursor cursor(ui.tbrMessages->document());
		do {
			cursor = ui.tbrMessages->document()->find(ui.lneTextSearch->text(),cursor,options);
			if (!cursor.isNull())
			{
				QTextEdit::ExtraSelection selection;
				selection.cursor = cursor;
				selection.format = cursor.charFormat();
				selection.format.setBackground(Qt::yellow);
				FSearchResults.insert(cursor.position(),selection);
				cursor.clearSelection();
			}
		} while (!cursor.isNull());
	}
	else
	{
		ui.lblTextSearchInfo->clear();
	}

	if (!FSearchResults.isEmpty())
	{
		ui.tbrMessages->setTextCursor(FSearchResults.lowerBound(0)->cursor);
		ui.tbrMessages->ensureCursorVisible();
		ui.lblTextSearchInfo->setText(tr("Found %n occurrence(s)",0,FSearchResults.count()));
	}
	else if (!ui.lneTextSearch->text().isEmpty())
	{
		QTextCursor cursor = ui.tbrMessages->textCursor();
		if (cursor.hasSelection())
		{
			cursor.clearSelection();
			ui.tbrMessages->setTextCursor(cursor);
		}
		ui.lblTextSearchInfo->setText(tr("Phrase was not found"));
	}

	if (!ui.lneTextSearch->text().isEmpty() && FSearchResults.isEmpty())
	{
		QPalette palette = ui.lneTextSearch->palette();
		palette.setColor(QPalette::Active,QPalette::Base,QColor(255,200,200));
		ui.lneTextSearch->setPalette(palette);
	}
	else
	{
		ui.lneTextSearch->setPalette(QPalette());
	}

	ui.tlbTextSearchNext->setEnabled(!FSearchResults.isEmpty());
	ui.tlbTextSearchPrev->setEnabled(!FSearchResults.isEmpty());

	FTextHilightTimer.start(0);
}

void ArchiveViewWindow::onTextSearchNextClicked()
{
	QMap<int,QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.upperBound(ui.tbrMessages->textCursor().position());
	if (it != FSearchResults.constEnd())
	{
		ui.tbrMessages->setTextCursor(it->cursor);
		ui.tbrMessages->ensureCursorVisible();
	}
}

void ArchiveViewWindow::onTextSearchPrevClicked()
{
	QMap<int,QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.lowerBound(ui.tbrMessages->textCursor().position());
	if (--it != FSearchResults.constEnd())
	{
		ui.tbrMessages->setTextCursor(it->cursor);
		ui.tbrMessages->ensureCursorVisible();
	}
}

void ArchiveViewWindow::onSetContactJidByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_HEADER_STREAM).toStringList();
		QStringList contacts = action->data(ADR_HEADER_WITH).toStringList();

		QMultiMap<Jid,Jid> address;
		for(int i=0; i<streams.count(); i++)
			address.insertMulti(streams.at(i),contacts.at(i));
		setAddresses(address);
	}
}

void ArchiveViewWindow::onRemoveCollectionsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action!=NULL && FRemoveRequests.isEmpty())
	{
		QVariantList headerStream = action->data(ADR_HEADER_STREAM).toList();
		QVariantList headerWith = action->data(ADR_HEADER_WITH).toList();
		QVariantList headerStart = action->data(ADR_HEADER_START).toList();
		QVariantList headerEnd = action->data(ADR_HEADER_END).toList();

		QSet<QString> conversationSet;
		for (int i=0; i<headerStream.count() && i<headerWith.count() && i<headerStart.count() && i<headerEnd.count(); i++)
		{
			QString name = contactName(headerStream.value(i).toString(),headerWith.value(i).toString(),headerEnd.at(i).isNull());
			if (!headerEnd.at(i).isNull())
			{
				conversationSet += tr("with <b>%1</b> for <b>%2 %3</b>?").arg(Qt::escape(name))
					.arg(QLocale().monthName(headerStart.at(i).toDate().month()))
					.arg(headerStart.at(i).toDate().year());
			}
			else if (!headerStart.at(i).isNull())
			{
				conversationSet += tr("with <b>%1</b> started at <b>%2</b>?").arg(Qt::escape(name)).arg(headerStart.at(i).toDateTime().toString());
			}
			else
			{
				conversationSet += tr("with <b>%1</b> for all time?").arg(Qt::escape(name));
			}
		}
		
		QStringList conversationList = conversationSet.toList();
		if (conversationSet.count() > 15)
		{
			conversationList = conversationList.mid(0,10);
			conversationList += tr("And %n other conversations","",conversationSet.count()-conversationList.count());
		}

		if (QMessageBox::question(this,
			tr("Remove conversation history"),
			tr("Do you want to remove the following conversations?") + QString("<ul><li>%1</li></ul>").arg(conversationList.join("</li><li>")),
			QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
		{
			for (int i=0; i<headerStream.count() && i<headerWith.count() && i<headerStart.count() && i<headerEnd.count(); i++)
			{
				IArchiveRequest request;
				request.with = headerWith.at(i).toString();
				request.start = headerStart.at(i).toDateTime();
				request.end = headerEnd.at(i).toDateTime();
				request.exactmatch = !request.with.isEmpty() && request.with.node().isEmpty();

				QString reqId = FArchiver->removeCollections(headerStream.at(i).toString(),request);
				if (!reqId.isEmpty())
					FRemoveRequests.insert(reqId,headerStream.at(i).toString());

				if (!FRemoveRequests.isEmpty())
					setRequestStatus(RequestStarted,tr("Removing conversations..."));
				else
					setRequestStatus(RequestError,tr("Failed to remove conversations: %1").arg(tr("Archive is not accessible")));
			}
		}
	}
}

void ArchiveViewWindow::onHeaderContextMenuRequested(const QPoint &APos)
{
	QList<QStandardItem *> items = filterChildItems(selectedItems());
	if (!items.isEmpty())
	{
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);

		QVariantList headerStream;
		QVariantList headerWith;
		QVariantList headerStart;
		QVariantList headerEnd;
		foreach(QStandardItem *item, items)
		{
			QMultiMap<Jid,Jid> address = itemAddresses(item);
			for (QMultiMap<Jid,Jid>::const_iterator it=address.constBegin(); it!=address.constEnd(); ++it)
			{
				headerStream.append(it.key().pFull());
				headerWith.append(it.value().pFull());

				int itemType = item->data(HDR_TYPE).toInt();
				if (itemType == HIT_DATEGROUP)
				{
					QDate date = item->data(HDR_DATEGROUP_DATE).toDate();
					headerStart.append(QDateTime(date));
					headerEnd.append(QDateTime(date).addMonths(1));
				}
				else if (itemType == HIT_HEADER)
				{
					headerStart.append(item->data(HDR_HEADER_START));
					headerEnd.append(QVariant());
				}
				else
				{
					headerStart.append(QVariant());
					headerEnd.append(QVariant());
				}
			}
		}

		QStandardItem *item = items.value(0);
		int itemType = item->data(HDR_TYPE).toInt();
		if (items.count() > 1)
		{
			Action *removeSelected = new Action(menu);
			removeSelected->setText(tr("Remove Selected Conversations"));
			removeSelected->setData(ADR_HEADER_STREAM,headerStream);
			removeSelected->setData(ADR_HEADER_WITH,headerWith);
			removeSelected->setData(ADR_HEADER_START,headerStart);
			removeSelected->setData(ADR_HEADER_END,headerEnd);
			connect(removeSelected,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeSelected,AG_DEFAULT+500);
		}
		else if (itemType == HIT_CONTACT)
		{
			Action *setContact = new Action(menu);
			setContact->setText(tr("Show Contact History"));
			setContact->setData(ADR_HEADER_STREAM,headerStream);
			setContact->setData(ADR_HEADER_WITH,headerWith);
			connect(setContact,SIGNAL(triggered()),SLOT(onSetContactJidByAction()));
			menu->addAction(setContact,AG_DEFAULT);

			Action *removeAll = new Action(menu);
			removeAll->setText(tr("Remove all History with %1").arg(item->text()));
			removeAll->setData(ADR_HEADER_STREAM,headerStream);
			removeAll->setData(ADR_HEADER_WITH,headerWith);
			removeAll->setData(ADR_HEADER_START,headerStart);
			removeAll->setData(ADR_HEADER_END,headerEnd);
			connect(removeAll,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeAll,AG_DEFAULT+500);
		}
		else if (itemType == HIT_DATEGROUP)
		{
			Action *removeDate = new Action(menu);
			QDate date = item->data(HDR_DATEGROUP_DATE).toDate();
			removeDate->setText(tr("Remove History for %1").arg(item->text()));
			removeDate->setData(ADR_HEADER_STREAM,headerStream);
			removeDate->setData(ADR_HEADER_WITH,headerWith);
			removeDate->setData(ADR_HEADER_START,headerStart);
			removeDate->setData(ADR_HEADER_END,headerEnd);
			connect(removeDate,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeDate,AG_DEFAULT+500);
		}
		else if (itemType == HIT_HEADER)
		{
			Action *removeHeader = new Action(menu);
			removeHeader->setText(tr("Remove this Conversation"));
			removeHeader->setData(ADR_HEADER_STREAM,headerStream);
			removeHeader->setData(ADR_HEADER_WITH,headerWith);
			removeHeader->setData(ADR_HEADER_START,headerStart);
			removeHeader->setData(ADR_HEADER_END,headerEnd);
			connect(removeHeader,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeHeader);
		}

		if (!menu->isEmpty())
			menu->popup(ui.trvHeaders->viewport()->mapToGlobal(APos));
		else
			delete menu;
	}
}

void ArchiveViewWindow::onPrintConversationsByAction()
{
	QPrinter printer;
	
	QPrintDialog *dialog = new QPrintDialog(&printer, this);
	dialog->setWindowTitle(tr("Print Conversation History"));
	
	if (ui.tbrMessages->textCursor().hasSelection())
		dialog->addEnabledOption(QAbstractPrintDialog::PrintSelection);

	if (dialog->exec() == QDialog::Accepted)
		ui.tbrMessages->print(&printer);
}

void ArchiveViewWindow::onExportConversationsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		bool isHtml = action->data(ADR_EXPORT_AS_HTML).toBool();
		QString filter = isHtml ? tr("HTML file (*.html)") : tr("Text file (*.txt)");
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save Conversations to File"),QString::null,filter);
		if (!fileName.isEmpty())
		{
			QFile file(fileName);
			if (file.open(QFile::WriteOnly|QFile::Truncate))
			{
				if (isHtml)
					file.write(ui.tbrMessages->toHtml().toUtf8());
				else
					file.write(ui.tbrMessages->toPlainText().toUtf8());
				file.close();
			}
			else
			{
				LOG_ERROR(QString("Failed to export conversation history to file: %1").arg(file.errorString()));
			}
		}
	}
}

void ArchiveViewWindow::onExportLabelLinkActivated(const QString &ALink)
{
	Q_UNUSED(ALink);
	if (!FSelectedHeaders.isEmpty())
	{
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);

		Action *exportPrinter = new Action(menu);
		exportPrinter->setText(tr("Print..."));
		exportPrinter->setData(ADR_EXPORT_AS_HTML, false);
		connect(exportPrinter,SIGNAL(triggered()),SLOT(onPrintConversationsByAction()));
		menu->addAction(exportPrinter);

		Action *exportHtml = new Action(menu);
		exportHtml->setText(tr("Save as HTML"));
		exportHtml->setData(ADR_EXPORT_AS_HTML, true);
		connect(exportHtml,SIGNAL(triggered()),SLOT(onExportConversationsByAction()));
		menu->addAction(exportHtml);

		Action *exportPlain = new Action(menu);
		exportPlain->setText(tr("Save as Text"));
		exportPlain->setData(ADR_EXPORT_AS_HTML, false);
		connect(exportPlain,SIGNAL(triggered()),SLOT(onExportConversationsByAction()));
		menu->addAction(exportPlain);

		menu->popup(QCursor::pos());
	}
}

void ArchiveViewWindow::onHeadersRequestTimerTimeout()
{
	if (FHeadersRequests.isEmpty())
	{
		IArchiveRequest request;
		if (FHistoryTime > 0)
		{
			request.end = QDateTime(QDate::currentDate().addMonths(HistoryTime[FHistoryTime-1]));
			request.end = request.end.addDays(1-request.end.date().day());
		}
		if (FHistoryTime < HistoryTimeCount)
		{
			request.start = QDateTime(QDate::currentDate().addMonths(HistoryTime[FHistoryTime]));
			request.start = request.start.addDays(1-request.start.date().day());
		}
		request.order = Qt::DescendingOrder;
		request.text = ui.lneArchiveSearch->text().trimmed();

		for(QMultiMap<Jid,Jid>::const_iterator it=FAddresses.constBegin(); it!=FAddresses.constEnd(); ++it)
		{
			request.with = it.value();
			request.exactmatch = request.with.isValid() && request.with.node().isEmpty();

			QString reqId = FArchiver->loadHeaders(it.key(),request);
			if (!reqId.isEmpty())
				FHeadersRequests.insert(reqId,it.key());
		}

		if (!FHeadersRequests.isEmpty())
			setHeaderStatus(RequestStarted);
		else
			setHeaderStatus(RequestError,tr("Archive is not accessible"));
	}
}

void ArchiveViewWindow::onHeadersLoadMoreLinkClicked()
{
	if (FHistoryTime < HistoryTimeCount)
	{
		FHistoryTime++;
		FHeadersRequestTimer.start(0);
	}
	else
	{
		setHeaderStatus(RequestFinished);
	}
}

void ArchiveViewWindow::onCollectionsRequestTimerTimeout()
{
	QList<ArchiveHeader> headers = itemsHeaders(selectedItems());
	qSort(headers);

	if (FSelectedHeaders != headers)
	{
		clearMessages();
		FSelectedHeaders = headers;

		setMessageStatus(RequestStarted);
		processCollectionsLoad();
	}
}

void ArchiveViewWindow::onCollectionsProcessTimerTimeout()
{
	processCollectionsLoad();
}

void ArchiveViewWindow::onCurrentSelectionChanged(const QItemSelection &ASelected, const QItemSelection &ADeselected)
{
	Q_UNUSED(ASelected); Q_UNUSED(ADeselected);
	if (ui.trvHeaders->selectionModel()->hasSelection())
		FCollectionsRequestTimer.start(LOAD_COLLECTION_TIMEOUT);
	else if (!ui.tbrMessages->document()->isEmpty())
		clearMessages();
}

void ArchiveViewWindow::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHeadersRequests.contains(AId))
	{
		FHeadersRequests.remove(AId);
		if (FHeadersRequests.isEmpty())
		{
			if (FHeadersLoaded == 0)
				setHeaderStatus(RequestError, AError.errorMessage());
			else if (FHeadersLoaded >= MIN_LOAD_HEADERS)
				setHeaderStatus(RequestFinished);
			else
				onHeadersLoadMoreLinkClicked();
		}
	}
	else if (FCollectionsRequests.contains(AId))
	{
		ArchiveHeader header = FCollectionsRequests.take(AId);
		if (loadingCollectionHeader() == header)
		{
			FSelectedHeaders.removeAt(FSelectedHeaderIndex);
			if (FSelectedHeaders.isEmpty())
				setMessageStatus(RequestError, AError.errorMessage());
			else
				processCollectionsLoad();
		}
	}
	else if (FRemoveRequests.contains(AId))
	{
		FRemoveRequests.remove(AId);
		if (FRemoveRequests.isEmpty())
			setRequestStatus(RequestError,tr("Failed to remove conversations: %1").arg(AError.errorMessage()));
	}
}

void ArchiveViewWindow::onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FHeadersRequests.contains(AId))
	{
		QList<ArchiveHeader> headers = convertHeaders(FHeadersRequests.take(AId),AHeaders);
		for (QList<ArchiveHeader>::const_iterator it = headers.constBegin(); it!=headers.constEnd(); ++it)
		{
			if (it->with.isValid() && it->start.isValid() && !FCollections.contains(*it))
			{
				ArchiveCollection collection;
				collection.header = *it;
				FCollections.insert(collection.header,collection);
				createHeaderItem(collection.header);
				FHeadersLoaded++;
			}
		}

		if (FHeadersRequests.isEmpty())
		{
			if (FHeadersLoaded >= MIN_LOAD_HEADERS)
				setHeaderStatus(RequestFinished);
			else
				onHeadersLoadMoreLinkClicked();
		}
	}
}

void ArchiveViewWindow::onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FCollectionsRequests.contains(AId))
	{
		ArchiveHeader header = FCollectionsRequests.take(AId);
		ArchiveCollection collection = convertCollection(header.stream,ACollection);

		FCollections.insert(header,collection);
		if (loadingCollectionHeader() == header)
		{
			showCollection(collection);
			processCollectionsLoad();
		}
	}
}

void ArchiveViewWindow::onArchiveCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest)
{
	if (FRemoveRequests.contains(AId))
	{
		Jid streamJid = FRemoveRequests.take(AId);
		if (FRemoveRequests.isEmpty())
			setRequestStatus(RequestFinished,tr("Conversation history removed successfully"));
		removeRequestItems(streamJid,ARequest);
	}
}

void ArchiveViewWindow::onRosterActiveChanged(IRoster *ARoster, bool AActive)
{
	if (!AActive && FAddresses.contains(ARoster->streamJid()))
	{
		FAddresses.remove(ARoster->streamJid());
		if (!FAddresses.isEmpty())
			removeRequestItems(ARoster->streamJid(),IArchiveRequest());
		else
			close();
	}
}

void ArchiveViewWindow::onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore)
{
	if (FAddresses.contains(ABefore))
	{
		foreach(const Jid &contactJid, FAddresses.values(ABefore))
			FAddresses.insertMulti(ARoster->streamJid(),contactJid);
		FAddresses.remove(ABefore);

		foreach(QStandardItem *item, findStreamItems(ABefore))
			item->setData(ARoster->streamJid().pFull(),HDR_HEADER_STREAM);

		QMap<ArchiveHeader,ArchiveCollection> collections;
		for (QMap<ArchiveHeader,ArchiveCollection>::iterator it=FCollections.begin(); it!=FCollections.end(); )
		{
			if (it.key().stream == ABefore)
			{
				ArchiveHeader header = it.key();
				ArchiveCollection collection = it.value();

				header.stream = ARoster->streamJid();
				collection.header.stream = header.stream;
				collections.insert(header,collection);

				it = FCollections.erase(it);
			}
			else
			{
				++it;
			}
		}
		FCollections.unite(collections);
	}
}
