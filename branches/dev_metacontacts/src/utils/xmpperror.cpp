#include "xmpperror.h"

#include <QApplication>

QMap<QString, QMap<QString, QMap<QString,QString> > > XmppError::FErrors;
QMap<XmppStreamError::ErrorCondition,QString> XmppStreamError::FErrorConditions;
QMap<XmppStanzaError::ErrorType,QString> XmppStanzaError::FErrorTypes;
QMap<XmppStanzaError::ErrorCondition,QString> XmppStanzaError::FErrorConditions;
QMap<XmppStanzaError::ErrorCondition,XmppStanzaError::ErrorType> XmppStanzaError::FConditionTypes;

XmppError XmppError::null = XmppError();
XmppStreamError XmppStreamError::null = XmppStreamError();
XmppStanzaError XmppStanzaError::null = XmppStanzaError();

//*********
//XmppError
//*********
XmppError::XmppError()
{
	initialize();
	d = new XmppErrorData;
	d->FKind = XmppErrorData::Internal;
}

XmppError::XmppError(QDomElement AErrorElem, const QString &AErrorNS)
{
	d = new XmppErrorData;
	d->FKind = XmppErrorData::Internal;

	if (!AErrorElem.isNull())
	{
		QDomElement elem = AErrorElem.firstChildElement();
		while (!elem.isNull())
		{
			QString nsURI = !elem.namespaceURI().isEmpty() ? elem.namespaceURI() : elem.parentNode().namespaceURI();
			if (nsURI == AErrorNS)
			{
				if (elem.tagName() == "text")
				{
					setErrorText(elem.text(),elem.attribute("xml:lang"));
				}
				else
				{
					setCondition(elem.tagName());
					setConditionText(elem.text());
				}
			}
			else if (!nsURI.isEmpty())
			{
				setAppCondition(nsURI,elem.tagName());
			}
			elem = elem.nextSiblingElement();
		}

		if (condition().isEmpty())
			setCondition("undefined-condition");
		setErrorNs(AErrorNS);
	}
}

XmppError::XmppError(const QString &ACondition, const QString &AText, const QString &AErrorNS)
{
	d = new XmppErrorData;
	d->FKind = XmppErrorData::Internal;

	setCondition(ACondition);
	setErrorText(AText);
	setErrorNs(AErrorNS);
}

XmppError::XmppError(const XmppErrorDataPointer &AData)
{
	d = AData;
}

bool XmppError::isNull() const
{
	return d==NULL || d->FErrorNS.isEmpty() || d->FCondition.isEmpty();
}

bool XmppError::isStreamError() const
{
	return d->FKind == XmppErrorData::Stream;
}

bool XmppError::isStanzaError() const
{
	return d->FKind == XmppErrorData::Stanza;
}

bool XmppError::isInternalError() const
{
	return d->FKind == XmppErrorData::Internal;
}

XmppStreamError XmppError::toStreamError() const
{
	return isStreamError() ? XmppStreamError(d) : XmppStreamError::null;
}

XmppStanzaError XmppError::toStanzaError() const
{
	return isStanzaError() ? XmppStanzaError(d) : XmppStanzaError::null;
}

QString XmppError::errorNs() const
{
	return d->FErrorNS;
}

void XmppError::setErrorNs(const QString &AErrorNs)
{
	d->FErrorNS = AErrorNs;
}

QString XmppError::condition() const
{
	return d->FCondition;
}

void XmppError::setCondition(const QString &ACondition)
{
	d->FCondition = ACondition;
}

QString XmppError::conditionText() const
{
	return d->FConditionText;
}

void XmppError::setConditionText(const QString &AText)
{
	d->FConditionText = AText;
}

QList<QString> XmppError::errorTextLangs() const
{
	return d->FErrorText.keys();
}

QString XmppError::errorText(const QString &ALang) const
{
	QString value;
	if (!d->FErrorText.isEmpty())
	{
		value = d->FErrorText.value(ALang);
		if (value.isEmpty() && !ALang.isEmpty())
			value = d->FErrorText.value(QString::null);
		if (value.isEmpty() && ALang!="en")
			value = d->FErrorText.value("en");
		if (value.isEmpty())
			value = d->FErrorText.constBegin().value();
	}
	return value;
}

void XmppError::setErrorText(const QString &AText, const QString &ALang)
{
	if (!AText.isEmpty())
		d->FErrorText.insert(ALang,AText);
	else
		d->FErrorText.remove(ALang);
}

