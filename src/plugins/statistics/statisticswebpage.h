#ifndef STATISTICSWEBPAGE_H
#define STATISTICSWEBPAGE_H

#include <QString>
#include <QWebPage>

class StatisticsWebPage : 
	public QWebPage
{
	Q_OBJECT;
public:
	explicit StatisticsWebPage(QObject *AParent=NULL);
	virtual ~StatisticsWebPage();
	void setVersion(const QString &AUserAgent);
protected:
	virtual QString userAgentForUrl(const QUrl &AUrl) const;
private:
	QString FVersion;
};

#endif // STATISTICSWEBPAGE_H
