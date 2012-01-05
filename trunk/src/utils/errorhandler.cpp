#include "errorhandler.h"

#include <QApplication>

QMultiHash<QString, ErrorHandler::ErrorItem *> ErrorHandler::FItemByNS;

ErrorHandler::ErrorHandler()
{
	init();
	FCode = 0;
}

ErrorHandler::ErrorHandler(int ACode, const QString &ANsURI)
{
	FNsURI = ANsURI;
	FCode = ACode;
	const ErrorItem *item = itemByCode(ACode, ANsURI);
	if (item)
	{
		FType = item->type;
		FCondition = item->condition;
		FMeaning = item->meaning;
	}
}

ErrorHandler::ErrorHandler(const QString &ACondition, const QString &ANsURI)
{
	FCode = 0;
	FNsURI = ANsURI;
	const ErrorItem *item = itemByCondition(ACondition,ANsURI);
	if (item)
	{
		FType = item->type;
		FCode = item->code;
		FMeaning = item->meaning;
	}
}

ErrorHandler::ErrorHandler(const QString &ACondition, int ACode, const QString &ANsURI)
{
	FNsURI = ANsURI;
	FCode = ACode;
	FCondition = ACondition;
	ErrorItem *item = itemByCodeCondition(ACode,ACondition,ANsURI);
	if (item)
	{
		FType = item->type;
		FMeaning = item->meaning;
	}
}

ErrorHandler::ErrorHandler(const QDomElement &AElem, const QString &ANsURI)
{
	init();
	parseElement(AElem,ANsURI);
}

ErrorHandler::~ErrorHandler()
{

}

ErrorHandler::ErrorType ErrorHandler::type() const
{
	return FType;
}

int ErrorHandler::code() const
{
	return FCode;
}

QString ErrorHandler::condition() const
{
	return FCondition;
}

QString ErrorHandler::namespaceURI() const
{
	return FNsURI;
}

QString ErrorHandler::meaning() const
{
	return FMeaning;
}

QString ErrorHandler::text() const
{
	return FText;
}

QString ErrorHandler::message() const
{
	QString msg;

	if (!FContext.isEmpty())
		msg +=FContext+"\n\n";

	msg += FText.isEmpty() ? (FMeaning.isEmpty() ? FCondition : FMeaning) : FText;

	return msg;
}

QString ErrorHandler::context() const
{
	return FContext;
}

ErrorHandler &ErrorHandler::setContext(const QString &AContext)
{
	FContext = AContext; 
	return *this;
}

ErrorHandler &ErrorHandler::parseElement(const QDomElement &AErrElem, const QString &ANsURI)
{
	FCode = 0;
	FNsURI = ANsURI;
	FCondition.clear();
	FMeaning.clear();
	FText.clear();

	if (AErrElem.isNull())
		return *this;

	QDomElement elem = AErrElem.firstChildElement("error");
	if (elem.isNull())
		elem = AErrElem;
	else
		FText = elem.text();

	ErrorItem *item = NULL;
	FCode = elem.attribute("code","0").toInt();

	QString type = elem.attribute("type");
	if (type == "cancel")
		FType = CANCEL;
	else if (type == "wait")
		FType = WAIT;
	else if (type == "modify")
		FType = MODIFY;
	else if (type == "auth")
		FType = AUTH;
	else
		FType = UNKNOWNTYPE;

	elem = elem.firstChildElement();
	while (!elem.isNull() && item == NULL)
	{
		if (elem.tagName() != "text")
		{
			bool defaultNS = false;
			item = itemByCondition(ANsURI,elem.tagName());
			if (item == NULL)
			{
				item = itemByCondition(EHN_DEFAULT,elem.tagName());
				defaultNS = true;
			}
			if (item != NULL)
			{
				FCondition = item->condition;
				if (FCode == 0)
					FCode = item->code;
				if (FType == UNKNOWNTYPE)
					FType = item->type;
				FMeaning = item->meaning;
				if (defaultNS)
					item = 0;
			}
			else if (FCondition.isEmpty())
				FCondition = elem.tagName();
		}
		else
			FText = elem.text();

		elem = elem.nextSiblingElement();
	}

	if (FCode == 0 && !FCondition.isEmpty())
		FCode = codeByCondition(FCondition,ANsURI);

	if (FCondition.isEmpty() && FCode != 0)
		FCondition = conditionByCode(FCode,ANsURI);

	if (FType == UNKNOWNTYPE && FCode != 0)
		FType = typeByCode(FCode,ANsURI);
	if (FType == UNKNOWNTYPE && !FCondition.isEmpty())
		FType = typeByCondition(FCondition,ANsURI);

	if (FMeaning.isEmpty() && !FCondition.isEmpty())
		FMeaning = meaningByCondition(FCondition,ANsURI);
	if (FMeaning.isEmpty() && FCode != 0)
		FMeaning = meaningByCode(FCode,ANsURI);
	if (FMeaning.isEmpty() && !FCondition.isEmpty())
		FMeaning = meaningByCondition(FCondition,EHN_DEFAULT);

	if (FCondition.isEmpty() && FMeaning.isEmpty() && FText.isEmpty() && FCode == 0)
		FMeaning = qApp->translate("ErrorHandler", "Unknown Error");

	return *this;
}

