#include "message.h"

#include <QUuid>

#define MESSAGE_NS_DELAY       "urn:xmpp:delay"
#define MESSAGE_NS_X_DELAY     "jabber:x:delay"

MessageData::MessageData() : FStanza(STANZA_KIND_MESSAGE,STANZA_NS_CLIENT)
{
	FDateTime = QDateTime::currentDateTime();
}

MessageData::MessageData(const Stanza &AStanza) : FStanza(AStanza)
{
	FDateTime = QDateTime::currentDateTime();

	QDomElement delayElem = FStanza.firstElement("delay",MESSAGE_NS_DELAY);
	if (delayElem.isNull())
		delayElem = FStanza.firstElement("x",MESSAGE_NS_X_DELAY);
	if (!delayElem.isNull())
	{
		DateTime dateTime(delayElem.attribute("stamp"));
		if (dateTime.isValid())
			FDateTime = dateTime.toLocal();
	}
}

MessageData::MessageData(const MessageData &AOther) : QSharedData(AOther), FStanza(AOther.FStanza)
{
	FData = AOther.FData;
	FDateTime = AOther.FDateTime;
}

Message::Message()
{
	d = new MessageData;
}

Message::Message(const Stanza &AStanza)
{
	d = new MessageData(AStanza);
}

void Message::detach()
{
	d.detach();
	d->FStanza.detach();
}

Stanza &Message::stanza()
{
	return d->FStanza;
}

const Stanza &Message::stanza() const
{
	return d->FStanza;
}

Message::MessageType Message::type() const
{
	if (d->FStanza.type() == MESSAGE_TYPE_CHAT)
		return Message::Chat;
	if (d->FStanza.type() == MESSAGE_TYPE_GROUPCHAT)
		return Message::GroupChat;
	if (d->FStanza.type() == MESSAGE_TYPE_HEADLINE)
		return Message::Headline;
	if (d->FStanza.type() == STANZA_TYPE_ERROR)
		return Message::Error;
	return Message::Normal;
}

Message &Message::setType(MessageType AType)
{
	switch (AType)
	{
	case Message::Normal:
		d->FStanza.setType(MESSAGE_TYPE_NORMAL);
		break;
	case Message::Chat:
		d->FStanza.setType(MESSAGE_TYPE_CHAT);
		break;
	case Message::GroupChat:
		d->FStanza.setType(MESSAGE_TYPE_GROUPCHAT);
		break;
	case Message::Headline:
		d->FStanza.setType(MESSAGE_TYPE_HEADLINE);
		break;
	case Message::Error:
		d->FStanza.setType(STANZA_TYPE_ERROR);
		break;
	}
	return *this;
}

QString Message::id() const
{
	return d->FStanza.id();
}

Message &Message::setRandomId()
{
	d->FStanza.setId(QUuid::createUuid().toString());
	return *this;
}

Message &Message::setId(const QString &AId)
{
	d->FStanza.setId(AId);
	return *this;
}

Jid Message::toJid() const
{
	return d->FStanza.toJid();
}

QString Message::to() const
{
	return d->FStanza.to();
}

Message &Message::setTo(const QString &ATo)
{
	d->FStanza.setTo(ATo);
	return *this;
}

Jid Message::fromJid() const
{
	return d->FStanza.fromJid();
}

QString Message::from() const
{
	return d->FStanza.from();
}

Message &Message::setFrom(const QString &AFrom)
{
	d->FStanza.setFrom(AFrom);
	return *this;
}

QString Message::defLang() const
{
	return d->FStanza.lang();
}

