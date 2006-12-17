#include "errorhandler.h"
#include "stanza.h"

Stanza::Stanza(const QDomElement &AElem, QObject *parent)
  : QObject(parent)
{
  FDoc.appendChild(FDoc.createElement(AElem.tagName()) = AElem);  
}

Stanza::Stanza(const QString &ATagName, QObject *parent)
  : QObject(parent)
{
  FDoc.appendChild(FDoc.createElement(ATagName));
}

Stanza::Stanza(const Stanza &AStanza)
  : QObject(AStanza.parent())
{
  operator =(AStanza);
}

Stanza::~Stanza()
{

}

QDomElement Stanza::firstElement(const QString &ATagName, 
                                 const QString ANamespace) const
{
  if (ATagName.isEmpty())
  {
    QDomNode node = FDoc.documentElement().firstChild();
    if (!ANamespace.isEmpty()) 
      while (!node.isNull() && (!node.isElement() || node.namespaceURI() != ANamespace))
        node = node.nextSibling(); 
    return node.toElement();  
  } 
  else if (ANamespace.isEmpty())
    return FDoc.documentElement().firstChildElement(ATagName);
  else
    return FDoc.documentElement().elementsByTagNameNS(ANamespace,ATagName).at(0).toElement(); 
}

QDomElement Stanza::addElement(const QString &ATagName, 
                               const QString &ANamespace)
{
  return FDoc.documentElement().appendChild(createElement(ATagName,ANamespace)).toElement();
}

QDomElement Stanza::createElement(const QString &ATagName, 
                                  const QString &ANamespace)
{
  if (ANamespace.isEmpty())
    return FDoc.createElement(ATagName);
  else
    return FDoc.createElementNS(ANamespace,ATagName);
}

QDomText Stanza::createTextNode(const QString &AData)
{
  return FDoc.createTextNode(AData); 
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
  if (code == ErrorHandler::UNKNOWN)
    code = ErrorHandler::conditionToCode(ANamespace,ACondition);
  else if (ACondition.isEmpty())
    condition = ErrorHandler::codeToCondition(ANamespace,code);
  QString type = ErrorHandler::conditionToType(ANamespace,condition);

  if (code != ErrorHandler::UNKNOWN)
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

Stanza &Stanza::operator =(const Stanza &AStanza)
{
  FDoc = AStanza.document().cloneNode(true).toDocument(); 
  return *this;
}