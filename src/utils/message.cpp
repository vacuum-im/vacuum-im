#include "message.h"

MessageData::MessageData() : FStanza("message")
{
	FDateTime = QDateTime::currentDateTime();
}

MessageData::MessageData(const Stanza &AStanza)
{
	FStanza = AStanza;
	updateDateTime();
}

MessageData::MessageData(const MessageData &AOther) : QSharedData(AOther)
{
	FData = AOther.FData;
	FStanza = AOther.FStanza;
	FDateTime = AOther.FDateTime;
}

void MessageData::updateDateTime()
{
	FDateTime = QDateTime::currentDateTime();

	QDomElement delayElem = FStanza.firstElement("x","urn:xmpp:delay");
	if (delayElem.isNull())
		delayElem = FStanza.firstElement("x","jabber:x:delay");
	if (!delayElem.isNull())
	{
		DateTime dateTime(delayElem.attribute("stamp"));
		if (dateTime.isValid())
			FDateTime = dateTime.toLocal();
	}
}

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

Message &Message::setStanza(const Stanza &AStanza)
{
	d->FStanza = AStanza;
	return *this;
}

QVariant Message::data(int ARole)  const
{
	return d->FData.value(ARole);
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
	while (it != AData.constEnd())
	{
		setData(it.key(),it.value());
		it++;
	}
}

QString Message::id() const
{
	return d->FStanza.id();
}

Message &Message::setId(const QString &AId)
{
	d->FStanza.setId(AId);
	return *this;
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

QString Message::to() const
{
	return d->FStanza.to();
}

Message &Message::setTo(const QString &ATo)
{
	d->FStanza.setTo(ATo);
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
	switch (AType)
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

bool Message::isDelayed() const
{
	return !d->FStanza.firstElement("x","urn:xmpp:delay").isNull() || !d->FStanza.firstElement("x","jabber:x:delay").isNull();
}

QDateTime Message::dateTime() const
{
	return d->FDateTime;
}

Message &Message::setDateTime(const QDateTime &ADateTime, bool ADelayed)
{
	d->FDateTime = ADateTime;
	if (ADelayed)
	{
		d->FStanza.detach();
		QDomElement elem = d->FStanza.firstElement("x","urn:xmpp:delay");
		if (elem.isNull())
			QDomElement elem = d->FStanza.firstElement("x","jabber:x:delay");
		if (elem.isNull())
			elem = d->FStanza.addElement("x","urn:xmpp:delay");
		elem.setAttribute("stamp",DateTime(ADateTime).toX85UTC());
	}
	return *this;
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
	addChildByLang(d->FStanza.element(),"body",ALang,ABody);
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

QDomElement Message::findChidByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang) const
{
	QString dLang = defLang();
	QString aLang = ALang.isEmpty() ? dLang : ALang;
	QDomElement elem = AParent.firstChildElement(ATagName);
	while (!elem.isNull() && elem.attribute("xml:lang",dLang)!=aLang)
		elem = elem.nextSiblingElement(ATagName);
	return elem;
}

// Be sure that d->FStanza.detach() is called before gettings AParent element for this function
QDomElement Message::addChildByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang, const QString &AText)
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

QString getDocumentBody(const QTextDocument &doc)
{
	QRegExp body("<body.*>(.*)</body>");
	body.setMinimal(false);
	QString html = doc.toHtml();
	html = html.indexOf(body)>=0 ? body.cap(1).trimmed() : html;

	// XXX Replace <P> inserted by QTextDocument with <SPAN>
	if (html.leftRef(3).compare("<p ", Qt::CaseInsensitive) == 0 &&
		html.rightRef(4).compare("</p>", Qt::CaseInsensitive) == 0)
	{
		html.replace(1, 1, "span");
		html.replace(html.length() - 2, 1, "span");
	}

	return html;
}
