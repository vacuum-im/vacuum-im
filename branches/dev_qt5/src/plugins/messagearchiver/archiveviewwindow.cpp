#include "archiveviewwindow.h"

#include <QLocale>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QNetworkAccessManager>

enum HistoryItemType {
	HIT_CONTACT,
	HIT_DATEGROUP,
	HIT_HEADER
};

enum HistoryDataRoles {
	HDR_TYPE                = Qt::UserRole+1,
	HDR_CONTACT_JID,
	HDR_DATEGROUP_DATE,
	HDR_HEADER_WITH,
	HDR_HEADER_START,
	HDR_HEADER_SUBJECT,
	HDR_HEADER_THREAD,
	HDR_HEADER_VERSION
};

#define ADR_HEADER_WITH              Action::DR_Parametr1
#define ADR_HEADER_START             Action::DR_Parametr2
#define ADR_HEADER_END               Action::DR_Parametr3

#define HEADERS_LOAD_TIMEOUT         500
#define COLLECTIONS_LOAD_TIMEOUT     200

#define MAX_HILIGHT_ITEMS            10
#define LOAD_EARLIER_COUNT           100

SortFilterProxyModel::SortFilterProxyModel(QObject *AParent) : QSortFilterProxyModel(AParent)
{
	FInvalidateTimer.setSingleShot(true);
	connect(&FInvalidateTimer,SIGNAL(timeout()),SLOT(invalidate()));
}

void SortFilterProxyModel::startInvalidate()
{
	FInvalidateTimer.start(0);
}

void SortFilterProxyModel::setVisibleInterval(const QDateTime &AStart, const QDateTime &AEnd)
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
	int indexKind = index.data(HDR_TYPE).toInt();
	if (indexKind == HIT_HEADER)
	{
		if (FStart.isValid() && FEnd.isValid())
		{
			QDateTime date = index.data(HDR_HEADER_START).toDateTime();
			return FStart<=date && date<=FEnd;
		}
	}
	else
	{
		for (int i=0; i<sourceModel()->rowCount(index);i++)
			if (filterAcceptsRow(i,index))
				return true;
		return false;
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

ArchiveViewWindow::ArchiveViewWindow(IPluginManager *APluginManager, IMessageArchiver *AArchiver, IRoster *ARoster, QWidget *AParent) : QMainWindow(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_HISTORY,0,0,"windowIcon");

	FRoster = ARoster;
	FArchiver = AArchiver;
	FFocusWidget = NULL;

	FStatusIcons = NULL;
	FUrlProcessor = NULL;
	FMessageStyles = NULL;
	FMessageProcessor = NULL;
	initialize(APluginManager);

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
	ui.trvHeaders->header()->setSortIndicator(0,Qt::AscendingOrder);
	connect(ui.trvHeaders->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
		SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.trvHeaders,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onHeaderContextMenuRequested(const QPoint &)));
	
	connect(ui.pbtLoadEarlierMessages,SIGNAL(clicked()),SLOT(onLoadEarlierMessageClicked()));
	connect(ui.spwSelectPage,SIGNAL(currentPageChanged(int, int)),SLOT(onCurrentPageChanged(int, int)));

	FHeadersRequestTimer.setSingleShot(true);
	connect(&FHeadersRequestTimer,SIGNAL(timeout()),SLOT(onHeadersRequestTimerTimeout()));

	FCollectionShowTimer.setSingleShot(true);
	connect(&FCollectionShowTimer,SIGNAL(timeout()),SLOT(onCollectionShowTimerTimeout()));

	FCollectionsRequestTimer.setSingleShot(true);
	connect(&FCollectionsRequestTimer,SIGNAL(timeout()),SLOT(onCollectionsRequestTimerTimeout()));

	FTextHilightTimer.setSingleShot(true);
	connect(&FTextHilightTimer,SIGNAL(timeout()),SLOT(onTextHilightTimerTimeout()));
	connect(ui.tbrMessages,SIGNAL(visiblePositionBoundaryChanged()),SLOT(onTextVisiblePositionBoundaryChanged()));

	ui.lneTextSearch->setPlaceholderText(tr("Search in text"));
	ui.tlbTextSearchNext->setIcon(style()->standardIcon(QStyle::SP_ArrowDown, NULL, this));
	ui.tlbTextSearchPrev->setIcon(style()->standardIcon(QStyle::SP_ArrowUp, NULL, this));
	connect(ui.tlbTextSearchNext,SIGNAL(clicked()),SLOT(onTextSearchNextClicked()));
	connect(ui.tlbTextSearchPrev,SIGNAL(clicked()),SLOT(onTextSearchPreviousClicked()));
	connect(ui.lneTextSearch,SIGNAL(searchStart()),SLOT(onTextSearchStart()));
	connect(ui.lneTextSearch,SIGNAL(searchNext()),SLOT(onTextSearchNextClicked()));
	connect(ui.chbTextSearchCaseSensitive,SIGNAL(stateChanged(int)),SLOT(onTextSearchCaseSensitivityChanged()));

	ui.lneArchiveSearch->setStartSearchTimeout(-1);
	ui.lneArchiveSearch->setPlaceholderText(tr("Search in history"));
	ui.lneArchiveSearch->setSelectTextOnFocusEnabled(false);
	connect(ui.lneArchiveSearch,SIGNAL(searchStart()),SLOT(onArchiveSearchStart()));

	ui.pbtHeadersUpdate->setIcon(style()->standardIcon(QStyle::SP_BrowserReload, NULL, this));
	connect(ui.pbtHeadersUpdate,SIGNAL(clicked()),SLOT(onHeadersUpdateButtonClicked()));

	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
		SLOT(onArchiveRequestFailed(const QString &, const XmppError &)));
	connect(FArchiver->instance(),SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
		SLOT(onArchiveHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
	connect(FArchiver->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
		SLOT(onArchiveCollectionLoaded(const QString &, const IArchiveCollection &)));
	connect(FArchiver->instance(),SIGNAL(collectionsRemoved(const QString &, const IArchiveRequest &)),
		SLOT(onArchiveCollectionsRemoved(const QString &, const IArchiveRequest &)));
	connect(FRoster->instance(),SIGNAL(destroyed(QObject *)),SLOT(close()));

	if (!restoreGeometry(Options::fileValue("history.archiveview.geometry").toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(960,640),this));
	if (!ui.sprSplitter->restoreState(Options::fileValue("history.archiveview.splitter-state").toByteArray()))
		ui.sprSplitter->setSizes(QList<int>() << 50 << 150);
	restoreState(Options::fileValue("history.archiveview.state").toByteArray());
	
	reset();
}

ArchiveViewWindow::~ArchiveViewWindow()
{
	Options::setFileValue(saveState(),"history.archiveview.state");
	Options::setFileValue(saveGeometry(),"history.archiveview.geometry");
	Options::setFileValue(ui.sprSplitter->saveState(),"history.archiveview.splitter-state");
	Options::node(OPV_HISTORY_ARCHIVEVIEW_FONTPOINTSIZE).setValue(ui.tbrMessages->font().pointSize());
}

Jid ArchiveViewWindow::streamJid() const
{
	return FRoster->streamJid();
}

Jid ArchiveViewWindow::contactJid() const
{
	return FContactJid;
}

void ArchiveViewWindow::setContactJid(const Jid &AContactJid)
{
	if (FContactJid != AContactJid)
	{
		FContactJid = AContactJid;
		reset();
	}
}

QString ArchiveViewWindow::searchString() const
{
	return FSearchString;
}

void ArchiveViewWindow::setSearchString(const QString &AText)
{
	if (FSearchString != AText)
	{
		FSearchString = AText;
		ui.lneArchiveSearch->setText(AText);
		reset();
	}
}

void ArchiveViewWindow::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IMessageProcessor").value(0);
	if (plugin)
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	
	plugin = APluginManager->pluginInterface("IMessageStyles").value(0,NULL);
	if (plugin)
		FMessageStyles = qobject_cast<IMessageStyles *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0);
	if (plugin)
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IUrlProcessor").value(0);
	if (plugin)
		FUrlProcessor = qobject_cast<IUrlProcessor *>(plugin->instance());
}

