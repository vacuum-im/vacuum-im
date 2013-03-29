#include "statistics.h"

#include <QDir>
#include <QTimer>
#include <QWebFrame>
#include <definitions/version.h>
#include <utils/filecookiejar.h>

#define DIR_STATISTICS               "statistics"
#define FILE_COOKIES                 "cookies.jar"

#define RELOAD_TIMEOUT               60000
#define DESTROY_TIMEOUT              30000

Statistics::Statistics()
{
	FPluginManager = NULL;

	FStatisticsView = new QWebView(NULL);
	connect(FStatisticsView,SIGNAL(loadFinished(bool)),SLOT(onStatisticsViewLoadFinished(bool)));
}

Statistics::~Statistics()
{
	delete FStatisticsView;
}

void Statistics::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Statistics");
	APluginInfo->description = tr("Allows to collect application statistics");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool Statistics::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;
	return true;
}

bool Statistics::initObjects()
{
	StatisticsWebPage *statPage = new StatisticsWebPage(FStatisticsView);
	statPage->setUserAgent(QString("%1/%2.%3").arg(CLIENT_NAME).arg(FPluginManager->version()).arg(FPluginManager->revision()));
	FStatisticsView->setPage(statPage);

	QNetworkAccessManager *accessManager = new QNetworkAccessManager(statPage);
	accessManager->setCookieJar(new FileCookieJar(getDataFilePath(FILE_COOKIES)));
	statPage->setNetworkAccessManager(accessManager);

	return true;
}

bool Statistics::startPlugin()
{
	FStatisticsView->load(QUrl("http://www.vacuum-im.org/statistics"));
	return true;
}

QString Statistics::getDataFilePath(const QString &AFileName) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_STATISTICS))
		dir.mkdir(DIR_STATISTICS);
	dir.cd(DIR_STATISTICS);
	return dir.absoluteFilePath(AFileName);
}

void Statistics::onStatisticsViewLoadFinished(bool AOk)
{
	if (AOk)
	{
		QTimer::singleShot(DESTROY_TIMEOUT,FStatisticsView,SLOT(deleteLater()));
		FStatisticsView = NULL;
	}
	else
	{
		QTimer::singleShot(RELOAD_TIMEOUT,FStatisticsView,SLOT(reload()));
	}
}

Q_EXPORT_PLUGIN2(plg_statistics, Statistics)
