#ifndef IROSTER_H
#define IROSTER_H

#include <QList>
#include <QSet>
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"

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

struct IRosterItem {
  IRosterItem() { isValid = false; subscription = SUBSCRIPTION_NONE; }
  bool isValid;
  Jid itemJid;
  QString name;
  QString subscription;
  QString ask;
  QSet<QString> groups;
};

class IRoster {
public:
  enum SubscriptionType {
    Subscribe,
    Subscribed,
    Unsubscribe,
    Unsubscribed
  };
public:
  virtual QObject *instance() =0;
  virtual const Jid &streamJid() const =0;
  virtual IXmppStream *xmppStream() const =0;
  virtual bool isOpen() const =0;
  virtual QString groupDelimiter() const =0;
  virtual IRosterItem rosterItem(const Jid &AItemJid) const =0;
  virtual QList<IRosterItem> rosterItems() const =0;
  virtual QSet<QString> groups() const =0;
  virtual QList<IRosterItem> groupItems(const QString &AGroup) const =0;
  virtual QSet<QString> itemGroups(const Jid &AItemJid) const =0;
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups) =0;
  virtual void removeItem(const Jid &AItemJid) =0;
  virtual void saveRosterItems(const QString &AFileName) const =0;
  virtual void loadRosterItems(const QString &AFileName) =0;
  //Operations  on subscription
  virtual void sendSubscription(const Jid &AItemJid, int ASubsType, const QString &AText = QString()) =0; 
  //Operations on items
  virtual void renameItem(const Jid &AItemJid, const QString &AName) =0;
  virtual void copyItemToGroup(const Jid &AItemJid, const QString &AGroup) =0;
  virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo) =0;
  virtual void removeItemFromGroup(const Jid &AItemJid, const QString &AGroup) =0;
  //Operations on group
  virtual void renameGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void copyGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void moveGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void removeGroup(const QString &AGroup) =0;
signals:
  virtual void jidAboutToBeChanged(const Jid &AAfter) =0;
  virtual void opened() =0;
  virtual void received(const IRosterItem &ARosterItem) =0;
  virtual void removed(const IRosterItem &ARosterItem) =0;
  virtual void subscription(const Jid &AItemJid, int ASubsType, const QString &AText) =0; 
  virtual void closed() =0;
};

class IRosterPlugin {
public:
  virtual QObject *instance() =0;
  virtual IRoster *addRoster(IXmppStream *AXmppStream) =0;
  virtual IRoster *getRoster(const Jid &AStreamJid) const =0;
  virtual void saveRosterItems(const Jid &AStreamJid) =0;
  virtual void loadRosterItems(const Jid &AStreamJid) =0;
  virtual void removeRoster(IXmppStream *AXmppStream) =0;
signals:
  virtual void rosterAdded(IRoster *ARoster) =0;
  virtual void rosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter) =0;
  virtual void rosterOpened(IRoster *ARoster) =0;
  virtual void rosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem) =0;
  virtual void rosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem) =0;
  virtual void rosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText) =0;
  virtual void rosterClosed(IRoster *ARoster) =0;
  virtual void rosterRemoved(IRoster *ARoster) =0;
};

Q_DECLARE_INTERFACE(IRoster,"Vacuum.Plugin.IRoster/1.0")
Q_DECLARE_INTERFACE(IRosterPlugin,"Vacuum.Plugin.IRosterPlugin/1.0")

#endif