QList<QString> XmppError::appConditionNsList() const
{
	return d->FAppConditions.keys();
}

QString XmppError::appCondition(const QString &ANsUri) const
{
	return d->FAppConditions.value(ANsUri);
}

QString XmppError::errorString(const QString &AContext) const
{
	foreach(QString appNs, appConditionNsList())
	{
		QString errString = getErrorString(appNs,appCondition(appNs),AContext);
		if (!errString.isEmpty())
			return errString;
	}
	return getErrorString(errorNs(),condition(),AContext);
}

QString XmppError::errorMessage(const QString &AContext, const QString &ALang) const
{
	return getErrorMessage(errorString(AContext),errorText(ALang));
}

void XmppError::setAppCondition(const QString &ANsUri, const QString &ACondition)
{
	if (!ACondition.isEmpty())
		d->FAppConditions.insert(ANsUri,ACondition);
	else
		d->FAppConditions.remove(ANsUri);
}

QString XmppError::getErrorMessage(const QString &AErrorString, const QString &AErrorText)
{
	if (AErrorString.isEmpty())
		return AErrorText;
	else if (!AErrorText.isEmpty())
		return QString("%1 (%2)").arg(AErrorString,AErrorText);
	return AErrorString;
}

QString XmppError::getErrorString(const QString &ANsUri, const QString &ACondition, const QString &AContext)
{
	registerErrors();
	const QMap<QString,QString> &texts = FErrors.value(ANsUri).value(ACondition);
	return !AContext.isEmpty() && texts.contains(AContext) ? texts.value(AContext) : texts.value(QString::null,ACondition);
}

void XmppError::registerError(const QString &ANsUri, const QString &ACondition, const QString &AErrorString, const QString &AContext)
{
	if (!ANsUri.isEmpty() && !ACondition.isEmpty() && !AErrorString.isEmpty())
		FErrors[ANsUri][ACondition].insert(AContext,AErrorString);
}

void XmppError::initialize()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		XmppStreamError::initialize();
		XmppStanzaError::initialize();
	}
}

void XmppError::registerErrors()
{
	static bool registered = false;
	if (!registered)
	{
		registered = true;
		XmppStreamError::registerStreamErrors();
		XmppStanzaError::registerStanzaErrors();
	}
}

//***************
//XmppStreamError
//***************
XmppStreamError::XmppStreamError() : XmppError()
{
	d->FKind = XmppErrorData::Stream;
}

XmppStreamError::XmppStreamError(QDomElement AErrorElem) : XmppError(AErrorElem,NS_XMPP_STREAM_ERROR)
{
	d->FKind = XmppErrorData::Stream;
}

XmppStreamError::XmppStreamError(ErrorCondition ACondition, const QString &AText) : XmppError(conditionByCode(ACondition),AText,NS_XMPP_STREAM_ERROR)
{
	d->FKind = XmppErrorData::Stream;
}

XmppStreamError::XmppStreamError(const XmppErrorDataPointer &AData) : XmppError(AData)
{

}

XmppStreamError::ErrorCondition XmppStreamError::conditionCode() const
{
	return codeByCondition(condition());
}

void XmppStreamError::setCondition(ErrorCondition ACondition)
{
	XmppError::setCondition(conditionByCode(ACondition));
}

QString XmppStreamError::conditionByCode(ErrorCondition ACode)
{
	return FErrorConditions.value(ACode);
}

XmppStreamError::ErrorCondition XmppStreamError::codeByCondition(const QString &ACondition)
{
	return FErrorConditions.key(ACondition,EC_UNDEFINED_CONDITION);
}