void ArchiveViewWindow::reset()
{
	FHeadersRequests.clear();
	FCollectionsRequests.clear();

	FModel->clear();
	FProxyModel->setVisibleInterval(QDateTime(),QDateTime());

	FLoadedPages.clear();
	FCollections.clear();
	FCurrentHeaders.clear();

	if (FContactJid.isEmpty())
	{
		ui.spwSelectPage->setVisible(true);
		ui.pbtLoadEarlierMessages->setVisible(false);
		setWindowTitle(tr("Conversation history - %1").arg(streamJid().uBare()));
	}
	else
	{
		ui.spwSelectPage->setVisible(false);
		ui.pbtLoadEarlierMessages->setVisible(true);
		ui.pbtLoadEarlierMessages->setText(tr("Load earlier messages"));
		setWindowTitle(tr("Conversation history with %1 - %2").arg(contactName(FContactJid),streamJid().uBare()));
	}

	clearMessages();
	setPageStatus(RequestStarted);
	FHeadersRequestTimer.start(0);
}

QString ArchiveViewWindow::contactName(const Jid &AContactJid, bool AShowResource) const
{
	IRosterItem ritem = FRoster->rosterItem(AContactJid);
	QString name = !ritem.name.isEmpty() ? ritem.name : AContactJid.uBare();
	if (AShowResource && !AContactJid.resource().isEmpty())
		name = name + "/" +AContactJid.resource();
	return name;
}

QStandardItem *ArchiveViewWindow::createContactItem(const Jid &AContactJid, QStandardItem *AParent)
{
	QStandardItem *item = findItem(HIT_CONTACT,HDR_CONTACT_JID,AContactJid.pFull(),AParent);
	if (item == NULL)
	{
		item = new QStandardItem();
		item->setData(HIT_CONTACT,HDR_TYPE);
		item->setData(AContactJid.pFull(),HDR_CONTACT_JID);
		item->setIcon(FStatusIcons!=NULL ? FStatusIcons->iconByJidStatus(AContactJid,IPresence::Online,SUBSCRIPTION_BOTH,false) : QIcon());
		AParent->appendRow(item);
	}
	return item;
}

