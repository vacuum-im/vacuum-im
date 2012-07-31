#include "webpage.h"

#include <QAction>

WebPage::WebPage(QObject *AParent) : QWebPage(AParent)
{
	setContentEditable(false);
	setNetworkAccessManager(NULL);
	setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}

WebPage::~WebPage()
{

}