void XmppStreamError::initialize()
{
	FErrorConditions.insert(EC_UNDEFINED_CONDITION,"undefined-condition");
	FErrorConditions.insert(EC_BAD_FORMAT,"bad-format");
	FErrorConditions.insert(EC_BAD_NAMESPACE_PREFIX,"bad-namespace-prefix");
	FErrorConditions.insert(EC_CONFLICT,"conflict");
	FErrorConditions.insert(EC_CONNECTION_TIMEOUT,"connection-timeout");
	FErrorConditions.insert(EC_HOST_GONE,"host-gone");
	FErrorConditions.insert(EC_HOST_UNKNOWN,"host-unknown");
	FErrorConditions.insert(EC_IMPROPER_ADDRESSING,"improper-addressing");
	FErrorConditions.insert(EC_INTERNAL_SERVER_ERROR,"internal-server-error");
	FErrorConditions.insert(EC_INVALID_FROM,"invalid-from");
	FErrorConditions.insert(EC_INVALID_NAMESPACE,"invalid-namespace");
	FErrorConditions.insert(EC_INVALID_XML,"invalid-xml");
	FErrorConditions.insert(EC_NOT_AUTHORIZED,"not-authorized");
	FErrorConditions.insert(EC_NOT_WELL_FORMED,"not-well-formed");
	FErrorConditions.insert(EC_POLICY_VIOLATION,"policy-violation");
	FErrorConditions.insert(EC_REMOTE_CONNECTION_FAILED,"remote-connection-failed");
	FErrorConditions.insert(EC_RESET,"reset");
	FErrorConditions.insert(EC_RESOURCE_CONSTRAINT,"resource-constraint");
	FErrorConditions.insert(EC_RESTRICTED_XML,"restricted-xml");
	FErrorConditions.insert(EC_SEE_OTHER_HOST,"see-other-host");
	FErrorConditions.insert(EC_SYSTEM_SHUTDOWN,"system-shutdown");
	FErrorConditions.insert(EC_UNSUPPORTED_ENCODING,"unsupported-encoding");
	FErrorConditions.insert(EC_UNSUPPORTED_FEATURE,"unsupported-feature");
	FErrorConditions.insert(EC_UNSUPPORTED_STANZA_TYPE,"unsupported-stanza-type");
	FErrorConditions.insert(EC_UNSUPPORTED_VERSION,"unsupported-version");
}

