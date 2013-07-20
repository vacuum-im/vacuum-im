#include "statistics.h"

#include <QDir>
#include <QUrlQuery>
#include <QWebFrame>
#include <QDataStream>
#include <QAuthenticator>
#include <QNetworkProxy>
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

#define STAT_PAGE_URL                "http://www.vacuum-im.org/statistics"

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
	FConnectionManager = NULL;

	FNetworkManager = new QNetworkAccessManager(this);
	connect(FNetworkManager,SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
	connect(FNetworkManager,SIGNAL(finished(QNetworkReply *)),SLOT(onNetworkManagerFinished(QNetworkReply *)));

	FPendingTimer.setSingleShot(true);
	connect(&FPendingTimer,SIGNAL(timeout()),SLOT(onPendingTimerTimeout()));
}

Statistics::~Statistics()
{

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

	IPlugin *plugin = APluginManager->pluginInterface("IConnectionManager").value(0,NULL);
	if (plugin)
	{
		FConnectionManager = qobject_cast<IConnectionManager *>(plugin->instance());
		if (FConnectionManager)
		{
			connect(FConnectionManager->instance(),SIGNAL(defaultProxyChanged(const QUuid &)),SLOT(onDefaultConnectionProxyChanged(const QUuid &)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

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
#ifndef DEBUG_MODE
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
#else
	Q_UNUSED(AHit);
#endif
	return false;
}

QUrl Statistics::buildHitUrl(const IStatisticsHit &AHit) const
{
	QUrl url(MP_URL);

	QList< QPair<QString,QString> > query;
	query.append(qMakePair<QString,QString>("v",MP_VER));
	query.append(qMakePair<QString,QString>("tid",MP_ID));

	QString cid = FProfileId.toString();
	cid.remove(0,1); cid.chop(1);
	query.append(qMakePair<QString,QString>("cid",cid));

	qint64 qt = AHit.timestamp.msecsTo(QDateTime::currentDateTime());
	if (qt > 0)
		query.append(qMakePair<QString,QString>("qt",QString::number(qt)));

	if (!AHit.description.isEmpty())
		query.append(qMakePair<QString,QString>("cd",AHit.description));

	if (AHit.session == IStatisticsHit::SessionStart)
		query.append(qMakePair<QString,QString>("sc","start"));
	else if (AHit.session == IStatisticsHit::SessionEnd)
		query.append(qMakePair<QString,QString>("sc","end"));

	query.append(qMakePair<QString,QString>("an",CLIENT_NAME));
	query.append(qMakePair<QString,QString>("av",QString("%1.%2").arg(FPluginManager->version(),FPluginManager->revision())));

	if (AHit.type == IStatisticsHit::HitView)
	{
		query.append(qMakePair<QString,QString>("t","appview"));

		query.append(qMakePair<QString,QString>("dt",AHit.view.title));

		if (!AHit.view.location.isEmpty())
			query.append(qMakePair<QString,QString>("dl",AHit.view.location));

		if (!AHit.view.host.isEmpty())
			query.append(qMakePair<QString,QString>("dh",AHit.view.host));

		if (!AHit.view.path.isEmpty())
			query.append(qMakePair<QString,QString>("dp",AHit.view.path));
	}
	else if (AHit.type == IStatisticsHit::HitEvent)
	{
		query.append(qMakePair<QString,QString>("t","event"));

		query.append(qMakePair<QString,QString>("ec",AHit.event.category));
		query.append(qMakePair<QString,QString>("ea",AHit.event.action));

		if (!AHit.event.label.isEmpty())
			query.append(qMakePair<QString,QString>("el",AHit.event.label));

		if (AHit.event.value >= 0)
			query.append(qMakePair<QString,QString>("ev",QString::number(AHit.event.value)));
	}
	else if (AHit.type == IStatisticsHit::HitTiming)
	{
		query.append(qMakePair<QString,QString>("t","timing"));
		query.append(qMakePair<QString,QString>("utc",AHit.timing.category));
		query.append(qMakePair<QString,QString>("utv",AHit.timing.variable));
		query.append(qMakePair<QString,QString>("utt",QString::number(AHit.timing.time)));

		if (!AHit.timing.label.isEmpty())
			query.append(qMakePair<QString,QString>("utl",AHit.timing.label));
	}
	else if (AHit.type == IStatisticsHit::HitException)
	{
		query.append(qMakePair<QString,QString>("t","exception"));
		query.append(qMakePair<QString,QString>("exd",AHit.exception.errString));
		query.append(qMakePair<QString,QString>("exf",AHit.exception.fatal ? "1" : "0"));
	}

	QUrlQuery urlQuery;
	urlQuery.setQueryDelimiters('=','&');
	urlQuery.setQueryItems(query);
	url.setQuery(urlQuery);

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
	Q_UNUSED(AOk);
	QWebView *statView = qobject_cast<QWebView *>(sender());
	if (statView)
		QTimer::singleShot(30000,statView,SLOT(deleteLater()));
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

#ifndef DEBUG_MODE
	QWebView *statView = new QWebView(NULL);
	connect(statView,SIGNAL(loadFinished(bool)),SLOT(onStatisticsViewLoadFinished(bool)));

	StatisticsWebPage *statPage = new StatisticsWebPage(statView);
	statPage->setNetworkAccessManager(FNetworkManager);
	statPage->setVersion(QString("%1.%2").arg(FPluginManager->version()).arg(FPluginManager->revision()));
	statView->setPage(statPage);

	statView->load(QUrl(STAT_PAGE_URL));
#endif
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

void Statistics::onDefaultConnectionProxyChanged( const QUuid &AProxyId )
{
	FNetworkManager->setProxy(FConnectionManager->proxyById(AProxyId).proxy);
}
