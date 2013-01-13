#include "bookmarks.h"

#include <QDesktopServices>

#define PST_BOOKMARKS           "storage"

#define ADR_STREAM_JID          Action::DR_StreamJid
#define ADR_BOOKMARK_INDEX      Action::DR_Parametr1
#define ADR_ROOM_JID            Action::DR_Parametr2
#define ADR_DISCO_JID           Action::DR_Parametr2
#define ADR_DISCO_NODE          Action::DR_Parametr3
#define ADR_DISCO_NAME          Action::DR_Parametr4
#define ADR_GROUP_SHIFT         Action::DR_Parametr1

#define FIRST_START_TIMEOUT     1000
#define NEXT_START_TIMEOUT      500

BookMarks::BookMarks()
{
	FPrivateStorage = NULL;
	FTrayManager = NULL;
	FMainWindowPlugin = NULL;
	FAccountManager = NULL;
	FMultiChatPlugin = NULL;
	FXmppUriQueries = NULL;
	FDiscovery = NULL;
	FOptionsManager = NULL;

	FBookMarksMenu = NULL;

	FStartTimer.setSingleShot(true);
	connect(&FStartTimer,SIGNAL(timeout()),SLOT(onStartTimerTimeout()));
}

BookMarks::~BookMarks()
{
	delete FBookMarksMenu;
}

void BookMarks::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Bookmarks");
	APluginInfo->description = tr("Allows to create bookmarks at the jabber conference and web pages");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool BookMarks::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(storageOpened(const Jid &)),SLOT(onPrivateStorageOpened(const Jid &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataError(const QString &, const QString &)),
				SLOT(onPrivateDataError(const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataLoadedSaved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataLoadedSaved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataRemoved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataRemoved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateDataChanged(const Jid &, const QString &, const QString &)));
			connect(FPrivateStorage->instance(),SIGNAL(storageClosed(const Jid &)),SLOT(onPrivateStorageClosed(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

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

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

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

	return FPrivateStorage!=NULL;
}

bool BookMarks::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_MUC_BOOKMARK, tr("Edit bookmark"), QKeySequence::UnknownKey);

	FBookMarksMenu = new Menu;
	FBookMarksMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS);
	FBookMarksMenu->setTitle(tr("Bookmarks"));
	FBookMarksMenu->menuAction()->setEnabled(false);
	FBookMarksMenu->menuAction()->setData(ADR_GROUP_SHIFT,1);

	if (FTrayManager)
	{
		FTrayManager->contextMenu()->addAction(FBookMarksMenu->menuAction(),AG_TMTM_BOOKMARKS,true);
	}
	if (FMainWindowPlugin)
	{
		ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->topToolBarChanger();
		QToolButton *button = changer->insertAction(FBookMarksMenu->menuAction(),TBG_MWTTB_BOOKMARKS);
		button->setPopupMode(QToolButton::InstantPopup);
	}
	if (FOptionsManager)
		FOptionsManager->insertOptionsHolder(this);
	return true;
}

bool BookMarks::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_IGNOREAUTOJOIN, false);
	Options::setDefaultValue(OPV_MUC_GROUPCHAT_SHOWAUTOJOINED,false);
	return true;
}

QMultiMap<int, IOptionsWidget *> BookMarks::optionsWidgets(const QString &ANodeId, QWidget *AParent)
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

QString BookMarks::addBookmark(const Jid &AStreamJid, const IBookMark &ABookmark)
{
	if (!ABookmark.name.isEmpty())
	{
		QList<IBookMark> bookmarkList = bookmarks(AStreamJid);
		bookmarkList.append(ABookmark);
		return setBookmarks(AStreamJid,bookmarkList);
	}
	return QString::null;
}

