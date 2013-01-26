#include "xmpperror.h"

#include <QApplication>

QMap<QString, QMap<QString, QMap<QString,QString> > > XmppError::FErrorStrings;
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
	d = new XmppErrorData;
}

XmppError::XmppError(QDomElement AErrorElem, const QString &ADefinedNS)
{
	d = new XmppErrorData;
	if (!AErrorElem.isNull())
	{
		QDomElement elem = AErrorElem.firstChildElement();
		while (!elem.isNull())
		{
			QString nsURI = !elem.namespaceURI().isEmpty() ? elem.namespaceURI() : elem.parentNode().namespaceURI();
			if (nsURI == ADefinedNS)
			{
				if (elem.tagName() == "text")
				{
					d->FText.insert(elem.attribute("xml:lang"),elem.text());
				}
				else
				{
					d->FCondition = elem.tagName();
					d->FConditionText = elem.text();
				}
			}
			else if (!nsURI.isEmpty())
			{
				d->FAppConditions.insert(nsURI,elem.tagName());
			}
			elem = elem.nextSiblingElement();
		}

		if (d->FCondition.isEmpty())
			d->FCondition = "undefined-condition";
	}
}

bool XmppError::isNull() const
{
	return d==NULL || d->FCondition.isEmpty();
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
	return d->FText.keys();
}

QString XmppError::errorText(const QString &ALang) const
{
	QString value;
	if (!d->FText.isEmpty())
	{
		value = d->FText.value(ALang);
		if (value.isEmpty() && !ALang.isEmpty())
			value = d->FText.value(QString::null);
		if (value.isEmpty() && ALang!="en")
			value = d->FText.value("en");
		if (value.isEmpty())
			value = d->FText.constBegin().value();
	}
	return value;
}

void XmppError::setErrorText( const QString &AText, const QString &ALang)
{
	if (!AText.isEmpty())
		d->FText.insert(ALang,AText);
	else
		d->FText.remove(ALang);
}

QList<QString> XmppError::appConditionNsList() const
{
	return d->FAppConditions.keys();
}

QString XmppError::appCondition(const QString &ANsUri) const
{
	return d->FAppConditions.value(ANsUri);
}

void XmppError::setAppCondition(const QString &ANsUri, const QString &ACondition)
{
	if (!ACondition.isEmpty())
		d->FAppConditions.insert(ANsUri,ACondition);
	else
		d->FAppConditions.remove(ANsUri);
}

QString XmppError::errorMessage(const QString &AErrorString, const QString &AErrorText)
{
	if (AErrorString.isEmpty())
		return AErrorText;
	else if (!AErrorText.isEmpty())
		return QString("%1 (%2)").arg(AErrorString,AErrorText);
	return AErrorString;
}

QString XmppError::errorString(const QString &ANsUri, const QString &ACondition, const QString &AContext)
{
	const QMap<QString,QString> &messages = FErrorStrings.value(ANsUri).value(ACondition);
	return !AContext.isEmpty() && messages.contains(AContext) ? messages.value(AContext) : messages.value(QString::null,ACondition);
}

void XmppError::registerErrorString(const QString &ANsUri, const QString &ACondition, const QString &AMessage, const QString &AContext)
{
	if (!ANsUri.isEmpty() && !ACondition.isEmpty() && !AMessage.isEmpty())
		FErrorStrings[ANsUri][ACondition].insert(AContext,AMessage);
}


//***************
//XmppStreamError
//***************
XmppStreamError::XmppStreamError() : XmppError()
{

}

XmppStreamError::XmppStreamError(QDomElement AErrorElem) : XmppError(AErrorElem,XMPP_STREAM_ERROR_NS)
{

}

XmppStreamError::XmppStreamError(ErrorCondition ACondition) : XmppError()
{
	setCondition(ACondition);
}

bool XmppStreamError::isValid() const
{
	return !isNull() && (codeByCondition(condition())!=EC_UNDEFINED_CONDITION || !appConditionNsList().isEmpty());
}

XmppStreamError::ErrorCondition XmppStreamError::conditionCode() const
{
	return codeByCondition(condition());
}

void XmppStreamError::setCondition(ErrorCondition ACondition)
{
	XmppError::setCondition(conditionByCode(ACondition));
}

