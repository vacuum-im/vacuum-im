#ifndef BOOKMARKS_H
#define BOOKMARKS_H

#include "../../definations/namespaces.h"
#include "../../definations/actiongroups.h"
#include "../../definations/accountvaluenames.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ibookmarks.h"
#include "../../interfaces/iprivatestorage.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/iaccountmanager.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../utils/menu.h"
#include "../../utils/skin.h"
#include "editbookmarkdialog.h"
#include "editbookmarksdialog.h"

class BookMarks : 
  public QObject,
  public IPlugin,
  public IBookMarks
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IBookMarks);
public:
  BookMarks();
  ~BookMarks();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return BOOKMARKS_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IBookMarks
  virtual QList<IBookMark> bookmarks(const Jid &AStreamJid) const { return FBookMarks.value(AStreamJid); }
  virtual QString addBookmark(const Jid &AStreamJid, const IBookMark &ABookmark);
  virtual QString setBookmarks(const Jid &AStreamJid, const QList<IBookMark> &ABookmarks);
  virtual int execEditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) const;
signals:
  virtual void bookmarksUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  virtual void bookmarksError(const QString &AId, const QString &AError);
protected:
  void updateBookmarksMenu();
  void removeStreamMenu(const Jid &AStreamJid);
  void startBookmark(const Jid &AStreamJid, const IBookMark &ABookmark, bool AShowWindow);
protected slots:
  void onStreamStateChanged(const Jid &AStreamJid, bool AStateOnline);
  void onStorageDataChanged(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onStorageDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onStorageDataError(const QString &AId, const QString &AError);
  void onMultiChatWindowCreated(IMultiUserChatWindow *AWindow);
  void onBookmarkActionTriggered(bool);
  void onAddBookmarkActionTriggered(bool);
  void onEditBookmarksActionTriggered(bool);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
private:
  IPrivateStorage *FStorage;
  IPresencePlugin *FPresencePlugin;
  ITrayManager *FTrayManager;
  IMainWindowPlugin *FMainWindowPlugin;
  IAccountManager *FAccountManager;
  IMultiUserChatPlugin *FMultiChatPlugin;
private:
  Menu *FBookMarksMenu;
  QHash<Jid, Menu *> FStreamMenu;
private:
  QHash<Jid, QList<IBookMark> > FBookMarks;
};

#endif // BOOKMARKS_H
