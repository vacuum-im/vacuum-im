#ifndef IROSTERMANAGER_H
#define IROSTERMANAGER_H

#include <QSet>
#include <QList>
#include <interfaces/ixmppstreammanager.h>
#include <utils/jid.h>

#define ROSTER_UUID "{5306971C-2488-40d9-BA8E-C83327B2EED5}"

#define SUBSCRIPTION_BOTH             "both"
#define SUBSCRIPTION_TO               "to"
#define SUBSCRIPTION_FROM             "from"
#define SUBSCRIPTION_NONE             "none"
#define SUBSCRIPTION_SUBSCRIBE        "subscribe"
#define SUBSCRIPTION_SUBSCRIBED       "subscribed"
#define SUBSCRIPTION_UNSUBSCRIBE      "unsubscribe"
#define SUBSCRIPTION_UNSUBSCRIBED     "unsubscribed"
#define SUBSCRIPTION_REMOVE           "remove"

#define ROSTER_GROUP_DELIMITER        "::"

struct IRosterItem 
{
	Jid itemJid;
	QString name;
	QString subscription;
	QString subscriptionAsk;
	QSet<QString> groups;

	IRosterItem() {
		subscription = SUBSCRIPTION_NONE;
	}
	inline bool isNull() const {
		return itemJid.isEmpty();
	}
	inline bool operator==(const IRosterItem &AOther) const {
		return itemJid==AOther.itemJid && name==AOther.name && groups==AOther.groups && subscription==AOther.subscription && subscriptionAsk==AOther.subscriptionAsk;
	}
	inline bool operator!=(const IRosterItem &AOther) const {
		return !operator==(AOther);
	}
};

class IRoster 
{
public:
	enum SubscriptionType {
		Subscribe,
		Subscribed,
		Unsubscribe,
		Unsubscribed
	};
public:
	virtual QObject *instance() =0;
	virtual Jid streamJid() const =0;
	virtual IXmppStream *xmppStream() const =0;
	virtual bool isOpen() const =0;
	virtual QString groupDelimiter() const =0;
	virtual QList<Jid> itemsJid() const =0;
	virtual QList<IRosterItem> items() const =0;
	virtual bool hasItem(const Jid &AItemJid) const =0;
	virtual IRosterItem findItem(const Jid &AItemJid) const =0;
	virtual QSet<QString> groups() const =0;
	virtual bool hasGroup(const QString &AGroup) const =0;
	virtual QSet<QString> itemGroups(const Jid &AItemJid) const =0;
	virtual QList<IRosterItem> groupItems(const QString &AGroup) const =0;
	virtual bool isSubgroup(const QString &ASubGroup, const QString &AGroup) const =0;
	virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups) =0;
	virtual void setItems(const QList<IRosterItem> &AItems) =0;
	virtual void removeItem(const Jid &AItemJid) =0;
	virtual void removeItems(const QList<IRosterItem> &AItems) =0;
	virtual void saveRosterItems(const QString &AFileName) const =0;
	virtual void loadRosterItems(const QString &AFileName) =0;
	//Item Subscription
	virtual QSet<Jid> subscriptionRequests() const =0;
	virtual void sendSubscription(const Jid &AItemJid, int ASubsType, const QString &AText = QString::null) =0;
	//Roster Items
	virtual void renameItem(const Jid &AItemJid, const QString &AName) =0;
	virtual void copyItemToGroup(const Jid &AItemJid, const QString &AGroup) =0;
	virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo) =0;
	virtual void removeItemFromGroup(const Jid &AItemJid, const QString &AGroup) =0;
	//Roster Groups
	virtual void renameGroup(const QString &AGroup, const QString &AGroupTo) =0;
	virtual void copyGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
	virtual void moveGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
	virtual void removeGroup(const QString &AGroup) =0;
protected:
	virtual void opened() =0;
	virtual void closed() =0;
	virtual void itemReceived(const IRosterItem &ARosterItem, const IRosterItem &ABefore) =0;
	virtual void subscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText) =0;
	virtual void subscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText) =0;
	virtual void streamJidAboutToBeChanged(const Jid &AAfter) =0;
	virtual void streamJidChanged(const Jid &ABefore) =0;
	virtual void rosterDestroyed() =0;
};

class IRosterManager 
{
public:
	virtual QObject *instance() =0;
	virtual QList<IRoster *> rosters() const =0;
	virtual IRoster *findRoster(const Jid &AStreamJid) const =0;
	virtual IRoster *createRoster(IXmppStream *AXmppStream) =0;
	virtual void destroyRoster(IRoster *ARoster) =0;
	virtual bool isRosterActive(IRoster *ARoster) const =0;
	virtual QString rosterFileName(const Jid &AStreamJid) const =0;
protected:
	virtual void rosterCreated(IRoster *ARoster) =0;
	virtual void rosterOpened(IRoster *ARoster) =0;
	virtual void rosterClosed(IRoster *ARoster) =0;
	virtual void rosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore) =0;
	virtual void rosterSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText) =0;
	virtual void rosterSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText) =0;
	virtual void rosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter) =0;
	virtual void rosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore) =0;
	virtual void rosterActiveChanged(IRoster *ARoster, bool AActive) =0;
	virtual void rosterDestroyed(IRoster *ARoster) =0;
};

Q_DECLARE_INTERFACE(IRoster,"Vacuum.Plugin.IRoster/1.3")
Q_DECLARE_INTERFACE(IRosterManager,"Vacuum.Plugin.IRosterManager/1.3")

#endif //IROSTERMANAGER_H
