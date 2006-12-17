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
  Jid(const QString &jidStr = QString(), QObject *parent = 0);
  Jid(const char *ch, QObject *parent = 0);
  Jid(const Jid &AJid);
  ~Jid();

  Jid &setJid(const QString &jidStr);
  void setNode(const QString &ANode) { FNode = ANode; }
  QString node() const { return FNode; }
  void setDomane(const QString &ADomane) { FDomane = ADomane; }
  QString domane() const { return FDomane; }
  void setResource(const QString &AResource) { FResource = AResource; }
  QString resource() const { return FResource; }
  bool isValid() const;
  QString full() const { return toString(true); }
  QString bare() const { return toString(false); }
  Jid prep() const;
  bool equals(const Jid &AJid, bool withRes = true) const;
  Jid& operator =(const Jid &AJid);
  Jid& operator =(const QString &jidStr) { return setJid(jidStr); }
  bool operator ==(const Jid &AJid) const { return equals(AJid); }
  bool operator ==(const QString &jidStr) const;
  bool operator !=(const Jid &AJid) const { return !equals(AJid); }
  bool operator !=(const QString &jidStr) const;
  bool operator <(const Jid &AJid) const { return prep().full() < AJid.prep().full(); }
  bool operator >(const Jid &AJid) const { return prep().full() > AJid.prep().full(); }
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
