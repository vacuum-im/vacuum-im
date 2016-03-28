#include "bookmarks.h"

#include <QUrlQuery>
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
#include <definitions/recentitemtypes.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/discoitemdataroles.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosteredithandlerorders.h>
#include <utils/advanceditemdelegate.h>
#include <utils/textmanager.h>
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
	FMultiChatManager = NULL;
	FXmppUriQueries = NULL;
	FDiscovery = NULL;
	FOptionsManager = NULL;
	FRostersModel = NULL;
	FRostersView = NULL;
	FPresenceManager = NULL;
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
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatManager").value(0,NULL);
	if (plugin)
	{
		FMultiChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());
		if (FMultiChatManager)
		{
			connect(FMultiChatManager->instance(),SIGNAL(multiChatWindowCreated(IMultiUserChatWindow *)),
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
		IRostersViewPlugin *rostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (rostersViewPlugin)
		{
			FRostersView = rostersViewPlugin->rostersView();
			connect(FRostersView->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersView->instance(), SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)),
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32 , Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresenceManager").value(0, NULL);
	if (plugin)
	{
		FPresenceManager = qobject_cast<IPresenceManager *>(plugin->instance());
		if (FPresenceManager)
		{
			connect(FPresenceManager->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
		}
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FPrivateStorage!=NULL;
}

bool Bookmarks::initObjects()
{
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
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
	Options::setDefaultValue(OPV_MUC_SHOWAUTOJOINED,false);
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> Bookmarks::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager)
	{
		QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
		if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="Additional")
		{
			OptionsNode options = Options::node(OPV_ACCOUNT_ITEM,nodeTree.at(1));
			widgets.insertMulti(OHO_ACCOUNTS_ADDITIONAL_CONFERENCES, FOptionsManager->newOptionsDialogHeader(tr("Conferences"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_ADDITIONAL_DISABLEAUTOJOIN, FOptionsManager->newOptionsDialogWidget(options.node("ignore-autojoin"),tr("Disable auto join to conferences on this computer"),AParent));
		}
		else if (ANodeId == OPN_CONFERENCES)
		{
			widgets.insertMulti(OWO_CONFERENCES_SHOWAUTOJOINED, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_MUC_SHOWAUTOJOINED),tr("Show windows of auto joined conferences at startup"),AParent));
		}
	}
	return widgets;
}

QList<int> Bookmarks::rosterDataRoles(int AOrder) const
{
	if (AOrder == RDHO_BOOKMARKS)
	{
		static const QList<int> roles = QList<int>() << RDR_NAME << RDR_MUC_NICK << RDR_MUC_PASSWORD;
		return roles;
	}
	return QList<int>();
}

QVariant Bookmarks::rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const
{
	if (AOrder==RDHO_BOOKMARKS && AIndex->kind()==RIK_MUC_ITEM)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		IRosterIndex *index = const_cast<IRosterIndex *>(AIndex);
		const IBookmark bookmark = FRoomIndexes.value(streamJid).value(index);
		switch (ARole)
		{
		case RDR_NAME:
			return !bookmark.name.isEmpty() ? QVariant(bookmark.name) : QVariant();
		case RDR_MUC_NICK:
			return !bookmark.room.nick.isEmpty() ? QVariant(bookmark.room.nick) : QVariant();
		case RDR_MUC_PASSWORD:
			return !bookmark.room.password.isEmpty() ? QVariant(bookmark.room.password) : QVariant();
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
		bookmark.type = IBookmark::TypeRoom;
		bookmark.room.roomJid = AIndex.data(RDR_PREP_BARE_JID).toString();

		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		if (FBookmarks.value(streamJid).contains(bookmark))
			return AdvancedDelegateItem::DisplayId;
	}
	return AdvancedDelegateItem::NullId;
}

AdvancedDelegateEditProxy *Bookmarks::rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex)
{
	Q_UNUSED(AIndex);
	return AOrder==REHO_BOOKMARKS_RENAME && ADataRole==RDR_NAME ? this : NULL;
}