QStandardItem *ArchiveViewWindow::createDateGroupItem(const QDateTime &ADateTime, QStandardItem *AParent)
{
	QDate date(ADateTime.date().year(),ADateTime.date().month(),1);
	QStandardItem *item = findItem(HIT_DATEGROUP,HDR_DATEGROUP_DATE,date,AParent);
	if (item == NULL)
	{
		item = new QStandardItem(date.toString(tr("MMMM yyyy","Date group name")));
		item->setData(HIT_DATEGROUP,HDR_TYPE);
		item->setData(date,HDR_DATEGROUP_DATE);
		item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_DATE));
		AParent->appendRow(item);
	}
	return item;
}

QStandardItem *ArchiveViewWindow::createParentItem(const IArchiveHeader &AHeader)
{
	QStandardItem *item = NULL;
	if (FContactJid.isEmpty())
	{
		item = createContactItem(AHeader.with.bare(),FModel->invisibleRootItem());
		item->setText(contactName(AHeader.with));
	}
	else
	{
		item = createDateGroupItem(AHeader.start,FModel->invisibleRootItem());
		item->setData(AHeader.with.pBare(),HDR_CONTACT_JID);
	}

	if (FContactJid!=AHeader.with && isConferencePrivateChat(AHeader.with))
	{
		QStandardItem *privateItem = createContactItem(AHeader.with,item);
		privateItem->setText(AHeader.with.resource());
		privateItem->setData(item->data(HDR_DATEGROUP_DATE),HDR_DATEGROUP_DATE);
		item = privateItem;
	}
	return item;
}

QStandardItem *ArchiveViewWindow::createHeaderItem(const IArchiveHeader &AHeader)
{
	QStandardItem *item = new QStandardItem(AHeader.start.toString(tr("dd MMM, dddd","Conversation name")));
	item->setData(HIT_HEADER,HDR_TYPE);
	item->setData(AHeader.with.pFull(),HDR_HEADER_WITH);
	item->setData(AHeader.start,HDR_HEADER_START);
	item->setData(AHeader.subject,HDR_HEADER_SUBJECT);
	item->setData(AHeader.threadId,HDR_HEADER_THREAD);
	item->setData(AHeader.version,HDR_HEADER_VERSION);
	item->setIcon(IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY_DATE));

	QString itemToolTip = AHeader.with.uFull().toHtmlEscaped();
	if (!AHeader.subject.isEmpty())
		itemToolTip += "<hr>" + AHeader.subject.toHtmlEscaped();
	item->setToolTip(itemToolTip);

	QStandardItem *parentItem = createParentItem(AHeader);
	if (parentItem)
		parentItem->appendRow(item);
	else
		FModel->appendRow(item);

	FProxyModel->startInvalidate();

	return item;
}

IArchiveHeader ArchiveViewWindow::modelIndexHeader(const QModelIndex &AIndex) const
{
	IArchiveHeader header;
	if (AIndex.data(HDR_TYPE).toInt() == HIT_HEADER)
	{
		header.with = AIndex.data(HDR_HEADER_WITH).toString();
		header.start = AIndex.data(HDR_HEADER_START).toDateTime();
		header = FCollections.value(header).header;
	}
	return header;
}

bool ArchiveViewWindow::isConferencePrivateChat(const Jid &AContactJid) const
{
	return !AContactJid.resource().isEmpty() && AContactJid.pDomain().startsWith("conference.");
}

