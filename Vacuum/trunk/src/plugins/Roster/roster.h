#ifndef ROSTER_H
#define ROSTER_H

#include "../../interfaces/iroster.h"
#include "../../interfaces/istanzaprocessor.h"
#include "rosteritem.h"

#define NS_JABBER_ROSTER "jabber:iq:roster"

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

  //IRoster
  virtual const Jid &streamJid() const { return FStream->jid(); }
  virtual IXmppStream *xmppStream() const { return FStream; }
  virtual bool isOpen() const { return FOpen; }
  virtual QString groupDelimiter() const { return "::"; }
  virtual IRosterItem *item(const Jid &AItemJid) const;
  virtual QList<IRosterItem *> items() const;
  virtual QSet<QString> groups() const;
  virtual QList<IRosterItem *> groupItems(const QString &AGroup) const;
  virtual QSet<QString> itemGroups(const Jid &AItemJid) const;
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups);
  virtual void sendSubscription(const Jid &AItemJid, SubscriptionType AType); 
  virtual void removeItem(const Jid &AItemJid);
  //Item operations
  virtual void renameItem(const Jid &AItemJid, const QString &AName);
  virtual void copyItemToGroup(const Jid &AItemJid, const QString &AGroup);
  virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo);
  virtual void removeItemFromGroup(const Jid &AItemJid, const QString &AGroup);
  //Group operations
  virtual void renameGroup(const QString &AGroupFrom, const QString &AGroupTo);
  virtual void removeGroup(const QString &AGroup);
public slots:
  virtual void open();
  virtual void close();
signals:
  virtual void opened();
  virtual void closed();
  virtual void itemPush(IRosterItem *);
  virtual void itemRemoved(IRosterItem *);
  virtual void subscription(const Jid &AItemJid, SubscriptionType AType);
protected slots:
  virtual void onStreamOpened(IXmppStream *);
  virtual void onStreamClosed(IXmppStream *);
protected:
  void clearItems();
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
