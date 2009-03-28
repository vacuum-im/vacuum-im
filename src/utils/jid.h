#ifndef JID_H
#define JID_H

#include <QList>
#include <QHash>
#include <QString>
#include <QMetaType>
#include <QSharedData>
#include "utilsexport.h"
#include "../thirdparty/idn/stringprep.h"

class JidData :
  public QSharedData
{
public:
  JidData();
  JidData(const JidData &AOther);
public:
  QString FNode, FEscNode, FPrepNode;
  QString FDomain, FPrepDomain;
  QString FResource, FPrepResource;
  bool FNodeValid, FDomainValid, FResourceValid;
};

class UTILS_EXPORT Jid
{
public:
  Jid(const char *AJidStr);
  Jid(const QString &AJidStr = QString::null);
  Jid(const QString &ANode, const QString &ADomane, const QString &AResource);
  ~Jid();
  bool isValid() const;
  bool isEmpty() const;
  QString node() const;
  QString hNode() const;
  QString eNode() const;
  QString pNode() const;
  void setNode(const QString &ANode);
  QString domain() const;
  QString pDomain() const;
  void setDomain(const QString &ADomain);
  QString resource() const;
  QString pResource() const;
  void setResource(const QString &AResource);
  Jid prepared() const;
  QString full() const;
  QString bare() const;
  QString hFull() const;
  QString hBare() const;
  QString eFull() const;
  QString eBare() const;
  QString pFull() const;
  QString pBare() const;
  Jid& operator =(const QString &AJidStr);
  bool operator ==(const Jid &AJid) const;
  bool operator ==(const QString &AJidStr) const;
  bool operator !=(const Jid &AJid) const;
  bool operator !=(const QString &AJidStr) const;
  bool operator &&(const Jid &AJid) const;
  bool operator &&(const QString &AJidStr) const;
  bool operator <(const Jid &AJid) const;
  bool operator >(const Jid &AJid) const;
public:
  static QString encode(const QString &AJidStr);
  static QString decode(const QString &AEncJid);
  static QString encode822(const QString &AJidStr);
  static QString decode822(const QString &AEncJid);
  static QString escape106(const QString &ANode);
  static QString unescape106(const QString &AEscNode);
  static QString stringPrepare(const Stringprep_profile *AProfile, const QString &AString);
  static QString nodePrepare(const QString &ANode);
  static QString domainPrepare(const QString &ADomain);
  static QString resourcePrepare(const QString &AResource);
protected:
  Jid &parseString(const QString &AJidStr);
  QString toString(bool AEscaped, bool APrepared, bool AFull) const;
  bool equals(const Jid &AJid, bool AFull) const;
private:
  QSharedDataPointer<JidData> d;
private:
  static QList<QChar> escChars;
  static QList<QString> escStrings;
  static QHash<QString, Jid> FCache;
};

#ifdef __cplusplus
extern "C" {
#endif

  UTILS_EXPORT uint qHash(const Jid &key);

#ifdef __cplusplus
}
#endif

Q_DECLARE_METATYPE(Jid);
#define JID_METATYPE_ID qMetaTypeId<Jid>()

#endif // JID_H
