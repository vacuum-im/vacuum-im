#include "bookmarks.h"

#include <QInputDialog>
#include <QDesktopServices>
#include <QItemEditorFactory>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/shortcuts.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/discoitemdataroles.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosteredithandlerorders.h>
#include <utils/advanceditemdelegate.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/logger.h>
#include <utils/menu.h>

#define PST_BOOKMARKS                    "storage"

#define ADR_STREAM_JID                   Action::DR_StreamJid
#define ADR_BOOKMARK_TYPE                Action::DR_Parametr1
#define ADR_BOOKMARK_NAME                Action::DR_Parametr2
#define ADR_BOOKMARK_ROOM_JID            Action::DR_UserDefined+1
#define ADR_BOOKMARK_ROOM_NICK           Action::DR_UserDefined+2
#define ADR_BOOKMARK_ROOM_PASSWORD       Action::DR_UserDefined+3
#define ADR_BOOKMARK_URL                 Action::DR_UserDefined+4
#define ADR_DISCO_JID                    Action::DR_Parametr1
#define ADR_DISCO_NODE                   Action::DR_Parametr2
#define ADR_DISCO_NAME                   Action::DR_Parametr3

Bookmarks::Bookmarks()
{
	FPrivateStorage = NULL;
	FAccountManager = NULL;
	FMultiChatPlugin = NULL;
	FXmppUriQueries = NULL;
	FDiscovery = NULL;
	FOptionsManager = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FRostersViewPlugin = NULL;
}

Bookmarks::~Bookmarks()
{

}

void Bookmarks::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Bookmarks");
	APluginInfo->description = tr("Allows to create bookmarks at the jabber conference and web pages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool Bookmarks::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataUpdated(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataUpdated(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataRemoved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataRemoved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorageClosed(const Jid &)));
		}
		else
		{
			LOG_WARNING("Failed to find required interface: IPrivateStorage");
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
		if (FMultiChatPlugin)
		{
			connect(FMultiChatPlugin->instance(),SIGNAL(multiChatWindowCreated(IMultiUserChatWindow *)),
				SLOT(onMultiChatWindowCreated(IMultiUserChatWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(), SIGNAL(discoItemsWindowCreated(IDiscoItemsWindow *)),SLOT(onDiscoItemsWindowCreated(IDiscoItemsWindow *)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0, NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0, NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexDestroyed(IRosterIndex *)),SLOT(onRosterIndexDestroyed(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0, NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			FRostersView = FRostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
		}
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FPrivateStorage!=NULL;
}

bool Bookmarks::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_BOOKMARK, tr("Edit bookmark"), QKeySequence::UnknownKey);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}

	if (FRostersModel)
	{
		FRostersModel->insertRosterDataHolder(RDHO_BOOKMARKS,this);
	}

	if (FRostersView)
	{
		FRostersView->insertEditHandler(REHO_BOOKMARKS_RENAME,this);
	}

	return true;
}

bool Bookmarks::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_IGNOREAUTOJOIN, false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWAUTOJOINED,false);
	return true;
}

QMultiMap<int, IOptionsWidget *> Bookmarks::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (FOptionsManager)
	{
		if (nodeTree.count()==2 && nodeTree.at(0)==OPN_ACCOUNTS)
		{
			OptionsNode aoptions = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));
			widgets.insertMulti(OWO_ACCOUNT_BOOKMARKS, FOptionsManager->optionsNodeWidget(aoptions.node("ignore-autojoin"),tr("Disable autojoin to conferences"),AParent));
		}
		else if (ANodeId == OPN_CONFERENCES)
		{
			widgets.insertMulti(OWO_CONFERENCES_SHOWAUTOJOINED, FOptionsManager->optionsNodeWidget(Options::node(OPV_MUC_GROUPCHAT_SHOWAUTOJOINED),tr("Automatically show window of conferences connected at startup"),AParent));
		}
	}
	return widgets;
}

QList<int> Bookmarks::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_BOOKMARKS)
		return QList<int>() << RDR_NAME << RDR_MUC_NICK << RDR_MUC_PASSWORD;
	return QList<int>();
}

QVariant Bookmarks::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder==RDHO_BOOKMARKS && AIndex->kind()==RIK_MUC_ITEM)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		IRosterIndex *index = const_cast<IRosterIndex *>(AIndex);
		const IBookmark bookmark = FBookmarkIndexes.value(streamJid).value(index);
		switch (ARole)
		{
		case RDR_NAME:
			return !bookmark.name.isEmpty() ? QVariant(bookmark.name) : QVariant();
		case RDR_MUC_NICK:
			return !bookmark.conference.nick.isEmpty() ? QVariant(bookmark.conference.nick) : QVariant();
		case RDR_MUC_PASSWORD:
			return !bookmark.conference.password.isEmpty() ? QVariant(bookmark.conference.password) : QVariant();
		}
	}
	return QVariant();
}