QString BookMarks::setBookmarks(const Jid &AStreamJid, const QList<IBookMark> &ABookmarks)
{
	QDomDocument doc;
	doc.appendChild(doc.createElement("bookmarks"));
	QDomElement elem = doc.documentElement().appendChild(doc.createElementNS(NS_STORAGE_BOOKMARKS,PST_BOOKMARKS)).toElement();
	foreach(IBookMark bookmark, ABookmarks)
	{
		if (!bookmark.name.isEmpty())
		{
			if (!bookmark.conference.isEmpty())
			{
				QDomElement markElem = elem.appendChild(doc.createElement("conference")).toElement();
				markElem.setAttribute("jid",bookmark.conference);
				markElem.setAttribute("name",bookmark.name);
				markElem.setAttribute("autojoin",QVariant(bookmark.autojoin).toString());
				markElem.appendChild(doc.createElement("nick")).appendChild(doc.createTextNode(bookmark.nick));
				if (!bookmark.password.isEmpty())
					markElem.appendChild(doc.createElement("password")).appendChild(doc.createTextNode(bookmark.password));
			}
			else if (!bookmark.url.isEmpty())
			{
				QDomElement markElem = elem.appendChild(doc.createElement("url")).toElement();
				markElem.setAttribute("name",bookmark.name);
				markElem.setAttribute("url",bookmark.url);
			}
		}
	}
	return FPrivateStorage->saveData(AStreamJid,elem);
}

int BookMarks::execEditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) const
{
	EditBookmarkDialog *dialog = new EditBookmarkDialog(ABookmark,AParent);
	return dialog->exec();
}

void BookMarks::showEditBookmarksDialog(const Jid &AStreamJid)
{
	if (FBookMarks.contains(AStreamJid))
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
}

void BookMarks::updateBookmarksMenu()
{
	bool enabled = false;
	QList<Action *> actionList = FBookMarksMenu->groupActions();
	for (int i=0; !enabled && i<actionList.count(); i++)
		enabled = actionList.at(i)->isVisible();
	FBookMarksMenu->menuAction()->setEnabled(enabled);
}

void BookMarks::startBookmark(const Jid &AStreamJid, const IBookMark &ABookmark, bool AShowWindow)
{
	if (!ABookmark.conference.isEmpty())
	{
		Jid roomJid = ABookmark.conference;
		IMultiUserChatWindow *window = FMultiChatPlugin!=NULL ? FMultiChatPlugin->getMultiChatWindow(AStreamJid,roomJid,ABookmark.nick,ABookmark.password) : NULL;
		if (window)
		{
			if (AShowWindow)
				window->showTabPage();
			if (!window->multiUserChat()->isOpen())
				window->multiUserChat()->setAutoPresence(true);
		}
	}
	else if (!ABookmark.url.isEmpty())
	{
		if (FXmppUriQueries && ABookmark.url.startsWith("xmpp:",Qt::CaseInsensitive))
			FXmppUriQueries->openXmppUri(AStreamJid, ABookmark.url);
		else
			QDesktopServices::openUrl(ABookmark.url);
	}
}

void BookMarks::onPrivateStorageOpened(const Jid &AStreamJid)
{
	FPrivateStorage->loadData(AStreamJid,PST_BOOKMARKS,NS_STORAGE_BOOKMARKS);
}

void BookMarks::onPrivateDataError(const QString &AId, const QString &AError)
{
	emit bookmarksError(AId,AError);
}

