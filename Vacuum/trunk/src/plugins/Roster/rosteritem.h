#ifndef ROSTERITEM_H
#define ROSTERITEM_H

#include "../../interfaces/iroster.h"

class RosterItem : 
  public QObject,
  public IRosterItem
{
  Q_OBJECT;
  Q_INTERFACES(IRosterItem);
  friend class Roster;

public:
  RosterItem(const Jid &AJid, QObject *parent);
  ~RosterItem();

  virtual QObject *instance() { return this; }
  virtual IRoster *roster() const { return FRoster; }
  virtual const Jid &jid() const {return FJid; }
  virtual QString name() const;
  virtual QString subscription() const { return FSubscr; }
  virtual QString ask() const { return FAsk; }
  virtual const QSet<QString> &groups() const { return FGroups; }
protected:
  virtual RosterItem &setName(const QString &AName) { FName = AName; return *this; }
  virtual RosterItem &setSubscription(const QString &ASubscr) { FSubscr = ASubscr; return *this; }
  virtual RosterItem &setAsk(const QString &AAsk) { FAsk = AAsk; return *this; }
  virtual RosterItem &setGroups(const QSet<QString> &AGroups) { FGroups = AGroups; return *this; }
private:
  IRoster *FRoster;
private:
  Jid FJid;
  QString FName;
  QString FSubscr;
  QString FAsk;
  QSet<QString> FGroups;
};

#endif // ROSTERITEM_H
