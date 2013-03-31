#include "statistics.h"

#include <QDir>
#include <QWebFrame>
#include <QDataStream>
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <definitions/version.h>
#include <definitions/optionvalues.h>
#include <definitions/statisticsparams.h>
#include <utils/filecookiejar.h>
#include <utils/options.h>

#define DIR_STATISTICS               "statistics"
#define FILE_COOKIES                 "cookies.dat"

#define MP_VER                       "1"
#define MP_ID                        "UA-11825394-9"
#define MP_URL                       "http://www.google-analytics.com/collect"

#define RELOAD_TIMEOUT               60000
#define DESTROY_TIMEOUT              30000

#define RESEND_PENDING_TIMEOUT       60000

QDataStream &operator>>(QDataStream &AStream, IStatisticsHit& AHit)
{
	AStream >> AHit.type;
	AStream >> AHit.session;
	AStream >> AHit.description;
	AStream >> AHit.timestamp;

	AStream >> AHit.view.title;
	AStream >> AHit.view.location;
	AStream >> AHit.view.host;
	AStream >> AHit.view.path;

	AStream >> AHit.event.category;
	AStream >> AHit.event.action;
	AStream >> AHit.event.label;
	AStream >> AHit.event.value;

	AStream >> AHit.timing.category;
	AStream >> AHit.timing.variable;
	AStream >> AHit.timing.label;
	AStream >> AHit.timing.time;

	AStream >> AHit.exception.errString;
	AStream >> AHit.exception.fatal;

	return AStream;
}

QDataStream &operator<<(QDataStream &AStream, const IStatisticsHit &AHit)
{
	AStream << AHit.type;
	AStream << AHit.session;
	AStream << AHit.description;
	AStream << AHit.timestamp;
	
	AStream << AHit.view.title;
	AStream << AHit.view.location;
	AStream << AHit.view.host;
	AStream << AHit.view.path;

	AStream << AHit.event.category;
	AStream << AHit.event.action;
	AStream << AHit.event.label;
	AStream << AHit.event.value;

	AStream << AHit.timing.category;
	AStream << AHit.timing.variable;
	AStream << AHit.timing.label;
	AStream << AHit.timing.time;

	AStream << AHit.exception.errString;
	AStream << AHit.exception.fatal;

	return AStream;
}

