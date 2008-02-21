#ifndef PRESENCE_H
#define PRESENCE_H

#include <QObject>
#include "../../definations/stanzahandlerpriority.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/errorhandler.h"

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
  virtual int show() const { return FShow; }
  virtual bool setShow(int AShow);
  virtual const QString &status() const { return FStatus; }
  virtual bool setStatus(const QString &AStatus);
  virtual int priority() const { return FPriority; }
  virtual bool setPriority(int APriority);
  virtual bool setPresence(int AShow, const QString &AStatus, int APriority);
  virtual IPresenceItem presenceItem(const Jid &AItemJid) const { return FItems.value(AItemJid); }
  virtual QList<IPresenceItem> presenceItems(const Jid &AItemJid = Jid()) const;
signals:
  virtual void opened();
  virtual void changed(int AShow, const QString &AStatus, int APriority);
  virtual void received(const IPresenceItem &APresenceItem);
  virtual void aboutToClose(int AShow, const QString &AStatus);
  virtual void closed();
protected:
  void clearItems();
protected slots:
  void onStreamError(IXmppStream *AXmppStream, const QString &AError);
  void onStreamClosed(IXmppStream *AXmppStream);
private:
  IXmppStream *FXmppStream;
  IStanzaProcessor *FStanzaProcessor;
private:
  bool FOpened;
  int FPresenceHandler;
  int FShow;
  int FPriority;
  QString FStatus;
  QHash<Jid, IPresenceItem> FItems;
};

#endif // PRESENCE_H
