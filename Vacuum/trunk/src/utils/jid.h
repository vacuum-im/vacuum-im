#ifndef JID_H
#define JID_H

#include <QObject>
#include <QString>
#include "utilsexport.h"

class UTILS_EXPORT Jid : 
  public QObject
{
  Q_OBJECT

public:
  Jid(const QString &ANode, const QString &ADomane, const QString &AResource, QObject *parent = 0);
  Jid(const QString &AJidStr = QString(), QObject *parent = 0);
  Jid(const char *ch, QObject *parent = 0);
  Jid(const Jid &AJid);
  ~Jid();

  Jid &setJid(const QString &AJidStr);
  void setNode(const QString &ANode) { FNode = ANode; }
  QString node() const { return FNode; }
  void setDomane(const QString &ADomane) { FDomane = ADomane; }
  QString domane() const { return FDomane; }
  void setResource(const QString &AResource) { FResource = AResource; }
  QString resource() const { return FResource; }
  bool isValid() const;
  QString full() const { return toString(true); }
  QString bare() const { return toString(false); }
  QString pFull() const { return prep().full(); }
  QString pBare() const { return prep().bare(); }
  Jid prep() const;
  bool equals(const Jid &AJid, bool withRes = true) const;
  Jid& operator =(const Jid &AJid) { return setJid(AJid.full()); }
  Jid& operator =(const QString &AJidStr) { return setJid(AJidStr); }
  bool operator ==(const Jid &AJid) const { return equals(AJid); }
  bool operator ==(const QString &AJidStr) const { return equals(Jid(AJidStr)); }
  bool operator !=(const Jid &AJid) const { return !equals(AJid); }
  bool operator !=(const QString &AJidStr) const { return !equals(Jid(AJidStr)); }
  bool operator <(const Jid &AJid) const { return pFull() < AJid.pFull(); }
  bool operator >(const Jid &AJid) const { return pFull() > AJid.pFull(); }
protected:
  QString toString(bool withRes = true) const;
private:
  QString FNode;
  QString FDomane;
  QString FResource;
};

#ifdef __cplusplus
extern "C" {
#endif

  UTILS_EXPORT uint qHash(const Jid &key);

#ifdef __cplusplus
}
#endif

#endif // JID_H
