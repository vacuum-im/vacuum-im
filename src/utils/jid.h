#ifndef JID_H
#define JID_H

#include <QDataStream>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QSharedData>
#include <QString>
#include <QStringRef>
#include "utilsexport.h"

class JidData :
	public QSharedData
{
public:
	JidData();
	JidData(const JidData &AOther);
public:
	QString FFull, FPrepFull;
	QStringRef FBare, FPrepBare;
	QStringRef FNode, FPrepNode;
	QStringRef FDomain, FPrepDomain;
	QStringRef FResource, FPrepResource;
	bool FNodeValid, FDomainValid, FResourceValid;
};

class UTILS_EXPORT Jid
{
public:
	Jid(const char *AJidStr);
	Jid(const QString &AJidStr = QString::null);
	Jid(const QString &ANode, const QString &ADomane, const QString &AResource);
	~Jid();
	bool isValid() const;
	bool isEmpty() const;
	QString node() const;
	QString pNode() const;
	QString uNode() const;
	void setNode(const QString &ANode);
	QString domain() const;
	QString pDomain() const;
	void setDomain(const QString &ADomain);
	QString resource() const;
	QString pResource() const;
	void setResource(const QString &AResource);
	QString bare() const;
	QString pBare() const;
	QString uBare() const;
	QString full() const;
	QString pFull() const;
	QString uFull() const;
	Jid& operator =(const QString &AJidStr);
	bool operator ==(const Jid &AJid) const;
	bool operator ==(const QString &AJidStr) const;
	bool operator !=(const Jid &AJid) const;
	bool operator !=(const QString &AJidStr) const;
	bool operator &&(const Jid &AJid) const;
	bool operator &&(const QString &AJidStr) const;
	bool operator <(const Jid &AJid) const;
	bool operator >(const Jid &AJid) const;
public:
	static const Jid null;
	static Jid fromUserInput(const QString &AJidStr);
	static QString escape(const QString &AUserNode);
	static QString unescape(const QString &AEscNode);
	static QString encode(const QString &AJidStr);
	static QString decode(const QString &AEncJid);
	static QString nodePrepare(const QString &ANode);
	static QString domainPrepare(const QString &ADomain);
	static QString resourcePrepare(const QString &AResource);
protected:
	Jid &parseFromString(const QString &AJidStr);
private:
	QSharedDataPointer<JidData> d;
private:
	static QHash<QString,Jid> FJidCache;
};

UTILS_EXPORT uint qHash(const Jid &AKey);
UTILS_EXPORT QDataStream &operator>>(QDataStream &AStream, Jid &AJid);
UTILS_EXPORT QDataStream &operator<<(QDataStream &AStream, const Jid &AJid);

Q_DECLARE_METATYPE(Jid);

#endif // JID_H
