#ifndef IROSTER_H
#define IROSTER_H

#include <QList>
#include <QSet>
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"

class IRoster;

class IRosterItem {
public:
  virtual QObject *instance() =0;
  virtual IRoster *roster() const =0;
  virtual const Jid &jid() const =0;
  virtual QString name() const =0;
  virtual QString subscription() const =0;
  virtual QString ask() const =0;
  virtual const QSet<QString> &groups() const =0;
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
  virtual IRosterItem *item(const Jid &) const=0;
  virtual QList<IRosterItem *> items() const =0;
  virtual QSet<QString> groups() const =0;
  virtual QList<IRosterItem *> groupItems(const QString &) const =0;
  virtual QSet<QString> itemGroups(const Jid &) const =0;
  virtual void setItem(const Jid &, const QString &, const QSet<QString> &) =0;
  virtual void sendSubscription(const Jid &, SubscriptionType) =0; 
  virtual void removeItem(const Jid &) =0;
  //Operations on items
  virtual void renameItem(const Jid &, const QString &) =0;
  virtual void copyItemToGroup(const Jid &, const QString &) =0;
  virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo) =0;
  virtual void removeItemFromGroup(const Jid &, const QString &) =0;
  //Operations on group
  virtual void renameGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void copyGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void moveGroupToGroup(const QString &AGroup, const QString &AGroupTo) =0;
  virtual void removeGroup(const QString &AGroup) =0;
public slots:
  virtual void open() =0;
  virtual void close() =0;
signals:
  virtual void opened() =0;
  virtual void closed() =0;
  virtual void itemPush(IRosterItem *) =0;
  virtual void itemRemoved(IRosterItem *) =0;
  virtual void subscription(const Jid &, SubscriptionType) =0; 
};

class IRosterPlugin {
public:
  virtual QObject *instance() =0;
  virtual IRoster *newRoster(IXmppStream *) =0;
  virtual IRoster *getRoster(const Jid &) const =0;
  virtual void removeRoster(const Jid &) =0;
signals:
  virtual void rosterAdded(IRoster *) =0;
  virtual void rosterOpened(IRoster *) =0;
  virtual void rosterClosed(IRoster *) =0;
  virtual void rosterItemPush(IRoster *, IRosterItem *) =0;
  virtual void rosterItemRemoved(IRoster *, IRosterItem *) =0;
  virtual void rosterRemoved(IRoster *) =0;
};

Q_DECLARE_INTERFACE(IRosterItem,"Vacuum.Plugin.IRosterItem/1.0")
Q_DECLARE_INTERFACE(IRoster,"Vacuum.Plugin.IRoster/1.0")
Q_DECLARE_INTERFACE(IRosterPlugin,"Vacuum.Plugin.IRosterPlugin/1.0")

#endif
