#ifndef ROSTER_H
#define ROSTER_H

#include "../../definations/stanzahandlerpriority.h"
#include "../../definations/namespaces.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/istanzaprocessor.h"

class Roster :
  public QObject,
  public IRoster,
  private IStanzaHandler,
  private IIqStanzaOwner
{
  Q_OBJECT;
  Q_INTERFACES(IRoster IStanzaHandler IIqStanzaOwner);
public:
  Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
  ~Roster();
  virtual QObject *instance() { return this; }
  //IStanzaProcessorHandler
  virtual bool editStanza(int, const Jid &, Stanza *, bool &) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaProcessorIqOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IRoster
  virtual const Jid &streamJid() const { return FXmppStream->jid(); }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool isOpen() const { return FOpen; }
  virtual QString groupDelimiter() const { return FGroupDelim; }
  virtual IRosterItem rosterItem(const Jid &AItemJid) const;
  virtual QList<IRosterItem> rosterItems() const;
  virtual QSet<QString> groups() const;
  virtual QList<IRosterItem> groupItems(const QString &AGroup) const;
  virtual QSet<QString> itemGroups(const Jid &AItemJid) const;
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups);
  virtual void sendSubscription(const Jid &AItemJid, int AType, const QString &AText = QString()); 
  virtual void removeItem(const Jid &AItemJid);
  virtual void saveRosterItems(const QString &AFileName) const;
  virtual void loadRosterItems(const QString &AFileName);
  //Operations on items
  virtual void renameItem(const Jid &AItemJid, const QString &AName);
  virtual void copyItemToGroup(const Jid &AItemJid, const QString &AGroup);
  virtual void moveItemToGroup(const Jid &AItemJid, const QString &AGroupFrom, const QString &AGroupTo);
  virtual void removeItemFromGroup(const Jid &AItemJid, const QString &AGroup);
  //Operations on group
  virtual void renameGroup(const QString &AGroup, const QString &AGroupTo);
  virtual void copyGroupToGroup(const QString &AGroup, const QString &AGroupTo);
  virtual void moveGroupToGroup(const QString &AGroup, const QString &AGroupTo);
  virtual void removeGroup(const QString &AGroup);
signals:
  virtual void jidAboutToBeChanged(const Jid &AAfter);
  virtual void opened();
  virtual void received(const IRosterItem &ARosterItem);
  virtual void removed(const IRosterItem &ARosterItem);
  virtual void subscription(const Jid &AItemJid, int ASubsType, const QString &AText);
  virtual void closed();
protected:
  bool processItemsElement(const QDomElement &AItemsElem, bool ARemoveOld = false);
  void removeRosterItem(const Jid &AItemJid);
  void requestGroupDelimiter();
  void setGroupDelimiter(const QString &ADelimiter);
  void requestRosterItems();
  void clearItems();
  void setStanzaHandlers();
  void removeStanzaHandlers();
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamJidAboutToBeChanged(IXmppStream *AXmppStream, const Jid &AAfter);
private:
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  bool FOpen;
  int FRosterHandler;
  int FSubscrHandler;
  QString FOpenId;
  QString FGroupDelimId;
  QString FGroupDelim;
  QHash<Jid, IRosterItem> FRosterItems;
};

#endif
