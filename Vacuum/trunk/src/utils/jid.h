#ifndef JID_H
#define JID_H

#include <QList>
#include <QHash>
#include <QString>
#include <QMetaType>
#include <QSharedData>
#include <QTextDocument>
#include "utilsexport.h"
#include "../thirdparty/libidn/stringprep.h"

class JidData :
  public QSharedData
{
public:
  JidData() {
    FNodeValid = true;
    FDomainValid = false;
    FResourceValid = true;
  };
  JidData(const JidData &AOther) : QSharedData(AOther)
  {
    FNode = AOther.FNode;
    FEscNode = AOther.FEscNode;
    FPrepNode = AOther.FPrepNode;
    FDomain = AOther.FDomain;
    FPrepDomain = AOther.FPrepDomain;
    FResource = AOther.FResource;
    FPrepResource = AOther.FPrepResource;
    FNodeValid = AOther.FNodeValid;
    FDomainValid = AOther.FDomainValid;
    FResourceValid = AOther.FResourceValid;
  };
  ~JidData() {};
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
  Jid(const QString &AJidStr = QString());
  Jid(const QString &ANode, const QString &ADomane, const QString &AResource);
  ~Jid();

  bool isValid() const;
  bool isEmpty() const;

  QString node() const { return d->FNode; }
  QString hNode() const { return Qt::escape(d->FEscNode); }
  QString eNode() const { return d->FEscNode; }
  QString pNode() const { return d->FPrepNode; }
  void setNode(const QString &ANode);
  QString domain() const { return d->FDomain; }
  QString pDomain() const { return d->FPrepDomain; }
  void setDomain(const QString &ADomain);
  QString resource() const { return d->FResource; }
  QString pResource() const { return d->FPrepResource; }
  void setResource(const QString &AResource);

  Jid prepared() const;
  QString full() const { return toString(false,false,true); }
  QString bare() const { return toString(false,false,false); }
  QString hFull() const { return Qt::escape(toString(false,false,true)); }
  QString hBare() const { return Qt::escape(toString(false,false,false)); }
  QString eFull() const { return toString(true,false,true); }
  QString eBare() const { return toString(true,false,false); }
  QString pFull() const { return toString(false,true,true); }
  QString pBare() const { return toString(false,true,false); }

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
