#ifndef RECENTCONTACTS_H
#define RECENTCONTACTS_H

#include <QMap>
#include <interfaces/ipluginmanager.h>
#include <interfaces/irecentcontacts.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>
#include <interfaces/iprivatestorage.h>

class RecentContacts : 
	public QObject,
	public IPlugin,
	public IRecentContacts,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRecentContacts IRosterDataHolder);
public:
	RecentContacts();
	~RecentContacts();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return RECENTCONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin();	
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRecentContacts
	virtual bool isItemValid(const IRecentItem &AItem) const;
	virtual QList<IRecentItem> visibleItems() const;
	virtual QList<IRecentItem> streamItems(const Jid &AStreamJid) const;
	virtual QList<IRecentItem> favoriteItems(const Jid &AStreamJid) const;
	virtual void setItemFavorite(const IRecentItem &AItem, bool AFavorite);
	virtual void setRecentItem(const IRecentItem &AItem, const QDateTime &ATime = QDateTime::currentDateTime());
	virtual QList<QString> itemHandlerTypes() const;
	virtual IRecentItemHandler *itemTypeHandler(const QString &AType) const;
	virtual void registerItemHandler(const QString &AType, IRecentItemHandler *AHandler);
signals:
	void visibleItemsChanged();
	void recentItemAdded(const IRecentItem &AItem);
	void recentItemChanged(const IRecentItem &AItem);
	void recentItemRemoved(const IRecentItem &AItem);
	void itemHandlerRegistered(const QString &AType, IRecentItemHandler *AHandler);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
	void updateVisibleItems();
	void updateItemIndex(const IRecentItem &AItem);
	void removeItemIndex(const IRecentItem &AItem);
	void insertRecentItems(const QList<IRecentItem> &AItems);
	QString recentFileName(const Jid &AStreamJid) const;
	void saveItemsToXML(QDomElement &AElement, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromXML(const Jid &AStreamJid, const QDomElement &AElement) const;
	void saveItemsToFile(const QString &AFileName, const QList<IRecentItem> &AItems) const;
	QList<IRecentItem> loadItemsFromFile(const Jid &AStreamJid, const QString &AFileName) const;
protected slots:
	void onRostersModelStreamAdded(const Jid &AStreamJid);
	void onRostersModelStreamRemoved(const Jid &AStreamJid);
	void onRostersModelStreamJidChanged(const Jid &ABefore, const Jid &AAfter);
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageAboutToClose(const Jid &AStreamJid);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
	IRostersModel *FRostersModel;
	IRostersViewPlugin *FRostersViewPlugin;
private:
	QMap<Jid, QList<IRecentItem> > FStreamItems;
	QMap<IRecentItem, IRosterIndex *> FVisibleItems;
private:
	IRosterIndex *FRootIndex;
	QMap<QString, IRecentItemHandler *> FItemHandlers;
};

#endif // RECENTCONTACTS_H
