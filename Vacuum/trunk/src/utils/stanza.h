#ifndef STANZA_H
#define STANZA_H

#include <QDomDocument>
#include <QObject>
#include "utilsexport.h"

class UTILS_EXPORT Stanza : 
  public QObject
{
  Q_OBJECT

public:
  Stanza(const QDomElement &AElem, QObject *parent = 0);
  Stanza(const QString &ATagName, QObject *parent = 0);
  Stanza(const Stanza &AStanza);
  ~Stanza();

  QDomDocument document() const { return FDoc; } 
  QDomElement element() const { return FDoc.documentElement(); }

  QString attribute(const QString &AName) const {
    return FDoc.documentElement().attribute(AName); }
  Stanza &setAttribute(const QString &AName, const QString &AValue) {
    FDoc.documentElement().setAttribute(AName,AValue); return *this; }

  QString tagName() const { return FDoc.documentElement().tagName(); }
  Stanza &setTagName(const QString &ATagName) { 
    FDoc.documentElement().setTagName(ATagName); return *this; }

  QString type() const { return attribute("type"); }
  Stanza &setType(const QString &AType) { setAttribute("type",AType); return *this; }

  QString id() const { return attribute("id"); }
  Stanza &setId(const QString &AId) { setAttribute("id",AId); return *this; }

  QString to() const { return attribute("to"); }
  Stanza &setTo(const QString &ATo) { setAttribute("to",ATo); return *this; }

  QString from() const { return attribute("from"); }
  Stanza &setFrom(const QString &AFrom) { setAttribute("from",AFrom); return *this; }

  QString lang() const { return attribute("xml:lang"); }
  Stanza &setLang(const QString &ALang) { setAttribute("xml:lang",ALang); return *this; }

  QDomElement firstElement(const QString &ATagName = "", const QString ANamespace = "") const;
  QDomElement addElement(const QString &ATagName, const QString &ANamespace = "");
  QDomElement createElement(const QString &ATagName, const QString &ANamespace = "");
  QDomText createTextNode(const QString &AData);

  virtual bool isValid() const;
  virtual bool canReplyError() const;
  virtual Stanza replyError(const QString &ACondition, 
    const QString &ANamespace = "urn:ietf:params:xml:ns:xmpp-stanzas",
    int ACode = -1, 
    const QString &AText = "") const;

  QString toString() const { return FDoc.toString(); }
  QByteArray toByteArray() const { return FDoc.toByteArray(); }

  virtual Stanza &operator =(const Stanza &AStanza);
private:
  QDomDocument FDoc;
};

#endif // STANZA_H
