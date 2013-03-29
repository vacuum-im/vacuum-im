#include "statisticswebpage.h"

StatisticsWebPage::StatisticsWebPage(QObject *AParent)	: QWebPage(AParent)
{

}

StatisticsWebPage::~StatisticsWebPage()
{

}

void StatisticsWebPage::setUserAgent(const QString &AUserAgent)
{
	FUserAgent = AUserAgent;
}

QString StatisticsWebPage::userAgentForUrl(const QUrl &AUrl) const
{
	QString agent = QWebPage::userAgentForUrl(AUrl);
	if (!FUserAgent.isEmpty())
		agent += " " + FUserAgent;
	return agent;
}
