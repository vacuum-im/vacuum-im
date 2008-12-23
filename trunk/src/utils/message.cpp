#include "message.h"

Message::Message()
{
  d = new MessageData;
}

Message::Message(const Stanza &AStanza)
{
  d = new MessageData(AStanza);
}

Message::~Message()
{

}

void Message::setData(int ARole, const QVariant &AData)
{
  QVariant befour = data(ARole);
  if (befour != AData)
  {
    if (AData.isValid())
      d->FData.insert(ARole,AData);
    else
      d->FData.remove(ARole);
  }
}

void Message::setData(const QHash<int, QVariant> &AData)
{
  QHash<int,QVariant>::const_iterator it = AData.constBegin();
  while(it != AData.constEnd())
  {
    setData(it.key(),it.value());
    it++;
  }
}

Message::MessageType Message::type() const
{
  if (d->FStanza.type() == "chat")
    return Chat;
  else if (d->FStanza.type() == "groupchat")
    return GroupChat;
  else if (d->FStanza.type() == "headline")
    return Headline;
  else if (d->FStanza.type() == "error")
    return Error;
  else
    return Normal;
}

Message &Message::setType(MessageType AType)
{
  switch(AType)
  {
  case Normal:
    d->FStanza.setType("normal");
  	break;
  case Chat:
    d->FStanza.setType("chat");
  	break;
  case GroupChat:
    d->FStanza.setType("groupchat");
    break;
  case Headline:
    d->FStanza.setType("headline");
    break;
  case Error:
    d->FStanza.setType("error");
    break;
  default:
    break;
  }
  return *this;
}

QDateTime Message::createDateTime() const
{
  return d->FCreateDateTime;
}

QDateTime Message::dateTime() const
{
  return d->FDateTime;
}

Message &Message::setDateTime(const QDateTime &ADateTime)
{
  d->FDateTime = ADateTime;
  return *this;
}

QString Message::subject(const QString &ALang) const
{
  return findChidByLang(d->FStanza.element(),"subject",ALang).text();
}

Message &Message::setSubject(const QString &ASubject, const QString &ALang)
{
  addChildByLang(d->FStanza.element(),"subject",ALang,ASubject);
  return *this;
}

QString Message::body(const QString &ALang) const
{
  return findChidByLang(d->FStanza.element(),"body",ALang).text();
}

Message &Message::setBody(const QString &ABody, const QString &ALang)
{
  addChildByLang(d->FStanza.element(),"body",ALang,ABody);
  return *this;
}

QString Message::threadId() const
{
  return d->FStanza.firstElement("thread").text();
}

Message &Message::setThreadId(const QString &AThreadId)
{
  QDomElement elem = d->FStanza.firstElement("thread");
  if (!AThreadId.isEmpty())
  {
    if (elem.isNull())
      elem = d->FStanza.addElement("thread");
    setTextToElem(elem,AThreadId);
  }
  else if (!elem.isNull())
    d->FStanza.element().removeChild(elem);
  return *this;
}

QStringList Message::availableLangs(const QDomElement &AParent, const QString &ATagName) const
{
  QStringList langs;
  QDomElement elem = AParent.firstChildElement(ATagName);
  while(!elem.isNull())
  {
    if (elem.hasAttribute("xml:lang"))
      langs.append(elem.attribute("xml:lang"));
    else
      langs.append(defLang());
    elem = elem.nextSiblingElement(ATagName);
  }
  return langs;
}

QDomElement Message::findChidByLang(const QDomElement &AParent, const QString &ATagName,
                                    const QString &ALang) const
{
  QString dLang = defLang();
  QString aLang = ALang.isEmpty() ? dLang : ALang;
  QDomElement elem = AParent.firstChildElement(ATagName);
  while(!elem.isNull() && elem.attribute("xml:lang",dLang)!=aLang)
    elem = elem.nextSiblingElement(ATagName);
  return elem;
}

QDomElement Message::addChildByLang(const QDomElement &AParent, const QString &ATagName,
                                    const QString &ALang, const QString &AText)
{
  QDomElement elem = findChidByLang(AParent,ATagName,ALang);
  if (elem.isNull() && !AText.isEmpty())
  {
    elem = d->FStanza.addElement(ATagName);
    if (!ALang.isEmpty() && ALang!=defLang())
      elem.setAttribute("xml:lang",ALang);
  }
  if (!AText.isEmpty())
    setTextToElem(elem,AText);
  else if (!elem.isNull())
    d->FStanza.element().removeChild(elem);
  return elem;
}

bool Message::operator<(const Message &AOther) const
{
  return dateTime()<AOther.dateTime();
}

QDomElement Message::setTextToElem(QDomElement &AElem, const QString &AText)
{
  if (!AElem.isNull())
  {
    QDomNode node = AElem.firstChild();
    while (!node.isNull() && !node.isText())
      node = node.nextSibling();
    if (node.isNull() && !AText.isEmpty())
      AElem.appendChild(d->FStanza.createTextNode(AText));
    else if (!node.isNull() && !AText.isNull())
      node.toText().setData(AText);
    else if (!node.isNull())
      AElem.removeChild(node);
  }
  return AElem;
}

