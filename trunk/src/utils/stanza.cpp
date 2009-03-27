#include "stanza.h"

StanzaData::StanzaData(const QString &ATagName)
{
  FDoc.appendChild(FDoc.createElement(ATagName));
}

StanzaData::StanzaData(const QDomElement &AElem)
{
  FDoc.appendChild(FDoc.importNode(AElem,true));
}

StanzaData::StanzaData(const StanzaData &AOther) : QSharedData(AOther)
{
  FDoc = AOther.FDoc.cloneNode(true).toDocument();
}


Stanza::Stanza(const QString &ATagName)
{
  d = new StanzaData(ATagName);
}

Stanza::Stanza(const QDomElement &AElem)
{
  d = new StanzaData(AElem);
}

Stanza::~Stanza()
{

}

bool Stanza::isValid() const
{
  if (element().isNull())
    return false;

  if (type() == "error" && firstElement("error").isNull())
    return false;

  return true;
}

QDomDocument Stanza::document() const
{
  return d->FDoc;
}

QDomElement Stanza::element() const
{
  return d->FDoc.documentElement();
}

QString Stanza::attribute(const QString &AName) const
{
  return d->FDoc.documentElement().attribute(AName);
}

Stanza &Stanza::setAttribute(const QString &AName, const QString &AValue)
{
  d->FDoc.documentElement().setAttribute(AName,AValue); 
  return *this;
}

QString Stanza::tagName() const
{
  return d->FDoc.documentElement().tagName();
}

Stanza &Stanza::setTagName(const QString &ATagName)
{
  d->FDoc.documentElement().setTagName(ATagName); 
  return *this;
}

QString Stanza::type() const
{
  return attribute("type");
}

Stanza &Stanza::setType(const QString &AType)
{
  setAttribute("type",AType); 
  return *this;
}

QString Stanza::id() const
{
  return attribute("id");
}

Stanza &Stanza::setId(const QString &AId)
{
  setAttribute("id",AId); 
  return *this;
}

QString Stanza::to() const
{
  return attribute("to");
}

Stanza &Stanza::setTo(const QString &ATo)
{
  setAttribute("to",ATo); 
  return *this;
}

QString Stanza::from() const
{
  return attribute("from");
}

Stanza &Stanza::setFrom(const QString &AFrom)
{
  setAttribute("from",AFrom); 
  return *this;
}

QString Stanza::lang() const
{
  return attribute("xml:lang");
}

Stanza &Stanza::setLang(const QString &ALang)
{
  setAttribute("xml:lang",ALang); 
  return *this;
}

bool Stanza::canReplyError() const
{
  if (tagName() != "iq")
    return false;

  if (type()!="set" && type()!="get")
    return false;

  if (!firstElement("error").isNull())
    return false;

  return true;
}

Stanza Stanza::replyError(const QString &ACondition, const QString &ANamespace, int ACode, const QString &AText) const
{
  Stanza reply(*this);
  reply.setType("error").setTo(from());
  reply.element().removeAttribute("from");
  QDomElement errElem = reply.addElement("error");
  int code = ACode;
  QString condition = ACondition;
  if (code == ErrorHandler::UNKNOWNCODE)
    code = ErrorHandler::codeByCondition(ACondition, ANamespace);
  else if (ACondition.isEmpty())
    condition = ErrorHandler::coditionByCode(code,ANamespace);
  QString type = ErrorHandler::typeToString(ErrorHandler::typeByCondition(condition,ANamespace));

  if (code != ErrorHandler::UNKNOWNCODE)
    errElem.setAttribute("code",code);
  if (!type.isEmpty())
    errElem.setAttribute("type",type);
  if (!condition.isEmpty())
    errElem.appendChild(reply.createElement(condition,ANamespace));
  if (!AText.isEmpty())
    errElem.appendChild(reply.createElement("text",ANamespace))
    .appendChild(reply.createTextNode(AText));

  return reply;
}

QDomElement Stanza::firstElement(const QString &ATagName, const QString ANamespace) const
{
  if (ATagName.isEmpty())
  {
    QDomNode node = d->FDoc.documentElement().firstChild();
    if (!ANamespace.isEmpty())
      while (!node.isNull() && (!node.isElement() || node.namespaceURI() != ANamespace))
        node = node.nextSibling();
    return node.toElement();
  }
  else if (ANamespace.isEmpty())
    return d->FDoc.documentElement().firstChildElement(ATagName);
  else
    return d->FDoc.documentElement().elementsByTagNameNS(ANamespace,ATagName).at(0).toElement();
}

QDomElement Stanza::addElement(const QString &ATagName, const QString &ANamespace)
{
  return d->FDoc.documentElement().appendChild(createElement(ATagName,ANamespace)).toElement();
}

QDomElement Stanza::createElement(const QString &ATagName, const QString &ANamespace)
{
  if (ANamespace.isEmpty())
    return d->FDoc.createElement(ATagName);
  else
    return d->FDoc.createElementNS(ANamespace,ATagName);
}

QDomText Stanza::createTextNode(const QString &AData)
{
  return d->FDoc.createTextNode(AData);
}

QString Stanza::toString(int AIndent) const
{
  return d->FDoc.toString(AIndent);
}

QByteArray Stanza::toByteArray() const
{
  static QByteArray befour = QString(">\n").toUtf8();
  static QByteArray after = QString(">").toUtf8();
  return d->FDoc.toByteArray(0).replace(befour,after);
}