Statistics::Statistics()
{
	FPluginManager = NULL;

	FStatisticsView = new QWebView(NULL);
	connect(FStatisticsView,SIGNAL(loadFinished(bool)),SLOT(onStatisticsViewLoadFinished(bool)));

	FNetworkManager = new QNetworkAccessManager(this);
	connect(FNetworkManager,SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
	connect(FNetworkManager,SIGNAL(finished(QNetworkReply *)),SLOT(onNetworkManagerFinished(QNetworkReply *)));

	FPendingTimer.setSingleShot(true);
	connect(&FPendingTimer,SIGNAL(timeout()),SLOT(onPendingTimerTimeout()));
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

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool Statistics::initObjects()
{
	StatisticsWebPage *statPage = new StatisticsWebPage(FStatisticsView);
	statPage->setNetworkAccessManager(FNetworkManager);
	statPage->setUserAgent(QString("%1/%2.%3").arg(CLIENT_NAME).arg(FPluginManager->version()).arg(FPluginManager->revision()));
	FStatisticsView->setPage(statPage);
	return true;
}

QUuid Statistics::profileId() const
{
	return FProfileId;
}

bool Statistics::isValidHit(const IStatisticsHit &AHit) const
{
	if (!AHit.timestamp.isValid() || AHit.timestamp>QDateTime::currentDateTime())
		return false;

	if (AHit.type == IStatisticsHit::HitView)
	{
		if (AHit.view.title.isEmpty())
			return false;
	}
	else if (AHit.type == IStatisticsHit::HitEvent)
	{
		if (AHit.event.category.isEmpty() || AHit.event.action.isEmpty())
			return false;
	}
	else if (AHit.type == IStatisticsHit::HitTiming)
	{
		if (AHit.timing.category.isEmpty() || AHit.timing.variable.isEmpty())
			return false;
		if (AHit.timing.time < 0)
			return false;
	}
	else if (AHit.type == IStatisticsHit::HitException)
	{
		if (AHit.exception.errString.isEmpty())
			return false;
	}
	else
	{
		return false;
	}

	return true;
}

bool Statistics::sendStatisticsHit(const IStatisticsHit &AHit)
{
	if (false && isValidHit(AHit) && !FProfileId.isNull())
	{
		QNetworkReply *reply = FNetworkManager->get(QNetworkRequest(buildHitUrl(AHit)));
		if (!reply->isFinished())
		{
			FReplyHits.insert(reply,AHit);
			FPluginManager->delayShutdown();
			return true;
		}
	}
	return false;
}

QUrl Statistics::buildHitUrl(const IStatisticsHit &AHit) const
{
	QUrl url(MP_URL);
	url.setQueryDelimiters('=','&');

	QList< QPair<QByteArray,QByteArray> > query;
	query.append(qMakePair<QByteArray,QByteArray>("v",QUrl::toPercentEncoding(MP_VER)));
	query.append(qMakePair<QByteArray,QByteArray>("tid",QUrl::toPercentEncoding(MP_ID)));

	QString cid = FProfileId.toString();
	cid.remove(0,1); cid.chop(1);
	query.append(qMakePair<QByteArray,QByteArray>("cid",QUrl::toPercentEncoding(cid)));

	qint64 qt = AHit.timestamp.msecsTo(QDateTime::currentDateTime());
	if (qt > 0)
		query.append(qMakePair<QByteArray,QByteArray>("qt",QUrl::toPercentEncoding(QString::number(qt))));

	if (!AHit.description.isEmpty())
		query.append(qMakePair<QByteArray,QByteArray>("cd",QUrl::toPercentEncoding(AHit.description)));

	if (AHit.session == IStatisticsHit::SessionStart)
		query.append(qMakePair<QByteArray,QByteArray>("sc",QUrl::toPercentEncoding("start")));
	else if (AHit.session == IStatisticsHit::SessionEnd)
		query.append(qMakePair<QByteArray,QByteArray>("sc",QUrl::toPercentEncoding("end")));

	query.append(qMakePair<QByteArray,QByteArray>("an",QUrl::toPercentEncoding(CLIENT_NAME)));
	query.append(qMakePair<QByteArray,QByteArray>("av",QUrl::toPercentEncoding(QString("%1.%2").arg(FPluginManager->version(),FPluginManager->revision()))));

	if (AHit.type == IStatisticsHit::HitView)
	{
		query.append(qMakePair<QByteArray,QByteArray>("t",QUrl::toPercentEncoding("appview")));

		query.append(qMakePair<QByteArray,QByteArray>("dt",QUrl::toPercentEncoding(AHit.view.title)));

		if (!AHit.view.location.isEmpty())
			query.append(qMakePair<QByteArray,QByteArray>("dl",QUrl::toPercentEncoding(AHit.view.location)));

		if (!AHit.view.host.isEmpty())
			query.append(qMakePair<QByteArray,QByteArray>("dh",QUrl::toPercentEncoding(AHit.view.host)));

		if (!AHit.view.path.isEmpty())
			query.append(qMakePair<QByteArray,QByteArray>("dp",QUrl::toPercentEncoding(AHit.view.path)));
	}
	else if (AHit.type == IStatisticsHit::HitEvent)
	{
		query.append(qMakePair<QByteArray,QByteArray>("t",QUrl::toPercentEncoding("event")));

		query.append(qMakePair<QByteArray,QByteArray>("ec",QUrl::toPercentEncoding(AHit.event.category)));
		query.append(qMakePair<QByteArray,QByteArray>("ea",QUrl::toPercentEncoding(AHit.event.action)));

		if (!AHit.event.label.isEmpty())
			query.append(qMakePair<QByteArray,QByteArray>("el",QUrl::toPercentEncoding(AHit.event.label)));

		if (AHit.event.value >= 0)
			query.append(qMakePair<QByteArray,QByteArray>("ev",QUrl::toPercentEncoding(QString::number(AHit.event.value))));
	}
	else if (AHit.type == IStatisticsHit::HitTiming)
	{
		query.append(qMakePair<QByteArray,QByteArray>("t",QUrl::toPercentEncoding("timing")));
		query.append(qMakePair<QByteArray,QByteArray>("utc",QUrl::toPercentEncoding(AHit.timing.category)));
		query.append(qMakePair<QByteArray,QByteArray>("utv",QUrl::toPercentEncoding(AHit.timing.variable)));
		query.append(qMakePair<QByteArray,QByteArray>("utt",QUrl::toPercentEncoding(QString::number(AHit.timing.time))));

		if (!AHit.timing.label.isEmpty())
			query.append(qMakePair<QByteArray,QByteArray>("utl",QUrl::toPercentEncoding(AHit.timing.label)));
	}
	else if (AHit.type == IStatisticsHit::HitException)
	{
		query.append(qMakePair<QByteArray,QByteArray>("t",QUrl::toPercentEncoding("exception")));
		query.append(qMakePair<QByteArray,QByteArray>("exd",QUrl::toPercentEncoding(AHit.exception.errString)));
		query.append(qMakePair<QByteArray,QByteArray>("exf",QUrl::toPercentEncoding(AHit.exception.fatal ? "1" : "0")));
	}

	url.setEncodedQueryItems(query);

	return url;
}

QString Statistics::getStatisticsFilePath(const QString &AFileName) const
{
	QDir dir(FPluginManager->homePath());
	if (!dir.exists(DIR_STATISTICS))
		dir.mkdir(DIR_STATISTICS);
	dir.cd(DIR_STATISTICS);

	if (!FProfileId.isNull())
	{
		QString profileDir = FProfileId.toString();
		if (!dir.exists(profileDir))
			dir.mkdir(profileDir);
		dir.cd(profileDir);
	}

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

void Statistics::onNetworkManagerFinished(QNetworkReply *AReply)
{
	AReply->deleteLater();
	if (FReplyHits.contains(AReply))
	{
		IStatisticsHit hit = FReplyHits.take(AReply);
		if (AReply->error() != QNetworkReply::NoError)
		{
			FPendingHits.append(hit);
			FPendingTimer.start(RESEND_PENDING_TIMEOUT);
		}
		else if (!FPendingHits.isEmpty())
		{
			FPendingTimer.start(0);
		}
		FPluginManager->continueShutdown();
	}
}

void Statistics::onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &AProxy, QAuthenticator *AAuth)
{
	AAuth->setUser(AProxy.user());
	AAuth->setPassword(AProxy.password());
}

void Statistics::onOptionsOpened()
{
	FPendingHits.clear();

	FProfileId = Options::node(OPV_STATISTICS_PROFILEID).value().toString();
	if (FProfileId.isNull())
	{
		FProfileId = QUuid::createUuid();
		Options::node(OPV_STATISTICS_PROFILEID).setValue(FProfileId.toString());
	}

	if (FNetworkManager->cookieJar() != NULL)
		FNetworkManager->cookieJar()->deleteLater();
	FNetworkManager->setCookieJar(new FileCookieJar(getStatisticsFilePath(FILE_COOKIES)));

	IStatisticsHit hit;
	hit.type = IStatisticsHit::HitEvent;
	hit.session = IStatisticsHit::SessionStart;
	hit.event.category = SEVC_APPLICATION;
	hit.event.action = SEVA_APPLICATION_LAUNCH;
	sendStatisticsHit(hit);

	FStatisticsView->load(QUrl("http://www.vacuum-im.org/statistics"));
}

void Statistics::onOptionsClosed()
{
	IStatisticsHit hit;
	hit.type = IStatisticsHit::HitEvent;
	hit.session = IStatisticsHit::SessionEnd;
	hit.event.category = SEVC_APPLICATION;
	hit.event.action = SEVA_APPLICATION_SHUTDOWN;
	sendStatisticsHit(hit);
}

void Statistics::onPendingTimerTimeout()
{
	while (!FPendingHits.isEmpty())
	{
		IStatisticsHit hit = FPendingHits.takeFirst();
		if (sendStatisticsHit(hit))
			break;
	}
}

Q_EXPORT_PLUGIN2(plg_statistics, Statistics)