bool Bookmarks::setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex)
{
	Q_UNUSED(AModel);
	if (ADelegate->editRole()==RDR_NAME && AIndex.data(RDR_KIND)==RIK_MUC_ITEM)
	{
		IBookmark bookmark;
		bookmark.type = IBookmark::TypeRoom;
		bookmark.room.roomJid = AIndex.data(RDR_PREP_BARE_JID).toString();

		Jid streamJid = AIndex.data(RDR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);
		
		int index = bookmarkList.indexOf(bookmark);
		if (index >= 0)
		{
			IBookmark &bookmark = bookmarkList[index];

			QVariant value = AEditor->property(ADVANCED_DELEGATE_EDITOR_VALUE_PROPERTY);
			QByteArray name = ADelegate->editorFactory()->valuePropertyName(value.type());
			QString newName = AEditor->property(name).toString();

			if (!newName.isEmpty() && bookmark.name!=newName)
			{
				LOG_STRM_INFO(streamJid,QString("Renaming bookmark %1 to %2 from roster").arg(bookmark.name,newName));
				bookmark.name = newName;
				setBookmarks(streamJid,bookmarkList);
			}
		}
		else
		{
			REPORT_ERROR("Failed to rename bookmark from roster: Invalid parameters");
		}
		return true;
	}
	return false;
}

bool Bookmarks::isReady(const Jid &AStreamJid) const
{
	return FBookmarks.contains(AStreamJid);
}

QList<IBookmark> Bookmarks::bookmarks(const Jid &AStreamJid) const
{
	return FBookmarks.value(AStreamJid);
}

bool Bookmarks::addBookmark(const Jid &AStreamJid, const IBookmark &ABookmark)
{
	if (isReady(AStreamJid) && ABookmark.isValid())
	{
		LOG_STRM_INFO(AStreamJid,QString("Adding new bookmark, name=%1").arg(ABookmark.name));
		QList<IBookmark> bookmarkList = bookmarks(AStreamJid);
		bookmarkList.append(ABookmark);
		return setBookmarks(AStreamJid,bookmarkList);
	}
	else if (!isReady(AStreamJid))
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to add bookmark: Stream is not ready");
	}
	else if (!ABookmark.isValid())
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
		LOG_STRM_WARNING(AStreamJid,"Failed to save bookmarks: Stream is not ready");
	}
	return false;
}

QDialog *Bookmarks::showEditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent)
{
	EditBookmarkDialog *dialog = new EditBookmarkDialog(ABookmark,AParent);
	dialog->show();
	return dialog;
}

QDialog *Bookmarks::showEditBookmarksDialog(const Jid &AStreamJid, QWidget *AParent)
{
	if (isReady(AStreamJid))
	{
		EditBookmarksDialog *dialog = FDialogs.value(AStreamJid);
		if (dialog == NULL)
		{
			dialog = new EditBookmarksDialog(this,AStreamJid,bookmarks(AStreamJid),AParent);
			connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onEditBookmarksDialogDestroyed()));
			FDialogs.insert(AStreamJid,dialog);
		}
		dialog->show();
		return dialog;
	}
	return NULL;
}

void Bookmarks::updateRoomIndexes(const Jid &AStreamJid)
{
	if (FRostersModel && FMultiChatManager)
	{
		QMap<IRosterIndex *, IBookmark> &roomIndexes = FRoomIndexes[AStreamJid];

		QList<IBookmark> curList = FBookmarks.value(AStreamJid);
		QList<IBookmark> oldList = roomIndexes.values();

		foreach(const IBookmark &bookmark, curList)
		{
			if (bookmark.type == IBookmark::TypeRoom)
			{
				IRosterIndex *index = FMultiChatManager->getMultiChatRosterIndex(AStreamJid,bookmark.room.roomJid,bookmark.room.nick,bookmark.room.password);

				IBookmark prevBookmark = roomIndexes.take(index);
				roomIndexes.insert(index,bookmark);

				if (bookmark.name != prevBookmark.name)
					emit rosterDataChanged(index,RDR_NAME);
				if (bookmark.room.nick != prevBookmark.room.nick)
					emit rosterDataChanged(index,RDR_MUC_NICK);
				if (bookmark.room.password != prevBookmark.room.password)
					emit rosterDataChanged(index,RDR_MUC_PASSWORD);

				oldList.removeOne(prevBookmark);
			}
		}

		foreach(const IBookmark &bookmark, oldList)
		{
			IRosterIndex *index = roomIndexes.key(bookmark);
			roomIndexes.remove(index);

			if (FMultiChatManager->findMultiChatWindow(AStreamJid,bookmark.room.roomJid) != NULL)
			{
				emit rosterDataChanged(index,RDR_NAME);
				emit rosterDataChanged(index,RDR_MUC_NICK);
				emit rosterDataChanged(index,RDR_MUC_PASSWORD);
			}
			else
			{
				FRostersModel->removeRosterIndex(index);
			}
		}
	}
}

void Bookmarks::updateMultiChatWindows(const Jid &AStreamJid)
{
	if (FMultiChatManager)
	{
		foreach(IMultiUserChatWindow *window, FMultiChatManager->multiChatWindows())
			if (window->streamJid() == AStreamJid)
				updateMultiChatWindow(window);
	}
}

