#ifndef BOOKMARKS_H
#define BOOKMARKS_H

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
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibookmarks.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/itraymanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/ipresence.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/menu.h>
#include "editbookmarkdialog.h"
#include "editbookmarksdialog.h"

class BookMarks :
			public QObject,
			public IPlugin,
			public IBookMarks,
			public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBookMarks IOptionsHolder);
public:
	BookMarks();
	~BookMarks();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return BOOKMARKS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//IBookMarks
	virtual QList<IBookMark> bookmarks(const Jid &AStreamJid) const { return FBookMarks.value(AStreamJid); }
	virtual QString addBookmark(const Jid &AStreamJid, const IBookMark &ABookmark);
	virtual QString setBookmarks(const Jid &AStreamJid, const QList<IBookMark> &ABookmarks);
	virtual int execEditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) const;
	virtual void showEditBookmarksDialog(const Jid &AStreamJid);
signals:
	void bookmarksUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void bookmarksError(const QString &AId, const QString &AError);
protected:
	void updateBookmarksMenu();
	void startBookmark(const Jid &AStreamJid, const IBookMark &ABookmark, bool AShowWindow);
protected slots:
	void onPresenceOpened(IPresence *APresence);
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateDataError(const QString &AId, const QString &AError);
	void onPrivateDataLoadedSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageClosed(const Jid &AStreamJid);
	void onMultiChatWindowCreated(IMultiUserChatWindow *AWindow);
	void onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow);
	void onDiscoIndexContextMenu(const QModelIndex &AIndex, Menu *AMenu);
	void onBookmarkActionTriggered(bool);
	void onAddRoomBookmarkActionTriggered(bool);
	void onAddDiscoBookmarkActionTriggered(bool);
	void onEditBookmarksActionTriggered(bool);
	void onEditBookmarksDialogDestroyed();
	void onAccountOptionsChanged(const OptionsNode &ANode);
private:
	IPrivateStorage *FPrivateStorage;
	ITrayManager *FTrayManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IAccountManager *FAccountManager;
	IMultiUserChatPlugin *FMultiChatPlugin;
	IXmppUriQueries *FXmppUriQueries;
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IPresencePlugin *FPresencePlugin;
private:
	Menu *FBookMarksMenu;
	QMap<Jid, Menu *> FStreamMenu;
private:
	QMap<Jid, QList<IBookMark> > FBookMarks;
	QMap<Jid, EditBookmarksDialog *> FDialogs;
};

#endif // BOOKMARKS_H
