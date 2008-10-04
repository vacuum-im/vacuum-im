#include "bookmarks.h"

#include <QDesktopServices>

#define BOOKMARKS_TAGNAME       "storage"

#define IN_BOOKMARKS            "psi/history"
#define IN_BOOKMARK_CONF        "psi/groupChat"
#define IN_BOOKMARK_URL         "psi/www"

#define ADR_STREAM_JID          Action::DR_StreamJid
#define ADR_BOOKMARK_INDEX      Action::DR_Parametr1
#define ADR_ROOMJID             Action::DR_Parametr2

BookMarks::BookMarks()
{
  FStorage = NULL;
  FPresencePlugin = NULL;
  FTrayManager = NULL;
  FMainWindowPlugin = NULL;
  FAccountManager = NULL;
  FMultiChatPlugin = NULL;

  FBookMarksMenu = new Menu;
  FBookMarksMenu->setIcon(SYSTEM_ICONSETFILE,IN_BOOKMARKS);
  FBookMarksMenu->setTitle(tr("Bookmarks"));
  FBookMarksMenu->menuAction()->setEnabled(false);
}

BookMarks::~BookMarks()
{
  delete FBookMarksMenu;
}

void BookMarks::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Storage of bookmarks to conference rooms and other entities in a Jabber user's account");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Bookmarks"); 
  APluginInfo->uid = BOOKMARKS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(PRIVATESTORAGE_UUID);
}

