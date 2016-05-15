#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QWebEnginePage>

class WebPage :
	public QWebEnginePage
{
	Q_OBJECT;
public:
	WebPage(QWebEngineProfile *AProfile, QObject *AParent);
signals:
	void linkClicked(const QUrl &AUrl);
private:
	bool acceptNavigationRequest(const QUrl &AUrl, NavigationType AType, bool AIsMainFrame);
};

#endif // WEBPAGE_H