Message &Message::setDefLang(const QString &ALang)
{
	d->FStanza.setLang(ALang);
	return *this;
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

bool Message::isDelayed() const
{
	return !d->FStanza.firstElement("delay",MESSAGE_NS_DELAY).isNull() || !d->FStanza.firstElement("x",MESSAGE_NS_X_DELAY).isNull();
}

Jid Message::delayedFromJid() const
{
	return delayedFrom();
}

QString Message::delayedFrom() const
{
	QDomElement elem = d->FStanza.firstElement("delay",MESSAGE_NS_DELAY);
	if (elem.isNull())
		elem = d->FStanza.firstElement("x",MESSAGE_NS_X_DELAY);
	return elem.attribute("from");
}

QDateTime Message::delayedStamp() const
{
	QDomElement elem = d->FStanza.firstElement("delay",MESSAGE_NS_DELAY);
	if (elem.isNull())
		elem = d->FStanza.firstElement("x",MESSAGE_NS_X_DELAY);
	return DateTime(elem.attribute("stamp")).toLocal();
}

Message &Message::setDelayed(const QDateTime &AStamp, const Jid &AFrom)
{
	d->FStanza.detach();
	QDomElement elem = d->FStanza.firstElement("delay",MESSAGE_NS_DELAY);
	if (elem.isNull())
		elem = d->FStanza.firstElement("x",MESSAGE_NS_X_DELAY);
	if (elem.isNull())
		elem = d->FStanza.addElement("delay",MESSAGE_NS_DELAY);
	if (AFrom.isValid())
		elem.setAttribute("from",AFrom.full());
	elem.setAttribute("stamp",DateTime(AStamp).toX85UTC());
	return setDateTime(AStamp);
}

QStringList Message::subjectLangs() const
{
	return availableLangs(d->FStanza.element(),"subject");
}

QString Message::subject(const QString &ALang) const
{
	return findChidByLang(d->FStanza.element(),"subject",ALang).text();
}

Message &Message::setSubject(const QString &ASubject, const QString &ALang)
{
	d->FStanza.detach();
	addChildByLang(d->FStanza.element(),"subject",ALang,ASubject);
	return *this;
}

QStringList Message::bodyLangs() const
{
	return availableLangs(d->FStanza.element(),"body");
}

QString Message::body(const QString &ALang) const
{
	return findChidByLang(d->FStanza.element(),"body",ALang).text();
}

Message &Message::setBody(const QString &ABody, const QString &ALang)
{
	d->FStanza.detach();
	addChildByLang(d->FStanza.element(),"body",ALang,ABody,STANZA_NS_CLIENT);
	return *this;
}

QString Message::threadId() const
{
	return d->FStanza.firstElement("thread").text();
}

Message &Message::setThreadId(const QString &AThreadId)
{
	d->FStanza.detach();
	QDomElement elem = d->FStanza.firstElement("thread");
	if (!AThreadId.isEmpty())
	{
		if (elem.isNull())
			elem = d->FStanza.addElement("thread");
		setTextToElem(elem,AThreadId);
	}
	else if (!elem.isNull())
	{
		d->FStanza.element().removeChild(elem);
	}
	return *this;
}

QVariant Message::data(int ARole)  const
{
	return d->FData.value(ARole);
}

void Message::setData(int ARole, const QVariant &AData)
{
	QVariant before = data(ARole);
	if (before != AData)
	{
		if (AData.isValid())
			d->FData.insert(ARole,AData);
		else
			d->FData.remove(ARole);
	}
}

void Message::setData(const QHash<int, QVariant> &AData)
{
	for (QHash<int,QVariant>::const_iterator it = AData.constBegin(); it != AData.constEnd(); ++it)
		setData(it.key(),it.value());
}

QStringList Message::availableLangs(const QDomElement &AParent, const QString &ATagName) const
{
	QStringList langs;
	QDomElement elem = AParent.firstChildElement(ATagName);
	while (!elem.isNull())
	{
		if (elem.hasAttribute("xml:lang"))
			langs.append(elem.attribute("xml:lang"));
		else
			langs.append(defLang());
		elem = elem.nextSiblingElement(ATagName);
	}
	return langs;
}

QDomElement Message::findChidByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang, const QString &ANamespace) const
{
	QString dLang = defLang();
	QString aLang = ALang.isEmpty() ? dLang : ALang;
	QDomElement elem = AParent.firstChildElement(ATagName);
	while (!elem.isNull() && ((elem.attribute("xml:lang",dLang)!=aLang) || (!ANamespace.isEmpty() && elem.namespaceURI()!=ANamespace)))
		elem = elem.nextSiblingElement(ATagName);
	return elem;
}

// Be sure that d->FStanza.detach() is called before sending AParent element for this function
QDomElement Message::addChildByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang, const QString &AText, const QString &ANamespace)
{
	QDomElement elem = findChidByLang(AParent,ATagName,ALang,ANamespace);
	if (elem.isNull() && !AText.isEmpty())
	{
		elem = d->FStanza.addElement(ATagName,ANamespace);
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

QDomElement Message::setTextToElem(QDomElement &AElem, const QString &AText) const
{
	if (!AElem.isNull())
	{
		QDomNode node = AElem.firstChild();
		while (!node.isNull() && !node.isText())
			node = node.nextSibling();
		if (node.isNull() && !AText.isEmpty())
			AElem.appendChild(AElem.ownerDocument().createTextNode(AText));
		else if (!node.isNull() && !AText.isNull())
			node.toText().setData(AText);
		else if (!node.isNull())
			AElem.removeChild(node);
	}
	return AElem;
}