bool Bookmarks::setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole)
{
	Q_UNUSED(AOrder); Q_UNUSED(AIndex); Q_UNUSED(ARole); Q_UNUSED(AValue);
	return false;
}

quint32 Bookmarks::rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const
{
	if (AOrder==REHO_BOOKMARKS_RENAME && ADataRole==RDR_NAME && AIndex.data(RDR_KIND).toInt()==RIK_MUC_ITEM)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::Conference;
		bookmark.conference.roomJid = AIndex.data(RDR_PREP_BARE_JID).toString();
		if (FBookmarks.value(AIndex.data(RDR_STREAM_JID).toString()).contains(bookmark))
			return AdvancedDelegateItem::DisplayId;
	}
	return AdvancedDelegateItem::NullId;
}

AdvancedDelegateEditProxy *Bookmarks::rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex)
{
	Q_UNUSED(AIndex);
	if (AOrder==REHO_BOOKMARKS_RENAME && ADataRole==RDR_NAME)
		return this;
	return NULL;
}

bool Bookmarks::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex)
{
	Q_UNUSED(AModel);
	if (ADelegate->editRole()==RDR_NAME && AIndex.data(RDR_KIND)==RIK_MUC_ITEM)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::Conference;
		bookmark.conference.roomJid = AIndex.data(RDR_PREP_BARE_JID).toString();

		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		int index = FBookmarks.value(streamJid).indexOf(bookmark);
		if (index >= 0)
		{
			QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);
			IBookmark bookmark = bookmarkList.at(index);

			QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY);
			QByteArray name = ADelegate->editorFactory()->valuePropertyName(value.type());
			QString newName = AEditor->property(name).toString();
			if (!newName.isEmpty() && bookmark.name!=newName)
			{
				LOG_STRM_INFO(streamJid,QString("Renaming bookmark %1 to %2 from roster").arg(bookmark.name,newName));
				bookmark.name = newName;
				bookmarkList.replace(index,bookmark);
				setBookmarks(streamJid,bookmarkList);
			}
		}
		else
		{
			REPORT_ERROR("Failed to rename bookmark from roster: Invalid params");
		}
		return true;
	}
	return false;
}

bool Bookmarks::isReady(const Jid &AStreamJid) const
{
	return FBookmarks.contains(AStreamJid);
}

bool Bookmarks::isValidBookmark(const IBookmark &ABookmark) const
{
	if (ABookmark.type == IBookmark::Url)
		return ABookmark.url.url.isValid();
	if (ABookmark.type == IBookmark::Conference)
		return ABookmark.conference.roomJid.isValid();
	return false;
}

QList<IBookmark> Bookmarks::bookmarks(const Jid &AStreamJid) const
{
	return FBookmarks.value(AStreamJid);
}

bool Bookmarks::addBookmark(const Jid &AStreamJid, const IBookmark &ABookmark)
{
	if (isReady(AStreamJid) && isValidBookmark(ABookmark))
	{
		LOG_STRM_INFO(AStreamJid,QString("Adding new bookmark, name=%1").arg(ABookmark.name));
		QList<IBookmark> bookmarkList = bookmarks(AStreamJid);
		bookmarkList.append(ABookmark);
		return setBookmarks(AStreamJid,bookmarkList);
	}
	else if (!isReady(AStreamJid))
	{
		REPORT_ERROR("Failed to add bookmark: Stream is not ready");
	}
	else if (!isValidBookmark(ABookmark))
	{
		REPORT_ERROR("Failed to add bookmark: Invalid bookmark");
	}
	return false;
}

bool Bookmarks::setBookmarks(const Jid &AStreamJid, const QList<IBookmark> &ABookmarks)
{
	if (isReady(AStreamJid))
	{
		QDomDocument doc;
		doc.appendChild(doc.createElement("bookmarks"));
		QDomElement elem = doc.documentElement().appendChild(doc.createElementNS(NS_STORAGE_BOOKMARKS,PST_BOOKMARKS)).toElement();
		saveBookmarksToXML(elem,ABookmarks);
		if (!FPrivateStorage->saveData(AStreamJid,elem).isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,"Bookmarks save request sent");
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send save bookmarks request");
		}
	}
	else
	{
		REPORT_ERROR("Failed to save bookmarks: Stream is not ready");
	}
	return false;
}

int Bookmarks::execEditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent) const
{
	EditBookmarkDialog *dialog = new EditBookmarkDialog(ABookmark,AParent);
	return dialog->exec();
}

