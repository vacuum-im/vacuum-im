#ifndef MESSAGE_H
#define MESSAGE_H

#include <QVariant>
#include <QMetaType>
#include <QDateTime>
#include <QStringList>
#include <QSharedData>
#include "utilsexport.h"
#include "stanza.h"
#include "datetime.h"

#define MESSAGE_TYPE_NORMAL      "normal"
#define MESSAGE_TYPE_CHAT        "chat"
#define MESSAGE_TYPE_GROUPCHAT   "groupchat"
#define MESSAGE_TYPE_HEADLINE    "headline"

class MessageData :
	public QSharedData
{
public:
	MessageData();
	MessageData(const Stanza &AStanza);
	MessageData(const MessageData &AOther);
	Stanza FStanza;
	QDateTime FDateTime;
	QHash<int, QVariant> FData;
};

class UTILS_EXPORT Message
{
public:
	enum MessageType {
		Normal      = 0x01,
		Chat        = 0x02,
		GroupChat   = 0x04,
		Headline    = 0x08,
		Error       = 0x10
	};
public:
	Message();
	Message(const Stanza &AStanza);
	void detach();
	Stanza &stanza();
	const Stanza &stanza() const;
	MessageType type() const;
	Message &setType(MessageType AType);
	QString id() const;
	Message &setRandomId();
	Message &setId(const QString &AId);
	Jid toJid() const;
	QString to() const;
	Message &setTo(const QString &ATo);
	Jid fromJid() const;
	QString from() const;
	Message &setFrom(const QString &AFrom);
	QString defLang() const;
	Message &setDefLang(const QString &ALang);
	QDateTime dateTime() const;
	Message &setDateTime(const QDateTime &ADateTime);
	bool isDelayed() const;
	Jid delayedFromJid() const;
	QString delayedFrom() const;
	QDateTime delayedStamp() const;
	Message &setDelayed(const QDateTime &AStamp, const Jid &AFrom);
	QStringList subjectLangs() const;
	QString subject(const QString &ALang=QString::null) const;
	Message &setSubject(const QString &ASubject, const QString &ALang=QString::null);
	QStringList bodyLangs() const;
	QString body(const QString &ALang=QString::null) const;
	Message &setBody(const QString &ABody, const QString &ALang=QString::null);
	QString threadId() const;
	Message &setThreadId(const QString &AThreadId);
	QVariant data(int ARole) const;
	void setData(int ARole, const QVariant &AData);
	void setData(const QHash<int, QVariant> &AData);
	QStringList availableLangs(const QDomElement &AParent, const QString &ATagName) const;
	QDomElement findChidByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang) const;
	QDomElement addChildByLang(const QDomElement &AParent, const QString &ATagName, const QString &ALang, const QString &AText);
	bool operator<(const Message &AOther) const;
protected:
	QDomElement setTextToElem(QDomElement &AElem, const QString &AText) const;
private:
	QSharedDataPointer<MessageData> d;
};

Q_DECLARE_METATYPE(Message);

#endif // MESSAGE_H