bool BookMarks::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance()); 
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(streamStateChanged(const Jid &, bool)),
        SLOT(onStreamStateChanged(const Jid &, bool)));
    }
  }

  plugin = APluginManager->getPlugins("IPrivateStorage").value(0,NULL);
  if (plugin)
  {
    FStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
    if (FStorage)
    {
      connect(FStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
        SLOT(onStorageDataChanged(const QString &, const Jid &, const QDomElement &)));
      connect(FStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
        SLOT(onStorageDataChanged(const QString &, const Jid &, const QDomElement &)));
      connect(FStorage->instance(),SIGNAL(dataRemoved(const QString &, const Jid &, const QDomElement &)),
        SLOT(onStorageDataRemoved(const QString &, const Jid &, const QDomElement &)));
      connect(FStorage->instance(),SIGNAL(dataError(const QString &, const QString &)),
        SLOT(onStorageDataError(const QString &, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMultiUserChatPlugin").value(0,NULL);
  if (plugin)
  {
    FMultiChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
    if (FMultiChatPlugin)
    {
      connect(FMultiChatPlugin->instance(),SIGNAL(multiChatWindowCreated(IMultiUserChatWindow *)),
        SLOT(onMultiChatWindowCreated(IMultiUserChatWindow *)));  
    }
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
    FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  return FStorage!=NULL;
}

bool BookMarks::initObjects()
{
  if (FTrayManager)
  {
    FTrayManager->addAction(FBookMarksMenu->menuAction(),AG_BOOKMARKS_TRAY,true);
  }
  if (FMainWindowPlugin)
  {
    ToolBarChanger *changer = FMainWindowPlugin->mainWindow()->topToolBarChanger();
    QToolButton *button = changer->addToolButton(FBookMarksMenu->menuAction(),AG_BOOKMARKS_MWTTB,false);
    button->setPopupMode(QToolButton::InstantPopup);
  }
  return true;
}

QString BookMarks::addBookmark(const Jid &AStreamJid, const IBookMark &ABookmark)
{
  if (!ABookmark.name.isEmpty())
  {
    QList<IBookMark> bookmarkList = bookmarks(AStreamJid);
    bookmarkList.append(ABookmark);
    return setBookmarks(AStreamJid,bookmarkList);
  }
  return QString();
}

QString BookMarks::setBookmarks(const Jid &AStreamJid, const QList<IBookMark> &ABookmarks)
{
  QDomDocument doc;
  doc.appendChild(doc.createElement("bookmarks"));
  QDomElement elem = doc.documentElement().appendChild(doc.createElementNS(NS_STORAGE_BOOKMARKS,BOOKMARKS_TAGNAME)).toElement();
  foreach(IBookMark bookmark, ABookmarks)
  {
    if (!bookmark.name.isEmpty())
    {
      if(!bookmark.conference.isEmpty() && !bookmark.nick.isEmpty())
      {
        QDomElement markElem = elem.appendChild(doc.createElement("conference")).toElement();
        markElem.setAttribute("jid",bookmark.conference);
        markElem.setAttribute("name",bookmark.name);
        markElem.setAttribute("autojoin",bookmark.autojoin ? "true" : "false");
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
  return FStorage->saveData(AStreamJid,elem);
}

int BookMarks::execEditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) const
{
  EditBookmarkDialog *dialog = new EditBookmarkDialog(ABookmark,AParent);
  return dialog->exec();
}

void BookMarks::updateBookmarksMenu()
{
  bool enabled = false;
  QList<Action *> actionList = FBookMarksMenu->actions();
  for (int i=0; !enabled && i<actionList.count(); i++)
    enabled = actionList.at(i)->isVisible();
  FBookMarksMenu->menuAction()->setEnabled(enabled);
}

void BookMarks::removeStreamMenu(const Jid &AStreamJid)
{
  if (FStreamMenu.contains(AStreamJid))
  {
    Menu *streamMenu = FStreamMenu.take(AStreamJid);
    streamMenu->clear();
    delete streamMenu;
    FBookMarks.remove(AStreamJid);
    updateBookmarksMenu();
  }
}

void BookMarks::startBookmark(const Jid &AStreamJid, const IBookMark &ABookmark, bool AShowWindow)
{
  if (!ABookmark.conference.isEmpty())
  {
    Jid roomJid = ABookmark.conference;
    IMultiUserChatWindow *window = FMultiChatPlugin->multiChatWindow(AStreamJid,roomJid);
    if (!window)
    {
      window = FMultiChatPlugin->getMultiChatWindow(AStreamJid,roomJid,ABookmark.nick,ABookmark.password);
      window->multiUserChat()->setAutoPresence(true);
    }
    if (AShowWindow)
      window->showWindow();
  }
  else if (!ABookmark.url.isEmpty())
  {
    QDesktopServices::openUrl(ABookmark.url);
  }
}

void BookMarks::onStreamStateChanged(const Jid &AStreamJid, bool AStateOnline)
{
  if (AStateOnline)
    FStorage->loadData(AStreamJid,BOOKMARKS_TAGNAME,NS_STORAGE_BOOKMARKS);
  else
    removeStreamMenu(AStreamJid);
}

void BookMarks::onStorageDataChanged(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
  if (AElement.tagName() == BOOKMARKS_TAGNAME && AElement.namespaceURI() == NS_STORAGE_BOOKMARKS)
  {
    QList<IBookMark> &streamBookmarks = FBookMarks[AStreamJid];
    Menu *streamMenu = FStreamMenu.value(AStreamJid,NULL);
    if (!streamMenu)
    {
      streamMenu = new Menu(FBookMarksMenu);
      streamMenu->setIcon(SYSTEM_ICONSETFILE,IN_BOOKMARKS);
      IAccount *account = FAccountManager->accountByStream(AStreamJid);
      if(account)
      {
        connect(account->instance(),SIGNAL(changed(const QString &, const QVariant &)),
          SLOT(onAccountChanged(const QString &, const QVariant &)));
        streamMenu->setTitle(account->name());
      }
      else
        streamMenu->setTitle(AStreamJid.full());
      
      Action *action = new Action(streamMenu);
      action->setIcon(SYSTEM_ICONSETFILE,IN_BOOKMARKS);
      action->setText(tr("Edit bookmarks"));
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onEditBookmarksActionTriggered(bool)));
      streamMenu->addAction(action,AG_BBM_BOOKMARKS_TOOLS,true);

      FStreamMenu.insert(AStreamJid,streamMenu);
      FBookMarksMenu->addAction(streamMenu->menuAction(),AG_BMM_BOOKMARKS_STREAMS,true);
    }
    else
    {
      qDeleteAll(streamMenu->actions(AG_DEFAULT));
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
        bookmark.autojoin = elem.attribute("autojoin")=="true";
        bookmark.nick = elem.firstChildElement("nick").text();
        bookmark.password = elem.firstChildElement("password").text();
        streamBookmarks.append(bookmark);
        if (bookmark.autojoin)
          startBookmark(AStreamJid,bookmark,false);
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
        action->setIcon(SYSTEM_ICONSETFILE ,bookmark.conference.isEmpty() ? IN_BOOKMARK_URL : IN_BOOKMARK_CONF);
        action->setText(bookmark.name);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_BOOKMARK_INDEX,streamBookmarks.count()-1);
        connect(action,SIGNAL(triggered(bool)),SLOT(onBookmarkActionTriggered(bool)));
        streamMenu->addAction(action,AG_DEFAULT,false);
        FBookMarksMenu->addAction(action,AG_BMM_BOOKMARKS_ITEMS,true);
      }
      elem = elem.nextSiblingElement();
    }
    updateBookmarksMenu();
    emit bookmarksUpdated(AId,AStreamJid,AElement);
  }
}

void BookMarks::onStorageDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
  if (AElement.tagName() == BOOKMARKS_TAGNAME && AElement.namespaceURI() == NS_STORAGE_BOOKMARKS)
  {
    if (FStreamMenu.contains(AStreamJid))
    {
      qDeleteAll(FStreamMenu[AStreamJid]->actions(AG_DEFAULT));
      FBookMarks[AStreamJid].clear();
    }
    updateBookmarksMenu();
    emit bookmarksUpdated(AId,AStreamJid,AElement);
  }
}

void BookMarks::onStorageDataError(const QString &AId, const QString &AError)
{
  emit bookmarksError(AId,AError);
}

void BookMarks::onMultiChatWindowCreated(IMultiUserChatWindow *AWindow)
{
  Action *action = new Action(AWindow->roomMenu());
  action->setText(tr("Bookmark this room"));
  action->setIcon(SYSTEM_ICONSETFILE,IN_BOOKMARKS);
  action->setData(ADR_STREAM_JID,AWindow->streamJid().full());
  action->setData(ADR_ROOMJID,AWindow->roomJid().bare());
  connect(action,SIGNAL(triggered(bool)),SLOT(onAddBookmarkActionTriggered(bool)));
  AWindow->roomMenu()->addAction(action,AG_MURM_BOOKMARKS,true);
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

void BookMarks::onAddBookmarkActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid roomJid = action->data(ADR_ROOMJID).toString();
    IMultiUserChatWindow *window = FMultiChatPlugin->multiChatWindow(streamJid,roomJid);
    if (window)
    {
      IBookMark bookmark;
      bookmark.name = roomJid.bare();
      bookmark.conference = roomJid.bare();
      bookmark.nick = window->multiUserChat()->nickName();
      bookmark.password = window->multiUserChat()->password();
      bookmark.autojoin = false;
      if (execEditBookmarkDialog(&bookmark,window) == QDialog::Accepted)
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
    if (FBookMarks.contains(streamJid))
    {
      EditBookmarksDialog *dialog = new EditBookmarksDialog(this,streamJid,FBookMarks.value(streamJid));
      dialog->show();
    }
  }
}

void BookMarks::onAccountChanged(const QString &AName, const QVariant &AValue)
{
  IAccount *account = qobject_cast<IAccount *>(sender());
  if (account && AName == AVN_NAME && FStreamMenu.contains(account->streamJid()))
    FStreamMenu[account->streamJid()]->setTitle(AValue.toString());
}

Q_EXPORT_PLUGIN2(BookMarksPlugin, BookMarks)