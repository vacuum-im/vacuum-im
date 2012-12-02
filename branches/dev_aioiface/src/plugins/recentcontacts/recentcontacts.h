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
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual IRosterIndex *recentItemProxyIndex(const IRecentItem &AItem) const;
	//IRecentContacts
	virtual bool isItemValid(const IRecentItem &AItem) const;
	virtual QList<IRecentItem> streamItems(const Jid &AStreamJid) const;
	virtual QList<IRecentItem> favoriteItems(const Jid &AStreamJid) const;
	virtual void setItemFavorite(const IRecentItem &AItem, bool AFavorite);
	virtual void setItemDateTime(const IRecentItem &AItem, const QDateTime &ATime = QDateTime::currentDateTime());
	virtual QList<IRecentItem> visibleItems() const;
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
	void insertRecentItems(const QList<IRecentItem> &AItems);
	IRecentItem &findRealItem(const IRecentItem &AItem);
	IRecentItem rosterIndexItem(const IRosterIndex *AIndex) const;
protected:
	QString recentFileName(const Jid &AStreamJid) const;
	void saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromXML(const Jid &AStreamJid, const QDomElement &AElement) const;
	void saveItemsToFile(const QString &AFileName, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromFile(const Jid &AStreamJid, const QString &AFileName) const;
protected slots:
	void onRostersModelStreamAdded(const Jid &AStreamJid);
	void onRostersModelStreamRemoved(const Jid &AStreamJid);
	void onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void onRostersModelIndexInserted(IRosterIndex *AIndex);
	void onRostersModelIndexRemoved(IRosterIndex *AIndex);
protected slots:
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageAboutToClose(const Jid &AStreamJid);
protected slots:
	void onProxyIndexDataChanged(IRosterIndex *AIndex, int ARole = 0);
	void onProxyIndexDestroyed(IRosterIndex *AIndex);
protected slots:
	void onHandlerRecentItemUpdated(const IRecentItem &AItem);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStatusIcons *FStatusIcons;
	IMessageProcessor *FMessageProcessor;
private:
	quint32 FInsertFavariteLabelId;
	quint32 FRemoveFavoriteLabelId;
private:
	QMap<Jid, QList<IRecentItem> > FStreamItems;
	QMap<IRecentItem, IRosterIndex *> FVisibleItems;
	QMap<const IRosterIndex *, IRosterIndex *> FIndexToProxy;
	QMap<const IRosterIndex *, IRosterIndex *> FProxyToIndex;
private:
	IRosterIndex *FRootIndex;
	QMap<QString, IRecentItemHandler *> FItemHandlers;
};

#endif // RECENTCONTACTS_H
