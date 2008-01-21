#ifndef PRESENCEITEM_H
#define PRESENCEITEM_H

#include <QObject>
#include "../../interfaces/ipresence.h"
#include "../../utils/jid.h"

class PresenceItem : 
  public QObject,
  public IPresenceItem
{
  Q_OBJECT;
  Q_INTERFACES(IPresenceItem);
  friend class Presence;
public:
  PresenceItem(const Jid &AItemJid, QObject *AParent);
  ~PresenceItem();
  //IPresenceItem
  virtual QObject *instance() { return this; }
  virtual IPresence *presence() const { return FPresence; }
  virtual const Jid &jid() const { return FItemJid; }
  virtual IPresence::Show show() const { return FShow; }
  virtual const QString &status() const { return FStatus; }
  virtual qint8 priority() const { return FPriority; }
protected:
  virtual IPresenceItem &setShow(IPresence::Show AShow) { FShow = AShow; return *this; }
  virtual IPresenceItem &setStatus(const QString &AStatus) { FStatus = AStatus; return *this; }
  virtual IPresenceItem &setPriority(qint8 APriority) { FPriority = APriority; return *this; }
private:
  IPresence *FPresence; 
private:
  Jid FItemJid;
  IPresence::Show FShow;
  QString FStatus;
  qint8 FPriority;
};

#endif // PRESENCEITEM_H