void Bookmarks::updateMultiChatWindow(IMultiUserChatWindow *AWindow)
{
	ToolBarChanger *tbc = AWindow->infoWidget()->infoToolBarChanger();
	Action *bookmarkAction = tbc->handleAction(tbc->groupItems(TBG_MWIWTB_BOOKMARKS).value(0));
	if (bookmarkAction)
	{
		if (isReady(AWindow->streamJid()))
		{
			IBookmark ref;
			ref.type = IBookmark::TypeRoom;
			ref.room.roomJid = AWindow->contactJid();

			if (!FBookmarks.value(AWindow->streamJid()).contains(ref))
			{
				if (bookmarkAction->menu() != NULL)
				{
					bookmarkAction->menu()->deleteLater();
					bookmarkAction->setMenu(NULL);
				}

				bookmarkAction->setText(tr("Add to Bookmarks"));
				bookmarkAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EMPTY);
			}
			else
			{
				if (bookmarkAction->menu() == NULL)
				{
					Menu *menu = new Menu(tbc->toolBar());

					Action *editAction = new Action(menu);
					editAction->setText(tr("Edit Bookmark"));
					connect(editAction,SIGNAL(triggered(bool)),SLOT(onMultiChatWindowEditBookmarkActionTriggered(bool)));
					menu->addAction(editAction);

					Action *removeAction = new Action(menu);
					removeAction->setText(tr("Remove from Bookmarks"));
					connect(removeAction,SIGNAL(triggered(bool)),SLOT(onMultiChatWindowRemoveBookmarkActionTriggered(bool)));
					menu->addAction(removeAction);

					bookmarkAction->setMenu(menu);
				}

				bookmarkAction->setText(tr("Edit Bookmark"));
				bookmarkAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS);
			}

			bookmarkAction->setEnabled(true);
		}
		else
		{
			bookmarkAction->setEnabled(false);
		}

		if (bookmarkAction->menu() != NULL)
		{
			foreach(Action *action, bookmarkAction->menu()->actions())
			{
				action->setData(ADR_STREAM_JID, AWindow->streamJid().full());
				action->setData(ADR_BOOKMARK_ROOM_JID, AWindow->contactJid().bare());
			}
		}
		bookmarkAction->setData(ADR_STREAM_JID, AWindow->streamJid().full());
		bookmarkAction->setData(ADR_BOOKMARK_ROOM_JID, AWindow->contactJid().bare());
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

