#ifndef XMPPERROR_H
#define XMPPERROR_H

#include <QMap>
#include <QList>
#include <QString>
#include <QSharedData>
#include <QDomElement>
#include "jid.h"
#include "stanza.h"
#include "utilsexport.h"

#define NS_INTERNAL_ERROR     "urn:vacuum:internal:errors"

#define NS_XMPP_ERRORS        "urn:xmpp:errors"
#define NS_XMPP_STREAM_ERROR  "urn:ietf:params:xml:ns:xmpp-streams"
#define NS_XMPP_STANZA_ERROR  "urn:ietf:params:xml:ns:xmpp-stanzas"

class XmppStreamError;
class XmppStanzaError;

class XmppErrorData : 
	public QSharedData
{
public:
	QString FType;
	QString FErrorBy;
	QString FErrorNS;
	QString FCondition;
	QString FConditionText;
	QMap<QString,QString> FErrorText;
	QMap<QString,QString> FAppConditions;
	enum {Internal, Stream, Stanza } FKind;
};
typedef QSharedDataPointer<XmppErrorData> XmppErrorDataPointer;

class UTILS_EXPORT XmppError
{
public:
	XmppError();
	XmppError(QDomElement AErrorElem, const QString &AErrorNS);
	XmppError(const QString &ACondition, const QString &AText=QString::null, const QString &AErrorNS=NS_INTERNAL_ERROR);
	bool isNull() const;
	bool isStreamError() const;
	bool isStanzaError() const;
	bool isInternalError() const;
	XmppStreamError toStreamError() const;
	XmppStanzaError toStanzaError() const;
	QString errorNs() const;
	void setErrorNs(const QString &AErrorNs);
	QString condition() const;
	void setCondition(const QString &ACondition);
	QString conditionText() const;
	void setConditionText(const QString &AText);
	QList<QString> errorTextLangs() const;
	QString errorText(const QString &ALang = QString::null) const;
	void setErrorText(const QString &AText, const QString &ALang = QString::null);
	QList<QString> appConditionNsList() const;
	QString appCondition(const QString &ANsUri) const;
	void setAppCondition(const QString &ANsUri, const QString &ACondition);
	QString errorString(const QString &AContext = QString::null) const;
	QString errorMessage(const QString &AContext = QString::null, const QString &ALang = QString::null) const;
public:
	static XmppError null;
	static QString getErrorMessage(const QString &AErrorString, const QString &AErrorText);
	static QString getErrorString(const QString &ANsUri, const QString &ACondition, const QString &AContext = QString::null);
	static void registerError(const QString &ANsUri, const QString &ACondition, const QString &AErrorString, const QString &AContext = QString::null);
private:
	static void initialize();
	static void registerErrors();
	static QMap<QString, QMap<QString, QMap<QString,QString> > > FErrors;
protected:
	XmppError(const XmppErrorDataPointer &AData);
	XmppErrorDataPointer d;
};

class UTILS_EXPORT XmppStreamError :
	public XmppError
{
public:
	enum ErrorCondition {
		EC_UNDEFINED_CONDITION,
		EC_BAD_FORMAT,
		EC_BAD_NAMESPACE_PREFIX,
		EC_CONFLICT,
		EC_CONNECTION_TIMEOUT,
		EC_HOST_GONE,
		EC_HOST_UNKNOWN,
		EC_IMPROPER_ADDRESSING,
		EC_INTERNAL_SERVER_ERROR,
		EC_INVALID_FROM,
		EC_INVALID_NAMESPACE,
		EC_INVALID_XML,
		EC_NOT_AUTHORIZED,
		EC_NOT_WELL_FORMED,
		EC_POLICY_VIOLATION,
		EC_REMOTE_CONNECTION_FAILED,
		EC_RESET,
		EC_RESOURCE_CONSTRAINT,
		EC_RESTRICTED_XML,
		EC_SEE_OTHER_HOST,
		EC_SYSTEM_SHUTDOWN,
		EC_UNSUPPORTED_ENCODING,
		EC_UNSUPPORTED_FEATURE,
		EC_UNSUPPORTED_STANZA_TYPE,
		EC_UNSUPPORTED_VERSION
	};
public:
	XmppStreamError();
	XmppStreamError(QDomElement AErrorElem);
	XmppStreamError(ErrorCondition ACondition, const QString &AText=QString::null);
	ErrorCondition conditionCode() const;
	void setCondition(ErrorCondition ACondition);
public:
	static XmppStreamError null;
	static QString conditionByCode(ErrorCondition ACode);
	static ErrorCondition codeByCondition(const QString &ACondition);
private:
	static void initialize();
	static void registerStreamErrors();
	static QMap<ErrorCondition,QString> FErrorConditions;
private:
	friend class XmppError;
	XmppStreamError(const XmppErrorDataPointer &AData);
};