void XmppStreamError::registerStreamErrors()
{
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"undefined-condition",qApp->translate("XmppStreamError","Undefined error condition"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"bad-format",qApp->translate("XmppStreamError","Bad request format"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"bad-namespace-prefix",qApp->translate("XmppStreamError","Bad namespace prefix"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"conflict",qApp->translate("XmppStreamError","Conflict"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"connection-timeout",qApp->translate("XmppStreamError","Connection timeout"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"host-gone",qApp->translate("XmppStreamError","Host is not serviced"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"host-unknown",qApp->translate("XmppStreamError","Unknown host"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"improper-addressing",qApp->translate("XmppStreamError","Improper addressing"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"internal-server-error",qApp->translate("XmppStreamError","Internal server error"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"invalid-from",qApp->translate("XmppStreamError","Invalid from address"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"invalid-namespace",qApp->translate("XmppStreamError","Invalid namespace"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"invalid-xml",qApp->translate("XmppStreamError","Invalid XML"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"not-authorized",qApp->translate("XmppStreamError","Not authorized"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"not-well-formed",qApp->translate("XmppStreamError","XML not well formed"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"policy-violation",qApp->translate("XmppStreamError","Policy violation"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"remote-connection-failed",qApp->translate("XmppStreamError","Remote connection failed"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"reset",qApp->translate("XmppStreamError","Stream need to be reseted"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"resource-constraint",qApp->translate("XmppStreamError","Resource constraint"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"restricted-xml",qApp->translate("XmppStreamError","Restricted XML"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"see-other-host",qApp->translate("XmppStreamError","See other host"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"system-shutdown",qApp->translate("XmppStreamError","System shutdown"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"unsupported-encoding",qApp->translate("XmppStreamError","Unsupported encoding"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"unsupported-feature",qApp->translate("XmppStreamError","Unsupported feature"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"unsupported-stanza-type",qApp->translate("XmppStreamError","Unsupported stanza type"));
	XmppError::registerError(NS_XMPP_STREAM_ERROR,"unsupported-version",qApp->translate("XmppStreamError","Unsupported version"));
}

//***************
//XmppStanzaError
//***************
XmppStanzaError::XmppStanzaError() : XmppError()
{
	d->FKind = XmppErrorData::Stanza;
}

XmppStanzaError::XmppStanzaError(QDomElement AErrorElem) : XmppError(AErrorElem,NS_XMPP_STANZA_ERROR)
{
	d->FKind = XmppErrorData::Stanza;
	d->FType = AErrorElem.attribute("type");
	d->FErrorBy = AErrorElem.attribute("by");
}

XmppStanzaError::XmppStanzaError(const Stanza &AStanza) : XmppError()
{
	*this = XmppStanzaError(AStanza.firstElement("error"));
}

XmppStanzaError::XmppStanzaError(ErrorCondition ACondition, const QString &AText, ErrorType AType, const QString &AErrorBy) : XmppError(conditionByCode(ACondition),AText,NS_XMPP_STANZA_ERROR)
{
	d->FKind = XmppErrorData::Stanza;
	setErrorBy(AErrorBy);
	setErrorType(AType);
}

XmppStanzaError::XmppStanzaError(const XmppErrorDataPointer &AData) : XmppError(AData)
{

}

QString XmppStanzaError::errorBy() const
{
	return d->FErrorBy;
}

void XmppStanzaError::setErrorBy(const QString &AErrorBy)
{
	d->FErrorBy = AErrorBy;
}

QString XmppStanzaError::errorType() const
{
	return d->FType;
}

XmppStanzaError::ErrorType XmppStanzaError::errorTypeCode() const
{
	return codeByType(errorType());
}

void XmppStanzaError::setErrorType(ErrorType AType)
{
	d->FType = typeByCode(AType==ET_UNKNOWN ? typeByCondition(conditionCode()) : AType);
}

XmppStanzaError::ErrorCondition XmppStanzaError::conditionCode() const
{
	return codeByCondition(condition());
}

void XmppStanzaError::setCondition(ErrorCondition ACondition)
{
	XmppError::setCondition(conditionByCode(ACondition));
}

QString XmppStanzaError::typeByCode(ErrorType ACode)
{
	return FErrorTypes.value(ACode);
}

XmppStanzaError::ErrorType XmppStanzaError::codeByType(const QString &AType)
{
	return FErrorTypes.key(AType,ET_UNKNOWN);
}

XmppStanzaError::ErrorType XmppStanzaError::typeByCondition(ErrorCondition ACondition)
{
	return FConditionTypes.value(ACondition,ET_UNKNOWN);
}

QString XmppStanzaError::conditionByCode(ErrorCondition ACode)
{
	return FErrorConditions.value(ACode);
}

XmppStanzaError::ErrorCondition XmppStanzaError::codeByCondition(const QString &ACondition)
{
	return FErrorConditions.key(ACondition,EC_UNDEFINED_CONDITION);
}

void XmppStanzaError::initialize()
{
	FErrorTypes.insert(ET_AUTH,"auth");
	FErrorTypes.insert(ET_CANCEL,"cancel");
	FErrorTypes.insert(ET_CONTINUE,"continue");
	FErrorTypes.insert(ET_MODIFY,"modify");
	FErrorTypes.insert(ET_WAIT,"wait");

	FConditionTypes.insert(EC_UNDEFINED_CONDITION,ET_MODIFY);
	FConditionTypes.insert(EC_BAD_REQUEST,ET_MODIFY);
	FConditionTypes.insert(EC_CONFLICT,ET_CANCEL);
	FConditionTypes.insert(EC_FEATURE_NOT_IMPLEMENTED,ET_CANCEL);
	FConditionTypes.insert(EC_FORBIDDEN,ET_AUTH);
	FConditionTypes.insert(EC_GONE,ET_CANCEL);
	FConditionTypes.insert(EC_INTERNAL_SERVER_ERROR,ET_CANCEL);
	FConditionTypes.insert(EC_ITEM_NOT_FOUND,ET_CANCEL);
	FConditionTypes.insert(EC_JID_MALFORMED,ET_MODIFY);
	FConditionTypes.insert(EC_NOT_ACCEPTABLE,ET_MODIFY);
	FConditionTypes.insert(EC_NOT_ALLOWED,ET_CANCEL);
	FConditionTypes.insert(EC_NOT_AUTHORIZED,ET_AUTH);
	FConditionTypes.insert(EC_POLICY_VIOLATION,ET_MODIFY);
	FConditionTypes.insert(EC_RECIPIENT_UNAVAILABLE,ET_WAIT);
	FConditionTypes.insert(EC_REDIRECT,ET_MODIFY);
	FConditionTypes.insert(EC_REGISTRATION_REQUIRED,ET_AUTH);
	FConditionTypes.insert(EC_REMOTE_SERVER_NOT_FOUND,ET_CANCEL);
	FConditionTypes.insert(EC_REMOTE_SERVER_TIMEOUT,ET_WAIT);
	FConditionTypes.insert(EC_RESOURCE_CONSTRAINT,ET_WAIT);
	FConditionTypes.insert(EC_SERVICE_UNAVAILABLE,ET_CANCEL);
	FConditionTypes.insert(EC_SUBSCRIPTION_REQUIRED,ET_AUTH);
	FConditionTypes.insert(EC_UNEXPECTED_REQUEST,ET_MODIFY);

	FErrorConditions.insert(EC_UNDEFINED_CONDITION,"undefined-condition");
	FErrorConditions.insert(EC_BAD_REQUEST,"bad-request");
	FErrorConditions.insert(EC_CONFLICT,"conflict");
	FErrorConditions.insert(EC_FEATURE_NOT_IMPLEMENTED,"feature-not-implemented");
	FErrorConditions.insert(EC_FORBIDDEN,"forbidden");
	FErrorConditions.insert(EC_GONE,"gone");
	FErrorConditions.insert(EC_INTERNAL_SERVER_ERROR,"internal-server-error");
	FErrorConditions.insert(EC_ITEM_NOT_FOUND,"item-not-found");
	FErrorConditions.insert(EC_JID_MALFORMED,"jid-malformed");
	FErrorConditions.insert(EC_NOT_ACCEPTABLE,"not-acceptable");
	FErrorConditions.insert(EC_NOT_ALLOWED,"not-allowed");
	FErrorConditions.insert(EC_NOT_AUTHORIZED,"not-authorized");
	FErrorConditions.insert(EC_POLICY_VIOLATION,"policy-violation");
	FErrorConditions.insert(EC_RECIPIENT_UNAVAILABLE,"recipient-unavailable");
	FErrorConditions.insert(EC_REDIRECT,"redirect");
	FErrorConditions.insert(EC_REGISTRATION_REQUIRED,"registration-required");
	FErrorConditions.insert(EC_REMOTE_SERVER_NOT_FOUND,"remote-server-not-found");
	FErrorConditions.insert(EC_REMOTE_SERVER_TIMEOUT,"remote-server-timeout");
	FErrorConditions.insert(EC_RESOURCE_CONSTRAINT,"resource-constraint");
	FErrorConditions.insert(EC_SERVICE_UNAVAILABLE,"service-unavailable");
	FErrorConditions.insert(EC_SUBSCRIPTION_REQUIRED,"subscription-required");
	FErrorConditions.insert(EC_UNEXPECTED_REQUEST,"unexpected-request");
}

void XmppStanzaError::registerStanzaErrors()
{
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"undefined-condition",qApp->translate("XmppStreamError","Undefined error condition"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"bad-request",qApp->translate("XmppStreamError","Bad request format"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"conflict",qApp->translate("XmppStreamError","Conflict"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"feature-not-implemented",qApp->translate("XmppStreamError","Feature not implemented"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"forbidden",qApp->translate("XmppStreamError","Insufficient permissions"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"gone",qApp->translate("XmppStreamError","Recipient changed address"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"internal-server-error",qApp->translate("XmppStreamError","Internal server error"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"item-not-found",qApp->translate("XmppStreamError","Requested item not found"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"jid-malformed",qApp->translate("XmppStreamError","Malformed XMPP address"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"not-acceptable",qApp->translate("XmppStreamError","Not accepted by the recipient"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"not-allowed",qApp->translate("XmppStreamError","Not allowed by the recipient"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"not-authorized",qApp->translate("XmppStreamError","Not authorized"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"policy-violation",qApp->translate("XmppStreamError","Policy violation"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"recipient-unavailable",qApp->translate("XmppStreamError","Recipient unavailable"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"redirect",qApp->translate("XmppStreamError","Redirect to another address"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"registration-required",qApp->translate("XmppStreamError","Registration required"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"remote-server-not-found",qApp->translate("XmppStreamError","Remote server not found"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"remote-server-timeout",qApp->translate("XmppStreamError","Remote server timeout"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"resource-constraint",qApp->translate("XmppStreamError","Resource constraint"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"service-unavailable",qApp->translate("XmppStreamError","Service unavailable"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"subscription-required",qApp->translate("XmppStreamError","Subscription required"));
	XmppError::registerError(NS_XMPP_STANZA_ERROR,"unexpected-request",qApp->translate("XmppStreamError","Unexpected request"));

	XmppError::registerError(NS_XMPP_ERRORS,"resource-limit-exceeded",qApp->translate("XmppStreamError","Resource limit exceeded"));
	XmppError::registerError(NS_XMPP_ERRORS,"stanza-too-big",qApp->translate("XmppStreamError","Stanza is too big"));
	XmppError::registerError(NS_XMPP_ERRORS,"too-many-stanzas",qApp->translate("XmppStreamError","Too many stanzas"));
}
