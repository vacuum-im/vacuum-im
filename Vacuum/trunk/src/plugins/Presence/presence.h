#ifndef PRESENCE_H
#define PRESENCE_H

#include <QObject>
#include "../../definations/stanzahandlerpriority.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/errorhandler.h"
#include "presenceitem.h"

class Presence : 
  public QObject,
  public IPresence,
  private IStanzaHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPresence IStanzaHandler);
public:
  Presence(IXmppStream *AXmppStream, IStanzaProcessor *AStanzaProcessor);
  ~Presence();
  virtual QObject *instance() { return this; }
  //IStanzaProcessorHandler
  virtual bool editStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza * /*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IPresence
  virtual const Jid &streamJid() const { return FXmppStream->jid(); }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool isOpen() const { return FOpened; }
  virtual Show show() const { return FShow; }
  virtual bool setShow(Show AShow, const Jid &AToJid = Jid());
  virtual const QString &status() const { return FStatus; }
  virtual bool setStatus(const QString &AStatus, const Jid &AToJid = Jid());
  virtual qint8 priority() const { return FPriority; }
  virtual bool setPriority(qint8 APriority, const Jid &AToJid = Jid());
  virtual bool setPresence(Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid = Jid());
  virtual IPresenceItem *item(const Jid &AItemJid) const;
  virtual QList<IPresenceItem *> items() const;
  virtual QList<IPresenceItem *> items(const Jid &AItemJid) const;
signals:
  virtual void opened();
  virtual void selfPresence(int AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid);
  virtual void presenceItem(IPresenceItem *APresenceItem);
  virtual void aboutToClose(int AShow, const QString &AStatus);
  virtual void closed();
protected:
  void clearItems();
protected slots:
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamError(IXmppStream *AXmppStream, const QString &AError);
private:
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  bool FOpened;
  int FPresenceHandler;
  QList<PresenceItem *> FPresenceItems;
  Show FShow;
  QString FStatus;
  qint8 FPriority;
};

#endif // PRESENCE_H
