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

class MessageData :
   public QSharedData
{
public:
	MessageData();
	MessageData(const Stanza &AStanza);
	MessageData(const MessageData &AOther);
protected:
	void updateDateTime();
public:
	Stanza FStanza;
	QDateTime FDateTime;
	QHash<int, QVariant> FData;
};

class UTILS_EXPORT Message
{
public:
	enum MessageType {
		AnyType     =0x00,
		Normal      =0x01,
		Chat        =0x02,
		GroupChat   =0x04,
		Headline    =0x08,
		Error       =0x10
	};
public:
	Message();
	Message(const Stanza &AStanza);
	~Message();
	void detach();
	Stanza &stanza();
	const Stanza &stanza() const;
	Message &setStanza(const Stanza &AStanza);
	QVariant data(int ARole) const;
	void setData(int ARole, const QVariant &AData);
	void setData(const QHash<int, QVariant> &AData);
	QString id() const;
	Message &setId(const QString &AId);
	QString from() const;
	Message &setFrom(const QString &AFrom);
	QString to() const;
	Message &setTo(const QString &ATo);
	QString defLang() const;
	Message &setDefLang(const QString &ALang);
	MessageType type() const;
	Message &setType(MessageType AType);
	bool isDelayed() const;
	QDateTime dateTime() const;
	Message &setDateTime(const QDateTime &ADateTime, bool ADelayed = false);
	QStringList subjectLangs() const;
	QString subject(const QString &ALang = QString::null) const;
	Message &setSubject(const QString &ASubject, const QString &ALang = QString::null);
	QStringList bodyLangs() const;
	QString body(const QString &ALang = QString::null) const;
	Message &setBody(const QString &ABody, const QString &ALang = QString::null);
	QString threadId() const;
	Message &setThreadId(const QString &AThreadId);
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
#define MESSAGE_METATYPE_ID qMetaTypeId<Message>()

#endif // MESSAGE_H
