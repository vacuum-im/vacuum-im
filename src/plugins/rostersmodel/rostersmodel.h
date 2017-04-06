#ifndef ROSTERSMODEL_H
#define ROSTERSMODEL_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/iaccountmanager.h>
#include "rootindex.h"
#include "rosterindex.h"
#include "dataholder.h"

class RostersModel :
	public AdvancedItemModel,
	public IPlugin,
	public IRostersModel,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRostersModel IRosterDataHolder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.RostersModel");
public:
	RostersModel();
	~RostersModel();
	virtual AdvancedItemModel *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTERSMODEL_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IRostersModel
	virtual QList<Jid> streams() const;
	virtual IRosterIndex *addStream(const Jid &AStreamJid);
	virtual void removeStream(const Jid &AStreamJid);
	virtual int streamsLayout() const;
	virtual void setStreamsLayout(StreamsLayout ALayout);
	virtual IRosterIndex *rootIndex() const;
	virtual IRosterIndex *contactsRoot() const;
	virtual IRosterIndex *streamRoot(const Jid &AStreamJid) const;
	virtual IRosterIndex *streamIndex(const Jid &AStreamJid) const;
	virtual IRosterIndex *newRosterIndex(int AKind);
	virtual void insertRosterIndex(IRosterIndex *AIndex, IRosterIndex *AParent);
	virtual void removeRosterIndex(IRosterIndex *AIndex, bool ADestroy = true);
	virtual IRosterIndex *findGroupIndex(int AKind, const QString &AGroup, IRosterIndex *AParent) const;
	virtual IRosterIndex *getGroupIndex(int AKind, const QString &AGroup, IRosterIndex *AParent);
	virtual QList<IRosterIndex *> findContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, IRosterIndex *AParent = NULL) const;
	virtual QList<IRosterIndex *> getContactIndexes(const Jid &AStreamJid, const Jid &AContactJid, IRosterIndex *AParent = NULL);
	virtual QModelIndex modelIndexFromRosterIndex(IRosterIndex *AIndex) const;
	virtual IRosterIndex *rosterIndexFromModelIndex(const QModelIndex &AIndex) const;
	virtual bool isGroupKind(int AKind) const;
	virtual QList<int> singleGroupKinds() const;
	virtual QString singleGroupName(int AKind) const;
	virtual void registerSingleGroup(int AKind, const QString &AName);
	virtual QMultiMap<int, IRosterDataHolder *> rosterDataHolders() const;
	virtual void insertRosterDataHolder(int AOrder, IRosterDataHolder *AHolder);
	virtual void removeRosterDataHolder(int AOrder, IRosterDataHolder *AHolder);
signals:
	//IRostersModel
	void streamAdded(const Jid &AStreamJid);
	void streamRemoved(const Jid &AStreamJid);
	void streamJidChanged(const Jid &ABefore, const Jid &AAfter);
	void streamsLayoutAboutToBeChanged(int AAfter);
	void streamsLayoutChanged(int ABefore);
	void indexCreated(IRosterIndex *AIndex);
	void indexInserted(IRosterIndex *AIndex);
	void indexRemoving(IRosterIndex *AIndex);
	void indexDestroyed(IRosterIndex *AIndex);
	void indexDataChanged(IRosterIndex *AIndex, int ARole);
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void updateStreamsLayout();
	void emitIndexDestroyed(IRosterIndex *AIndex);
	void removeEmptyGroup(IRosterIndex *AGroupIndex);
	QString getGroupName(int AKind, const QString &AGroup) const;
	bool isChildIndex(IRosterIndex *AIndex, IRosterIndex *AParent) const;
protected slots:
	void onAdvancedItemInserted(QStandardItem *AItem);
	void onAdvancedItemRemoving(QStandardItem *AItem);
	void onAdvancedItemDataChanged(QStandardItem *AItem, int ARole);
protected slots:
	void onAccountOptionsChanged(const OptionsNode &ANode);
	void onAccountActiveChanged(IAccount *AAccount, bool AActive);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void onPresenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriority);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
private:
	friend class RosterIndex;
private:
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IAccountManager *FAccountManager;
private:
	StreamsLayout FLayout;
	RootIndex *FRootIndex;
	IRosterIndex *FContactsRoot;
	QMap<int, QString> FSingleGroups;
	QMap<Jid,IRosterIndex *> FStreamIndexes;
	QMultiMap<int, IRosterDataHolder *> FRosterDataHolders;
	QMap<IRosterDataHolder *, DataHolder *> FAdvancedDataHolders;
private:
	// streamRoot->bareJid->index
	QHash<IRosterIndex *, QMultiHash<Jid, IRosterIndex *> > FContactsCache;
	// parent->name->index
	QHash<IRosterIndex *, QMultiHash<QString, IRosterIndex *> > FGroupsCache;
};

#endif // ROSTERSMODEL_H