void Bookmarks::showEditBookmarksDialog(const Jid &AStreamJid)
{
	if (isReady(AStreamJid))
	{
		EditBookmarksDialog *dialog = FDialogs.value(AStreamJid,NULL);
		if (!dialog)
		{
			dialog = new EditBookmarksDialog(this,AStreamJid,bookmarks(AStreamJid),NULL);
			FDialogs.insert(AStreamJid,dialog);
			connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onEditBookmarksDialogDestroyed()));
		}
		dialog->show();
	}
	else
	{
		REPORT_ERROR("Failed to open edit bookmarks dialog: Stream is not ready");
	}
}

void Bookmarks::updateConferenceIndexes(const Jid &AStreamJid)
{
	if (FMultiChatPlugin)
	{
		QSet<IBookmark> newBookmarks = FBookmarks.value(AStreamJid).toSet();
		QSet<IBookmark> curBookarks = FBookmarkIndexes.value(AStreamJid).values().toSet();
		QSet<IBookmark> removeBookmarks = curBookarks - newBookmarks;

		foreach(const IBookmark &bookmark, newBookmarks)
		{
			if (bookmark.type == IBookmark::Conference)
			{
				IRosterIndex *index = FMultiChatPlugin->getMultiChatRosterIndex(AStreamJid,bookmark.conference.roomJid,bookmark.conference.nick,bookmark.conference.password);
				FBookmarkIndexes[AStreamJid].insert(index,bookmark);
				emit rosterDataChanged(index,RDR_NAME);
				emit rosterDataChanged(index,RDR_MUC_NICK);
				emit rosterDataChanged(index,RDR_MUC_PASSWORD);
			}
		}

		foreach(const IBookmark &bookmark, removeBookmarks)
		{
			IRosterIndex *index = FBookmarkIndexes.value(AStreamJid).key(bookmark);
			IMultiUserChatWindow *window = FMultiChatPlugin->findMultiChatWindow(AStreamJid,bookmark.conference.roomJid);
			if (window == NULL)
				FRostersModel->removeRosterIndex(index);
			FBookmarkIndexes[AStreamJid].remove(index);
		}
	}
}

bool Bookmarks::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	int singleKind = -1;
	bool hasReadyStreams = false;
	for(int i=0; i<ASelected.count(); i++)
	{
		IRosterIndex *index = ASelected.at(i);
		int indexKind = index->kind();
		if (indexKind!=RIK_MUC_ITEM && indexKind!=RIK_STREAM_ROOT)
			return false;
		else if (singleKind!=-1 && singleKind!=indexKind)
			return false;
		else if (indexKind==RIK_MUC_ITEM && !isReady(index->data(RDR_STREAM_JID).toString()))
			return false;
		else if (indexKind==RIK_STREAM_ROOT && isReady(index->data(RDR_STREAM_JID).toString()))
			hasReadyStreams = true;
		else if (indexKind==RIK_STREAM_ROOT && !hasReadyStreams && i==ASelected.count()-1)
			return false;
		singleKind = indexKind;
	}
	return !ASelected.isEmpty();
}

QList<IBookmark> Bookmarks::loadBookmarksFromXML(const QDomElement &AElement) const
{
	QList<IBookmark> bookmarkList;

	QDomElement elem = AElement.firstChildElement();
	while (!elem.isNull())
	{
		if (elem.tagName() == "conference")
		{
			IBookmark bookmark;
			bookmark.type = IBookmark::Conference;
			bookmark.name = elem.attribute("name","conference");
			bookmark.conference.roomJid = elem.attribute("jid");
			bookmark.conference.autojoin = QVariant(elem.attribute("autojoin")).toBool();
			bookmark.conference.nick = elem.firstChildElement("nick").text();
			bookmark.conference.password = elem.firstChildElement("password").text();
			bookmark.name = bookmark.name.isEmpty() ? bookmark.conference.roomJid.uBare() : bookmark.name;
			bookmarkList.append(bookmark);
		}
		else if (elem.tagName() == "url")
		{
			IBookmark bookmark;
			bookmark.type = IBookmark::Url;
			bookmark.name = elem.attribute("name","url");
			bookmark.url.url = elem.attribute("url");
			bookmark.name = bookmark.name.isEmpty() ? bookmark.url.url.host() : bookmark.name;
			bookmarkList.append(bookmark);
		}
		else
		{
			LOG_WARNING(QString("Failed to load bookmark from XML: Unexpected element=%1").arg(elem.tagName()));
		}
		elem = elem.nextSiblingElement();
	}

	return bookmarkList;
}

