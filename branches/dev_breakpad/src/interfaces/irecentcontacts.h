#ifndef IRECENTCONTACTS_H
#define IRECENTCONTACTS_H

#include <QIcon>
#include <QList>
#include <QDateTime>
#include <interfaces/irostersmodel.h>
#include <utils/jid.h>
#include <utils/menu.h>

#define RECENTCONTACTS_UUID "{8AD56476-F9FC-4967-B196-78B616DDFD21}"

struct IRecentItem
{
	QString type;
	Jid streamJid;
	QString reference;
	QDateTime activeTime;
	QDateTime updateTime;
	QMap<QString, QVariant> properties;
	bool operator<(const IRecentItem &AOther) const {
		if (type != AOther.type)
			return type < AOther.type;
		if (streamJid != AOther.streamJid)
			return streamJid < AOther.streamJid;
		return reference < AOther.reference;
	}
	bool operator==(const IRecentItem &AOther) const {
		return type==AOther.type && streamJid==AOther.streamJid && reference==AOther.reference;
	}
};

class IRecentItemHandler
{
public:
	virtual QObject *instance() = 0;
	virtual bool recentItemValid(const IRecentItem &AItem) const =0;
	virtual bool recentItemCanShow(const IRecentItem &AItem) const =0;
	virtual QIcon recentItemIcon(const IRecentItem &AItem) const =0;
	virtual QString recentItemName(const IRecentItem &AItem) const =0;
	virtual IRecentItem recentItemForIndex(const IRosterIndex *AIndex) const =0;
	virtual QList<IRosterIndex *> recentItemProxyIndexes(const IRecentItem &AItem) const =0;
protected:
	virtual void recentItemUpdated(const IRecentItem &AItem) =0;
};

class IRecentContacts
{
public:
	virtual QObject *instance() = 0;
	virtual bool isReady(const Jid &AStreamJid) const =0;
	virtual bool isValidItem(const IRecentItem &AItem) const =0;
	virtual QList<IRecentItem> streamItems(const Jid &AStreamJid) const =0;
	virtual QVariant itemProperty(const IRecentItem &AItem, const QString &AName) const =0;
	virtual void setItemProperty(const IRecentItem &AItem, const QString &AName, const QVariant &AValue) =0;
	virtual void setItemActiveTime(const IRecentItem &AItem, const QDateTime &ATime = QDateTime::currentDateTime()) =0;
	virtual void removeItem(const IRecentItem &AItem) =0;
	virtual QList<IRecentItem> visibleItems() const =0;
	virtual IRecentItem rosterIndexItem(const IRosterIndex *AIndex) const =0;
	virtual IRosterIndex *itemRosterIndex(const IRecentItem &AItem) const =0;
	virtual IRosterIndex *itemRosterProxyIndex(const IRecentItem &AItem) const =0;
	virtual QList<QString> itemHandlerTypes() const =0;
	virtual IRecentItemHandler *itemTypeHandler(const QString &AType) const =0;
	virtual void registerItemHandler(const QString &AType, IRecentItemHandler *AHandler) =0;
protected:
	virtual void visibleItemsChanged() =0;
	virtual void recentItemAdded(const IRecentItem &AItem) =0;
	virtual void recentItemChanged(const IRecentItem &AItem) =0;
	virtual void recentItemRemoved(const IRecentItem &AItem) =0;
	virtual void recentItemIndexCreated(const IRecentItem &AItem, IRosterIndex *AIndex) =0;
	virtual void itemHandlerRegistered(const QString &AType, IRecentItemHandler *AHandler) =0;
};

Q_DECLARE_METATYPE(IRecentItem);

Q_DECLARE_INTERFACE(IRecentItemHandler,"Vacuum.Plugin.IRecentItemHandler/1.0")
Q_DECLARE_INTERFACE(IRecentContacts,"Vacuum.Plugin.IRecentContacts/1.2")

#endif //IRECENTCONTACTS_H
