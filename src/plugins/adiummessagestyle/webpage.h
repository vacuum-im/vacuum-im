#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEnginePage>
#include "webhittestresult.h"

class WebPage :
	public QWebEnginePage
{
	Q_OBJECT;
public:
	WebPage(QWebEngineProfile *AProfile, QObject *AParent);
	QString requestWebHitTest(const QPoint &APosition);
signals:
	void linkClicked(const QUrl &AUrl);
	void webHitTestResult(const QString &AId, const WebHitTestResult &AResult);
private:
	bool acceptNavigationRequest(const QUrl &AUrl, NavigationType AType, bool AIsMainFrame);
};

#endif // WEBPAGE_H
