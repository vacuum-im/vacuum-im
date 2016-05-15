#include "webpage.h"


WebPage::WebPage(QWebEngineProfile *AProfile, QObject *AParent) : QWebEnginePage(AProfile, AParent)
{

}

bool WebPage::acceptNavigationRequest(const QUrl &AUrl, QWebEnginePage::NavigationType AType, bool AIsMainFrame)
{
	if (AIsMainFrame && AType==QWebEnginePage::NavigationTypeLinkClicked)
		emit linkClicked(AUrl);
	return false;
}
