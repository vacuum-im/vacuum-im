#ifndef IPRESENCE_H
#define IPRESENCE_H

#include <QList>
#include "../../interfaces/ixmppstreams.h"
#include "../../utils/jid.h"

class IPresenceItem;

class IPresence
{
public:
  enum Show {
    Offline,
    Online,
    Chat,
    Away,
    DoNotDistrib,
    ExtendedAway,
    Error
  };
public:
  virtual QObject *instance() =0;
  virtual const Jid &streamJid() const =0;
  virtual Show show() const =0;
  virtual const QString &status() const =0;
  virtual qint8 priority() const =0;
  virtual IPresenceItem *item(const Jid &) const =0;
  virtual QList<IPresenceItem *> items() const =0;
  virtual QList<IPresenceItem *> items(const Jid &) const =0;
public slots:
  virtual bool setPresence(Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid = Jid()) =0;
  virtual bool setShow(Show AShow, const Jid &AToJid = Jid()) =0;
  virtual bool setStatus(const QString &AStatus, const Jid &AToJid = Jid()) =0;
  virtual bool setPriority(qint8 APriority, const Jid &AToJid = Jid()) =0;
signals:
  virtual void opened() =0;
  virtual void selfPresence(Show , const QString &, qint8 , const Jid &) =0;
  virtual void presenceItem(IPresenceItem *) =0;
  virtual void closed() =0;
};

class IPresenceItem
{
public:
  virtual QObject *instance() =0;
  virtual IPresence *presence() const =0;
  virtual const Jid &jid() const =0;
  virtual IPresence::Show show() const =0;
  virtual const QString &status() const =0;
  virtual qint8 priority() const =0;
};

class IPresencePlugin
{
public:
  virtual QObject *instance() =0;
  virtual IPresence *newPresence(IXmppStream *) =0;
  virtual IPresence *getPresence(const Jid &) const =0;
  virtual void removePresence(const Jid &) =0;
signals:
  virtual void presenceAdded(IPresence *) =0;
  virtual void presenceOpened(IPresence *) =0;
  virtual void selfPresence(IPresence *,IPresence::Show , const QString &, qint8 , const Jid &) =0;
  virtual void presenceItem(IPresence *, IPresenceItem *) =0;
  virtual void presenceClosed(IPresence *) =0;
  virtual void presenceRemoved(IPresence *) =0;
};

Q_DECLARE_INTERFACE(IPresenceItem,"Vacuum.Plugin.IPresenceItem/1.0")
Q_DECLARE_INTERFACE(IPresence,"Vacuum.Plugin.IPresence/1.0")
Q_DECLARE_INTERFACE(IPresencePlugin,"Vacuum.Plugin.IPresencePlugin/1.0")

#endif  //IPRESENCE_H