bool ArchiveViewWindow::isJidMatched(const Jid &ARequested, const Jid &AHeaderJid) const
{
	if (ARequested.pBare() != AHeaderJid.pBare())
		return false;
	else if (!ARequested.resource().isEmpty() && ARequested.pResource()!=AHeaderJid.pResource())
		return false;
	return true;
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

QList<QStandardItem *> ArchiveViewWindow::findHeaderItems(const IArchiveRequest &ARequest, QStandardItem *AParent) const
{
	QList<QStandardItem *> items;
	QStandardItem *parent = AParent!=NULL ? AParent : FModel->invisibleRootItem();
	for (int row=0; row<parent->rowCount(); row++)
	{
		QStandardItem *item = parent->child(row);
		if (item->data(HDR_TYPE) == HIT_HEADER)
		{
			bool checked = true;
			if (ARequest.with.isValid() && !isJidMatched(ARequest.with,item->data(HDR_HEADER_WITH).toString()))
				checked = false;
			else if (ARequest.start.isValid() && ARequest.start>item->data(HDR_HEADER_START).toDateTime())
				checked = false;
			else if (ARequest.end.isValid() && ARequest.end<item->data(HDR_HEADER_START).toDateTime())
				checked = false;
			else if (!ARequest.threadId.isEmpty() && ARequest.threadId!=item->data(HDR_HEADER_THREAD).toString())
				checked = false;
			if (checked)
				items.append(item);
		}
		if (item->rowCount() > 0)
		{
			items += findHeaderItems(ARequest,item);
		}
	}
	return items;
}

QDate ArchiveViewWindow::currentPage() const
{
	return QDate(ui.spwSelectPage->yearShown(),ui.spwSelectPage->monthShown(),1);
}

void ArchiveViewWindow::setRequestStatus(RequestStatus AStatus, const QString &AMessage)
{
	Q_UNUSED(AStatus);
	ui.stbStatusBar->showMessage(AMessage);
}

void ArchiveViewWindow::setPageStatus(RequestStatus AStatus, const QString &AMessage)
{
	if (AStatus == RequestStarted)
		FFocusWidget = focusWidget();

	ui.wdtArchiveSearch->setEnabled(AStatus == RequestFinished);
	ui.trvHeaders->setEnabled(AStatus == RequestFinished);
	ui.pbtHeadersUpdate->setEnabled(AStatus != RequestStarted);
	ui.pbtLoadEarlierMessages->setEnabled(AStatus != RequestStarted);

	if (AStatus == RequestFinished)
	{
		if (FFocusWidget)
			FFocusWidget->setFocus(Qt::MouseFocusReason);
		ui.trvHeaders->selectionModel()->clearSelection();
		ui.trvHeaders->setCurrentIndex(QModelIndex());
		ui.stbStatusBar->showMessage(tr("Conversation headers loaded"));
	}
	else if(AStatus == RequestStarted)
	{
		ui.stbStatusBar->showMessage(tr("Loading conversation headers..."));
	}
	else if (AStatus == RequestError)
	{
		if (FFocusWidget)
			FFocusWidget->setFocus(Qt::MouseFocusReason);
		ui.stbStatusBar->showMessage(tr("Failed to load conversation headers: %1").arg(AMessage));
	}
}

void ArchiveViewWindow::setMessagesStatus(RequestStatus AStatus, const QString &AMessage)
{
	if (AStatus == RequestFinished)
	{
		if (FCurrentHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Select contact or single conversation"));
		else
			ui.stbStatusBar->showMessage(tr("%n conversation(s) loaded",0,FCurrentHeaders.count()));
		onTextSearchStart();
	}
	else if(AStatus == RequestStarted)
	{
		if (FCurrentHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Loading conversations..."));
		else
			ui.stbStatusBar->showMessage(tr("Loading %1 of %2 conversations...").arg(FLoadHeaderIndex+1).arg(FCurrentHeaders.count()));
	}
	else if (AStatus == RequestError)
	{
		ui.stbStatusBar->showMessage(tr("Failed to load conversations: %1").arg(AMessage));
	}
	ui.wdtTextSearch->setEnabled(AStatus==RequestFinished && !FCurrentHeaders.isEmpty());
}

void ArchiveViewWindow::clearMessages()
{
	FLoadHeaderIndex = 0;
	FCurrentHeaders.clear();
	ui.tbrMessages->clear();
	FSearchResults.clear();
	FCollectionShowTimer.stop();
	setMessagesStatus(RequestFinished);
}

void ArchiveViewWindow::processCollectionsLoad()
{
	if (FLoadHeaderIndex < FCurrentHeaders.count())
	{
		IArchiveHeader header = currentLoadingHeader();
		IArchiveCollection collection = FCollections.value(header);
		if (collection.body.messages.isEmpty() && collection.body.notes.isEmpty())
		{
			QString requestId = FArchiver->loadCollection(streamJid(),header);
			if (!requestId.isEmpty())
			{
				FCollectionsRequests.insert(requestId,header);
				setMessagesStatus(RequestStarted);
			}
			else
			{
				setMessagesStatus(RequestError,tr("Archive is not accessible"));
			}
		}
		else
		{
			showCollection(collection);
			setMessagesStatus(RequestStarted);
			FCollectionShowTimer.start(0);
		}
	}

	if (FCurrentHeaders.isEmpty())
		clearMessages();
	else if (FLoadHeaderIndex == FCurrentHeaders.count())
		setMessagesStatus(RequestFinished);
}

IArchiveHeader ArchiveViewWindow::currentLoadingHeader() const
{
	return FCurrentHeaders.value(FLoadHeaderIndex);
}

bool ArchiveViewWindow::updateHeaders(const IArchiveRequest &ARequest)
{
	QString requestId = FArchiver->loadHeaders(streamJid(),ARequest);
	if (!requestId.isEmpty())
	{
		FHeadersRequests.insert(requestId,ARequest.start.date());
		return true;
	}
	return false;
}

void ArchiveViewWindow::removeHeaderItems(const IArchiveRequest &ARequest)
{
	bool updateMessages = false;
	QStandardItem *currentItem = FModel->itemFromIndex(FProxyModel->mapToSource(ui.trvHeaders->selectionModel()->currentIndex()));
	foreach(QStandardItem *item, findHeaderItems(ARequest))
	{
		if (!updateMessages && currentItem!=NULL && (currentItem==item || currentItem==item->parent()))
			updateMessages = true;

		FCollections.remove(modelIndexHeader(FModel->indexFromItem(item)));
		
		if (item->parent() != NULL)
			item->parent()->removeRow(item->row());
		else
			qDeleteAll(FModel->takeRow(item->row()));
		
		FProxyModel->startInvalidate();
	}
	if (updateMessages)
	{
		clearMessages();
		FCollectionShowTimer.start(0);
	}
}

QString ArchiveViewWindow::showCollectionInfo(const IArchiveCollection &ACollection)
{
	static const QString infoTmpl =
		"<table width='100%' cellpadding='0' cellspacing='0' style='margin-top:10px;'>"
		"  <tr bgcolor='%bgcolor%'>"
		"    <td style='padding-top:5px; padding-bottom:5px; padding-left:15px; padding-right:15px;'><span style='color:darkCyan;'>%info%</span>%subject%</td>"
		"  </tr>"
		"</table>";

	QString info;
	QString startDate = ACollection.header.start.toString().toHtmlEscaped();
	if (FViewOptions.isPrivateChat)
	{
		QString withName = ACollection.header.with.resource().toHtmlEscaped();
		QString confName = ACollection.header.with.uBare().toHtmlEscaped();
		info = tr("Conversation with <b>%1</b> in conference %2 started at <b>%3</b>.").arg(withName,confName,startDate);
	}
	else if (FViewOptions.isGroupChat)
	{
		QString confName = ACollection.header.with.uBare().toHtmlEscaped();
		info = tr("Conversation in conference %1 started at <b>%2</b>.").arg(confName,startDate);
	}
	else
	{
		QString withName = contactName(ACollection.header.with,true).toHtmlEscaped();
		info = tr("Conversation with %1 started at <b>%2</b>.").arg(withName,startDate);
	}

	QString subject;
	if (!ACollection.header.subject.isEmpty())
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
			subject += ACollection.header.subject.toHtmlEscaped();
		}
	}

	QString tmpl = infoTmpl;
	tmpl.replace("%bgcolor%",ui.tbrMessages->palette().color(QPalette::AlternateBase).name());
	tmpl.replace("%info%",info);
	tmpl.replace("%subject%",subject);

	return tmpl;
}

QString ArchiveViewWindow::showNote(const QString &ANote, const IMessageContentOptions &AOptions)
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

	QString tmpl = statusTmpl;
	tmpl.replace("%time%",AOptions.time.toString(AOptions.timeFormat));
	tmpl.replace("%message%",ANote.toHtmlEscaped());

	return tmpl;
}