class UTILS_EXPORT XmppStanzaError :
	public XmppError
{
public:
	enum ErrorType {
		ET_UNKNOWN,
		ET_AUTH,
		ET_CANCEL,
		ET_CONTINUE,
		ET_MODIFY,
		ET_WAIT
	};
	enum ErrorCondition {
		EC_UNDEFINED_CONDITION,         // type=modify
		EC_BAD_REQUEST,                 // type=modify
		EC_CONFLICT,                    // type=cancel
		EC_FEATURE_NOT_IMPLEMENTED,     // type=cancel or modify
		EC_FORBIDDEN,                   // type=auth
		EC_GONE,                        // type=cancel
		EC_INTERNAL_SERVER_ERROR,       // type=cancel
		EC_ITEM_NOT_FOUND,              // type=cancel
		EC_JID_MALFORMED,               // type=modify
		EC_NOT_ACCEPTABLE,              // type=modify
		EC_NOT_ALLOWED,                 // type=cancel
		EC_NOT_AUTHORIZED,              // type=auth
		EC_POLICY_VIOLATION,            // type=modify or wait
		EC_RECIPIENT_UNAVAILABLE,       // type=wait
		EC_REDIRECT,                    // type=modify
		EC_REGISTRATION_REQUIRED,       // type=auth
		EC_REMOTE_SERVER_NOT_FOUND,     // type=cancel
		EC_REMOTE_SERVER_TIMEOUT,       // type=wait
		EC_RESOURCE_CONSTRAINT,         // type=wait
		EC_SERVICE_UNAVAILABLE,         // type=cancel
		EC_SUBSCRIPTION_REQUIRED,       // type=auth
		EC_UNEXPECTED_REQUEST           // type=modify
	};
public:
	XmppStanzaError();
	XmppStanzaError(QDomElement AErrorElem);
	XmppStanzaError(const Stanza &AStanza);
	XmppStanzaError(ErrorCondition ACondition, const QString &AText=QString::null, ErrorType AType = ET_UNKNOWN, const QString &AErrorBy = QString::null);
	QString errorBy() const;
	void setErrorBy(const QString &AErrorBy);
	QString errorType() const;
	ErrorType errorTypeCode() const;
	void setErrorType(ErrorType AType);
	ErrorCondition conditionCode() const;
	void setCondition(ErrorCondition ACondition);
public:
	static XmppStanzaError null;
	static QString typeByCode(ErrorType ACode);
	static ErrorType codeByType(const QString &AType);
	static ErrorType typeByCondition(ErrorCondition ACondition);
	static QString conditionByCode(ErrorCondition ACode);
	static ErrorCondition codeByCondition(const QString &ACondition);
private:
	static void initialize();
	static void registerStanzaErrors();
	static QMap<ErrorType,QString> FErrorTypes;
	static QMap<ErrorCondition,QString> FErrorConditions;
	static QMap<ErrorCondition,ErrorType> FConditionTypes;
private:
	friend class XmppError;
	XmppStanzaError(const XmppErrorDataPointer &AData);
};

#endif // XMPPERROR_H
