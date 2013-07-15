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
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/rosterdataholderorders.h>
#include <definitions/rosteredithandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibookmarks.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <utils/advanceditemdelegate.h>
#include <utils/textmanager.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/menu.h>
#include "editbookmarkdialog.h"
#include "editbookmarksdialog.h"

class Bookmarks :
	public QObject,
	public IPlugin,
	public IBookmarks,
	public IOptionsHolder,
	public IRosterDataHolder,
	public IRostersEditHandler,
	public AdvancedDelegateEditProxy
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IBookmarks IOptionsHolder IRosterDataHolder IRostersEditHandler);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IBookmarks");
#endif
public:
	Bookmarks();
	~Bookmarks();
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
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersEditHandler
	virtual quint32 rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const;
	virtual AdvancedDelegateEditProxy *rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex);
	//AdvancedDelegateEditProxy
	virtual bool setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex);
	//IBookmarks
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual bool isValidBookmark(const IBookmark &ABookmark) const;
	virtual QList<IBookmark> bookmarks(const Jid &AStreamJid) const;
	virtual bool addBookmark(const Jid &AStreamJid, const IBookmark &ABookmark);
	virtual bool setBookmarks(const Jid &AStreamJid, const QList<IBookmark> &ABookmarks);
	virtual int execEditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent) const;
	virtual void showEditBookmarksDialog(const Jid &AStreamJid);
signals:
	void bookmarksChanged(const Jid &AStreamJid);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void updateConferenceIndexes(const Jid &AStreamJid);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	QList<IBookmark> loadBookmarksFromXML(const QDomElement &AElement) const;
	void saveBookmarksToXML(QDomElement &AElement, const QList<IBookmark> &ABookmarks) const;
	void renameBookmark(const Jid &AStreamJid, const IBookmark &ABookmark);
	void startBookmark(const Jid &AStreamJid, const IBookmark &ABookmark, bool AShowWindow);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateDataUpdated(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageClosed(const Jid &AStreamJid);
protected slots:
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
protected slots: 
	void onRosterIndexDestroyed(IRosterIndex *AIndex);
	void onMultiChatWindowCreated(IMultiUserChatWindow *AWindow);
	void onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow);
	void onDiscoIndexContextMenu(const QModelIndex &AIndex, Menu *AMenu);
	void onStartBookmarkActionTriggered(bool);
	void onEditBookmarkActionTriggered(bool);
	void onRenameBookmarkActionTriggered(bool);
	void onChangeBookmarkAutoJoinActionTriggered(bool);
	void onAddBookmarksActionTriggered(bool);
	void onRemoveBookmarksActionTriggered(bool);
	void onEditBookmarksActionTriggered(bool);
	void onMultiChatWindowAddBookmarkActionTriggered(bool);
	void onDiscoWindowAddBookmarkActionTriggered(bool);
	void onEditBookmarksDialogDestroyed();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IPrivateStorage *FPrivateStorage;
	IAccountManager *FAccountManager;
	IMultiUserChatPlugin *FMultiChatPlugin;
	IXmppUriQueries *FXmppUriQueries;
	IServiceDiscovery *FDiscovery;
	IOptionsManager *FOptionsManager;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QMap<Jid, QList<IBookmark> > FBookmarks;
	QMap<Jid, EditBookmarksDialog *> FDialogs;
	QMap<Jid, QMap<IRosterIndex *, IBookmark> > FBookmarkIndexes;
};

#endif // BOOKMARKS_H