void Bookmarks::renameBookmark(const Jid &AStreamJid, const IBookmark &ABookmark)
{
	QList<IBookmark> bookmarkList = FBookmarks.value(AStreamJid);

	int index = bookmarkList.indexOf(ABookmark);
	if (index >= 0)
	{
		IBookmark &bookmark = bookmarkList[index];
		QString newName = QInputDialog::getText(NULL,tr("Rename Bookmark"),tr("Enter bookmark name:"),QLineEdit::Normal,bookmark.name);
		if (!newName.isEmpty() && newName!=bookmark.name)
		{
			LOG_STRM_INFO(AStreamJid,QString("Renaming bookmark %1 to %2").arg(bookmark.name,newName));
			bookmark.name = newName;
			setBookmarks(AStreamJid,bookmarkList);
		}
	}
	else
	{
		REPORT_ERROR("Failed to rename bookmark: Bookmark not found");
	}
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
			bookmark.type = IBookmark::TypeRoom;
			bookmark.name = elem.attribute("name");
			bookmark.room.roomJid = elem.attribute("jid");
			bookmark.room.nick = elem.firstChildElement("nick").text();
			bookmark.room.password = elem.firstChildElement("password").text();
			bookmark.room.autojoin = QVariant(elem.attribute("autojoin")).toBool();
			bookmark.name = bookmark.name.isEmpty() ? bookmark.room.roomJid.uBare() : bookmark.name;
			
			if (!bookmark.isValid())
				LOG_WARNING(QString("Skipped invalid conference bookmark, name=%1").arg(bookmark.name));
			else if (bookmarkList.contains(bookmark))
				LOG_WARNING(QString("Skipped duplicate conference bookmark, room=%1").arg(bookmark.room.roomJid.bare()));
			else
				bookmarkList.append(bookmark);
		}
		else if (elem.tagName() == "url")
		{
			IBookmark bookmark;
			bookmark.type = IBookmark::TypeUrl;
			bookmark.name = elem.attribute("name");
			bookmark.url.url = elem.attribute("url");
			bookmark.name = bookmark.name.isEmpty() ? bookmark.url.url.host() : bookmark.name;

			if (!bookmark.isValid())
				LOG_WARNING(QString("Skipped invalid url bookmark, name=%1").arg(bookmark.name));
			else if (bookmarkList.contains(bookmark))
				LOG_WARNING(QString("Skipped duplicate url bookmark, url=%1").arg(bookmark.url.url.toString()));
			else
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
		if (bookmark.type == IBookmark::TypeRoom)
		{
			QDomElement markElem = AElement.appendChild(AElement.ownerDocument().createElement("conference")).toElement();
			markElem.setAttribute("name",bookmark.name);
			markElem.setAttribute("jid",bookmark.room.roomJid.bare());
			if (!bookmark.room.nick.isEmpty())
				markElem.appendChild(AElement.ownerDocument().createElement("nick")).appendChild(AElement.ownerDocument().createTextNode(bookmark.room.nick));
			if (!bookmark.room.password.isEmpty())
				markElem.appendChild(AElement.ownerDocument().createElement("password")).appendChild(AElement.ownerDocument().createTextNode(bookmark.room.password));
			markElem.setAttribute("autojoin",QVariant(bookmark.room.autojoin).toString());
		}
		else if (bookmark.type == IBookmark::TypeUrl)
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

void Bookmarks::autoStartBookmarks(const Jid &AStreamJid) const
{
	IPresence *presence = FPresenceManager!=NULL ? FPresenceManager->findPresence(AStreamJid) : NULL;
	if (presence!=NULL && presence->isOpen() && isReady(AStreamJid))
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountByStream(AStreamJid) : NULL;
		if (account!=NULL && !account->optionsNode().value("ignore-autojoin").toBool())
		{
			LOG_STRM_INFO(AStreamJid,"Auto joining bookmark conferences");
			bool showAutoJoined = Options::node(OPV_MUC_SHOWAUTOJOINED).value().toBool();
			foreach(const IBookmark &bookmark, FBookmarks.value(AStreamJid))
			{
				if (bookmark.type==IBookmark::TypeRoom && bookmark.room.autojoin)
				{
					if (showAutoJoined && FMultiChatManager!=NULL && FMultiChatManager->findMultiChatWindow(AStreamJid,bookmark.room.roomJid)==NULL)
						startBookmark(AStreamJid,bookmark,true);
					else
						startBookmark(AStreamJid,bookmark,false);
				}
			}
		}
	}
}

void Bookmarks::startBookmark(const Jid &AStreamJid, const IBookmark &ABookmark, bool AShowWindow) const
{
	if (ABookmark.isValid())
	{
		LOG_STRM_INFO(AStreamJid,QString("Starting bookmark, name=%1").arg(ABookmark.name));
		if (FMultiChatManager && ABookmark.type==IBookmark::TypeRoom)
		{
			IMultiUserChatWindow *window = FMultiChatManager->getMultiChatWindow(AStreamJid,ABookmark.room.roomJid,ABookmark.room.nick,ABookmark.room.password);
			if (window)
			{
				if (window->multiUserChat()->state() == IMultiUserChat::Closed)
					window->multiUserChat()->sendStreamPresence();
				if (AShowWindow)
					window->showTabPage();
			}
		}
		else if (ABookmark.type == IBookmark::TypeUrl)
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
		bool wasReady = isReady(AStreamJid);

		LOG_STRM_INFO(AStreamJid,"Bookmarks loaded or updated");
		FBookmarks[AStreamJid] = loadBookmarksFromXML(AElement);
		updateRoomIndexes(AStreamJid);
		updateMultiChatWindows(AStreamJid);

		if (!wasReady)
		{
			autoStartBookmarks(AStreamJid);
			emit bookmarksOpened(AStreamJid);
		}
		else
		{
			emit bookmarksChanged(AStreamJid);
		}
	}
}