void Bookmarks::saveBookmarksToXML(QDomElement &AElement, const QList<IBookmark> &ABookmarks) const
{
	foreach(const IBookmark &bookmark, ABookmarks)
	{
		if (bookmark.type == IBookmark::Conference)
		{
			QDomElement markElem = AElement.appendChild(AElement.ownerDocument().createElement("conference")).toElement();
			markElem.setAttribute("name",bookmark.name);
			markElem.setAttribute("jid",bookmark.conference.roomJid.bare());
			markElem.setAttribute("autojoin",QVariant(bookmark.conference.autojoin).toString());
			markElem.appendChild(AElement.ownerDocument().createElement("nick")).appendChild(AElement.ownerDocument().createTextNode(bookmark.conference.nick));
			if (!bookmark.conference.password.isEmpty())
				markElem.appendChild(AElement.ownerDocument().createElement("password")).appendChild(AElement.ownerDocument().createTextNode(bookmark.conference.password));
		}
		else if (bookmark.type == IBookmark::Url)
		{
			QDomElement markElem = AElement.appendChild(AElement.ownerDocument().createElement("url")).toElement();
			markElem.setAttribute("name",bookmark.name);
			markElem.setAttribute("url",bookmark.url.url.toString());
		}
		else
		{
			REPORT_ERROR(QString("Failed to save bookmark to XML: Unexpected bookmark type=%1").arg(bookmark.type));
		}
	}
}

void Bookmarks::renameBookmark(const Jid &AStreamJid, const IBookmark &ABookmark)
{
	QList<IBookmark> bookmarkList = FBookmarks.value(AStreamJid);

	int index = bookmarkList.indexOf(ABookmark);
	if (index >= 0)
	{
		IBookmark bookmark = bookmarkList.at(index);
		QString newName = QInputDialog::getText(NULL,tr("Rename Bookmark"),tr("Enter bookmark name:"),QLineEdit::Normal,bookmark.name);
		if (!newName.isEmpty() && newName!=bookmark.name)
		{
			LOG_STRM_INFO(AStreamJid,QString("Renaming bookmark %1 to %2").arg(bookmark.name,newName));
			bookmark.name = newName;
			bookmarkList.replace(index,bookmark);
			setBookmarks(AStreamJid,bookmarkList);
		}
	}
	else
	{
		REPORT_ERROR("Failed to rename bookmark: Bookmark not found");
	}
}

void Bookmarks::startBookmark(const Jid &AStreamJid, const IBookmark &ABookmark, bool AShowWindow)
{
	if (isValidBookmark(ABookmark))
	{
		LOG_STRM_INFO(AStreamJid,QString("Starting bookmark, name=%1").arg(ABookmark.name));
		if (FMultiChatPlugin && ABookmark.type==IBookmark::Conference)
		{
			IMultiUserChatWindow *window = FMultiChatPlugin->getMultiChatWindow(AStreamJid,ABookmark.conference.roomJid,ABookmark.conference.nick,ABookmark.conference.password);
			if (window)
			{
				if (AShowWindow)
					window->showTabPage();
				if (!window->multiUserChat()->isConnected())
					window->multiUserChat()->sendStreamPresence();
			}
		}
		else if (ABookmark.type == IBookmark::Url)
		{
			if (FXmppUriQueries && ABookmark.url.url.scheme()=="xmpp")
				FXmppUriQueries->openXmppUri(AStreamJid, ABookmark.url.url);
			else
				QDesktopServices::openUrl(ABookmark.url.url);
		}
	}
	else
	{
		REPORT_ERROR("Failed to start bookmark: Invalid bookmark");
	}
}

void Bookmarks::onPrivateStorageOpened(const Jid &AStreamJid)
{
	if (!FPrivateStorage->loadData(AStreamJid,PST_BOOKMARKS,NS_STORAGE_BOOKMARKS).isEmpty())
		LOG_STRM_INFO(AStreamJid,"Bookmarks load request sent");
	else
		LOG_STRM_WARNING(AStreamJid,"Failed to send load bookmarks request");
}

void Bookmarks::onPrivateDataUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.tagName()==PST_BOOKMARKS && AElement.namespaceURI()==NS_STORAGE_BOOKMARKS)
	{
		LOG_STRM_INFO(AStreamJid,"Bookmarks loaded");
		FBookmarks[AStreamJid] = loadBookmarksFromXML(AElement);

		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
		if (account==NULL || !account->optionsNode().value("ignore-autojoin").toBool())
		{
			bool showAutoJoined = Options::node(OPV_MUC_GROUPCHAT_SHOWAUTOJOINED).value().toBool();
			foreach(const IBookmark &bookmark, FBookmarks.value(AStreamJid))
			{
				if (bookmark.type==IBookmark::Conference && bookmark.conference.autojoin)
				{
					if (showAutoJoined && FMultiChatPlugin && FMultiChatPlugin->findMultiChatWindow(AStreamJid,bookmark.conference.roomJid)!=NULL)
						showAutoJoined = false;
					startBookmark(AStreamJid,bookmark,showAutoJoined);
				}
			}
		}

		updateConferenceIndexes(AStreamJid);
		emit bookmarksChanged(AStreamJid);
	}
}

void Bookmarks::onPrivateDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.tagName()==PST_BOOKMARKS && AElement.namespaceURI()==NS_STORAGE_BOOKMARKS)
	{
		FBookmarks[AStreamJid].clear();
		updateConferenceIndexes(AStreamJid);
		emit bookmarksChanged(AStreamJid);
	}
}

