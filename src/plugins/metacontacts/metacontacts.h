#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <QMap>
#include <QHash>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irecentcontacts.h>
#include "combinecontactsdialog.h"
#include "metasortfilterproxymodel.h"

struct MetaMergedContact {
	QUuid id;
	Jid stream;
	Jid itemJid;
	QString name;
	QSet<QString> groups;
	IPresenceItem presence;
	QMultiMap<Jid, Jid> items;
	QMultiMap<Jid, IPresenceItem> presences;
};

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts,
	public IRosterDataHolder,
	public IRostersLabelHolder,
	public IRostersClickHooker,
	public IRostersDragDropHandler,
	public IRostersEditHandler,
	public IRecentItemHandler,
	public AdvancedDelegateEditProxy
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts IRosterDataHolder IRostersLabelHolder IRostersClickHooker IRostersDragDropHandler IRostersEditHandler IRecentItemHandler);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.MetaContacts");
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject *instance() { return this; }
	// IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	// IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	// IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	// IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AHover, Menu *AMenu);
	// IRostersEditHandler
	virtual quint32 rosterEditLabel(int AOrder, int ADataRole, const QModelIndex &AIndex) const;
	virtual AdvancedDelegateEditProxy *rosterEditProxy(int AOrder, int ADataRole, const QModelIndex &AIndex);
	// IRecentItemHandler
	virtual bool recentItemValid(const IRecentItem &AItem) const;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual IRecentItem recentItemForIndex(const IRosterIndex *AIndex) const;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const;
	// AdvancedDelegateEditProxy
	virtual bool setModelData(const AdvancedItemDelegate *ADelegate, QWidget *AEditor, QAbstractItemModel *AModel, const QModelIndex &AIndex);
	// IMetaContacts
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual QList<Jid> findMetaStreams(const QUuid &AMetaId) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const Jid &AItemJid) const;
	virtual IMetaContact findMetaContact(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual QList<IRosterIndex *> findMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId) const;
	virtual bool createMetaContact(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName, const QList<Jid> &AItems);
	virtual bool setMetaContactName(const Jid &AStreamJid, const QUuid &AMetaId, const QString &AName);
	virtual bool setMetaContactGroups(const Jid &AStreamJid, const QUuid &AMetaId, const QSet<QString> &AGroups);
	virtual bool insertMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
	virtual bool removeMetaContactItems(const Jid &AStreamJid, const QUuid &AMetaId, const QList<Jid> &AItems);
signals:
	void metaContactsOpened(const Jid &AStreamJid);
	void metaContactsClosed(const Jid &AStreamJid);
	void metaContactChanged(const Jid &AStreamJid, const IMetaContact &AMetaContact, const IMetaContact &ABefore);
	// IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
	// IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
	// IRecentItemHandler
	void recentItemUpdated(const IRecentItem &AItem);
protected:
	IRosterIndex *getMetaIndexRoot(const Jid &AStreamJid) const;
	MetaMergedContact getMergedContact(const Jid &AStreamJid, const QUuid &AMetaId) const;
	QList<IRecentItem> findMetaRecentContacts(const Jid &StreamJid, const QUuid &AMetaId) const;
protected:
	void updateMetaIndexes(const Jid &AStreamJid, const QUuid &AMetaId);
	void updateMetaIndexItems(IRosterIndex *AMetaIndex, const MetaMergedContact &AMeta);
	void updateMetaWindows(const Jid &AStreamJid, const QUuid &AMetaId);
	void updateMetaRecentItems(const Jid &AStreamJid, const QUuid &AMetaId);
	bool updateMetaContact(const Jid &AStreamJid, const IMetaContact &AMetaContact);
	void updateMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AMetaContacts);
	void startUpdateMetaContact(const Jid &AStreamJid, const QUuid &AMetaId);
protected:
	bool isReadyStreams(const QStringList &AStreams) const;
	bool isValidItem(const Jid &AStreamJid, const Jid &AItemJid) const;
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
	bool hasProxiedIndexes(const QList<IRosterIndex *> &AIndexes) const;
	QList<IRosterIndex *> indexesProxies(const QList<IRosterIndex *> &AIndexes, bool ASelfProxy=true) const;
	QMap<int, QStringList> indexesRolesMap(const QList<IRosterIndex *> &AIndexes, const QList<int> &ARoles) const;