QString ArchiveViewWindow::showMessage(const Message &AMessage, const IMessageContentOptions &AOptions)
{
	QString tmpl;
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
		tmpl = meMessageTmpl;
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
		tmpl = firstMessageTmpl;
	}
	else
	{
		static const QString nextMessageTmpl =
			"<table width='100%' cellpadding='0' cellspacing='0'>"
			"  <tr>"
			"    <td style='padding-left:10px; white-space:pre-wrap;'>%message%</td>"
			"  </tr>"
			"</table>";
		tmpl = nextMessageTmpl;
	}

	FViewOptions.lastTime = AOptions.time;
	FViewOptions.lastSenderId = AOptions.senderId;

	tmpl.replace("%sender%",AOptions.senderName);
	tmpl.replace("%senderColor%",AOptions.senderColor);
	tmpl.replace("%time%",AOptions.time.toString(AOptions.timeFormat));

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

	tmpl.replace("%message%",TextManager::getDocumentBody(doc));

	return tmpl;
}

void ArchiveViewWindow::showCollection(const IArchiveCollection &ACollection)
{
	if (FLoadHeaderIndex == 0)
	{
		ui.tbrMessages->clear();

		FViewOptions.isPrivateChat = isConferencePrivateChat(ACollection.header.with);

		FViewOptions.isGroupChat = false;
		if (!FViewOptions.isPrivateChat)
			for (int i=0; !FViewOptions.isGroupChat && i<ACollection.body.messages.count(); i++)
				FViewOptions.isGroupChat = ACollection.body.messages.at(i).type()==Message::GroupChat;

		if (FMessageStyles)
		{
			IMessageStyleOptions soptions = FMessageStyles->styleOptions(FViewOptions.isGroupChat ? Message::GroupChat : Message::Chat);
			FViewOptions.style = FViewOptions.isGroupChat ? FMessageStyles->styleForOptions(soptions) : NULL;
		}

		if (!FViewOptions.isPrivateChat)
			FViewOptions.contactName = FMessageStyles!=NULL ? FMessageStyles->contactName(streamJid(),ACollection.header.with).toHtmlEscaped() : contactName(ACollection.header.with).toHtmlEscaped();
		else
			FViewOptions.contactName = ACollection.header.with.resource().toHtmlEscaped();
		FViewOptions.selfName = FMessageStyles!=NULL ? FMessageStyles->contactName(streamJid()).toHtmlEscaped() : streamJid().uBare().toHtmlEscaped();
	}

	FViewOptions.lastTime = QDateTime();
	FViewOptions.lastSenderId = QString::null;

	QString html = showCollectionInfo(ACollection);

	IMessageContentOptions options;
	QList<Message>::const_iterator messageIt = ACollection.body.messages.constBegin();
	QMultiMap<QDateTime,QString>::const_iterator noteIt = ACollection.body.notes.constBegin();
	while (noteIt!=ACollection.body.notes.constEnd() || messageIt!=ACollection.body.messages.constEnd())
	{
		if (messageIt!=ACollection.body.messages.constEnd() && (noteIt==ACollection.body.notes.constEnd() || messageIt->dateTime()<noteIt.key()))
		{
			Jid senderJid = !messageIt->from().isEmpty() ? messageIt->from() : streamJid();

			options.type = IMessageContentOptions::TypeEmpty;
			options.kind = IMessageContentOptions::KindMessage;
			options.senderId = senderJid.full();
			options.time = messageIt->dateTime();
			options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time,ACollection.header.start) : QString::null;

			if (FViewOptions.isGroupChat)
			{
				options.type |= IMessageContentOptions::TypeGroupchat;
				options.direction = IMessageContentOptions::DirectionIn;
				options.senderName =senderJid.resource().toHtmlEscaped();
				options.senderColor = FViewOptions.style!=NULL ? FViewOptions.style->senderColor(options.senderName) : "blue";
			}
			else if (ACollection.header.with == senderJid)
			{
				options.direction = IMessageContentOptions::DirectionIn;
				options.senderName = FViewOptions.contactName;
				options.senderColor = "blue";
			}
			else
			{
				options.direction = IMessageContentOptions::DirectionOut;
				options.senderName = FViewOptions.selfName;
				options.senderColor = "red";
			}

			html += showMessage(*messageIt,options);
			++messageIt;
		}
		else if (noteIt != ACollection.body.notes.constEnd())
		{
			options.kind = IMessageContentOptions::KindStatus;
			options.type = IMessageContentOptions::TypeEmpty;
			options.senderId = QString::null;
			options.senderName = QString::null;
			options.time = noteIt.key();
			options.timeFormat = FMessageStyles!=NULL ? FMessageStyles->timeFormat(options.time,ACollection.header.start) : QString::null;

			html += showNote(*noteIt,options);
			++noteIt;
		}
	}

	QTextCursor cursor(ui.tbrMessages->document());
	cursor.movePosition(QTextCursor::End);
	cursor.insertHtml(html);

	FLoadHeaderIndex++;
}