void Bookmarks::onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (ATagName==PST_BOOKMARKS && ANamespace==NS_STORAGE_BOOKMARKS)
	{
		if (!FPrivateStorage->loadData(AStreamJid,PST_BOOKMARKS,NS_STORAGE_BOOKMARKS).isEmpty())
			LOG_STRM_INFO(AStreamJid,"Bookmarks reload request sent");
		else
			LOG_STRM_WARNING(AStreamJid,"Failed to send reload bookmarks request");
	}
}

void Bookmarks::onPrivateStorageClosed(const Jid &AStreamJid)
{
	delete FDialogs.take(AStreamJid);
	FBookmarks.remove(AStreamJid);
	updateConferenceIndexes(AStreamJid);
	FBookmarkIndexes.remove(AStreamJid);
}

void Bookmarks::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void Bookmarks::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		bool isMultiSelection = AIndexes.count()>1;
		int indexKind = AIndexes.first()->kind();
		if (indexKind == RIK_STREAM_ROOT)
		{
			Menu *bookmarksMenu = new Menu(AMenu);
			bookmarksMenu->setTitle(tr("Bookmarks"));
			bookmarksMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS);

			int streamGroup = AG_BBM_BOOKMARKS_ITEMS;
			foreach(IRosterIndex *index, AIndexes)
			{
				QList<IBookmark> bookmarkList = FBookmarks.value(index->data(RDR_STREAM_JID).toString());
				if (!bookmarkList.isEmpty())
				{
					for (int i=0; i<bookmarkList.count(); i++)
					{
						const IBookmark &bookmark = bookmarkList.at(i);
						if (isValidBookmark(bookmark) && !bookmark.name.isEmpty())
						{
							Action *startAction = new Action(bookmarksMenu);
							startAction->setIcon(RSR_STORAGE_MENUICONS,bookmark.type==IBookmark::Url ? MNI_BOOKMARKS_URL : MNI_BOOKMARKS_ROOM);
							startAction->setText(TextManager::getElidedString(bookmark.name,Qt::ElideRight,50));
							startAction->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
							startAction->setData(ADR_BOOKMARK_TYPE,bookmark.type);
							startAction->setData(ADR_BOOKMARK_ROOM_JID,bookmark.conference.roomJid.bare());
							startAction->setData(ADR_BOOKMARK_URL,bookmark.url.url.toString());
							connect(startAction,SIGNAL(triggered(bool)),SLOT(onStartBookmarkActionTriggered(bool)));
							bookmarksMenu->addAction(startAction,streamGroup);
						}
					}
					streamGroup++;
				}
			}

			if (!bookmarksMenu->isEmpty())
				AMenu->addAction(bookmarksMenu->menuAction(),AG_RVCM_BOOKMARS_MENU);
			else
				delete bookmarksMenu;

			if (!isMultiSelection)
			{
				Action *editAction = new Action(AMenu);
				editAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
				editAction->setText(tr("Edit Bookmarks"));
				editAction->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID).toString());
				connect(editAction,SIGNAL(triggered(bool)),SLOT(onEditBookmarksActionTriggered(bool)));
				AMenu->addAction(editAction,AG_RVCM_BOOKMARS_MENU);
			}
		}
		else if (indexKind == RIK_MUC_ITEM)
		{
			IBookmark bookmark = !isMultiSelection ? FBookmarkIndexes.value(AIndexes.first()->data(RDR_STREAM_JID).toString()).value(AIndexes.first()) : IBookmark();
			QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_NAME<<RDR_PREP_BARE_JID<<RDR_MUC_NICK<<RDR_MUC_PASSWORD,RDR_PREP_BARE_JID,RDR_STREAM_JID);

			if (isMultiSelection || !isValidBookmark(bookmark))
			{
				Action *appendAction = new Action(AMenu);
				appendAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_ADD);
				appendAction->setText(tr("Add to Bookmarks"));
				appendAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				appendAction->setData(ADR_BOOKMARK_NAME,rolesMap.value(RDR_NAME));
				appendAction->setData(ADR_BOOKMARK_ROOM_JID,rolesMap.value(RDR_PREP_BARE_JID));
				appendAction->setData(ADR_BOOKMARK_ROOM_NICK,rolesMap.value(RDR_MUC_NICK));
				appendAction->setData(ADR_BOOKMARK_ROOM_PASSWORD,rolesMap.value(RDR_MUC_PASSWORD));
				connect(appendAction,SIGNAL(triggered(bool)),SLOT(onAddBookmarksActionTriggered(bool)));
				AMenu->addAction(appendAction,AG_RVCM_BOOKMARS_TOOLS);
			}

			if (isMultiSelection || isValidBookmark(bookmark))
			{
				Action *removeAction = new Action(AMenu);
				removeAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_REMOVE);
				removeAction->setText(tr("Remove from Bookmarks"));
				removeAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				removeAction->setData(ADR_BOOKMARK_ROOM_JID,rolesMap.value(RDR_PREP_BARE_JID));
				connect(removeAction,SIGNAL(triggered(bool)),SLOT(onRemoveBookmarksActionTriggered(bool)));
				AMenu->addAction(removeAction,AG_RVCM_BOOKMARS_TOOLS);
			}

			if (!isMultiSelection && isValidBookmark(bookmark))
			{
				Action *editAction = new Action(AMenu);
				editAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
				editAction->setText(tr("Edit Bookmark"));
				editAction->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID));
				editAction->setData(ADR_BOOKMARK_ROOM_JID,bookmark.conference.roomJid.bare());
				connect(editAction,SIGNAL(triggered(bool)),SLOT(onEditBookmarkActionTriggered(bool)));
				AMenu->addAction(editAction,AG_RVCM_BOOKMARS_TOOLS);

				Action *renameAction = new Action(AMenu);
				renameAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
				renameAction->setText(tr("Rename Bookmark"));
				renameAction->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID));
				renameAction->setData(ADR_BOOKMARK_ROOM_JID,bookmark.conference.roomJid.bare());
				renameAction->setShortcutId(SCT_ROSTERVIEW_RENAME);
				connect(renameAction,SIGNAL(triggered(bool)),SLOT(onRenameBookmarkActionTriggered(bool)));
				AMenu->addAction(renameAction,AG_RVCM_BOOKMARS_TOOLS);
			}

			if (!isMultiSelection)
			{
				Action *autoJoinAction = new Action(AMenu);
				autoJoinAction->setCheckable(true);
				autoJoinAction->setChecked(bookmark.conference.autojoin);
				autoJoinAction->setText(tr("Join to Conference at Startup"));
				autoJoinAction->setData(ADR_STREAM_JID,AIndexes.first()->data(RDR_STREAM_JID));
				autoJoinAction->setData(ADR_BOOKMARK_NAME,AIndexes.first()->data(RDR_NAME));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_JID,AIndexes.first()->data(RDR_PREP_BARE_JID));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_NICK,AIndexes.first()->data(RDR_MUC_NICK));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_PASSWORD,AIndexes.first()->data(RDR_MUC_PASSWORD));
				connect(autoJoinAction,SIGNAL(triggered(bool)),SLOT(onChangeBookmarkAutoJoinActionTriggered(bool)));
				AMenu->addAction(autoJoinAction,AG_RVCM_BOOKMARS_TOOLS);
			}
		}
	}
}