//Static members
void ErrorHandler::addErrorItem(const QString &ACondition, ErrorType AType, int ACode, const QString &AMeaning, const QString &ANsURI)
{
	init();
	ErrorItem *item = itemByCondition(ACondition,ANsURI);
	if (item == NULL)
	{
		item = new ErrorItem;
		item->code = ACode;
		item->condition = ACondition;
		item->type = AType;
		item->meaning = AMeaning;
		FItemByNS.insert(ANsURI,item);
	}
}

ErrorHandler::ErrorItem *ErrorHandler::itemByCode(int &ACode, const QString &ANsURI)
{
	init();
	QList<ErrorItem *> items = FItemByNS.values(ANsURI);
	foreach(ErrorItem *item, items)
		if (item->code == ACode)
			return item;
	return NULL;
}

ErrorHandler::ErrorItem *ErrorHandler::itemByCondition(const QString &ACondition, const QString &ANsURI)
{
	init();
	QList<ErrorItem *> items = FItemByNS.values(ANsURI);
	foreach(ErrorItem *item, items)
		if (item->condition == ACondition)
			return item;
	return NULL;
}

ErrorHandler::ErrorItem *ErrorHandler::itemByCodeCondition(int &ACode, const QString &ACondition, const QString &ANsURI)
{
	init();
	QList<ErrorItem *> items = FItemByNS.values(ANsURI);
	foreach(ErrorItem *item, items)
		if (item->condition == ACondition && item->code == ACode)
			return item;
	return NULL;
}

ErrorHandler::ErrorType ErrorHandler::typeByCode(int ACode, const QString &ANsURI)
{
	ErrorItem *item = itemByCode(ACode,ANsURI);
	return item != NULL ? item->type : UNKNOWNTYPE;
}

ErrorHandler::ErrorType ErrorHandler::typeByCondition(const QString &ACondition, const QString &ANsURI)
{
	ErrorItem *item = itemByCondition(ACondition,ANsURI);
	return item != NULL ? item->type : UNKNOWNTYPE;
}

QString ErrorHandler::typeToString(ErrorType AErrorType)
{
	switch (AErrorType)
	{
	case CANCEL:
		return "cancel";
	case WAIT:
		return "wait";
	case MODIFY:
		return "modify";
	case AUTH:
		return "auth";
	default:
		return "unknown";
	}
}

int ErrorHandler::codeByCondition(const QString &ACondition, const QString &ANsURI)
{
	ErrorItem *item = itemByCondition(ACondition,ANsURI);
	return item != NULL ? item->code : UNKNOWNCODE;
}

QString ErrorHandler::conditionByCode(int ACode, const QString &ANsURI)
{
	ErrorItem *item = itemByCode(ACode,ANsURI);
	return item != NULL ? item->condition : QString::null;
}

QString ErrorHandler::meaningByCode(int ACode, const QString &ANsURI)
{
	ErrorItem *item = itemByCode(ACode,ANsURI);
	return item != NULL ? item->meaning : QString::null;
}