void Bookmarks::onPrivateDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	Q_UNUSED(AId);
	if (AElement.tagName()==PST_BOOKMARKS && AElement.namespaceURI()==NS_STORAGE_BOOKMARKS)
	{
		FBookmarks[AStreamJid].clear();
		updateRoomIndexes(AStreamJid);
		updateMultiChatWindows(AStreamJid);
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

	updateRoomIndexes(AStreamJid);
	updateMultiChatWindows(AStreamJid);
	FRoomIndexes.remove(AStreamJid);
	
	emit bookmarksClosed(AStreamJid);
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
		IRosterIndex *index = AIndexes.first();
		if (index->kind() == RIK_STREAM_ROOT)
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
					foreach(const IBookmark &bookmark, bookmarkList)
					{
						if (bookmark.isValid() && !bookmark.name.isEmpty())
						{
							Action *startAction = new Action(bookmarksMenu);
							startAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS);
							startAction->setText(TextManager::getElidedString(bookmark.name,Qt::ElideRight,50));
							startAction->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
							startAction->setData(ADR_BOOKMARK_TYPE,bookmark.type);
							startAction->setData(ADR_BOOKMARK_ROOM_JID,bookmark.room.roomJid.bare());
							startAction->setData(ADR_BOOKMARK_URL,bookmark.url.url.toString());
							connect(startAction,SIGNAL(triggered(bool)),SLOT(onStartBookmarkActionTriggered(bool)));
							bookmarksMenu->addAction(startAction,streamGroup);
						}
					}
					streamGroup++;
				}
			}

			if (!bookmarksMenu->isEmpty())
				AMenu->addAction(bookmarksMenu->menuAction(),AG_RVCM_BOOKMARS_LIST);
			else
				delete bookmarksMenu;

			if (!isMultiSelection)
			{
				Action *editAction = new Action(AMenu);
				editAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
				editAction->setText(tr("Edit Bookmarks"));
				editAction->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID).toString());
				connect(editAction,SIGNAL(triggered(bool)),SLOT(onEditBookmarksActionTriggered(bool)));
				AMenu->addAction(editAction,AG_RVCM_BOOKMARS_EDIT);
			}
		}
		else if (index->kind() == RIK_MUC_ITEM)
		{
			IBookmark bookmark = !isMultiSelection ? FRoomIndexes.value(index->data(RDR_STREAM_JID).toString()).value(index) : IBookmark();
			QMap<int, QStringList> rolesMap = FRostersView->indexesRolesMap(AIndexes,QList<int>()<<RDR_STREAM_JID<<RDR_NAME<<RDR_PREP_BARE_JID<<RDR_MUC_NICK<<RDR_MUC_PASSWORD,RDR_PREP_BARE_JID,RDR_STREAM_JID);

			if (isMultiSelection || !bookmark.isValid())
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

			if (isMultiSelection || bookmark.isValid())
			{
				Action *removeAction = new Action(AMenu);
				removeAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_REMOVE);
				removeAction->setText(tr("Remove from Bookmarks"));
				removeAction->setData(ADR_STREAM_JID,rolesMap.value(RDR_STREAM_JID));
				removeAction->setData(ADR_BOOKMARK_ROOM_JID,rolesMap.value(RDR_PREP_BARE_JID));
				connect(removeAction,SIGNAL(triggered(bool)),SLOT(onRemoveBookmarksActionTriggered(bool)));
				AMenu->addAction(removeAction,AG_RVCM_BOOKMARS_TOOLS);
			}

			if (!isMultiSelection && bookmark.isValid())
			{
				Action *editAction = new Action(AMenu);
				editAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
				editAction->setText(tr("Edit Bookmark"));
				editAction->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
				editAction->setData(ADR_BOOKMARK_ROOM_JID,bookmark.room.roomJid.bare());
				connect(editAction,SIGNAL(triggered(bool)),SLOT(onEditBookmarkActionTriggered(bool)));
				AMenu->addAction(editAction,AG_RVCM_BOOKMARS_TOOLS);
			}

			if (!isMultiSelection)
			{
				Action *autoJoinAction = new Action(AMenu);
				autoJoinAction->setCheckable(true);
				autoJoinAction->setChecked(bookmark.room.autojoin);
				autoJoinAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_AUTO_JOIN);
				autoJoinAction->setText(tr("Join to Conference at Startup"));
				autoJoinAction->setData(ADR_STREAM_JID,index->data(RDR_STREAM_JID));
				autoJoinAction->setData(ADR_BOOKMARK_NAME,index->data(RDR_NAME));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_JID,index->data(RDR_PREP_BARE_JID));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_NICK,index->data(RDR_MUC_NICK));
				autoJoinAction->setData(ADR_BOOKMARK_ROOM_PASSWORD,index->data(RDR_MUC_PASSWORD));
				connect(autoJoinAction,SIGNAL(triggered(bool)),SLOT(onChangeBookmarkAutoJoinActionTriggered(bool)));
				AMenu->addAction(autoJoinAction,AG_RVCM_BOOKMARS_TOOLS);
			}
		}
	}
}