void Bookmarks::onRosterIndexDestroyed(IRosterIndex *AIndex)
{
	Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
	if (FBookmarks.contains(streamJid))
		FBookmarkIndexes[streamJid].remove(AIndex);
}

void Bookmarks::onMultiChatWindowCreated(IMultiUserChatWindow *AWindow)
{
	Action *action = new Action(AWindow->instance());
	action->setText(tr("Edit Bookmark"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
	action->setShortcutId(SCT_MESSAGEWINDOWS_MUC_BOOKMARK);
	connect(action,SIGNAL(triggered(bool)),SLOT(onMultiChatWindowAddBookmarkActionTriggered(bool)));
	AWindow->toolBarWidget()->toolBarChanger()->insertAction(action,TBG_MCWTBW_BOOKMARKS);
}

void Bookmarks::onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow)
{
	connect(AWindow->instance(),SIGNAL(indexContextMenu(const QModelIndex &, Menu *)),SLOT(onDiscoIndexContextMenu(const QModelIndex &, Menu *)));
}

void Bookmarks::onDiscoIndexContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
	Action *action = new Action(AMenu);
	action->setText(tr("Add to Bookmarks"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_ADD);
	action->setData(ADR_STREAM_JID,AIndex.data(DIDR_STREAM_JID));
	action->setData(ADR_DISCO_JID,AIndex.data(DIDR_JID));
	action->setData(ADR_DISCO_NODE,AIndex.data(DIDR_NODE));
	action->setData(ADR_DISCO_NAME,AIndex.data(DIDR_NAME));
	connect(action,SIGNAL(triggered(bool)),SLOT(onDiscoWindowAddBookmarkActionTriggered(bool)));
	AMenu->addAction(action, TBG_DIWT_DISCOVERY_ACTIONS, true);
}

void Bookmarks::onStartBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark bookmark;
		bookmark.type = action->data(ADR_BOOKMARK_TYPE).toInt();
		bookmark.conference.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();
		bookmark.url.url = action->data(ADR_BOOKMARK_URL).toString();

		Jid streamJid = action->data(ADR_STREAM_JID).toString();

		int index = FBookmarks.value(streamJid).indexOf(bookmark);
		if (index >= 0)
			startBookmark(streamJid,FBookmarks.value(streamJid).at(index),true);
		else
			REPORT_ERROR("Failed to start bookmark by action: Bookmark not found");
	}
}

