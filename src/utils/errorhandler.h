#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QList>
#include <QMultiHash>
#include <QDomDocument>
#include "utilsexport.h"

#define EHN_DEFAULT "urn:ietf:params:xml:ns:xmpp-stanzas"

class UTILS_EXPORT ErrorHandler
{
public:
	enum ErrorType
	{
		UNKNOWNTYPE,
		CANCEL,
		WAIT,
		MODIFY,
		AUTH
	};
	enum ErrorCode
	{
		UNKNOWNCODE                   = 000,
		REDIRECT                      = 302,
		GONE                          = 302,
		BAD_REQUEST                   = 400,
		JID_MALFORMED                 = 400,
		UNEXPECTED_REQUEST            = 400,
		NOT_AUTHORIZED                = 401,
		PAYMENT_REQUIRED              = 402,
		FORBIDDEN                     = 403,
		ITEM_NOT_FOUND                = 404,
		REMOTE_SERVER_NOT_FOUND       = 404,
		RECIPIENT_UNAVAILABLE         = 404,
		NOT_ALLOWED                   = 405,
		NOT_ACCEPTABLE                = 406,
		REGISTRATION_REQUIRED         = 407,
		SUBSCRIPTION_REQUIRED         = 407,
		REQUEST_TIMEOUT               = 408,
		CONFLICT                      = 409,
		INTERNAL_SERVER_ERROR         = 500,
		RESOURCE_CONSTRAINT           = 500,
		UNDEFINED_CONDITION           = 500,
		FEATURE_NOT_IMPLEMENTED       = 501,
		REMOUTE_SERVER_ERROR          = 502,
		SERVICE_UNAVAILABLE           = 503,
		REMOTE_SERVER_TIMEOUT         = 504,
		DISCONNECTED                  = 510,
	};
	struct ErrorItem
	{
		QString condition;
		ErrorType type;
		int code;
		QString meaning;
	};
public:
	ErrorHandler();
	ErrorHandler(int ACode, const QString &ANsURI = EHN_DEFAULT);
	ErrorHandler(const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
	ErrorHandler(const QString &ACondition, int ACode, const QString &ANsURI = EHN_DEFAULT);
	ErrorHandler(const QDomElement &AElem, const QString &ANsURI = EHN_DEFAULT);
	~ErrorHandler();
	ErrorType type() const { return FType; }
	int code() const { return FCode; }
	QString condition() const { return FCondition; }
	QString meaning() const { return FMeaning; }
	QString text() const { return FText; }
	QString message() const;
	QString context() const { return FContext; }
	ErrorHandler &setContext(const QString &AContext) { FContext = AContext; return *this; }
	ErrorHandler &parseElement(const QDomElement &AErrElem, const QString &ANsURI = EHN_DEFAULT);
public:
	static void addErrorItem(const QString &ACondition, ErrorType AType, int ACode,
	                         const QString &AMeaning, const QString &ANsURI = EHN_DEFAULT);
	static ErrorItem *itemByCode(int &ACode, const QString &ANsURI = EHN_DEFAULT);
	static ErrorItem *itemByCondition(const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
	static ErrorItem *itemByCodeCondition(int &ACode, const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
	static ErrorType typeByCode(int ACode, const QString &ANsURI = EHN_DEFAULT);
	static ErrorType typeByCondition(const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
	static QString typeToString(ErrorType AErrorType);
	static int codeByCondition(const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
	static QString coditionByCode(int ACode, const QString &ANsURI = EHN_DEFAULT);
	static QString meaningByCode(int ACode, const QString &ANsURI = EHN_DEFAULT);
	static QString meaningByCondition(const QString &ACondition, const QString &ANsURI = EHN_DEFAULT);
protected:
	static void init();
private:
	QString   FNsURI;
	ErrorType FType;
	int			  FCode;
	QString   FCondition;
	QString   FMeaning;
	QString   FText;
	QString   FContext;
private:
	static QMultiHash<QString, ErrorHandler::ErrorItem *> FItemByNS;
};

#endif // ERRORHANDLER_H
