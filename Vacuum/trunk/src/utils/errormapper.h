#ifndef ERRORMAPPER_H
#define ERRORMAPPER_H

#include <QDomElement>
#include <QObject>

class ErrorMapper : 
	public QObject
{
	Q_OBJECT

public:
	static enum ErrorCode
	{
		WRONG_ERROR_CODE							=	-01,
		UNKNOWN												=	000,
		REDIRECT											=	302,
		GONE													=	302,
		BAD_REQUEST										= 400,
		JID_MALFORMED									= 400,
		UNEXPECTED_REQUEST						=	400,
		NOT_AUTHORIZED								=	401,
		PAYMENT_REQUIRED							=	402,
		FORBIDDEN											=	403,
		ITEM_NOT_FOUND								=	404,
		REMOTE_SERVER_NOT_FOUND				= 404,
		RECIPIENT_UNAVAILABLE					=	404,
		NOT_ALLOWED										=	405,
		NOT_ACCEPTABLE								=	406,
		REGISTRATION_REQUIRED					=	407,
		SUBSCRIPTION_REQUIRED					=	407,
		REQUEST_TIMEOUT								=	408,
		CONFLICT											= 409,
		INTERNAL_SERVER_ERROR					=	500,
		RESOURCE_CONSTRAINT						=	500,
		UNDEFINED_CONDITION						=	500,
		FEATURE_NOT_IMPLEMENTED 			= 501,
		REMOUTE_SERVER_ERROR					=	502,
		SERVICE_UNAVAILABLE						=	503,
		REMOTE_SERVER_TIMEOUT					=	504,
		DISCONNECTED									=	510,
	};

	ErrorMapper(const QDomElement &AErrElem, 
							const QString &AErrNS="urn:ietf:params:xml:ns:xmpp-stanzas",
							QObject *parent=0);
	ErrorMapper(const ErrorMapper &AErrorMapper);
	~ErrorMapper();

	static int conditionToCode(const QString &ACond);
	static QString conditionToType(const QString &ACond);
	static QString codeToCondition(const int ACode);
	static QString codeToMeaning(const int ACode);

	ErrorMapper &loadFromElement(const QDomElement &AErrElem, 
															 const QString &AErrNS="urn:ietf:params:xml:ns:xmpp-stanzas");

	bool isValid() const { return FCode != -1 || !FCondition.isEmpty() || !FText.isEmpty(); }
	const int code() const { return FCode; }
	const QString type() const { return FType; }
	const QString condition() const { return FCondition; }
	const QString meaning() const { return FMeaning; } 
	const QString text() const { return FText; }
	const QString message() const;
	ErrorMapper &operator=(const ErrorMapper &AErrorMapper);
private:
	int			FCode;
	QString FType;
	QString FCondition;
	QString FMeaning;
	QString FText;
};

#endif // ERRORMAPPER_H