void ArchiveViewWindow::onHeadersUpdateButtonClicked()
{
	reset();
}

void ArchiveViewWindow::onHeadersRequestTimerTimeout()
{
	if (FContactJid.isEmpty())
	{
		QDate start = currentPage();
		QDate end = start.addMonths(1);
		if (!FLoadedPages.contains(start))
		{
			IArchiveRequest request;
			request.with = isConferencePrivateChat(FContactJid) ? FContactJid : FContactJid.bare();
			request.exactmatch = request.with.node().isEmpty();
			request.start = QDateTime(start);
			request.end = QDateTime(end);
			request.text = searchString();

			if (updateHeaders(request))
				FLoadedPages.append(start);
			else
				setPageStatus(RequestError,tr("Archive is not accessible"));
		}
	}
	else if (FHeadersRequests.isEmpty())
	{
		IArchiveRequest request;
		request.with = isConferencePrivateChat(FContactJid) ? FContactJid : FContactJid.bare();
		request.exactmatch = request.with.node().isEmpty();
		request.maxItems = LOAD_EARLIER_COUNT;
		request.order = Qt::DescendingOrder;
		request.text = searchString();

		QMap<IArchiveHeader,IArchiveCollection>::const_iterator it = FCollections.constBegin();
		request.end = it!=FCollections.constEnd() ? it.key().start.addMSecs(-1) : QDateTime();

		if (!updateHeaders(request))
			setPageStatus(RequestError,tr("Archive is not accessible"));
	}
	else
	{
		setPageStatus(RequestFinished);
	}
}

void ArchiveViewWindow::onLoadEarlierMessageClicked()
{
	FHeadersRequestTimer.start(0);
	setPageStatus(RequestStarted);
}

void ArchiveViewWindow::onCurrentPageChanged(int AYear, int AMonth)
{
	QDate start(AYear,AMonth,1);
	FProxyModel->setVisibleInterval(QDateTime(start),QDateTime(start.addMonths(1)));

	clearMessages();
	if (!FLoadedPages.contains(start))
	{
		FHeadersRequestTimer.start(HEADERS_LOAD_TIMEOUT);
		setPageStatus(RequestStarted);
	}
	else if (!FHeadersRequests.values().contains(start))
	{
		FHeadersRequestTimer.stop();
		setPageStatus(RequestFinished);
	}
	else
	{
		setPageStatus(RequestStarted);
	}
}

void ArchiveViewWindow::onCollectionShowTimerTimeout()
{
	processCollectionsLoad();
}

void ArchiveViewWindow::onCollectionsRequestTimerTimeout()
{
	QModelIndex index = ui.trvHeaders->selectionModel()->currentIndex();
	if (index.isValid())
	{
		if(index.data(HDR_TYPE).toInt() == HIT_HEADER)
		{
			IArchiveHeader header = modelIndexHeader(index);
			if (header.with.isValid() && header.start.isValid())
				FCurrentHeaders.append(header);
		}
		else
		{
			int rows = index.model()->rowCount(index);
			for (int row=0; row<rows; row++)
			{
				IArchiveHeader header = modelIndexHeader(index.child(row,0));
				if (header.with.isValid() && header.start.isValid())
					FCurrentHeaders.append(header);
			}
		}
		qSort(FCurrentHeaders);
		processCollectionsLoad();
	}
}