QString ErrorHandler::meaningByCondition(const QString &ACondition, const QString &ANsURI)
{
	ErrorItem *item = itemByCondition(ACondition,ANsURI);
	return item != NULL ? item->meaning : QString::null;
}

void ErrorHandler::init()
{
	static bool inited = false;
	if (!inited)
	{
		inited = true;
		addErrorItem("redirect",                MODIFY, REDIRECT,                qApp->translate("ErrorHandler", "Redirect"));
		addErrorItem("gone",                    MODIFY, GONE,                    qApp->translate("ErrorHandler", "Redirect"));
		addErrorItem("bad-request",             MODIFY, BAD_REQUEST,             qApp->translate("ErrorHandler", "Bad Request"));
		addErrorItem("unexpected-request",      WAIT,   UNEXPECTED_REQUEST,      qApp->translate("ErrorHandler", "Unexpected Request"));
		addErrorItem("jid-malformed",           MODIFY, JID_MALFORMED,           qApp->translate("ErrorHandler", "Jid Malformed"));
		addErrorItem("not-authorized",          AUTH,   NOT_AUTHORIZED,          qApp->translate("ErrorHandler", "Not Authorized"));
		addErrorItem("payment-required",        AUTH,   PAYMENT_REQUIRED,        qApp->translate("ErrorHandler", "Payment Required"));
		addErrorItem("forbidden",               AUTH,   FORBIDDEN,               qApp->translate("ErrorHandler", "Forbidden"));
		addErrorItem("item-not-found",          CANCEL, ITEM_NOT_FOUND,          qApp->translate("ErrorHandler", "Not Found"));
		addErrorItem("recipient-unavailable",   WAIT,   RECIPIENT_UNAVAILABLE,   qApp->translate("ErrorHandler", "Recipient Unavailable"));
		addErrorItem("remote-server-not-found", CANCEL, REMOTE_SERVER_NOT_FOUND, qApp->translate("ErrorHandler", "Remote Server Not Found"));
		addErrorItem("not-allowed",             CANCEL, NOT_ALLOWED,             qApp->translate("ErrorHandler", "Not Allowed"));
		addErrorItem("not-acceptable",          MODIFY, NOT_ACCEPTABLE,          qApp->translate("ErrorHandler", "Not Acceptable"));
		addErrorItem("registration-required",   AUTH,   REGISTRATION_REQUIRED,   qApp->translate("ErrorHandler", "Registration Required"));
		addErrorItem("subscription-required",   AUTH,   SUBSCRIPTION_REQUIRED,   qApp->translate("ErrorHandler", "Subscription Required"));
		addErrorItem("request-timeout",         WAIT,   REQUEST_TIMEOUT,         qApp->translate("ErrorHandler", "Request Timeout"));
		addErrorItem("conflict",                CANCEL, CONFLICT,                qApp->translate("ErrorHandler", "Conflict"));
		addErrorItem("internal-server-error",   WAIT,   INTERNAL_SERVER_ERROR,   qApp->translate("ErrorHandler", "Internal Server Error"));
		addErrorItem("resource-constraint",     WAIT,   RESOURCE_CONSTRAINT,     qApp->translate("ErrorHandler", "Resource Constraint"));
		addErrorItem("undefined-condition",     CANCEL, UNDEFINED_CONDITION,     qApp->translate("ErrorHandler", "Undefined Condition"));
		addErrorItem("feature-not-implemented", CANCEL, FEATURE_NOT_IMPLEMENTED, qApp->translate("ErrorHandler", "Not Implemented"));
		addErrorItem("remote-server-error",     CANCEL, REMOUTE_SERVER_ERROR,    qApp->translate("ErrorHandler", "Remote Server Error"));
		addErrorItem("service-unavailable",     CANCEL, SERVICE_UNAVAILABLE,     qApp->translate("ErrorHandler", "Service Unavailable"));
		addErrorItem("remote-server-timeout",   WAIT,   REMOTE_SERVER_TIMEOUT,   qApp->translate("ErrorHandler", "Remote Server timeout"));
		addErrorItem("disconnected",            CANCEL, DISCONNECTED,            qApp->translate("ErrorHandler", "Disconnected"));
	}
}
