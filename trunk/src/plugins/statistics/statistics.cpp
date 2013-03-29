#include "statistics.h"

#include <QDir>
#include <QWebFrame>
#include <utils/filecookiejar.h>

#define DIR_STATISTICS               "statistics"
#define FILE_COOKIES                 "cookies.jar"

Statistics::Statistics()
{
	FPluginManager = NULL;

	FStatisticsView = new QWebView(NULL);
	connect(FStatisticsView,SIGNAL(loadFinished(bool)),SLOT(onWebViewLoadFinished(bool)));
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
	QNetworkAccessManager *accessManager = new QNetworkAccessManager(FStatisticsView);
	accessManager->setCookieJar(new FileCookieJar(getDataFilePath(FILE_COOKIES)));
	FStatisticsView->page()->setNetworkAccessManager(accessManager);
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

void Statistics::onWebViewLoadFinished(bool AOk)
{
	Q_UNUSED(AOk);
}

Q_EXPORT_PLUGIN2(plg_statistics, Statistics)
