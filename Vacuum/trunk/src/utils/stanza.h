#ifndef STANZA_H
#define STANZA_H

#include <QSharedData>
#include <QDomDocument>
#include "utilsexport.h"
#include "errorhandler.h"

class StanzaData :
  public QSharedData
{
public:
  StanzaData(const QString &ATagName) 
  {
    FDoc.appendChild(FDoc.createElement(ATagName));
  };
  StanzaData(const QDomElement &AElem) 
  {
    FDoc.appendChild(FDoc.createElement(AElem.tagName()) = AElem);  
  };
  StanzaData(const StanzaData &AOther) : QSharedData(AOther)
  {
    FDoc = AOther.FDoc.cloneNode(true).toDocument();
  };
  ~StanzaData() {};
public:
  QDomDocument FDoc;
};


class UTILS_EXPORT Stanza
{
public:
  Stanza(const QString &ATagName = "message");
  Stanza(const QDomElement &AElem);
  ~Stanza();

  QDomDocument document() const { return d->FDoc; } 
  QDomElement element() const { return d->FDoc.documentElement(); }

  QString attribute(const QString &AName) const {
    return d->FDoc.documentElement().attribute(AName); }
  Stanza &setAttribute(const QString &AName, const QString &AValue) {
    d->FDoc.documentElement().setAttribute(AName,AValue); return *this; }

  QString tagName() const { return d->FDoc.documentElement().tagName(); }
  Stanza &setTagName(const QString &ATagName) { 
    d->FDoc.documentElement().setTagName(ATagName); return *this; }

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

  bool isValid() const;
  bool canReplyError() const;
  Stanza replyError(const QString &ACondition, 
    const QString &ANamespace = EHN_DEFAULT,
    int ACode = ErrorHandler::UNKNOWNCODE, 
    const QString &AText = "") const;

  QString toString() const { return d->FDoc.toString(); }
  QByteArray toByteArray() const { return d->FDoc.toByteArray(); }
private:
  QSharedDataPointer<StanzaData> d;  
};

#endif // STANZA_H