void Bookmarks::onEditBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::Conference;
		bookmark.conference.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();

		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(bookmark);
		if (index >= 0)
		{
			IBookmark bookmark = FBookmarks.value(streamJid).at(index);
			if (execEditBookmarkDialog(&bookmark,NULL) == QDialog::Accepted)
			{
				LOG_STRM_INFO(streamJid,QString("Editing bookmark by action, name=%1").arg(bookmark.name));
				bookmarkList.replace(index,bookmark);
				setBookmarks(streamJid,bookmarkList);
			}
		}
		else
		{
			REPORT_ERROR("Failed to edit bookmark by action: Bookmark not found");
		}
	}
}

void Bookmarks::onRenameBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::Conference;
		bookmark.conference.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();

		QString streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(bookmark);
		if (index >= 0)
		{
			bool editInRoster = false;
			if (FRostersView && FMultiChatPlugin)
			{
				IRosterIndex *mucIndex = FMultiChatPlugin->findMultiChatRosterIndex(streamJid,bookmark.conference.roomJid);
				if (mucIndex)
					editInRoster = FRostersView->editRosterIndex(mucIndex,RDR_NAME);
			}
			if (!editInRoster)
			{
				renameBookmark(streamJid,bookmark);
			}
		}
		else
		{
			REPORT_ERROR("Failed to rename bookmark by action: Bookmark not found");
		}
	}
}

void Bookmarks::onChangeBookmarkAutoJoinActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::Conference;
		bookmark.name = action->data(ADR_BOOKMARK_NAME).toString();
		bookmark.conference.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();
		bookmark.conference.nick = action->data(ADR_BOOKMARK_ROOM_NICK).toString();
		bookmark.conference.password = action->data(ADR_BOOKMARK_ROOM_PASSWORD).toString();

		QString streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(bookmark);
		if (index >= 0)
		{
			LOG_STRM_INFO(streamJid,QString("Chaning bookmark auto join by action, name=%1").arg(bookmark.name));
			IBookmark bookmark = bookmarkList.at(index);
			bookmark.conference.autojoin = !bookmark.conference.autojoin;
			bookmarkList.replace(index,bookmark);
			setBookmarks(streamJid,bookmarkList);
		}
		else if (isValidBookmark(bookmark))
		{
			LOG_STRM_INFO(streamJid,QString("Adding bookmark with auto join by action, name=%1").arg(bookmark.name));
			bookmark.conference.autojoin = true;
			bookmarkList.append(bookmark);
			setBookmarks(streamJid,bookmarkList);
		}
		else
		{
			REPORT_ERROR("Failed to change bookmark auto join by action: Invalid params");
		}
	}
}

void Bookmarks::onAddBookmarksActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList names = action->data(ADR_BOOKMARK_NAME).toStringList();
		QStringList rooms = action->data(ADR_BOOKMARK_ROOM_JID).toStringList();
		QStringList nicks = action->data(ADR_BOOKMARK_ROOM_NICK).toStringList();
		QStringList passwords = action->data(ADR_BOOKMARK_ROOM_PASSWORD).toStringList();

		QMap<Jid, QList<IBookmark> > bookmarksMap;
		for (int i=0; i<streams.count(); i++)
		{
			if (isReady(streams.at(i)))
			{
				if (!bookmarksMap.contains(streams.at(i)))
					bookmarksMap[streams.at(i)] = FBookmarks.value(streams.at(i));

				IBookmark bookmark;
				bookmark.type = IBookmark::Conference;
				bookmark.name = names.at(i);
				bookmark.conference.roomJid = rooms.at(i);
				bookmark.conference.nick = nicks.at(i);
				bookmark.conference.password = passwords.at(i);

				QList<IBookmark> &bookmarkList = bookmarksMap[streams.at(i)];
				if (!bookmarkList.contains(bookmark))
					bookmarkList.append(bookmark);
			}
		}

		for (QMap<Jid, QList<IBookmark> >::const_iterator it=bookmarksMap.constBegin(); it!=bookmarksMap.constEnd(); ++it)
		{
			LOG_STRM_INFO(it.key(),"Adding bookmarks by action");
			setBookmarks(it.key(),it.value());
		}
	}
}

