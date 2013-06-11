#include "statisticswebpage.h"

StatisticsWebPage::StatisticsWebPage(QObject *AParent)	: QWebPage(AParent)
{

}

StatisticsWebPage::~StatisticsWebPage()
{

}

void StatisticsWebPage::setVersion(const QString &AUserAgent)
{
	FVersion = AUserAgent;
}

QString StatisticsWebPage::userAgentForUrl(const QUrl &AUrl) const
{
	QString agent = QWebPage::userAgentForUrl(AUrl);
	if (!FVersion.isEmpty())
		agent = agent.left(agent.indexOf("Safari/")+7)+FVersion;
	return agent;
}
