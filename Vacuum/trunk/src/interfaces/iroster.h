#ifndef IROSTER_H
#define IROSTER_H

#include <QList>
#include <QSet>
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"

#define ROSTER_UUID "{5306971C-2488-40d9-BA8E-C83327B2EED5}"

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
  enum SubsType {
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
  virtual IRosterItem *item(const Jid &AItemJid) const=0;
  virtual QList<IRosterItem *> items() const =0;
  virtual QSet<QString> groups() const =0;
  virtual QList<IRosterItem *> groupItems(const QString &AGroup) const =0;
  virtual QSet<QString> itemGroups(const Jid &AItemJid) const =0;
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups) =0;
  virtual void removeItem(const Jid &AItemJid) =0;
  virtual void saveRosterItems(const QString &AFileName) const =0;
  virtual void loadRosterItems(const QString &AFileName) =0;
  //Operations  on subscription
  virtual void sendSubscription(const Jid &AItemJid, SubsType ASubsType, const QString &AText = QString()) =0; 
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
  virtual void opened() =0;
  virtual void itemPush(IRosterItem *ARosterItem) =0;
  virtual void itemRemoved(IRosterItem *ARosterItem) =0;
  virtual void subscription(const Jid &AItemJid, IRoster::SubsType ASubsType, const QString &AText) =0; 
  virtual void closed() =0;
  virtual void jidAboutToBeChanged(const Jid &AAfter) =0;
};

class IRosterPlugin {
public:
  virtual QObject *instance() =0;
  virtual IRoster *addRoster(IXmppStream *AXmppStream) =0;
  virtual IRoster *getRoster(const Jid &AStreamJid) const =0;
  virtual void loadRosterItems(const Jid &AStreamJid) =0;
  virtual void removeRoster(IXmppStream *AXmppStream) =0;
signals:
  virtual void rosterAdded(IRoster *ARoster) =0;
  virtual void rosterOpened(IRoster *ARoster) =0;
  virtual void rosterItemPush(IRoster *ARoster, IRosterItem *ARosterItem) =0;
  virtual void rosterItemRemoved(IRoster *ARoster, IRosterItem *ARosterItem) =0;
  virtual void rosterSubscription(IRoster *ARoster, const Jid &AJid, IRoster::SubsType, const QString &AText) =0;
  virtual void rosterClosed(IRoster *ARoster) =0;
  virtual void rosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter) =0;
  virtual void rosterRemoved(IRoster *ARoster) =0;
};

Q_DECLARE_INTERFACE(IRosterItem,"Vacuum.Plugin.IRosterItem/1.0")
Q_DECLARE_INTERFACE(IRoster,"Vacuum.Plugin.IRoster/1.0")
Q_DECLARE_INTERFACE(IRosterPlugin,"Vacuum.Plugin.IRosterPlugin/1.0")

#endif