void ArchiveViewWindow::onCurrentItemChanged(const QModelIndex &ACurrent, const QModelIndex &ABefore)
{
	Q_UNUSED(ABefore);
	clearMessages();
	if (ACurrent.isValid())
	{
		setMessagesStatus(RequestStarted);
		FCollectionsRequestTimer.start(COLLECTIONS_LOAD_TIMEOUT);
	}
}

void ArchiveViewWindow::onArchiveSearchStart()
{
	setSearchString(ui.lneArchiveSearch->text());
	ui.lneTextSearch->setText(ui.lneArchiveSearch->text());
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
		QTextDocument::FindFlags options = ui.chbTextSearchCaseSensitive->isChecked() ? QTextDocument::FindCaseSensitively : (QTextDocument::FindFlag)0;
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

		ui.lblTextSearchInfo->setVisible(true);
	}
	else
	{
		ui.lblTextSearchInfo->setVisible(false);
	}

	if (!FSearchResults.isEmpty())
	{
		ui.tbrMessages->setTextCursor(FSearchResults.lowerBound(0)->cursor);
		ui.tbrMessages->ensureCursorVisible();
		ui.lblTextSearchInfo->setText(tr("Found %n occurrence(s)",0,FSearchResults.count()));
	}
	else
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
		palette.setColor(QPalette::Active,QPalette::Base,QColor("orangered"));
		palette.setColor(QPalette::Active,QPalette::Text,Qt::white);
		ui.lneTextSearch->setPalette(palette);
	}
	else
	{
		ui.lneTextSearch->setPalette(QPalette());
	}

	ui.tlbTextSearchNext->setEnabled(!FSearchResults.isEmpty());
	ui.tlbTextSearchPrev->setEnabled(!FSearchResults.isEmpty());
	ui.chbTextSearchCaseSensitive->setEnabled(!FSearchResults.isEmpty() || !ui.lneTextSearch->text().isEmpty());

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

void ArchiveViewWindow::onTextSearchPreviousClicked()
{
	QMap<int,QTextEdit::ExtraSelection>::const_iterator it = FSearchResults.lowerBound(ui.tbrMessages->textCursor().position());
	if (--it != FSearchResults.constEnd())
	{
		ui.tbrMessages->setTextCursor(it->cursor);
		ui.tbrMessages->ensureCursorVisible();
	}
}

void ArchiveViewWindow::onTextSearchCaseSensitivityChanged()
{
	QTimer::singleShot(0,this,SLOT(onTextSearchStart()));
}

void ArchiveViewWindow::onSetContactJidByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		setContactJid(action->data(ADR_HEADER_WITH).toString());
	}
}

void ArchiveViewWindow::onRemoveCollectionsByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IArchiveRequest request;
		request.with = action->data(ADR_HEADER_WITH).toString();
		request.exactmatch = request.with.node().isEmpty();
		request.start = action->data(ADR_HEADER_START).toDateTime();
		request.end = action->data(ADR_HEADER_END).toDateTime();

		QString message;
		if (request.end.isValid())
		{
			message = tr("Do you want to remove conversation history with <b>%1</b> for <b>%2 %3</b>?")
				.arg(contactName(request.with).toHtmlEscaped())
				.arg(QLocale().monthName(request.start.date().month()))
				.arg(request.start.date().year());
		}
		else if (request.start.isValid())
		{
			message = tr("Do you want to remove conversation with <b>%1</b> started at <b>%2</b>?")
				.arg(contactName(request.with,true).toHtmlEscaped())
				.arg(request.start.toString());
		}
		else
		{
			message = tr("Do you want to remove <b>all</b> conversation history with <b>%1</b>?")
				.arg(contactName(request.with,true).toHtmlEscaped());
		}

		if (QMessageBox::question(this, tr("Remove conversation history"), message, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes)
		{
			QString id = FArchiver->removeCollections(streamJid(),request);
			if (!id.isEmpty())
			{
				FRemoveRequests.insert(id,request);
				setRequestStatus(RequestStarted,tr("Removing conversations..."));
			}
			else
			{
				setRequestStatus(RequestError,tr("Failed to remove conversations: %1").arg(tr("Archive is not accessible")));
			}
		}
	}
}

