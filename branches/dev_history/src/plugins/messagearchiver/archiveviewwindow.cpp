#include "archiveviewwindow.h"

#include <QLocale>
#include <QItemSelectionModel>

#define HEADERS_LOAD_TIMEOUT         500
#define COLLECTIONS_LOAD_TIMEOUT     200
#define TEXT_SEARCH_TIMEOUT          500

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
	ui.lneArchiveSearch->setPlaceholderText(tr("Search in archive"));
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
		setWindowTitle(tr("Chat history - %1").arg(streamJid().bare()));
	else
		setWindowTitle(tr("Chat history with %1 - %2").arg(contactName(FContactJid),streamJid().bare()));

	clearMessages();
	setPageStatus(PageLoading);
	FHeadersRequestTimer.start(0);
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

QDate ArchiveViewWindow::currentPage() const
{
	return QDate(ui.spwSelectPage->yearShown(),ui.spwSelectPage->monthShown(),1);
}

void ArchiveViewWindow::setPageStatus(PageStatus AStatus, const QString &AMessage)
{
	ui.trvHeaders->setEnabled(AStatus == PageReady);
	ui.wdtArchiveSearch->setEnabled(AStatus == PageReady);
	ui.pbtHeadersUpdate->setEnabled(AStatus != PageLoading);

	if (AStatus == PageReady)
	{
		ui.trvHeaders->setFocus(Qt::MouseFocusReason);
		ui.trvHeaders->selectionModel()->clearSelection();
		ui.trvHeaders->setCurrentIndex(QModelIndex());
		ui.stbStatusBar->showMessage(tr("Conversation headers loaded"));
	}
	else if(AStatus == PageLoading)
	{
		ui.stbStatusBar->showMessage(tr("Loading conversation headers..."));
	}
	else if (AStatus == PageLoadError)
	{
		ui.stbStatusBar->showMessage(tr("Failed to load conversation headers: %1").arg(AMessage));
	}

	onArchiveSearchChanged(ui.lneArchiveSearch->text());
}

void ArchiveViewWindow::setMessagesStatus(MessagesStatus AStatus, const QString &AMessage)
{
	if (AStatus == MessagesReady)
	{
		if (FCurrentHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Select contact or single conversation"));
		else
			ui.stbStatusBar->showMessage(tr("%n conversation(s) loaded",0,FCurrentHeaders.count()));
		onTextSearchTimerTimeout();
	}
	else if(AStatus == MessagesLoading)
	{
		if (FCurrentHeaders.isEmpty())
			ui.stbStatusBar->showMessage(tr("Loading conversations..."));
		else
			ui.stbStatusBar->showMessage(tr("Loading %1 of %2 conversation...").arg(FLoadHeaderIndex+1).arg(FCurrentHeaders.count()));
	}
	else if (AStatus == MessagesLoadError)
	{
		ui.stbStatusBar->showMessage(tr("Failed to load conversations: %1").arg(AMessage));
	}
	ui.wdtTextSearch->setEnabled(AStatus==MessagesReady && !FCurrentHeaders.isEmpty());
}

void ArchiveViewWindow::clearMessages()
{
	FLoadHeaderIndex = 0;
	FCurrentHeaders.clear();
	ui.tbrMessages->clear();
	FCollectionShowTimer.stop();
	setMessagesStatus(MessagesReady);
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
				setMessagesStatus(MessagesLoading);
			}
			else
			{
				setMessagesStatus(MessagesLoadError,tr("Archive is not accessible"));
			}
		}
		else
		{
			showCollection(collection);
			setMessagesStatus(MessagesLoading);
			FCollectionShowTimer.start(0);
		}
	}

	if (FCurrentHeaders.isEmpty())
		clearMessages();
	else if (FLoadHeaderIndex == FCurrentHeaders.count())
		setMessagesStatus(MessagesReady);
}

IArchiveHeader ArchiveViewWindow::currentLoadingHeader() const
{
	return FCurrentHeaders.value(FLoadHeaderIndex);
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
		info = Qt::escape(tr("Conversation with %1 started at %2.").arg(contactName(ACollection.header.with),ACollection.header.start.toString()));
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
		request.start = QDateTime(start);
		request.end = QDateTime(end);
		request.text = searchString();
		QString requestId = FArchiver->loadHeaders(streamJid(),request);
		if (!requestId.isEmpty())
		{
			FLoadedPages.append(start);
			FHeadersRequests.insert(requestId,start);
		}
		else
		{
			setPageStatus(PageLoadError,tr("Archive is not accessible"));
		}
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
		setPageStatus(PageLoading);
	}
	else if (!FHeadersRequests.values().contains(start))
	{
		FHeadersRequestTimer.stop();
		setPageStatus(PageReady);
	}
	else
	{
		setPageStatus(PageLoading);
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
		setMessagesStatus(MessagesLoading);
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

void ArchiveViewWindow::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FHeadersRequests.contains(AId))
	{
		QDate start = FHeadersRequests.take(AId);
		if (currentPage() == start)
			setPageStatus(PageLoadError, AError);
		FLoadedPages.removeAll(start);
	}
	else if (FCollectionsRequests.contains(AId))
	{
		IArchiveHeader header = FCollectionsRequests.take(AId);
		if (currentLoadingHeader() == header)
			setMessagesStatus(MessagesLoadError, AError);
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
			setPageStatus(PageReady);
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