QString XmppStreamError::errorString(const QString &AContext) const
{
	initialize();
	foreach(QString appNs, appConditionNsList())
	{
		QString errString = XmppError::errorString(appNs,appCondition(appNs),AContext);
		if (!errString.isEmpty())
			return errString;
	}
	return XmppError::errorString(XMPP_STREAM_ERROR_NS,condition(),AContext);
}

QString XmppStreamError::errorMessage(const QString &AContext, const QString &ALang) const
{
	return XmppError::errorMessage(errorString(AContext),errorText(ALang));
}

QString XmppStreamError::conditionByCode(ErrorCondition ACode)
{
	initialize();
	return FErrorConditions.value(ACode);
}

XmppStreamError::ErrorCondition XmppStreamError::codeByCondition(const QString &ACondition)
{
	initialize();
	return FErrorConditions.key(ACondition,EC_UNDEFINED_CONDITION);
}

void XmppStreamError::initialize()
{
	if (FErrorConditions.isEmpty())
	{
		FErrorConditions.insert(EC_UNDEFINED_CONDITION,"undefined-condition");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"undefined-condition",qApp->translate("XmppStreamError","Undefined error condition"));

		FErrorConditions.insert(EC_BAD_FORMAT,"bad-format");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"bad-format",qApp->translate("XmppStreamError","Bad request format"));

		FErrorConditions.insert(EC_BAD_NAMESPACE_PREFIX,"bad-namespace-prefix");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"bad-namespace-prefix",qApp->translate("XmppStreamError","Bad namespace prefix"));

		FErrorConditions.insert(EC_CONFLICT,"conflict");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"conflict",qApp->translate("XmppStreamError","Conflict"));

		FErrorConditions.insert(EC_CONNECTION_TIMEOUT,"connection-timeout");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"connection-timeout",qApp->translate("XmppStreamError","Connection timeout"));

		FErrorConditions.insert(EC_HOST_GONE,"host-gone");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"host-gone",qApp->translate("XmppStreamError","Host is not serviced"));

		FErrorConditions.insert(EC_HOST_UNKNOWN,"host-unknown");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"host-unknown",qApp->translate("XmppStreamError","Unknown host"));

		FErrorConditions.insert(EC_IMPROPER_ADDRESSING,"improper-addressing");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"improper-addressing",qApp->translate("XmppStreamError","Improper addressing"));

		FErrorConditions.insert(EC_INTERNAL_SERVER_ERROR,"internal-server-error");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"internal-server-error",qApp->translate("XmppStreamError","Internal server error"));

		FErrorConditions.insert(EC_INVALID_FROM,"invalid-from");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"invalid-from",qApp->translate("XmppStreamError","Invalid from address"));

		FErrorConditions.insert(EC_INVALID_NAMESPACE,"invalid-namespace");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"invalid-namespace",qApp->translate("XmppStreamError","Invalid namespace"));

		FErrorConditions.insert(EC_INVALID_XML,"invalid-xml");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"invalid-xml",qApp->translate("XmppStreamError","Invalid XML"));

		FErrorConditions.insert(EC_NOT_AUTHORIZED,"not-authorized");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"not-authorized",qApp->translate("XmppStreamError","Not authorized"));

		FErrorConditions.insert(EC_NOT_WELL_FORMED,"not-well-formed");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"not-well-formed",qApp->translate("XmppStreamError","XML not well formed"));

		FErrorConditions.insert(EC_POLICY_VIOLATION,"policy-violation");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"policy-violation",qApp->translate("XmppStreamError","Policy violation"));

		FErrorConditions.insert(EC_REMOTE_CONNECTION_FAILED,"remote-connection-failed");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"remote-connection-failed",qApp->translate("XmppStreamError","Remote connection failed"));

		FErrorConditions.insert(EC_RESET,"reset");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"reset",qApp->translate("XmppStreamError","Stream need to be reseted"));

		FErrorConditions.insert(EC_RESOURCE_CONSTRAINT,"resource-constraint");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"resource-constraint",qApp->translate("XmppStreamError","Resource constraint"));

		FErrorConditions.insert(EC_RESTRICTED_XML,"restricted-xml");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"restricted-xml",qApp->translate("XmppStreamError","Restricted XML"));

		FErrorConditions.insert(EC_SEE_OTHER_HOST,"see-other-host");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"see-other-host",qApp->translate("XmppStreamError","See other host"));

		FErrorConditions.insert(EC_SYSTEM_SHUTDOWN,"system-shutdown");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"system-shutdown",qApp->translate("XmppStreamError","System shutdown"));

		FErrorConditions.insert(EC_UNSUPPORTED_ENCODING,"unsupported-encoding");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"unsupported-encoding",qApp->translate("XmppStreamError","Unsupported encoding"));

		FErrorConditions.insert(EC_UNSUPPORTED_FEATURE,"unsupported-feature");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"unsupported-feature",qApp->translate("XmppStreamError","Unsupported feature"));

		FErrorConditions.insert(EC_UNSUPPORTED_STANZA_TYPE,"unsupported-stanza-type");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"unsupported-stanza-type",qApp->translate("XmppStreamError","Unsupported stanza type"));

		FErrorConditions.insert(EC_UNSUPPORTED_VERSION,"unsupported-version");
		XmppError::registerErrorString(XMPP_STREAM_ERROR_NS,"unsupported-version",qApp->translate("XmppStreamError","Unsupported version"));
	}
}