void BookMarks::onPrivateDataLoadedSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
	bool ignoreAutoJoin = account && account->optionsNode().value("ignore-autojoin").toBool();

	if (AElement.tagName()==PST_BOOKMARKS && AElement.namespaceURI()==NS_STORAGE_BOOKMARKS)
	{
		QList<IBookMark> &streamBookmarks = FBookMarks[AStreamJid];
		Menu *streamMenu = FStreamMenu.value(AStreamJid,NULL);
		int groupShift = streamMenu!=NULL ? streamMenu->menuAction()->data(ADR_GROUP_SHIFT).toInt() : FBookMarksMenu->menuAction()->data(ADR_GROUP_SHIFT).toInt();
		if (!streamMenu)
		{
			streamMenu = new Menu(FBookMarksMenu);
			streamMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS);
			IAccount *account = FAccountManager->accountByStream(AStreamJid);
			if (account)
			{
				connect(account->instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onAccountOptionsChanged(const OptionsNode &)));
				streamMenu->setTitle(account->name());
			}
			else
			{
				streamMenu->setTitle(AStreamJid.uFull());
			}
			streamMenu->menuAction()->setData(ADR_GROUP_SHIFT,groupShift);

			Action *action = new Action(streamMenu);
			action->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_EDIT);
			action->setText(tr("Edit bookmarks"));
			action->setData(ADR_STREAM_JID,AStreamJid.full());
			connect(action,SIGNAL(triggered(bool)),SLOT(onEditBookmarksActionTriggered(bool)));
			streamMenu->addAction(action,AG_BBM_BOOKMARKS_TOOLS,true);

			FStreamMenu.insert(AStreamJid,streamMenu);
			FBookMarksMenu->addAction(streamMenu->menuAction(),AG_BMM_BOOKMARKS_STREAMS,false);
			FBookMarksMenu->menuAction()->setData(ADR_GROUP_SHIFT,groupShift+1);
		}
		else
		{
			qDeleteAll(streamMenu->groupActions(AG_BMM_BOOKMARKS_ITEMS));
			qDeleteAll(FBookMarksMenu->groupActions(AG_BMM_BOOKMARKS_ITEMS + groupShift));
			streamBookmarks.clear();
		}

		QDomElement elem = AElement.firstChildElement();
		while (!elem.isNull())
		{
			IBookMark bookmark;
			if (elem.tagName() == "conference")
			{
				bookmark.name = elem.attribute("name","conference");
				bookmark.conference = elem.attribute("jid");
				bookmark.autojoin = QVariant(elem.attribute("autojoin")).toBool();
				bookmark.nick = elem.firstChildElement("nick").text();
				bookmark.password = elem.firstChildElement("password").text();
				streamBookmarks.append(bookmark);
				if (bookmark.autojoin && !ignoreAutoJoin)
				{
					FPendingBookMarks.insertMulti(AStreamJid,bookmark);
					FStartTimer.start(FIRST_START_TIMEOUT);
			}
			}
			else if (elem.tagName() == "url")
			{
				bookmark.name = elem.attribute("name","url");
				bookmark.url = elem.attribute("url");
				streamBookmarks.append(bookmark);
			}
			if (!bookmark.name.isEmpty())
			{
				Action *action = new Action(streamMenu);
				action->setIcon(RSR_STORAGE_MENUICONS ,bookmark.conference.isEmpty() ? MNI_BOOKMARKS_URL : MNI_BOOKMARKS_ROOM);
				action->setText(bookmark.name);
				action->setData(ADR_STREAM_JID,AStreamJid.full());
				action->setData(ADR_BOOKMARK_INDEX,streamBookmarks.count()-1);
				connect(action,SIGNAL(triggered(bool)),SLOT(onBookmarkActionTriggered(bool)));
				streamMenu->addAction(action,AG_BMM_BOOKMARKS_ITEMS,false);
				FBookMarksMenu->addAction(action,AG_BMM_BOOKMARKS_ITEMS + groupShift,false);
			}
			elem = elem.nextSiblingElement();
		}
		updateBookmarksMenu();
		emit bookmarksUpdated(AId,AStreamJid,AElement);
	}
}

void BookMarks::onPrivateDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	if (AElement.tagName()==PST_BOOKMARKS && AElement.namespaceURI()==NS_STORAGE_BOOKMARKS)
	{
		if (FStreamMenu.contains(AStreamJid))
		{
			qDeleteAll(FStreamMenu[AStreamJid]->groupActions(AG_BMM_BOOKMARKS_ITEMS));
			FBookMarks[AStreamJid].clear();
			FPendingBookMarks.remove(AStreamJid);
		}
		updateBookmarksMenu();
		emit bookmarksUpdated(AId,AStreamJid,AElement);
	}
}

void BookMarks::onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (ATagName==PST_BOOKMARKS && ANamespace==NS_STORAGE_BOOKMARKS)
	{
		FPrivateStorage->loadData(AStreamJid,PST_BOOKMARKS,NS_STORAGE_BOOKMARKS);
	}
}

void BookMarks::onPrivateStorageClosed(const Jid &AStreamJid)
{
	delete FDialogs.take(AStreamJid);
	delete FStreamMenu.take(AStreamJid);
	FBookMarks.remove(AStreamJid);
	FPendingBookMarks.remove(AStreamJid);
	updateBookmarksMenu();
}

void BookMarks::onMultiChatWindowCreated(IMultiUserChatWindow *AWindow)
{
	Action *action = new Action(AWindow->instance());
	action->setText(tr("Append to bookmarks"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_ADD);
	action->setShortcutId(SCT_MESSAGEWINDOWS_MUC_BOOKMARK);
	connect(action,SIGNAL(triggered(bool)),SLOT(onAddRoomBookmarkActionTriggered(bool)));
	AWindow->toolBarWidget()->toolBarChanger()->insertAction(action, TBG_MCWTBW_BOOKMARKS);
}

void BookMarks::onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow)
{
	connect(AWindow->instance(),SIGNAL(indexContextMenu(const QModelIndex &, Menu *)),SLOT(onDiscoIndexContextMenu(const QModelIndex &, Menu *)));
}

