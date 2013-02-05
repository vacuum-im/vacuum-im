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
#include <interfaces/ipresence.h>
#include <utils/options.h>

class RecentContacts : 
	public QObject,
	public IPlugin,
	public IRecentContacts,
	public IRosterDataHolder,
	public IRostersDragDropHandler,
	public IRostersLabelHolder,
	public IRostersClickHooker,
	public IRecentItemHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRecentContacts IRosterDataHolder IRostersDragDropHandler IRostersLabelHolder IRostersClickHooker IRecentItemHandler);
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
	//IRostersDragDropHandler
	virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag);
	virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
	virtual bool rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover);
	virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
	virtual bool rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu);
	//IRostersLabelHolder
	virtual QList<quint32> rosterLabels(int AOrder, const IRosterIndex *AIndex) const;
	virtual AdvancedDelegateItem rosterLabel(int AOrder, quint32 ALabelId, const IRosterIndex *AIndex) const;
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IRecentItemHandler
	virtual bool recentItemValid(const IRecentItem &AItem) const;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const;
	virtual QString recentItemName(const IRecentItem &AItem) const;
	virtual IRecentItem recentItemForIndex(const IRosterIndex *AIndex) const;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const;
	//IRecentContacts
	virtual bool isReady(const Jid &AStreamJid) const;
	virtual bool isValidItem(const IRecentItem &AItem) const;
	virtual QList<IRecentItem> streamItems(const Jid &AStreamJid) const;
	virtual QVariant itemProperty(const IRecentItem &AItem, const QString &AName) const;
	virtual void setItemProperty(const IRecentItem &AItem, const QString &AName, const QVariant &AValue);
	virtual void setItemActiveTime(const IRecentItem &AItem, const QDateTime &ATime = QDateTime::currentDateTime());
	virtual void removeItem(const IRecentItem &AItem);
	virtual QList<IRecentItem> visibleItems() const;
	virtual IRecentItem rosterIndexItem(const IRosterIndex *AIndex) const;
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
	//IRostersLabelHolder
	void rosterLabelChanged(quint32 ALabelId, IRosterIndex *AIndex = NULL);
	//IRecentItemHandler
	void recentItemUpdated(const IRecentItem &AItem);
protected:
	void updateVisibleItems();
	void createItemIndex(const IRecentItem &AItem);
	void updateItemIndex(const IRecentItem &AItem);
	void removeItemIndex(const IRecentItem &AItem);
	void updateItemProxy(const IRecentItem &AItem);
	void updateItemProperties(const IRecentItem &AItem);
	IRecentItem &findRealItem(const IRecentItem &AItem);
	IRecentItem findRealItem(const IRecentItem &AItem) const;
	void mergeRecentItems(const Jid &AStreamJid, const QList<IRecentItem> &AItems, bool AReplace);
	QList<IRosterIndex *> sortItemProxies(const QList<IRosterIndex *> &AIndexes) const;
	QList<IRosterIndex *> indexesProxies(const QList<IRosterIndex *> &AIndexes, bool AExclusive=true) const;
protected:
	void startSaveItemsToStorage(const Jid &AStreamJid);
	bool saveItemsToStorage(const Jid &AStreamJid);
protected:
	QString recentFileName(const Jid &AStreamJid) const;
	QList<IRecentItem> loadItemsFromXML(const QDomElement &AElement) const;
	void saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromFile(const QString &AFileName) const;
	void saveItemsToFile(const QString &AFileName, const QList<IRecentItem> &AItems) const;
protected:
	bool isSelectionAccepted(const QList<IRosterIndex *> &AIndexes) const;
	bool isRecentSelectionAccepted(const QList<IRosterIndex *> &AIndexes) const;
	void removeRecentItems(const QStringList &ATypes, const QStringList &AStreamJids, const QStringList &AReferences);
	void setItemsFavorite(bool AFavorite, const QStringList &ATypes, const QStringList &AStreamJids, const QStringList &AReferences);
protected slots:
	void onRostersModelStreamAdded(const Jid &AStreamJid);
	void onRostersModelStreamRemoved(const Jid &AStreamJid);
	void onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void onRostersModelIndexInserted(IRosterIndex *AIndex);
	void onRostersModelIndexDataChanged(IRosterIndex *AIndex, int ARole = 0);
	void onRostersModelIndexRemoved(IRosterIndex *AIndex);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
protected slots:
	void onRostersViewIndexContextMenuAboutToShow();
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMap<int, QString> &AToolTips);
	void onRostersViewNotifyInserted(int ANotifyId);
	void onRostersViewNotifyRemoved(int ANotifyId);
	void onRostersViewNotifyActivated(int ANotifyId);
protected slots:
	void onHandlerRecentItemUpdated(const IRecentItem &AItem);
protected slots:
	void onRemoveFromRecentByAction();
	void onInsertToFavoritesByAction();
	void onRemoveFromFavoritesByAction();
	void onSaveItemsToStorageTimerTimeout();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onChangeAlwaysShowOfflineItems();
	void onChangeHideInactiveItems();
	void onChangeSimpleContactsView();
	void onChangeSortByLastActivity();
	void onChangeShowOnlyFavorite();
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
private:
	quint8 FMaxVisibleItems;
	quint8 FInactiveDaysTimeout;
	QMap<Jid, QList<IRecentItem> > FStreamItems;
	QMap<IRecentItem, IRosterIndex *> FVisibleItems;
private:
	QTimer FSaveTimer;
	QSet<Jid> FSaveStreams;
private:
	QMap<int, int> FProxyToIndexNotify;
	QMap<Menu *, QSet<Action *> > FProxyContextMenuActions;
	QMap<const IRosterIndex *, IRosterIndex *> FIndexToProxy;
	QMap<const IRosterIndex *, IRosterIndex *> FProxyToIndex;
	QMap<IRosterIndex *, QList<IRosterIndex *> > FIndexProxies;
	QList<IRostersDragDropHandler *> FMovedProxyDragHandlers;
	QList<IRostersDragDropHandler *> FExteredProxyDragHandlers;
private:
	bool FHideLaterContacts;
	bool FAllwaysShowOffline;
	bool FSimpleContactsView;
	bool FSortByLastActivity;
	bool FShowOnlyFavorite;
	IRosterIndex *FRootIndex;
	QMap<QString, IRecentItemHandler *> FItemHandlers;
};

#endif // RECENTCONTACTS_H