//***************
//XmppStanzaError
//***************
XmppStanzaError::XmppStanzaError() : XmppError()
{
	d = new XmppStanzaErrorData;
}

XmppStanzaError::XmppStanzaError(QDomElement AErrorElem) : XmppError(AErrorElem,XMPP_STANZA_ERROR_NS)
{
	d = new XmppStanzaErrorData;
	d->FType = AErrorElem.attribute("type");
	d->FErrorBy = AErrorElem.attribute("by");
}

XmppStanzaError::XmppStanzaError(const Stanza &AStanza)
{
	*this = XmppStanzaError(AStanza.firstElement("error"));
}

XmppStanzaError::XmppStanzaError(ErrorCondition ACondition, ErrorType AType, const QString &AErrorBy) : XmppError()
{
	d = new XmppStanzaErrorData;
	setErrorBy(AErrorBy);
	setErrorType(AType);
	setCondition(ACondition);
}

bool XmppStanzaError::isValid() const
{
	return !isNull() && (codeByCondition(condition())!=EC_UNDEFINED_CONDITION || !appConditionNsList().isEmpty());
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

QString XmppStanzaError::errorString(const QString &AContext) const
{
	initialize();
	foreach(QString appNs, appConditionNsList())
	{
		QString errString = XmppError::errorString(appNs,appCondition(appNs),AContext);
		if (!errString.isEmpty())
			return errString;
	}
	return XmppError::errorString(XMPP_STANZA_ERROR_NS,condition(),AContext);
}

QString XmppStanzaError::errorMessage(const QString &AContext, const QString &ALang) const
{
	return XmppError::errorMessage(errorString(AContext),errorText(ALang));
}

QString XmppStanzaError::typeByCode(ErrorType ACode)
{
	initialize();
	return FErrorTypes.value(ACode);
}

XmppStanzaError::ErrorType XmppStanzaError::codeByType(const QString &AType)
{
	initialize();
	return FErrorTypes.key(AType,ET_UNKNOWN);
}

XmppStanzaError::ErrorType XmppStanzaError::typeByCondition(ErrorCondition ACondition)
{
	return FConditionTypes.value(ACondition,ET_UNKNOWN);
}

QString XmppStanzaError::conditionByCode(ErrorCondition ACode)
{
	initialize();
	return FErrorConditions.value(ACode);
}

XmppStanzaError::ErrorCondition XmppStanzaError::codeByCondition(const QString &ACondition)
{
	initialize();
	return FErrorConditions.key(ACondition,EC_UNDEFINED_CONDITION);
}

void XmppStanzaError::initialize()
{
	if (FErrorTypes.isEmpty())
	{
		FErrorTypes.insert(ET_AUTH,"auth");
		FErrorTypes.insert(ET_CANCEL,"cancel");
		FErrorTypes.insert(ET_CONTINUE,"continue");
		FErrorTypes.insert(ET_MODIFY,"modify");
		FErrorTypes.insert(ET_WAIT,"wait");
	}
	if (FConditionTypes.isEmpty())
	{
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
	}
	if (FErrorConditions.isEmpty())
	{
		FErrorConditions.insert(EC_UNDEFINED_CONDITION,"undefined-condition");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"undefined-condition",qApp->translate("XmppStreamError","Undefined error condition"));

		FErrorConditions.insert(EC_BAD_REQUEST,"bad-request");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"bad-request",qApp->translate("XmppStreamError","Bad request format"));

		FErrorConditions.insert(EC_CONFLICT,"conflict");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"conflict",qApp->translate("XmppStreamError","Conflict"));

		FErrorConditions.insert(EC_FEATURE_NOT_IMPLEMENTED,"feature-not-implemented");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"feature-not-implemented",qApp->translate("XmppStreamError","Feature not implemented"));

		FErrorConditions.insert(EC_FORBIDDEN,"forbidden");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"forbidden",qApp->translate("XmppStreamError","Insufficient permissions"));

		FErrorConditions.insert(EC_GONE,"gone");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"gone",qApp->translate("XmppStreamError","Recipient changed address"));

		FErrorConditions.insert(EC_INTERNAL_SERVER_ERROR,"internal-server-error");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"internal-server-error",qApp->translate("XmppStreamError","Internal server error"));

		FErrorConditions.insert(EC_ITEM_NOT_FOUND,"item-not-found");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"item-not-found",qApp->translate("XmppStreamError","Requested item not found"));

		FErrorConditions.insert(EC_JID_MALFORMED,"jid-malformed");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"jid-malformed",qApp->translate("XmppStreamError","Malformed XMPP address"));

		FErrorConditions.insert(EC_NOT_ACCEPTABLE,"not-acceptable");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"not-acceptable",qApp->translate("XmppStreamError","Not accepted by the recipient"));

		FErrorConditions.insert(EC_NOT_ALLOWED,"not-allowed");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"not-allowed",qApp->translate("XmppStreamError","Not allowed by the recipient"));

		FErrorConditions.insert(EC_NOT_AUTHORIZED,"not-authorized");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"not-authorized",qApp->translate("XmppStreamError","Not authorized"));

		FErrorConditions.insert(EC_POLICY_VIOLATION,"policy-violation");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"policy-violation",qApp->translate("XmppStreamError","Policy violation"));

		FErrorConditions.insert(EC_RECIPIENT_UNAVAILABLE,"recipient-unavailable");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"recipient-unavailable",qApp->translate("XmppStreamError","Recipient unavailable"));

		FErrorConditions.insert(EC_REDIRECT,"redirect");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"redirect",qApp->translate("XmppStreamError","Redirect to another address"));

		FErrorConditions.insert(EC_REGISTRATION_REQUIRED,"registration-required");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"registration-required",qApp->translate("XmppStreamError","Registration required"));

		FErrorConditions.insert(EC_REMOTE_SERVER_NOT_FOUND,"remote-server-not-found");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"remote-server-not-found",qApp->translate("XmppStreamError","Remote server not found"));

		FErrorConditions.insert(EC_REMOTE_SERVER_TIMEOUT,"remote-server-timeout");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"remote-server-timeout",qApp->translate("XmppStreamError","Remote server timeout"));

		FErrorConditions.insert(EC_RESOURCE_CONSTRAINT,"resource-constraint");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"resource-constraint",qApp->translate("XmppStreamError","Resource constraint"));

		FErrorConditions.insert(EC_SERVICE_UNAVAILABLE,"service-unavailable");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"service-unavailable",qApp->translate("XmppStreamError","Service unavailable"));

		FErrorConditions.insert(EC_SUBSCRIPTION_REQUIRED,"subscription-required");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"subscription-required",qApp->translate("XmppStreamError","Subscription required"));

		FErrorConditions.insert(EC_UNEXPECTED_REQUEST,"unexpected-request");
		XmppError::registerErrorString(XMPP_STANZA_ERROR_NS,"unexpected-request",qApp->translate("XmppStreamError","Unexpected request"));

		XmppError::registerErrorString(XMPP_ERRORS_NS,"resource-limit-exceeded",qApp->translate("XmppStreamError","Resource limit exceeded"));
		XmppError::registerErrorString(XMPP_ERRORS_NS,"stanza-too-big",qApp->translate("XmppStreamError","Stanza is too big"));
		XmppError::registerErrorString(XMPP_ERRORS_NS,"too-many-stanzas",qApp->translate("XmppStreamError","Too many stanzas"));
	}
}
