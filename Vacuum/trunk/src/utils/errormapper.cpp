#include "errormapper.h"

ErrorMapper::ErrorMapper(const QDomElement &AErrElem, 
												 const QString &AErrNS,
												 QObject *parent)
	: QObject(parent)
{
	loadFromElement(AErrElem,AErrNS);
}

ErrorMapper::ErrorMapper(const ErrorMapper &AErrorMapper)
	:	QObject(AErrorMapper.parent())
{
	operator=(AErrorMapper);
}

ErrorMapper::~ErrorMapper()
{

}

ErrorMapper &ErrorMapper::loadFromElement(const QDomElement &AErrElem,
																					const QString &AErrNS)
{
	FCode = -1;
	FType.clear();
	FCondition.clear();
	FMeaning.clear();
	FText.clear(); 
	if (AErrElem.isNull()) 
		return *this;
	const QDomNode *node = &AErrElem;
	if (node->toElement().tagName() != "error")
		node = &node->toElement().elementsByTagName("error").at(0);
	if (node->isNull()) 
		node = &AErrElem;

	FCode = node->toElement().attribute("code","-1").toInt();
	FType = node->toElement().attribute("type");
	node = &node->firstChild();
	while (!node->isNull() && (FCondition.isNull() || FText.isNull())) 
	{
		if (FText.isNull() && node->isText())
			FText = node->toText().data(); 

		if (node->toElement().namespaceURI() == AErrNS)
		{
			if (node->toElement().tagName() != "text")
				FCondition = node->toElement().tagName();
			else
				FText = node->firstChild().toText().data();   
		};
		node = &node->nextSibling(); 
	};

	if (FCode == -1 && !FCondition.isEmpty())
		FCode = ErrorMapper::conditionToCode(FCondition);
	if (FCondition.isEmpty() && FCode != -1)
		FCondition = ErrorMapper::codeToCondition(FCode);
	if (FType.isEmpty() && !FCondition.isEmpty())
		FType = ErrorMapper::conditionToType(FCondition);
	FMeaning = ErrorMapper::codeToMeaning(FCode);

	return *this;
}

const QString ErrorMapper::message() const
{
	QString msg;
	msg = FCode == -1 ? FCondition : FMeaning;
	if (!msg.isEmpty() && !FText.isEmpty())
		msg += ":\n"+FText;
	if (msg.isEmpty())
		msg = FText;
	return msg;
}

ErrorMapper &ErrorMapper::operator =(const ErrorMapper &AErrorMapper)
{
	FCode = AErrorMapper.code();
	FType = AErrorMapper.type();
	FCondition = AErrorMapper.condition();
	FMeaning = AErrorMapper.meaning();
	FText = AErrorMapper.text(); 
	return *this;
}

int ErrorMapper::conditionToCode(const QString &ACond)
{
	if (ACond == "bad-request")								return BAD_REQUEST;
	if (ACond == "conflict")									return CONFLICT;
	if (ACond == "feature-not-implemented")		return FEATURE_NOT_IMPLEMENTED;
	if (ACond == "forbidden")									return FORBIDDEN;
	if (ACond == "gone")											return GONE;
	if (ACond == "internal-server-error")			return INTERNAL_SERVER_ERROR;
	if (ACond == "item-not-found")						return ITEM_NOT_FOUND;
	if (ACond == "jid-malformed")							return JID_MALFORMED;
	if (ACond == "not-acceptable")						return NOT_ACCEPTABLE;
	if (ACond == "not-allowed")								return NOT_ALLOWED;
	if (ACond == "not-authorized")						return NOT_AUTHORIZED;
	if (ACond == "payment-required")					return PAYMENT_REQUIRED;
	if (ACond == "recipient-unavailable")			return RECIPIENT_UNAVAILABLE;
	if (ACond == "redirect")									return REDIRECT;
	if (ACond == "registration-required")			return REGISTRATION_REQUIRED;
	if (ACond == "remote-server-not-found")		return REMOTE_SERVER_NOT_FOUND;
	if (ACond == "remote-server-timeout")			return REMOTE_SERVER_TIMEOUT;
	if (ACond == "resource-constraint")				return RESOURCE_CONSTRAINT;
	if (ACond == "service-unavailable")				return SERVICE_UNAVAILABLE;
	if (ACond == "subscription-required")			return SUBSCRIPTION_REQUIRED;
	if (ACond == "undefined-condition")				return UNDEFINED_CONDITION;
	if (ACond == "unexpected-request")				return UNEXPECTED_REQUEST;
	return WRONG_ERROR_CODE;
}

