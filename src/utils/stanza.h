#ifndef STANZA_H
#define STANZA_H

#include <QMetaType>
#include <QSharedData>
#include <QDomDocument>
#include "jid.h"
#include "utilsexport.h"

#define STANZA_NS_CLIENT      "jabber:client"

#define STANZA_KIND_IQ        "iq"
#define STANZA_KIND_MESSAGE   "message"
#define STANZA_KIND_PRESENCE  "presence"

#define STANZA_TYPE_GET       "get"
#define STANZA_TYPE_SET       "set"
#define STANZA_TYPE_RESULT    "result"
#define STANZA_TYPE_ERROR     "error"

class StanzaData :
	public QSharedData
{
public:
	StanzaData(const StanzaData &AOther);
	StanzaData(const QDomElement &AElem);
	StanzaData(const QString &AKind, const QString &ANamespace);
public:
	QDomDocument FDoc;
};

class UTILS_EXPORT Stanza
{
public:
	Stanza(const QDomElement &AElem);
	Stanza(const QString &AKind=STANZA_KIND_MESSAGE, const QString &ANamespace=STANZA_NS_CLIENT);
	void detach();
	bool isNull() const;
	bool isResult() const;
	bool isError() const;
	bool isFromServer() const;
	QDomDocument document() const;
	QDomElement element() const;
	QString namespaceURI() const;
	QString kind() const;
	Stanza &setKind(const QString &AName);
	QString type() const;
	Stanza &setType(const QString &AType);
	QString id() const;
	Stanza &setUniqueId();
	Stanza &setId(const QString &AId);
	Jid toJid() const;
	QString to() const;
	Stanza &setTo(const QString &ATo);
	Jid fromJid() const;
	QString from() const;
	Stanza &setFrom(const QString &AFrom);
	QString lang() const;
	Stanza &setLang(const QString &ALang);
	bool hasAttribute(const QString &AName) const;
	QString attribute(const QString &AName, const QString &ADefault=QString()) const;
	Stanza &setAttribute(const QString &AName, const QString &AValue);
	QDomElement firstElement(const QString &ATagName=QString(), const QString &ANamespace=QString()) const;
	QDomElement addElement(const QString &AName, const QString &ANamespace=QString());
	QDomElement createElement(const QString &AName, const QString &ANamespace=QString());
	QDomText createTextNode(const QString &AData);
	QString toString(int AIndent = 1) const;
	QByteArray toByteArray() const;
public:
	static bool isValidXmlChar(quint32 ACode);
	static QString replaceInvalidXmlChars(QString &AXml, const QChar &AWithChar='?');
	static QDomElement findElement(const QDomElement &AParent, const QString &ATagName=QString(), const QString &ANamespace=QString());
private:
	QSharedDataPointer<StanzaData> d;
};

Q_DECLARE_METATYPE(Stanza);

#endif // STANZA_H