void Bookmarks::onRemoveBookmarksActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList rooms = action->data(ADR_BOOKMARK_ROOM_JID).toStringList();

		QMap<Jid, QList<IBookmark> > bookmarksMap;
		for (int i=0; i<streams.count(); i++)
		{
			if (isReady(streams.at(i)))
			{
				if (!bookmarksMap.contains(streams.at(i)))
					bookmarksMap[streams.at(i)] = FBookmarks.value(streams.at(i));

				IBookmark bookmark;
				bookmark.type = IBookmark::Conference;
				bookmark.conference.roomJid = rooms.at(i);

				QList<IBookmark> &bookmarkList = bookmarksMap[streams.at(i)];
				int index = bookmarkList.indexOf(bookmark);
				if (index >= 0)
					bookmarkList.removeAt(index);
			}
		}

		for (QMap<Jid, QList<IBookmark> >::const_iterator it=bookmarksMap.constBegin(); it!=bookmarksMap.constEnd(); ++it)
		{
			LOG_STRM_INFO(it.key(),"Removing bookmarks by action");
			setBookmarks(it.key(),it.value());
		}
	}
}

void Bookmarks::onEditBookmarksActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		showEditBookmarksDialog(streamJid);
	}
}

void Bookmarks::onMultiChatWindowAddBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(action->parent());
		if (window && isReady(window->streamJid()))
		{
			QList<IBookmark> bookmarkList = bookmarks(window->streamJid());

			int index = 0;
			while (index<bookmarkList.count() && window->contactJid()!=bookmarkList.at(index).conference.roomJid)
				index++;

			if (index == bookmarkList.count())
				bookmarkList.append(IBookmark());

			IBookmark &bookmark = bookmarkList[index];
			if (bookmark.type == IBookmark::None)
			{
				bookmark.type = IBookmark::Conference;
				bookmark.name = window->multiUserChat()->roomName();
				bookmark.conference.roomJid = window->contactJid();
				bookmark.conference.nick = window->multiUserChat()->nickName();
				bookmark.conference.password = window->multiUserChat()->password();
				bookmark.conference.autojoin = false;
			}

			if (execEditBookmarkDialog(&bookmark,window->instance()) == QDialog::Accepted)
			{
				LOG_STRM_INFO(window->streamJid(),QString("Adding/changing bookmark from groupchat window, name=%1").arg(bookmark.name));
				setBookmarks(window->streamJid(),bookmarkList);
			}
		}
	}
}

void Bookmarks::onDiscoWindowAddBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QString discoJid = action->data(ADR_DISCO_JID).toString();
		QString discoNode = action->data(ADR_DISCO_NODE).toString();
		QString discoName = action->data(ADR_DISCO_NAME).toString();
		if (streamJid.isValid() && !discoJid.isEmpty())
		{
			QUrl url;
			url.setScheme("xmpp");
			url.setQueryDelimiters('=',';');
			url.setPath(discoJid);

			QList< QPair<QString, QString> > queryItems;
			queryItems << qMakePair(QString("disco"),QString()) << qMakePair(QString("type"),QString("get")) << qMakePair(QString("request"),QString("items"));
			if (!discoNode.isEmpty())
				queryItems << qMakePair(QString("node"),discoNode);
			url.setQueryItems(queryItems);

			IBookmark bookmark;
			bookmark.type = IBookmark::Url;
			bookmark.name = "XMPP: ";
			bookmark.name += !discoName.isEmpty() ? discoName + " | " : QString::null;
			bookmark.name += discoJid;
			bookmark.name += !discoNode.isEmpty() ? " | " + discoNode : QString::null;
			bookmark.url.url = url.toString().replace("?disco=;","?disco;");

			if (execEditBookmarkDialog(&bookmark,NULL) == QDialog::Accepted)
			{
				LOG_STRM_INFO(streamJid,QString("Adding bookmark from disco window, name=%1").arg(bookmark.name));
				addBookmark(streamJid,bookmark);
			}
		}
	}
}

void Bookmarks::onEditBookmarksDialogDestroyed()
{
	EditBookmarksDialog *dialog = qobject_cast<EditBookmarksDialog *>(sender());
	if (dialog)
		FDialogs.remove(dialog->streamJid());
}

void Bookmarks::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersView && AWidget==FRostersView->instance())
	{
		QList<IRosterIndex *> indexes = FRostersView->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_RENAME && !FRostersView->hasMultiSelection())
		{
			IRosterIndex *index = indexes.first();
			if (isSelectionAccepted(indexes) && !FRostersView->editRosterIndex(index,RDR_NAME))
			{
				Jid streamJid = index->data(RDR_STREAM_JID).toString();
				IBookmark bookmark = FBookmarkIndexes.value(streamJid).value(index);
				renameBookmark(streamJid,bookmark);
			}
		}
	}
}

uint qHash(const IBookmark &AKey)
{
	switch (AKey.type)
	{
	case IBookmark::Url:
		return qHash(AKey.url.url);
	case IBookmark::Conference:
		return qHash(AKey.conference.roomJid);
	default:
		return qHash(QString::null);
	}
}

Q_EXPORT_PLUGIN2(plg_bookmarks, Bookmarks)
