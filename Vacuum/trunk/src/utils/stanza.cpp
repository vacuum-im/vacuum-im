#include "stanza.h"

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

QDomElement Stanza::firstElement(const QString &ATagName,
                                 const QString ANamespace) const
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

QDomElement Stanza::addElement(const QString &ATagName,
                               const QString &ANamespace)
{
  return d->FDoc.documentElement().appendChild(createElement(ATagName,ANamespace)).toElement();
}

QDomElement Stanza::createElement(const QString &ATagName,
                                  const QString &ANamespace)
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

bool Stanza::isValid() const
{
  if (element().isNull())
    return false;

  if (type() == "error" && firstElement("error").isNull())
    return false;

  return true;
}

bool Stanza::canReplyError() const
{
  if (tagName() != "iq")
    return false;

  if (type() == "error")
    return false;

  if (!firstElement("error").isNull())
    return false;

  return true;
}

Stanza Stanza::replyError(const QString &ACondition,
                          const QString &ANamespace,
                          int ACode,
                          const QString &AText) const
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

QByteArray Stanza::toByteArray() const
{
  static QByteArray befour = QString(">\n").toUtf8();
  static QByteArray after = QString(">").toUtf8();
  return d->FDoc.toByteArray(0).replace(befour,after);
}