void ArchiveViewWindow::onHeaderContextMenuRequested(const QPoint &APos)
{
	QStandardItem *item =  FModel->itemFromIndex(FProxyModel->mapToSource(ui.trvHeaders->indexAt(APos)));
	if (item)
	{
		Menu *menu = new Menu(this);
		menu->setAttribute(Qt::WA_DeleteOnClose,true);

		int itemType = item->data(HDR_TYPE).toInt();
		if (itemType == HIT_CONTACT)
		{
			Action *setContact = new Action(menu);
			setContact->setText(tr("Show history for this contact"));
			setContact->setData(ADR_HEADER_WITH,item->data(HDR_CONTACT_JID));
			connect(setContact,SIGNAL(triggered()),SLOT(onSetContactJidByAction()));
			menu->addAction(setContact,AG_DEFAULT);
		}
		if (itemType==HIT_CONTACT || itemType==HIT_DATEGROUP)
		{
			Action *removeAll = new Action(menu);
			removeAll->setText(tr("Remove all History"));
			removeAll->setData(ADR_HEADER_WITH,item->data(HDR_CONTACT_JID));
			connect(removeAll,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeAll,AG_DEFAULT+500);

			Action *removePage = new Action(menu);
			QDate date = FContactJid.isEmpty() ? currentPage() : item->data(HDR_DATEGROUP_DATE).toDate();
			removePage->setText(tr("Remove History for %1 %2").arg(QLocale().monthName(date.month())).arg(date.year()));
			removePage->setData(ADR_HEADER_WITH,item->data(HDR_CONTACT_JID));
			removePage->setData(ADR_HEADER_START,QDateTime(date));
			removePage->setData(ADR_HEADER_END,QDateTime(date).addMonths(1));
			connect(removePage,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removePage,AG_DEFAULT+500);
		}
		if (itemType == HIT_HEADER)
		{
			Action *removeHeader = new Action(menu);
			removeHeader->setText(tr("Remove this Conversation"));
			removeHeader->setData(ADR_HEADER_WITH,item->data(HDR_HEADER_WITH));
			removeHeader->setData(ADR_HEADER_START,item->data(HDR_HEADER_START));
			connect(removeHeader,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeHeader);
		}
		!menu->isEmpty() ? menu->popup(ui.trvHeaders->viewport()->mapToGlobal(APos)) : delete menu;
	}
}

void ArchiveViewWindow::onArchiveRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FHeadersRequests.contains(AId))
	{
		QDate start = FHeadersRequests.take(AId);
		if (FContactJid.isEmpty())
		{
			if (currentPage() == start)
				setPageStatus(RequestError, AError.errorMessage());
		}
		else
		{
			FHeadersRequests.clear();
			setPageStatus(RequestError, AError.errorMessage());
		}
		FLoadedPages.removeAll(start);
	}
	else if (FCollectionsRequests.contains(AId))
	{
		IArchiveHeader header = FCollectionsRequests.take(AId);
		if (currentLoadingHeader() == header)
			setMessagesStatus(RequestError, AError.errorMessage());
	}
	else if (FRemoveRequests.contains(AId))
	{
		IArchiveRequest request = FRemoveRequests.take(AId);
		request.text = searchString();
		request.end = !request.end.isValid() ? request.start : request.end;
		setRequestStatus(RequestError,tr("Failed to remove conversations: %1").arg(AError.errorMessage()));
		updateHeaders(request);
		removeHeaderItems(request);
	}
}

void ArchiveViewWindow::onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FHeadersRequests.contains(AId))
	{
		QDate start = FHeadersRequests.take(AId);
		for (QList<IArchiveHeader>::const_iterator it = AHeaders.constBegin(); it!=AHeaders.constEnd(); ++it)
		{
			if (!FCollections.contains(*it) && it->with.isValid() && it->start.isValid())
			{
				IArchiveCollection collection;
				collection.header = *it;
				FCollections.insert(collection.header,collection);
				createHeaderItem(collection.header);
			}
		}
		if (FContactJid.isEmpty())
		{
			if (currentPage() == start)
				setPageStatus(RequestFinished);
		}
		else if (FHeadersRequests.isEmpty())
		{
			setPageStatus(RequestFinished);
			if (AHeaders.count() >= LOAD_EARLIER_COUNT)
			{
				ui.pbtLoadEarlierMessages->setEnabled(true);
				QMap<IArchiveHeader,IArchiveCollection>::const_iterator it = FCollections.constBegin();
				QDateTime before = it!=FCollections.constEnd() ? it.key().start.addMSecs(-1) : QDateTime::currentDateTime();
				ui.pbtLoadEarlierMessages->setText(tr("Load message earlier %1").arg(before.toString(tr("dd MMM yyyy","Load messages earlier date"))));
			}
			else
			{
				ui.pbtLoadEarlierMessages->setEnabled(false);
				ui.pbtLoadEarlierMessages->setText(tr("All messages loaded"));
			}
			ui.pbtLoadEarlierMessages->setToolTip(ui.pbtLoadEarlierMessages->text());
		}
	}
}

void ArchiveViewWindow::onArchiveCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FCollectionsRequests.contains(AId))
	{
		IArchiveHeader header = FCollectionsRequests.take(AId);
		FCollections.insert(header,ACollection);
		if (currentLoadingHeader() == header)
		{
			showCollection(ACollection);
			processCollectionsLoad();
		}
	}
}

void ArchiveViewWindow::onArchiveCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest)
{
	Q_UNUSED(ARequest);
	if (FRemoveRequests.contains(AId))
	{
		IArchiveRequest request = FRemoveRequests.take(AId);
		request.text = searchString();
		request.end = !request.end.isValid() ? request.start : request.end;
		setRequestStatus(RequestFinished,tr("Conversation history removed successfully"));
		updateHeaders(request);
		removeHeaderItems(request);
	}
}
