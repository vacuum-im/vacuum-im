#ifndef RECENTCONTACTS_H
#define RECENTCONTACTS_H

#include <QMap>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irecentcontacts.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istatusicons.h>
#include <interfaces/imessageprocessor.h>

class RecentContacts : 
	public QObject,
	public IPlugin,
	public IRecentContacts,
	public IRosterDataHolder,
	public IRecentItemHandler,
	public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRecentContacts IRosterDataHolder IRecentItemHandler IRostersClickHooker);
public:
	RecentContacts();
	~RecentContacts();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return RECENTCONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IRecentItemHandler
	virtual bool recentItemValid(const IRecentItem &AItem) const;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const;
	//IRecentContacts
	virtual bool isItemValid(const IRecentItem &AItem) const;
	virtual QList<IRecentItem> streamItems(const Jid &AStreamJid) const;
	virtual QList<IRecentItem> favoriteItems(const Jid &AStreamJid) const;
	virtual void setItemFavorite(const IRecentItem &AItem, bool AFavorite);
	virtual void setItemActiveTime(const IRecentItem &AItem, const QDateTime &ATime = QDateTime::currentDateTime());
	virtual QList<IRecentItem> visibleItems() const;
	virtual quint8 maximumVisibleItems() const;
	virtual void setMaximumVisibleItems(quint8 ACount);
	virtual IRosterIndex *itemRosterIndex(const IRecentItem &AItem) const;
	virtual IRosterIndex *itemRosterProxyIndex(const IRecentItem &AItem) const;
	virtual QList<QString> itemHandlerTypes() const;
	virtual IRecentItemHandler *itemTypeHandler(const QString &AType) const;
	virtual void registerItemHandler(const QString &AType, IRecentItemHandler *AHandler);
signals:
	void visibleItemsChanged();
	void recentItemAdded(const IRecentItem &AItem);
	void recentItemChanged(const IRecentItem &AItem);
	void recentItemRemoved(const IRecentItem &AItem);
	void recentItemIndexCreated(const IRecentItem &AItem, IRosterIndex *AIndex);
	void itemHandlerRegistered(const QString &AType, IRecentItemHandler *AHandler);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
	//IRecentItemHandler
	void recentItemUpdated(const IRecentItem &AItem);
protected:
	void updateVisibleItems();
	void createItemIndex(const IRecentItem &AItem);
	void updateItemIndex(const IRecentItem &AItem);
	void removeItemIndex(const IRecentItem &AItem);
	void updateItemProxy(const IRecentItem &AItem);
	void mergeRecentItems(const QList<IRecentItem> &AItems);
	IRecentItem &findRealItem(const IRecentItem &AItem);
	IRecentItem rosterIndexItem(const IRosterIndex *AIndex) const;
	QList<IRosterIndex *> sortItemProxies(const QList<IRosterIndex *> &AIndexes) const;
	QList<IRosterIndex *> indexesProxies(const QList<IRosterIndex *> &AIndexes, bool AExclusive=true) const;
protected:
	void startSaveItemsToStorage(const Jid &AStreamJid);
	bool saveItemsToStorage(const Jid &AStreamJid);
protected:
	QString recentFileName(const Jid &AStreamJid) const;
	void saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromXML(const Jid &AStreamJid, const QDomElement &AElement) const;
	void saveItemsToFile(const QString &AFileName, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromFile(const Jid &AStreamJid, const QString &AFileName) const;
protected:
	bool isRecentSelectionAccepted(const QList<IRosterIndex *> AIndexes) const;
	bool isContactsSelectionAccepted(const QList<IRosterIndex *> AIndexes) const;
	void setItemsFavorite(bool AFavorite, QStringList AIndexTypes, QStringList AStreamJids, QStringList ARecentTypes, QStringList ARefs, QStringList AContactsJids);
protected slots:
	void onRostersModelStreamAdded(const Jid &AStreamJid);
	void onRostersModelStreamRemoved(const Jid &AStreamJid);
	void onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void onRostersModelIndexInserted(IRosterIndex *AIndex);
	void onRostersModelIndexRemoved(IRosterIndex *AIndex);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
protected slots:
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMultiMap<int,QString> &AToolTips);
	void onRostersViewNotifyInserted(int ANotifyId);
	void onRostersViewNotifyRemoved(int ANotifyId);
	void onRostersViewNotifyActivated(int ANotifyId);
protected slots:
	void onProxyIndexDataChanged(IRosterIndex *AIndex, int ARole = 0);
	void onHandlerRecentItemUpdated(const IRecentItem &AItem);
protected slots:
	void onInsertToFavoritesByAction();
	void onRemoveFromFavoritesByAction();
	void onSaveItemsToStorageTimerTimeout();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStatusIcons *FStatusIcons;
	IMessageProcessor *FMessageProcessor;
private:
	quint32 FShowFavariteLabelId;
	quint32 FInsertFavariteLabelId;
	quint32 FRemoveFavoriteLabelId;
private:
	quint8 FMaxVisibleItems;
	QMap<Jid, QList<IRecentItem> > FStreamItems;
	QMap<IRecentItem, IRosterIndex *> FVisibleItems;
private:
	QTimer FSaveTimer;
	QSet<Jid> FSaveStreams;
private:
	QMap<int, int> FProxyToIndexNotify;
	QMap<const IRosterIndex *, IRosterIndex *> FIndexToProxy;
	QMap<const IRosterIndex *, IRosterIndex *> FProxyToIndex;
	QMap<IRosterIndex *, QList<IRosterIndex *> > FIndexProxies;
private:
	IRosterIndex *FRootIndex;
	QMap<QString, IRecentItemHandler *> FItemHandlers;
};

#endif // RECENTCONTACTS_H