void BookMarks::onDiscoIndexContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
	Action *action = new Action(AMenu);
	action->setText(tr("Append to bookmarks"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_BOOKMARKS_ADD);
	action->setData(ADR_STREAM_JID,AIndex.data(DIDR_STREAM_JID));
	action->setData(ADR_DISCO_JID,AIndex.data(DIDR_JID));
	action->setData(ADR_DISCO_NODE,AIndex.data(DIDR_NODE));
	action->setData(ADR_DISCO_NAME,AIndex.data(DIDR_NAME));
	connect(action,SIGNAL(triggered(bool)),SLOT(onAddDiscoBookmarkActionTriggered(bool)));
	AMenu->addAction(action, TBG_DIWT_DISCOVERY_ACTIONS, true);
}

void BookMarks::onBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		int index = action->data(ADR_BOOKMARK_INDEX).toInt();
		IBookMark bookmark = FBookMarks.value(streamJid).value(index);
		startBookmark(streamJid,bookmark,true);
	}
}

void BookMarks::onAddRoomBookmarkActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMultiUserChatWindow *window = qobject_cast<IMultiUserChatWindow *>(action->parent());
		if (window)
		{
			if (FPrivateStorage && FPrivateStorage->isOpen(window->streamJid()))
			{
				QList<IBookMark> bookmarkList = bookmarks(window->streamJid());

				int index = 0;
				while (index<bookmarkList.count() && window->roomJid()!=bookmarkList.at(index).conference)
					index++;

				if (index == bookmarkList.count())
					bookmarkList.append(IBookMark());

				IBookMark &bookmark = bookmarkList[index];
				if (bookmark.conference.isEmpty())
				{
					bookmark.name = window->roomJid().uBare();
					bookmark.conference = window->roomJid().bare();
					bookmark.nick = window->multiUserChat()->nickName();
					bookmark.password = window->multiUserChat()->password();
					bookmark.autojoin = false;
				}

				if (execEditBookmarkDialog(&bookmark,window->instance()) == QDialog::Accepted)
				{
					setBookmarks(window->streamJid(),bookmarkList);
				}
			}
		}
	}
}

void BookMarks::onAddDiscoBookmarkActionTriggered(bool)
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

			IBookMark bookmark;
			bookmark.name = "XMPP: ";
			bookmark.name += !discoName.isEmpty() ? discoName + " | " : QString::null;
			bookmark.name += discoJid;
			bookmark.name += !discoNode.isEmpty() ? " | " + discoNode : QString::null;
			bookmark.url = url.toString().replace("?disco=;","?disco;");
			if (execEditBookmarkDialog(&bookmark,NULL) == QDialog::Accepted)
				addBookmark(streamJid,bookmark);
		}
	}
}

void BookMarks::onEditBookmarksActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		showEditBookmarksDialog(streamJid);
	}
}

void BookMarks::onEditBookmarksDialogDestroyed()
{
	EditBookmarksDialog *dialog = qobject_cast<EditBookmarksDialog *>(sender());
	if (dialog)
		FDialogs.remove(dialog->streamJid());
}

void BookMarks::onAccountOptionsChanged(const OptionsNode &ANode)
{
	IAccount *account = qobject_cast<IAccount *>(sender());
	if (account && account->isActive() && account->optionsNode().childPath(ANode)=="name" &&  FStreamMenu.contains(account->xmppStream()->streamJid()))
		FStreamMenu[account->xmppStream()->streamJid()]->setTitle(ANode.value().toString());
}

void BookMarks::onStartTimerTimeout()
{
	QMultiMap<Jid, IBookMark>::iterator it = FPendingBookMarks.begin();
	if (it != FPendingBookMarks.end())
	{
		bool showAutoJoined = Options::node(OPV_MUC_GROUPCHAT_SHOWAUTOJOINED).value().toBool();
		if (it->conference.isEmpty())
			showAutoJoined = false;
		else if (FMultiChatPlugin && FMultiChatPlugin->multiChatWindow(it.key(),it->conference)!=NULL)
			showAutoJoined = false;
		startBookmark(it.key(),it.value(), showAutoJoined);
		FPendingBookMarks.erase(it);
		FStartTimer.start(NEXT_START_TIMEOUT);
	}
}

Q_EXPORT_PLUGIN2(plg_bookmarks, BookMarks)
