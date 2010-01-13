#ifndef ROSTER_H
#define ROSTER_H

#include <definations/namespaces.h>
#include <interfaces/iroster.h>
#include <interfaces/istanzaprocessor.h>

class Roster :
  public QObject,
  public IRoster,
  private IStanzaHandler,
  private IStanzaRequestOwner
{
  Q_OBJECT;
  Q_INTERFACES(IRoster IStanzaHandler IStanzaRequestOwner);
public:
  Roster(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
  ~Roster();
  virtual QObject *instance() { return this; }
  //IStanzaProcessorHandler
  virtual bool stanzaEdit(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza &/*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaProcessorIqOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IRoster
  virtual Jid streamJid() const { return FXmppStream->streamJid(); }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool isOpen() const { return FOpened; }
  virtual QString groupDelimiter() const { return FGroupDelim; }
  virtual IRosterItem rosterItem(const Jid &AItemJid) const;
  virtual QList<IRosterItem> rosterItems() const;
  virtual QSet<QString> groups() const;
  virtual QList<IRosterItem> groupItems(const QString &AGroup) const;
  virtual QSet<QString> itemGroups(const Jid &AItemJid) const;
  virtual void setItem(const Jid &AItemJid, const QString &AName, const QSet<QString> &AGroups);
  virtual void setItems(const QList<IRosterItem> &AItems);
  virtual void sendSubscription(const Jid &AItemJid, int AType, const QString &AText = QString()); 
  virtual void removeItem(const Jid &AItemJid);
  virtual void removeItems(const QList<IRosterItem> &AItems);
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
  void opened();
  void received(const IRosterItem &ARosterItem);
  void removed(const IRosterItem &ARosterItem);
  void subscription(const Jid &AItemJid, int ASubsType, const QString &AText);
  void closed();
  void streamJidAboutToBeChanged(const Jid &AAfter);
  void streamJidChanged(const Jid &ABefore);
protected:
  void processItemsElement(const QDomElement &AItemsElem, bool ACompleteRoster);
  void removeRosterItem(const Jid &AItemJid);
  void requestGroupDelimiter();
  void setGroupDelimiter(const QString &ADelimiter);
  void requestRosterItems();
  void clearItems();
  void setStanzaHandlers();
  void removeStanzaHandlers();
protected slots:
  void onStreamOpened();
  void onStreamClosed();
  void onStreamJidAboutToBeChanged(const Jid &AAfter);
  void onStreamJidChanged(const Jid &ABefore);
private:
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  bool FOpened;
  int FSHIRosterPush;
  int FSHISubscription;
  QString FOpenRequestId;
  QString FDelimRequestId;
  QString FGroupDelim;
  QString FRosterVer;
  QHash<Jid, IRosterItem> FRosterItems;
};

#endif