void Bookmarks::onMultiChatPropertiesChanged()
{
	IMultiUserChat *chat = qobject_cast<IMultiUserChat *>(sender());
	if (chat!=NULL && isReady(chat->streamJid()))
	{
		QList<IBookmark> bookmarkList = FBookmarks.value(chat->streamJid());
		for (QList<IBookmark>::iterator it=bookmarkList.begin(); it!=bookmarkList.end(); ++it)
		{
			if (it->type==IBookmark::TypeRoom && chat->roomJid()==it->room.roomJid)
			{
				if (it->room.nick!=chat->nickname() || it->room.password!=chat->password())
				{
					LOG_STRM_INFO(chat->streamJid(),QString("Automatically updating conference bookmark nick and password, name=%1").arg(it->name));
					it->room.nick = chat->nickname();
					it->room.password = chat->password();
					setBookmarks(chat->streamJid(),bookmarkList);
				}
				break;
			}
		}
	}
}

void Bookmarks::onMultiChatWindowToolsMenuAboutToShow()
{
	IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(sender());
	if (window && isReady(window->streamJid()))
	{
		Menu *toolsMenu = window->roomToolsMenu();

		IBookmark ref;
		ref.type = IBookmark::TypeRoom;
		ref.room.roomJid = window->multiUserChat()->roomJid();

		QList<IBookmark> bookmarkList = FBookmarks.value(window->streamJid());
		IBookmark bookmark = bookmarkList.value(bookmarkList.indexOf(ref));

		Action *autoJoinAction = new Action(toolsMenu);
		autoJoinAction->setCheckable(true);
		autoJoinAction->setChecked(bookmark.room.autojoin);
		autoJoinAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_AUTO_JOIN);
		autoJoinAction->setText(tr("Join to Conference at Startup"));
		autoJoinAction->setData(ADR_STREAM_JID,window->streamJid().full());
		autoJoinAction->setData(ADR_BOOKMARK_NAME,window->multiUserChat()->roomName());
		autoJoinAction->setData(ADR_BOOKMARK_ROOM_JID,window->multiUserChat()->roomJid().pBare());
		autoJoinAction->setData(ADR_BOOKMARK_ROOM_NICK,window->multiUserChat()->nickname());
		autoJoinAction->setData(ADR_BOOKMARK_ROOM_PASSWORD,window->multiUserChat()->password());
		connect(autoJoinAction,SIGNAL(triggered(bool)),SLOT(onChangeBookmarkAutoJoinActionTriggered(bool)));
		connect(toolsMenu,SIGNAL(aboutToHide()),autoJoinAction,SLOT(deleteLater()));
		toolsMenu->addAction(autoJoinAction,AG_MUTM_BOOKMARKS_AUTOJOIN,true);
	}
}

void Bookmarks::onMultiChatWindowAddBookmarkActionTriggered(bool)
{
	Action *addAction = qobject_cast<Action *>(sender());
	if (addAction)
	{
		Jid streamJid = addAction->data(ADR_STREAM_JID).toString();
		Jid roomJid = addAction->data(ADR_BOOKMARK_ROOM_JID).toString();
		IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(streamJid,roomJid) : NULL;
		if (window!=NULL && isReady(window->streamJid()))
		{
			QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

			IBookmark ref;
			ref.type = IBookmark::TypeRoom;
			ref.room.roomJid = roomJid;

			int index = bookmarkList.indexOf(ref);
			if (index < 0)
			{
				LOG_STRM_INFO(streamJid,QString("Adding bookmark from conference window, room=%1").arg(roomJid.bare()));

				IBookmark bookmark = ref;
				bookmark.name = window->multiUserChat()->roomTitle();
				bookmark.room.nick = window->multiUserChat()->nickname();
				bookmark.room.password = window->multiUserChat()->password();
				bookmark.room.autojoin = true;

				if (showEditBookmarkDialog(&bookmark,window->instance())->exec() == QDialog::Accepted)
				{
					bookmarkList.append(bookmark);
					setBookmarks(window->streamJid(),bookmarkList);
				}
			}
		}
	}
}

void Bookmarks::onMultiChatWindowEditBookmarkActionTriggered(bool)
{
	Action *editAction = qobject_cast<Action *>(sender());
	if (editAction)
	{
		Jid streamJid = editAction->data(ADR_STREAM_JID).toString();
		Jid roomJid = editAction->data(ADR_BOOKMARK_ROOM_JID).toString();
		IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(streamJid,roomJid) : NULL;
		if (window!=NULL && isReady(window->streamJid()))
		{
			QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

			IBookmark ref;
			ref.type = IBookmark::TypeRoom;
			ref.room.roomJid = roomJid;

			int index = bookmarkList.indexOf(ref);
			if (index >= 0)
			{
				LOG_STRM_INFO(streamJid,QString("Editing bookmark from conference window, room=%1").arg(roomJid.bare()));

				IBookmark &bookmark = bookmarkList[index];
				if (showEditBookmarkDialog(&bookmark,window->instance())->exec() == QDialog::Accepted)
					setBookmarks(window->streamJid(),bookmarkList);
			}
		}
	}
}

