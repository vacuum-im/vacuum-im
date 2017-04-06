#ifndef WEBHITTESTRESULT_H
#define WEBHITTESTRESULT_H

#include <QUrl>
#include <QPoint>
#include <QVariantMap>

class WebHitTestResult
{
public:
	explicit WebHitTestResult();
	explicit WebHitTestResult(const QPoint &APosition, const QVariantMap &AParams);

	bool isNull() const;
	QPoint position() const;
	QUrl baseUrl() const;
	QString alternateText() const;
	QUrl imageUrl() const;
	bool isContentEditable() const;
	bool isContentSelected() const;
	QString linkTitle() const;
	QUrl linkUrl() const;
	QUrl mediaUrl() const;
	bool mediaPaused() const;
	bool mediaMuted() const;
	QString tagName() const;
	QString innerHtml() const;
	QString outerHtml() const;

private:
	QPoint FPosition;
	QVariantMap FParams;
};

#endif // WEBHITTESTRESULT_H
