#include "webhittestresult.h"

WebHitTestResult::WebHitTestResult()
{

}

WebHitTestResult::WebHitTestResult(const QPoint &APosition, const QVariantMap &AParams)
	: FPosition(APosition)
	, FParams(AParams)
{
}

bool WebHitTestResult::isNull() const
{
	return FParams.isEmpty();
}

QPoint WebHitTestResult::position() const
{
	return FPosition;
}

QUrl WebHitTestResult::baseUrl() const
{
	return FParams.value(QLatin1String("baseUrl")).toUrl();
}

QString WebHitTestResult::alternateText() const
{
	return FParams.value(QLatin1String("alternateText")).toString();
}

QUrl WebHitTestResult::imageUrl() const
{
	return FParams.value(QLatin1String("imageUrl")).toUrl();
}

bool WebHitTestResult::isContentEditable() const
{
	return FParams.value(QLatin1String("contentEditable")).toBool();
}

bool WebHitTestResult::isContentSelected() const
{
	return FParams.value(QLatin1String("contentSelected")).toBool();
}

QString WebHitTestResult::linkTitle() const
{
	return FParams.value(QLatin1String("linkTitle")).toString();
}

QUrl WebHitTestResult::linkUrl() const
{
	return FParams.value(QLatin1String("linkUrl")).toUrl();
}

QUrl WebHitTestResult::mediaUrl() const
{
	return FParams.value(QLatin1String("mediaUrl")).toUrl();
}

bool WebHitTestResult::mediaPaused() const
{
	return FParams.value(QLatin1String("mediaPaused")).toBool();
}

bool WebHitTestResult::mediaMuted() const
{
	return FParams.value(QLatin1String("mediaMuted")).toBool();
}

QString WebHitTestResult::tagName() const
{
	return FParams.value(QLatin1String("tagName")).toString();
}

QString WebHitTestResult::innerHtml() const
{
	return FParams.value("innerHTML").toString();
}

QString WebHitTestResult::outerHtml() const
{
	return FParams.value("outerHTML").toString();
}