void Bookmarks::onMultiChatWindowRemoveBookmarkActionTriggered(bool)
{
	Action *removeAction = qobject_cast<Action *>(sender());
	if (removeAction)
	{
		Jid streamJid = removeAction->data(ADR_STREAM_JID).toString();
		Jid roomJid = removeAction->data(ADR_BOOKMARK_ROOM_JID).toString();
		IMultiUserChatWindow *window = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(streamJid,roomJid) : NULL;
		if (window!=NULL && isReady(window->streamJid()))
		{
			QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

			IBookmark ref;
			ref.type = IBookmark::TypeRoom;
			ref.room.roomJid = roomJid;

			int index = bookmarkList.indexOf(ref);
			if (index >= 0)
			{
				LOG_STRM_INFO(streamJid,QString("Removing bookmark from conference window, room=%1").arg(roomJid.bare()));

				bookmarkList.removeAt(index);
				setBookmarks(window->streamJid(),bookmarkList);
			}
		}
	}
}

void Bookmarks::onMultiChatWindowCreated(IMultiUserChatWindow *AWindow)
{
	Action *bookmarkAction = new Action(AWindow->instance());
	bookmarkAction->setText(tr("Add to Bookmarks"));
	bookmarkAction->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EMPTY);
	connect(bookmarkAction,SIGNAL(triggered(bool)),SLOT(onMultiChatWindowAddBookmarkActionTriggered(bool)));
	AWindow->infoWidget()->infoToolBarChanger()->insertAction(bookmarkAction,TBG_MWIWTB_BOOKMARKS)->setPopupMode(QToolButton::InstantPopup);

	connect(AWindow->instance(),SIGNAL(roomToolsMenuAboutToShow()),SLOT(onMultiChatWindowToolsMenuAboutToShow()));

	connect(AWindow->multiUserChat()->instance(),SIGNAL(passwordChanged(const QString &)),SLOT(onMultiChatPropertiesChanged()));
	connect(AWindow->multiUserChat()->instance(),SIGNAL(nicknameChanged(const QString &, const XmppError &)),SLOT(onMultiChatPropertiesChanged()));

	updateMultiChatWindow(AWindow);
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
		if (isReady(streamJid) && !discoJid.isEmpty())
		{
			QUrl url;
			QUrlQuery query;
			url.setScheme("xmpp");
			query.setQueryDelimiters('=',';');
			url.setPath(discoJid);

			QList< QPair<QString, QString> > queryItems;
			queryItems << qMakePair(QString("disco"),QString()) << qMakePair(QString("type"),QString("get")) << qMakePair(QString("request"),QString("items"));
			if (!discoNode.isEmpty())
				queryItems << qMakePair(QString("node"),discoNode);
			query.setQueryItems(queryItems);

			QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

			IBookmark ref;
			ref.type = IBookmark::TypeUrl;
			ref.url.url = url.toString().replace("?disco=;","?disco;");

			int index = bookmarkList.indexOf(ref);
			if (index < 0)
			{
				IBookmark bookmark = ref;
				bookmark.name = "XMPP: ";
				bookmark.name += !discoName.isEmpty() ? discoName + " | " : QString::null;
				bookmark.name += discoJid;
				bookmark.name += !discoNode.isEmpty() ? " | " + discoNode : QString::null;

				index = bookmarkList.count();
				bookmarkList.append(bookmark);
			}

			IBookmark &bookmark = bookmarkList[index];
			if (showEditBookmarkDialog(&bookmark,NULL)->exec() == QDialog::Accepted)
			{
				LOG_STRM_INFO(streamJid,QString("Adding bookmark from disco window, name=%1").arg(ref.name));
				setBookmarks(streamJid,bookmarkList);
			}
			url.setQuery(query);
		}
	}
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

void Bookmarks::onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow)
{
	connect(AWindow->instance(),SIGNAL(indexContextMenu(const QModelIndex &, Menu *)),SLOT(onDiscoIndexContextMenu(const QModelIndex &, Menu *)));
}

void Bookmarks::onPresenceOpened(IPresence *APresence)
{
	autoStartBookmarks(APresence->streamJid());
}

void Bookmarks::onRosterIndexDestroyed(IRosterIndex *AIndex)
{
	if (AIndex->kind() == RIK_MUC_ITEM)
	{
		Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
		if (isReady(streamJid))
			FRoomIndexes[streamJid].remove(AIndex);
	}
}

