#include "archiveviewwindow.h"

#include <QLocale>
#include <QMessageBox>
#include <QItemSelectionModel>

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

#define ADR_HEADER_WITH              Action::DR_Parametr1
#define ADR_HEADER_START             Action::DR_Parametr2
#define ADR_HEADER_END               Action::DR_Parametr3

#define HEADERS_LOAD_TIMEOUT         500
#define COLLECTIONS_LOAD_TIMEOUT     200
#define TEXT_SEARCH_TIMEOUT          500

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
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_HISTORY_VIEW,0,0,"windowIcon");

	FRoster = ARoster;
	FArchiver = AArchiver;

	FStatusIcons = NULL;
	FMessageStyles = NULL;
	FMessageProcessor = NULL;
	initialize(APluginManager);

	FModel = new QStandardItemModel(this);
	FProxyModel = new SortFilterProxyModel(FModel);
	FProxyModel->setSourceModel(FModel);
	FProxyModel->setDynamicSortFilter(true);
	FProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

	QPalette palette = ui.tbrMessages->palette();
	palette.setColor(QPalette::Inactive,QPalette::Highlight,palette.color(QPalette::Active,QPalette::Highlight));
	palette.setColor(QPalette::Inactive,QPalette::HighlightedText,palette.color(QPalette::Active,QPalette::HighlightedText));
	ui.tbrMessages->setPalette(palette);

	ui.trvHeaders->setModel(FProxyModel);
	ui.trvHeaders->header()->setSortIndicator(0,Qt::AscendingOrder);
	connect(ui.trvHeaders->selectionModel(),SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
		SLOT(onCurrentItemChanged(const QModelIndex &, const QModelIndex &)));
	connect(ui.trvHeaders,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onHeaderContextMenuRequested(const QPoint &)));
	connect(ui.spwSelectPage,SIGNAL(currentPageChanged(int, int)),SLOT(onCurrentPageChanged(int, int)));

	FHeadersRequestTimer.setSingleShot(true);
	connect(&FHeadersRequestTimer,SIGNAL(timeout()),SLOT(onHeadersRequestTimerTimeout()));

	FCollectionShowTimer.setSingleShot(true);
	connect(&FCollectionShowTimer,SIGNAL(timeout()),SLOT(onCollectionShowTimerTimeout()));

	FCollectionsRequestTimer.setSingleShot(true);
	connect(&FCollectionsRequestTimer,SIGNAL(timeout()),SLOT(onCollectionsRequestTimerTimeout()));

	FTextSearchTimer.setSingleShot(true);
	connect(&FTextSearchTimer,SIGNAL(timeout()),SLOT(onTextSearchTimerTimeout()));

	ui.tlbTextSearchNext->setIcon(style()->standardIcon(QStyle::SP_ArrowDown, NULL, this));
	ui.tlbTextSearchPrev->setIcon(style()->standardIcon(QStyle::SP_ArrowUp, NULL, this));
	connect(ui.tlbTextSearchNext,SIGNAL(clicked()),SLOT(onTextSearchNextClicked()));
	connect(ui.tlbTextSearchPrev,SIGNAL(clicked()),SLOT(onTextSearchPreviousClicked()));
	connect(ui.lneTextSearch,SIGNAL(returnPressed()),SLOT(onTextSearchNextClicked()));
	connect(ui.lneTextSearch,SIGNAL(textChanged(const QString &)),SLOT(onTextSearchTextChanged(const QString &)));
	connect(ui.chbTextSearchCaseSensitive,SIGNAL(stateChanged(int)),SLOT(onTextSearchCaseSensitivityChanged()));

#if QT_VERSION >= QT_VERSION_CHECK(4,7,0)
	ui.lneArchiveSearch->setPlaceholderText(tr("Search in history"));
#endif
	ui.tlbArchiveSearchUpdate->setIcon(style()->standardIcon(QStyle::SP_BrowserReload, NULL, this));
	connect(ui.tlbArchiveSearchUpdate,SIGNAL(clicked()),SLOT(onArchiveSearchUpdate()));
	connect(ui.lneArchiveSearch,SIGNAL(returnPressed()),SLOT(onArchiveSearchUpdate()));
	connect(ui.lneArchiveSearch,SIGNAL(textChanged(const QString &)),SLOT(onArchiveSearchChanged(const QString &)));

	ui.pbtHeadersUpdate->setIcon(style()->standardIcon(QStyle::SP_BrowserReload, NULL, this));
	connect(ui.pbtHeadersUpdate,SIGNAL(clicked()),SLOT(onHeadersUpdateButtonClicked()));

	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
		SLOT(onArchiveRequestFailed(const QString &, const QString &)));
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

	onCurrentPageChanged(ui.spwSelectPage->yearShown(),ui.spwSelectPage->monthShown());

	reset();
}

