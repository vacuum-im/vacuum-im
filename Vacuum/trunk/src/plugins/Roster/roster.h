#ifndef ROSTER_H
#define ROSTER_H

#include "interfaces/iroster.h"
#include "interfaces/istanzaprocessor.h"

#define NS_JABBER_ROSTER "jabber:iq:roster"

class RosterItem : 
  public QObject,
  public IRosterItem
{
  Q_OBJECT;
  Q_INTERFACES(IRosterItem);
  friend class Roster;

public:
  RosterItem(const Jid &AJid, QObject *parent) : QObject(parent) {
    FJid = AJid; 
    FRoster = qobject_cast<IRoster *>(parent); 
  }
  ~RosterItem() {}

  virtual QObject *instance() { return this; }
  virtual IRoster *roster() const { return FRoster; }
  virtual const Jid &jid() const {return FJid; }
  virtual QString name() const { if (!FName.isEmpty()) return FName; else return FJid.bare();  }
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


class Roster :
  public QObject,
  public IRoster,
  private IStanzaProcessorHandler,
  private IStanzaProcessorIqOwner
{
  Q_OBJECT;
  Q_INTERFACES(IRoster IStanzaProcessorHandler IStanzaProcessorIqOwner);

public:
  Roster(IXmppStream *AStream, IStanzaProcessor *AStanzaProcessor, QObject *parent);
  ~Roster();

  virtual QObject *instance() { return this; }

  //IStanzaProcessorHandler
  virtual bool editStanza(HandlerId, const Jid &, Stanza *, bool &) { return false; }
  virtual bool stanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);

  //IStanzaProcessorIqOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);

  //Roster
  virtual const Jid &streamJid() const { return FStream->jid(); }
  virtual bool isOpen() const { return FOpen; }
  virtual QString groupDelimiter() const { return "::"; }
  virtual IRosterItem *item(const Jid &AItemJid) const;
  virtual QList<IRosterItem *> items() const;
  virtual QList<IRosterItem *> groupItems(const QString &AGroup) const;
  virtual QSet<QString> groups() const;
public slots:
  virtual void open();
  virtual void close();
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups);
  virtual void removeItem(const Jid &AItemJid);
  virtual void sendSubscription(const Jid &AItemJid, SubscriptionType AType); 
signals:
  virtual void opened();
  virtual void closed();
  virtual void itemPush(IRosterItem *);
  virtual void itemRemoved(IRosterItem *);
  virtual void subscription(const Jid &AItemJid, SubscriptionType AType); 
private:
  IXmppStream *FStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  bool FOpen;
  QString FOpenId;
  QList<RosterItem *> FItems;
  HandlerId FRosterHandler;
  HandlerId FSubscrHandler;
};

#endif
