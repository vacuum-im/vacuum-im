#include "webpage.h"

WebPage::WebPage(QObject *AParent) : QWebEnginePage(AParent)
{

}

bool WebPage::acceptNavigationRequest(const QUrl &AUrl, QWebEnginePage::NavigationType AType, bool AIsMainFrame)
{
	if (AIsMainFrame && AType==QWebEnginePage::NavigationTypeLinkClicked)
		emit linkClicked(AUrl);
	return false;
}
