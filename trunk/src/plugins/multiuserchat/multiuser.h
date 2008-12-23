#ifndef MULTIUSER_H
#define MULTIUSER_H

#include "../../definations/multiuserdataroles.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/ipresence.h"

class MultiUser : 
  public QObject,
  public IMultiUser
{
  Q_OBJECT;
  Q_INTERFACES(IMultiUser);
public:
  MultiUser(const Jid &ARoomJid, const QString &ANickName, QObject *AParent);
  ~MultiUser();
  virtual QObject *instance() { return this; }
  virtual Jid roomJid() const { return FRoomJid; }
  virtual Jid contactJid() const { return FContactJid; }
  virtual QString nickName() const { return FNickName; }
  virtual QString role() const { return data(MUDR_ROLE).toString(); }
  virtual QString affiliation() const { return data(MUDR_AFFILIATION).toString(); }
  virtual QVariant data(int ARole) const;
  virtual void setData(int ARole, const QVariant &AValue);
public:
  void setNickName(const QString &ANickName);
signals:
  virtual void dataChanged(int ARole, const QVariant &ABefour, const QVariant &AAfter);
private:
  Jid FRoomJid;
  Jid FContactJid;
  QString FNickName;
  QHash<int, QVariant> FData;
};

#endif // MULTIUSER_H
