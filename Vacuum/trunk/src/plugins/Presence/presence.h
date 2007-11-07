#ifndef PRESENCE_H
#define PRESENCE_H

#include <QObject>
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"
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
  virtual bool editStanza(HandlerId, const Jid &, Stanza *, bool &) { return false; }
  virtual bool readStanza(HandlerId AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  
  //IPresence
  virtual const Jid &streamJid() const { return FXmppStream->jid(); }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool setPresence(Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid = Jid());
  virtual Show show() const { return FShow; }
  virtual bool setShow(Show AShow, const Jid &AToJid = Jid());
  virtual const QString &status() const { return FStatus; }
  virtual bool setStatus(const QString &AStatus, const Jid &AToJid = Jid());
  virtual qint8 priority() const { return FPriority; }
  virtual bool setPriority(qint8 APriority, const Jid &AToJid = Jid());
  virtual IPresenceItem *item(const Jid &AItemJid) const;
  virtual QList<IPresenceItem *> items() const;
  virtual QList<IPresenceItem *> items(const Jid &AItemJid) const;
signals:
  virtual void opened();
  virtual void selfPresence(IPresence::Show, const QString &, qint8, const Jid &);
  virtual void presenceItem(IPresenceItem *APresenceItem);
  virtual void closed();
protected:
  void clearItems();
  void setStanzaHandlers();
  void removeStanzaHandlers();
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamError(IXmppStream *AXmppStream, const QString &AError);
private:
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  HandlerId FPresenceHandler;
  QList<PresenceItem *> FPresenceItems;
  Show FShow;
  QString FStatus;
  qint8 FPriority;
};

#endif // PRESENCE_H