void Bookmarks::onStartBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark ref;
		ref.type = action->data(ADR_BOOKMARK_TYPE).toInt();
		ref.room.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();
		ref.url.url = action->data(ADR_BOOKMARK_URL).toString();

		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(ref);
		if (index >= 0)
			startBookmark(streamJid,bookmarkList.at(index),true);
		else
			REPORT_ERROR("Failed to start bookmark by action: Bookmark not found");
	}
}

void Bookmarks::onEditBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark ref;
		ref.type = IBookmark::TypeRoom;
		ref.room.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();

		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(ref);
		if (index >= 0)
		{
			IBookmark &bookmark = bookmarkList[index];
			if (showEditBookmarkDialog(&bookmark,NULL)->exec() == QDialog::Accepted)
			{
				LOG_STRM_INFO(streamJid,QString("Editing bookmark by action, name=%1").arg(bookmark.name));
				setBookmarks(streamJid,bookmarkList);
			}
		}
		else
		{
			REPORT_ERROR("Failed to edit bookmark by action: Bookmark not found");
		}
	}
}

void Bookmarks::onChangeBookmarkAutoJoinActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IBookmark newBookmark;
		newBookmark.type = IBookmark::TypeRoom;
		newBookmark.name = action->data(ADR_BOOKMARK_NAME).toString();
		newBookmark.room.roomJid = action->data(ADR_BOOKMARK_ROOM_JID).toString();
		newBookmark.room.nick = action->data(ADR_BOOKMARK_ROOM_NICK).toString();
		newBookmark.room.password = action->data(ADR_BOOKMARK_ROOM_PASSWORD).toString();
		newBookmark.room.autojoin = true;

		QString streamJid = action->data(ADR_STREAM_JID).toString();
		QList<IBookmark> bookmarkList = FBookmarks.value(streamJid);

		int index = bookmarkList.indexOf(newBookmark);
		if (index >= 0)
		{
			LOG_STRM_INFO(streamJid,QString("Changing bookmark auto join by action, name=%1").arg(newBookmark.name));
			IBookmark &bookmark = bookmarkList[index];
			bookmark.room.autojoin = !bookmark.room.autojoin;
			setBookmarks(streamJid,bookmarkList);
		}
		else if (newBookmark.isValid())
		{
			LOG_STRM_INFO(streamJid,QString("Adding bookmark with auto join by action, name=%1").arg(newBookmark.name));
			bookmarkList.append(newBookmark);
			setBookmarks(streamJid,bookmarkList);
		}
		else
		{
			REPORT_ERROR("Failed to change bookmark auto join by action: Invalid bookmark");
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

		QMap<Jid, QList<IBookmark> > updateMap;
		for (int i=0; i<streams.count(); i++)
		{
			Jid streamJid = streams.at(i);
			if (isReady(streamJid))
			{
				IBookmark bookmark;
				bookmark.type = IBookmark::TypeRoom;
				bookmark.name = names.at(i);
				bookmark.room.roomJid = rooms.at(i);
				bookmark.room.nick = nicks.at(i);
				bookmark.room.password = passwords.at(i);

				if (!updateMap.contains(streamJid))
					updateMap[streamJid] = FBookmarks.value(streamJid);
				QList<IBookmark> &updateList = updateMap[streamJid];

				if (!updateList.contains(bookmark))
					updateList.append(bookmark);
			}
		}

		for (QMap<Jid, QList<IBookmark> >::const_iterator it=updateMap.constBegin(); it!=updateMap.constEnd(); ++it)
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

		QMap<Jid, QList<IBookmark> > updateMap;
		for (int i=0; i<streams.count(); i++)
		{
			Jid streamJid = streams.at(i);
			if (isReady(streamJid))
			{
				IBookmark ref;
				ref.type = IBookmark::TypeRoom;
				ref.room.roomJid = rooms.at(i);

				if (!updateMap.contains(streamJid))
					updateMap[streamJid] = FBookmarks.value(streamJid);
				updateMap[streamJid].removeOne(ref);
			}
		}

		for (QMap<Jid, QList<IBookmark> >::const_iterator it=updateMap.constBegin(); it!=updateMap.constEnd(); ++it)
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
		showEditBookmarksDialog(action->data(ADR_STREAM_JID).toString());
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
		if (AId==SCT_ROSTERVIEW_RENAME && indexes.count()==1)
		{
			IRosterIndex *index = indexes.first();
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (FRoomIndexes.value(streamJid).contains(index))
			{
				if (!FRostersView->editRosterIndex(index,RDR_NAME))
					renameBookmark(streamJid,FRoomIndexes.value(streamJid).value(index));
			}
		}
	}
}