ArchiveViewWindow::~ArchiveViewWindow()
{
	Options::setFileValue(saveState(),"history.archiveview.state");
	Options::setFileValue(saveGeometry(),"history.archiveview.geometry");
	Options::setFileValue(ui.sprSplitter->saveState(),"history.archiveview.splitter-state");
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
}

void ArchiveViewWindow::reset()
{
	FHeadersRequests.clear();
	FCollectionsRequests.clear();

	FModel->clear();
	FContactModelItems.clear();

	FLoadedPages.clear();
	FCollections.clear();
	FCurrentHeaders.clear();

	if (FContactJid.isEmpty())
		setWindowTitle(tr("Conversation history - %1").arg(streamJid().bare()));
	else
		setWindowTitle(tr("Conversation history with %1 - %2").arg(contactName(FContactJid),streamJid().bare()));

	clearMessages();
	setPageStatus(RequestStarted);
	FHeadersRequestTimer.start(0);
}

QString ArchiveViewWindow::contactName(const Jid &AContactJid, bool AShowResource) const
{
	IRosterItem ritem = FRoster->rosterItem(AContactJid);
	QString name = !ritem.name.isEmpty() ? ritem.name : AContactJid.bare();
	if (AShowResource && !AContactJid.resource().isEmpty())
		name = name + "/" +AContactJid.resource();
	return name;
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
	item->setData(AHeader.with.pFull(),HDR_HEADER_WITH);
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

bool ArchiveViewWindow::isJidMatched(const Jid &ARequested, const Jid &AHeaderJid) const
{
	if (ARequested.pBare() != AHeaderJid.pBare())
		return false;
	else if (!ARequested.resource().isEmpty() && ARequested.pResource()!=AHeaderJid.pResource())
		return false;
	return true;
}

QList<QStandardItem *> ArchiveViewWindow::findHeaderItems(const IArchiveRequest &ARequest, QStandardItem *AParent) const
{
	QList<QStandardItem *> items;
	QStandardItem *parent = AParent!=NULL ? AParent : FModel->invisibleRootItem();
	for (int row = 0; row<parent->rowCount() ; row++)
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
	ui.trvHeaders->setEnabled(AStatus == RequestFinished);
	ui.wdtArchiveSearch->setEnabled(AStatus == RequestFinished);
	ui.pbtHeadersUpdate->setEnabled(AStatus != RequestStarted);

	if (AStatus == RequestFinished)
	{
		ui.trvHeaders->setFocus(Qt::MouseFocusReason);
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
		ui.stbStatusBar->showMessage(tr("Failed to load conversation headers: %1").arg(AMessage));
	}

	onArchiveSearchChanged(ui.lneArchiveSearch->text());
}

void ArchiveViewWindow::setMessagesStatus(RequestStatus AStatus, const QString &AMessage)
{
	if (AStatus == RequestFinished)
	{
		if (FCurrentHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Select contact or single conversation"));
		else
			ui.stbStatusBar->showMessage(tr("%n conversation(s) loaded",0,FCurrentHeaders.count()));
		onTextSearchTimerTimeout();
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
		"<table width='100%' cellpadding='0' cellspacing='0'>"
		"  <tr>"
		"    <td style='padding-left:15px; padding-right:15px;'><hr><span style='color:darkCyan;'>%info%</span>%subject%<hr></td>"
		"  </tr>"
		"</table>";

	QString info;
	if (!FViewOptions.isGroupchat)
		info = Qt::escape(tr("Conversation with %1 started at %2.").arg(contactName(ACollection.header.with,true),ACollection.header.start.toString()));
	else
		info = Qt::escape(tr("Conversation in conference %1 started at %2.").arg(ACollection.header.with.bare(),ACollection.header.start.toString()));

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
			subject += Qt::escape(ACollection.header.subject);
		}
	}

	QString tmpl = infoTmpl;
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
	tmpl.replace("%message%",Qt::escape(ANote));

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

		FViewOptions.isGroupchat = false;
		for (int i=0; !FViewOptions.isGroupchat && i<ACollection.body.messages.count(); i++)
			FViewOptions.isGroupchat = ACollection.body.messages.at(i).type()==Message::GroupChat;

		if (FMessageStyles)
		{
			IMessageStyleOptions soptions = FMessageStyles->styleOptions(FViewOptions.isGroupchat ? Message::GroupChat : Message::Chat);
			
			QFont font;
			int fontSize = soptions.extended.value("fontSize").toInt();
			if (fontSize>0)
				font.setPointSize(fontSize);
			QString fontFamily = soptions.extended.value("fontFamily").toString();
			if (!fontFamily.isEmpty())
				font.setFamily(fontFamily);
			ui.tbrMessages->document()->setDefaultFont(font);

			FViewOptions.style = FViewOptions.isGroupchat ? FMessageStyles->styleForOptions(soptions) : NULL;
		}

		FViewOptions.selfName = Qt::escape(FMessageStyles!=NULL ? FMessageStyles->contactName(streamJid()) : streamJid().bare());
		FViewOptions.contactName = Qt::escape(FMessageStyles!=NULL ? FMessageStyles->contactName(ACollection.header.with) : contactName(ACollection.header.with));
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

			if (FViewOptions.isGroupchat)
			{
				options.type |= IMessageContentOptions::TypeGroupchat;
				options.direction = IMessageContentOptions::DirectionIn;
				options.senderName = Qt::escape(senderJid.resource());
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
			messageIt++;
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
			noteIt++;
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
	QDate start = currentPage();
	QDate end = start.addMonths(1);
	if (!FLoadedPages.contains(start))
	{
		IArchiveRequest request;
		request.with = contactJid().bare();
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
		if (index.data(HDR_TYPE).toInt() == HIT_CONTACT)
		{
			int rows = index.model()->rowCount(index);
			for (int row=0; row<rows; row++)
			{
				IArchiveHeader header = modelIndexHeader(index.child(row,0));
				if (header.with.isValid() && header.start.isValid())
					FCurrentHeaders.append(header);
			}
		}
		else if(index.data(HDR_TYPE).toInt() == HIT_HEADER)
		{
			IArchiveHeader header = modelIndexHeader(index);
			if (header.with.isValid() && header.start.isValid())
				FCurrentHeaders.append(header);
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

void ArchiveViewWindow::onArchiveSearchUpdate()
{
	setSearchString(ui.lneArchiveSearch->text());
	ui.lneTextSearch->setText(ui.lneArchiveSearch->text());
}

void ArchiveViewWindow::onArchiveSearchChanged(const QString &AText)
{
	ui.tlbArchiveSearchUpdate->setEnabled(searchString()!=AText);
}

void ArchiveViewWindow::onTextSearchTimerTimeout()
{
	QList<QTextEdit::ExtraSelection> selections;
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
				selections.append(selection);
				cursor.clearSelection();
			}
		} while (!cursor.isNull());

		ui.lblTextSearchInfo->setVisible(true);
	}
	else
	{
		ui.lblTextSearchInfo->setVisible(false);
	}

	if (!selections.isEmpty())
	{
		ui.tbrMessages->setTextCursor(selections.first().cursor);
		ui.tbrMessages->ensureCursorVisible();
		ui.lblTextSearchInfo->setText(tr("Found %n occurrence(s)",0,selections.count()));
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

	if (!ui.lneTextSearch->text().isEmpty() && selections.isEmpty())
	{
		QPalette palette = ui.lblTextSearch->palette();
		palette.setColor(QPalette::Active,QPalette::Base,QColor("orangered"));
		palette.setColor(QPalette::Active,QPalette::Text,Qt::white);
		ui.lneTextSearch->setPalette(palette);
	}
	else
	{
		ui.lneTextSearch->setPalette(QPalette());
	}

	ui.tbrMessages->setExtraSelections(selections);
	ui.tlbTextSearchNext->setEnabled(!selections.isEmpty());
	ui.tlbTextSearchPrev->setEnabled(!selections.isEmpty());
	ui.chbTextSearchCaseSensitive->setEnabled(!selections.isEmpty() || !ui.lneTextSearch->text().isEmpty());
}

void ArchiveViewWindow::onTextSearchNextClicked()
{
	QList<QTextEdit::ExtraSelection> selections = ui.tbrMessages->extraSelections();
	if (!selections.isEmpty())
	{
		QTextEdit::ExtraSelection selection;
		QListIterator<QTextEdit::ExtraSelection> it(selections);
		it.toFront();
		do {
			selection = it.next();
		} while (it.hasNext() && selection.cursor.position()<=ui.tbrMessages->textCursor().position());
		ui.tbrMessages->setTextCursor(selection.cursor);
		ui.tbrMessages->ensureCursorVisible();
	}
}

void ArchiveViewWindow::onTextSearchPreviousClicked()
{
	QList<QTextEdit::ExtraSelection> selections = ui.tbrMessages->extraSelections();
	if (!selections.isEmpty())
	{
		QTextEdit::ExtraSelection selection;
		QListIterator<QTextEdit::ExtraSelection> it(selections);
		it.toBack();
		do {
			selection = it.previous();
		} while (it.hasPrevious() && selection.cursor.position()>=ui.tbrMessages->textCursor().position());
		ui.tbrMessages->setTextCursor(selection.cursor);
		ui.tbrMessages->ensureCursorVisible();
	}
}

void ArchiveViewWindow::onTextSearchCaseSensitivityChanged()
{
	FTextSearchTimer.start(0);
}

void ArchiveViewWindow::onTextSearchTextChanged(const QString &AText)
{
	Q_UNUSED(AText);
	FTextSearchTimer.start(TEXT_SEARCH_TIMEOUT);
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
				.arg(Qt::escape(contactName(request.with)))
				.arg(QLocale().monthName(request.start.date().month()))
				.arg(request.start.date().year());
		}
		else if (request.start.isValid())
		{
			message = tr("Do you want to remove conversation with <b>%1</b> started at <b>%2</b>?")
				.arg(Qt::escape(contactName(request.with,true)))
				.arg(request.start.toString());
		}
		else
		{
			message = tr("Do you want to remove <b>all</b> conversation history with <b>%1</b>?")
				.arg(Qt::escape(contactName(request.with,true)));
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
			Action *removeAll = new Action(menu);
			removeAll->setText(tr("Remove all History"));
			removeAll->setData(ADR_HEADER_WITH,item->data(HDR_CONTACT_JID));
			connect(removeAll,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removeAll);

			Action *removePage = new Action(menu);
			removePage->setText(tr("Remove History for %1 %2").arg(QLocale().monthName(ui.spwSelectPage->monthShown())).arg(ui.spwSelectPage->yearShown()));
			removePage->setData(ADR_HEADER_WITH,item->data(HDR_CONTACT_JID));
			removePage->setData(ADR_HEADER_START,QDateTime(currentPage()));
			removePage->setData(ADR_HEADER_END,QDateTime(currentPage()).addMonths(1));
			connect(removePage,SIGNAL(triggered()),SLOT(onRemoveCollectionsByAction()));
			menu->addAction(removePage);
		}
		else if (itemType == HIT_HEADER)
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

void ArchiveViewWindow::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FHeadersRequests.contains(AId))
	{
		QDate start = FHeadersRequests.take(AId);
		if (currentPage() == start)
			setPageStatus(RequestError, AError);
		FLoadedPages.removeAll(start);
	}
	else if (FCollectionsRequests.contains(AId))
	{
		IArchiveHeader header = FCollectionsRequests.take(AId);
		if (currentLoadingHeader() == header)
			setMessagesStatus(RequestError, AError);
	}
	else if (FRemoveRequests.contains(AId))
	{
		IArchiveRequest request = FRemoveRequests.take(AId);
		request.text = searchString();
		request.end = !request.end.isValid() ? request.start : request.end;
		setRequestStatus(RequestError,tr("Failed to remove conversations: %1").arg(AError));
		updateHeaders(request);
		removeHeaderItems(request);
	}
}

void ArchiveViewWindow::onArchiveHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FHeadersRequests.contains(AId))
	{
		QDate start = FHeadersRequests.take(AId);
		for (QList<IArchiveHeader>::const_iterator it = AHeaders.constBegin(); it!=AHeaders.constEnd(); it++)
		{
			if (!FCollections.contains(*it) && it->with.isValid() && it->start.isValid())
			{
				IArchiveCollection collection;
				collection.header = *it;
				FCollections.insert(collection.header,collection);
				createHeaderItem(collection.header);
			}
		}
		if (currentPage() == start)
		{
			setPageStatus(RequestFinished);
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