protected:
	void renameMetaContact(const QStringList &AStreams, const QStringList &AMetas);
	void removeMetaItems(const QStringList &AStreams, const QStringList &AContacts);
	void combineMetaItems(const QStringList &AStreams, const QStringList &AContacts, const QStringList &AMetas);
	void destroyMetaContacts(const QStringList &AStreams, const QStringList &AMetas);
protected:
	void startSaveContactsToStorage(const Jid &AStreamJid);
	bool saveContactsToStorage(const Jid &AStreamJid) const;
	QString metaContactsFileName(const Jid &AStreamJid) const;
	QList<IMetaContact> loadMetaContactsFromXML(const QDomElement &AElement) const;
	void saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const;
	QList<IMetaContact> loadMetaContactsFromFile(const QString &AFileName) const;
	void saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const;
protected slots:
	void onRosterOpened(IRoster *ARoster);
	void onRosterActiveChanged(IRoster *ARoster, bool AActive);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
protected slots:
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
protected slots:
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
	void onPrivateStorageClosed(const Jid &AStreamJid);
protected slots:
	void onRostersModelStreamsLayoutChanged(int ABefore);
	void onRostersModelIndexInserted(IRosterIndex *AIndex);
	void onRostersModelIndexDestroyed(IRosterIndex *AIndex);
	void onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole);
protected slots:
	void onRostersViewIndexContextMenuAboutToShow();
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onRostersViewNotifyInserted(int ANotifyId);
	void onRostersViewNotifyRemoved(int ANotifyId);
	void onRostersViewNotifyActivated(int ANotifyId);
protected slots:
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
	void onMessageChatWindowChanged();
	void onMessageChatWindowDestroyed();
protected slots:
	void onRecentContactsOpened(const Jid &AStreamJid);
	void onRecentItemChanged(const IRecentItem &AItem);
	void onRecentItemRemoved(const IRecentItem &AItem);
protected slots:
	void onRemoveMetaItemsByAction();
	void onCombineMetaItemsByAction();
	void onRenameMetaContactByAction();
	void onDestroyMetaContactsByAction();
	void onCopyMetaContactToGroupByAction();
	void onMoveMetaContactToGroupByAction();
protected slots:
	void onUpdateContactsTimerTimeout();
	void onLoadContactsFromFileTimerTimeout();
	void onSaveContactsToStorageTimerTimeout();
protected slots:
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStatusIcons *FStatusIcons;
	IMessageWidgets *FMessageWidgets;
	IRecentContacts *FRecentContacts;
private:
	QTimer FSaveTimer;
	QTimer FUpdateTimer;
	QSet<Jid> FSaveStreams;
	QSet<Jid> FLoadStreams;
	QMap<Jid, QString> FLoadRequestId;
	QMap<Jid, QSet<QUuid> > FUpdateMeta;
private:
	QMap<Jid, QHash<Jid, QUuid> > FItemMetaId;
	QMap<Jid, QHash<QUuid, IMetaContact> > FMetaContacts;
private:
	QMap<int, int> FNotifyProxyToIndex;
	QMap<Menu *, Menu *> FProxyContextMenu;
	MetaSortFilterProxyModel *FFilterProxyModel;
private:
	QHash<const IRosterIndex *, IRosterIndex *> FMetaIndexToProxy;
	QHash<const IRosterIndex *, IRosterIndex *> FMetaProxyToIndex;
	QMap<const IRosterIndex *, QHash<QUuid, QList<IRosterIndex *> > > FMetaIndexes;
private:
	QHash<const IRosterIndex *, IRosterIndex *> FMetaItemIndexToProxy;
	QMultiHash<const IRosterIndex *, IRosterIndex *> FMetaItemProxyToIndex;
	QHash<const IRosterIndex *, QMap<Jid, QMap<Jid, IRosterIndex *> > > FMetaIndexItems;
private:
	QMap<const IRosterIndex *, QHash<QUuid, IMessageChatWindow *> > FMetaChatWindows;
private:
	IRecentItem FUpdatingRecentItem;
	QMap<const IRosterIndex *, QHash<QUuid, IRecentItem> > FMetaRecentItems;
};

#endif // METACONTACTS_H