QString ErrorMapper::conditionToType(const QString &ACond)
{
	if (ACond == "bad-request")								return "modify";
	if (ACond == "conflict")									return "cancel";
	if (ACond == "feature-not-implemented")		return "cancel";
	if (ACond == "forbidden")									return "auth";
	if (ACond == "gone")											return "modify";
	if (ACond == "internal-server-error")			return "wait";
	if (ACond == "item-not-found")						return "cancel";
	if (ACond == "jid-malformed")							return "modify";
	if (ACond == "not-acceptable")						return "modify";
	if (ACond == "not-allowed")								return "cancel";
	if (ACond == "not-authorized")						return "auth";
	if (ACond == "payment-required")					return "auth";
	if (ACond == "recipient-unavailable")			return "wait";
	if (ACond == "redirect")									return "modify";
	if (ACond == "registration-required")			return "auth";
	if (ACond == "remote-server-not-found")		return "cancel";
	if (ACond == "remote-server-timeout")			return "wait";
	if (ACond == "resource-constraint")				return "wait";
	if (ACond == "service-unavailable")				return "cancel";
	if (ACond == "subscription-required")			return "auth";
	if (ACond == "undefined-condition")				return QString::null;
	if (ACond == "unexpected-request")				return "wait";
	return QString::null; 
}

QString ErrorMapper::codeToCondition(const int ACode)
{
	switch (ACode)
	{
		case WRONG_ERROR_CODE:				return QString::null; 
		case UNKNOWN:									return QString::null; 
		case REDIRECT:								return "redirect";
		case BAD_REQUEST:							return "bad-request";
		case NOT_AUTHORIZED:					return "not-authorized";
		case PAYMENT_REQUIRED:				return "payment-required";
		case FORBIDDEN:								return "forbidden";
		case ITEM_NOT_FOUND:					return "item-not-found";
		case NOT_ALLOWED:							return "not-allowed";
		case NOT_ACCEPTABLE:					return "not-acceptable";
		case REGISTRATION_REQUIRED:		return "registration-required";
		case REQUEST_TIMEOUT:					return "remote-server-timeout";
		case CONFLICT:								return "conflict";
		case INTERNAL_SERVER_ERROR:		return "internal-server-error";
		case FEATURE_NOT_IMPLEMENTED: return "feature-not-implemented";
		case REMOUTE_SERVER_ERROR:		return "service-unavailable";
		case SERVICE_UNAVAILABLE:			return "service-unavailable";
		case REMOTE_SERVER_TIMEOUT:		return "remote-server-timeout";
		case DISCONNECTED:						return "service-unavailable";
		default:											return "undefined-condition";
	};
}

QString ErrorMapper::codeToMeaning(const int ACode)
{
	switch (ACode)
	{
		case UNKNOWN:									return tr("Unknown error"); 
		case REDIRECT:								return tr("Redirect");
		case BAD_REQUEST:							return tr("Bad Request");
		case NOT_AUTHORIZED:					return tr("Not Authorized");
		case PAYMENT_REQUIRED:				return tr("Payment Required");
		case FORBIDDEN:								return tr("Forbidden");
		case ITEM_NOT_FOUND:					return tr("Not Found");
		case NOT_ALLOWED:							return tr("Not Allowed");
		case NOT_ACCEPTABLE:					return tr("Not Acceptable");
		case REGISTRATION_REQUIRED:		return tr("Registration Required");
		case REQUEST_TIMEOUT:					return tr("Request Timeout");
		case CONFLICT:								return tr("Conflict");
		case INTERNAL_SERVER_ERROR:		return tr("Internal Server Error");
		case FEATURE_NOT_IMPLEMENTED: return tr("Not Implemented");
		case REMOUTE_SERVER_ERROR:		return tr("Remoute Server Error");
		case SERVICE_UNAVAILABLE:			return tr("Service Unavailable");
		case REMOTE_SERVER_TIMEOUT:		return tr("Remote Server timeout");
		case DISCONNECTED:						return tr("Disconnected");
		default:											return tr("Wrong Error Code");
	};
}