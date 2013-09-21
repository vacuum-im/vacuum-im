#include "statistics.h"

#include <QDir>
#include <QDataStream>
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <definitions/version.h>
#include <definitions/optionvalues.h>
#include <definitions/statisticsparams.h>
#include <utils/filecookiejar.h>
#include <utils/options.h>

#ifdef Q_OS_WIN32
#	include <windows.h>
#endif

//#define DEBUG_STATISTICS

#define MP_VER                       "1"
#define MP_ID                        "UA-11825394-10"
#define MP_URL                       "http://www.google-analytics.com/collect"

#define DIR_STATISTICS               "statistics"
#define FILE_COOKIES                 "cookies.dat"
#define RESEND_TIMEOUT               60000

QDataStream &operator>>(QDataStream &AStream, IStatisticsHit& AHit)
{
	AStream >> AHit.type;
	AStream >> AHit.session;
	AStream >> AHit.description;
	AStream >> AHit.timestamp;

	AStream >> AHit.view.descr;
	AStream >> AHit.view.title;
	AStream >> AHit.view.host;
	AStream >> AHit.view.path;
	AStream >> AHit.view.location;

	AStream >> AHit.event.category;
	AStream >> AHit.event.action;
	AStream >> AHit.event.label;
	AStream >> AHit.event.value;

	AStream >> AHit.timing.category;
	AStream >> AHit.timing.variable;
	AStream >> AHit.timing.label;
	AStream >> AHit.timing.time;

	AStream >> AHit.exception.descr;
	AStream >> AHit.exception.fatal;

	return AStream;
}

QDataStream &operator<<(QDataStream &AStream, const IStatisticsHit &AHit)
{
	AStream << AHit.type;
	AStream << AHit.session;
	AStream << AHit.description;
	AStream << AHit.timestamp;
	
	AStream << AHit.view.descr;
	AStream << AHit.view.title;
	AStream << AHit.view.host;
	AStream << AHit.view.path;
	AStream << AHit.view.location;

	AStream << AHit.event.category;
	AStream << AHit.event.action;
	AStream << AHit.event.label;
	AStream << AHit.event.value;

	AStream << AHit.timing.category;
	AStream << AHit.timing.variable;
	AStream << AHit.timing.label;
	AStream << AHit.timing.time;

	AStream << AHit.exception.descr;
	AStream << AHit.exception.fatal;

	return AStream;
}

Statistics::Statistics()
{
	FPluginManager = NULL;
	FConnectionManager = NULL;

	FDesktopWidget = new QDesktopWidget;

	FNetworkManager = new QNetworkAccessManager(this);
	connect(FNetworkManager,SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
	connect(FNetworkManager,SIGNAL(finished(QNetworkReply *)),SLOT(onNetworkManagerFinished(QNetworkReply *)));

	FPendingTimer.setSingleShot(true);
	connect(&FPendingTimer,SIGNAL(timeout()),SLOT(onPendingTimerTimeout()));
}

Statistics::~Statistics()
{
	delete FDesktopWidget;
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
			connect(FConnectionManager->instance(),SIGNAL(defaultProxyChanged(const QUuid &)),SLOT(onDefaultConnectionProxyChanged(const QUuid &)));
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool Statistics::initObjects()
{
	FUserAgent = userAgent();
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

	switch (AHit.type)
	{
	case IStatisticsHit::HitView:
		if (AHit.view.descr.isEmpty())
			return false;
		if (!AHit.view.path.isEmpty() && !AHit.view.path.startsWith('/'))
			return false;
		break;
	case IStatisticsHit::HitEvent:
		if (AHit.event.category.isEmpty() || AHit.event.action.isEmpty())
			return false;
		break;
	case IStatisticsHit::HitTiming:
		if (AHit.timing.category.isEmpty() || AHit.timing.variable.isEmpty())
			return false;
		if (AHit.timing.time < 0)
			return false;
		break;
	case IStatisticsHit::HitException:
		if (AHit.exception.descr.isEmpty())
			return false;
		break;
	default:
		return false;
	}

	return true;
}

bool Statistics::sendStatisticsHit(const IStatisticsHit &AHit)
{
#if !defined(DEBUG_MODE) || defined(DEBUG_STATISTICS)
	if (isValidHit(AHit) && !FProfileId.isNull())
	{
		QNetworkRequest request(buildHitUrl(AHit));
		request.setRawHeader("User-Agent",FUserAgent.toUtf8());
		QNetworkReply *reply = FNetworkManager->get(request);
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

QString Statistics::userAgent() const
{
	static QString firstPart;
	static QString secondPart;
	static QString thirdPart;

	if (firstPart.isNull() || secondPart.isNull() || thirdPart.isNull()) 
	{
		QString firstPartTemp;
		firstPartTemp.reserve(150);
		firstPartTemp += QString::fromLatin1(CLIENT_NAME)+QString::fromLatin1("/")+FPluginManager->version();

		firstPartTemp += QString::fromLatin1(" ("
			// Platform
#ifdef Q_WS_MAC
			"Macintosh; "
#elif defined Q_WS_QWS
			"QtEmbedded; "
#elif defined Q_WS_MAEMO_5
			"Maemo"
#elif defined Q_WS_MAEMO_6
			"MeeGo"
#elif defined Q_WS_WIN
			// Nothing
#elif defined Q_WS_X11
			"X11; "
#else
			"Unknown; "
#endif
			);

#if defined(QT_NO_OPENSSL)
		// No SSL support
		firstPartTemp += QString::fromLatin1("N; ");
#endif

		// Operating system
#ifdef Q_OS_AIX
		firstPartTemp += QString::fromLatin1("AIX");
#elif defined Q_OS_WIN32
		firstPartTemp += windowsVersion();
#elif defined Q_OS_DARWIN
#if defined(__powerpc__)
		firstPartTemp += QString::fromLatin1("PPC Mac OS X");
#else
		firstPartTemp += QString::fromLatin1("Intel Mac OS X");
#endif

#elif defined Q_OS_BSDI
		firstPartTemp += QString::fromLatin1("BSD");
#elif defined Q_OS_BSD4
		firstPartTemp += QString::fromLatin1("BSD Four");
#elif defined Q_OS_CYGWIN
		firstPartTemp += QString::fromLatin1("Cygwin");
#elif defined Q_OS_DGUX
		firstPartTemp += QString::fromLatin1("DG/UX");
#elif defined Q_OS_DYNIX
		firstPartTemp += QString::fromLatin1("DYNIX/ptx");
#elif defined Q_OS_FREEBSD
		firstPartTemp += QString::fromLatin1("FreeBSD");
#elif defined Q_OS_HPUX
		firstPartTemp += QString::fromLatin1("HP-UX");
#elif defined Q_OS_HURD
		firstPartTemp += QString::fromLatin1("GNU Hurd");
#elif defined Q_OS_IRIX
		firstPartTemp += QString::fromLatin1("SGI Irix");
#elif defined Q_OS_LINUX
#if !defined(Q_WS_MAEMO_5) && !defined(Q_WS_MAEMO_6)
#if defined(__x86_64__)
		firstPartTemp += QString::fromLatin1("Linux x86_64");
#elif defined(__i386__)
		firstPartTemp += QString::fromLatin1("Linux i686");
#else
		firstPartTemp += QString::fromLatin1("Linux");
#endif
#endif
#elif defined Q_OS_LYNX
		firstPartTemp += QString::fromLatin1("LynxOS");
#elif defined Q_OS_NETBSD
		firstPartTemp += QString::fromLatin1("NetBSD");
#elif defined Q_OS_OS2
		firstPartTemp += QString::fromLatin1("OS/2");
#elif defined Q_OS_OPENBSD
		firstPartTemp += QString::fromLatin1("OpenBSD");
#elif defined Q_OS_OS2EMX
		firstPartTemp += QString::fromLatin1("OS/2");
#elif defined Q_OS_OSF
		firstPartTemp += QString::fromLatin1("HP Tru64 UNIX");
#elif defined Q_OS_QNX6
		firstPartTemp += QString::fromLatin1("QNX RTP Six");
#elif defined Q_OS_QNX
		firstPartTemp += QString::fromLatin1("QNX");
#elif defined Q_OS_RELIANT
		firstPartTemp += QString::fromLatin1("Reliant UNIX");
#elif defined Q_OS_SCO
		firstPartTemp += QString::fromLatin1("SCO OpenServer");
#elif defined Q_OS_SOLARIS
		firstPartTemp += QString::fromLatin1("Sun Solaris");
#elif defined Q_OS_ULTRIX
		firstPartTemp += QString::fromLatin1("DEC Ultrix");
#elif defined Q_OS_UNIX
		firstPartTemp += QString::fromLatin1("UNIX BSD/SYSV system");
#elif defined Q_OS_UNIXWARE
		firstPartTemp += QString::fromLatin1("UnixWare Seven, Open UNIX Eight");
#else
		firstPartTemp += QString::fromLatin1("Unknown");
#endif
		firstPartTemp += QString::fromLatin1(")");
		firstPartTemp.squeeze();
		firstPart = firstPartTemp;

		secondPart = QString::fromLatin1("Qt/") + QString::fromLatin1(qVersion());

		QString thirdPartTemp;
		thirdPartTemp.reserve(150);
#if defined(Q_OS_SYMBIAN) || defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6)
		thirdPartTemp += QString::fromLatin1("Mobile Safari/");
#else
		thirdPartTemp += QString::fromLatin1("Safari/");
#endif
		thirdPartTemp += QString::fromLatin1(QT_VERSION_STR);
		thirdPartTemp.squeeze();
		thirdPart = thirdPartTemp;

		Q_ASSERT(!firstPart.isNull());
		Q_ASSERT(!secondPart.isNull());
		Q_ASSERT(!thirdPart.isNull());
	}

	return firstPart + " " + secondPart+ " " + thirdPart;
}

QString Statistics::windowsVersion() const
{
#ifdef Q_OS_WIN32
	OSVERSIONINFOEX versionInfo;

	ZeroMemory(&versionInfo, sizeof(versionInfo));
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&versionInfo));

	int majorVersion = versionInfo.dwMajorVersion;
	int minorVersion = versionInfo.dwMinorVersion;

	return QString("Windows NT %1.%2").arg(majorVersion).arg(minorVersion);
#endif
	return QString::null;
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

	query.append(qMakePair<QByteArray,QByteArray>("ul",QUrl::toPercentEncoding(QLocale().name())));

	QRect sr = FDesktopWidget->screenGeometry();
	query.append(qMakePair<QByteArray,QByteArray>("sr",QUrl::toPercentEncoding(QString("%1.%2").arg(sr.width()).arg(sr.height()))));

	if (AHit.type == IStatisticsHit::HitView)
	{
		query.append(qMakePair<QByteArray,QByteArray>("t",QUrl::toPercentEncoding("appview")));

		query.append(qMakePair<QByteArray,QByteArray>("cd",QUrl::toPercentEncoding(AHit.view.descr)));

		if (!AHit.view.title.isEmpty())
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
		query.append(qMakePair<QByteArray,QByteArray>("exd",QUrl::toPercentEncoding(AHit.exception.descr)));
		query.append(qMakePair<QByteArray,QByteArray>("exf",QUrl::toPercentEncoding(AHit.exception.fatal ? "1" : "0")));
	}

	query.append(qMakePair<QByteArray,QByteArray>("z",QUrl::toPercentEncoding(QString::number(qrand()))));

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

void Statistics::onNetworkManagerFinished(QNetworkReply *AReply)
{
	AReply->deleteLater();
	if (FReplyHits.contains(AReply))
	{
		IStatisticsHit hit = FReplyHits.take(AReply);
		if (AReply->error() != QNetworkReply::NoError)
		{
			FPendingHits.append(hit);
			FPendingTimer.start(RESEND_TIMEOUT);
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
	hit.type = IStatisticsHit::HitView;
	hit.view.descr = SAVD_PROFILE_OPENED;
	hit.session = IStatisticsHit::SessionStart;
	sendStatisticsHit(hit);
}

void Statistics::onOptionsClosed()
{
	IStatisticsHit hit;
	hit.type = IStatisticsHit::HitView;
	hit.view.descr = SAVD_PROFILE_CLOSED;
	hit.session = IStatisticsHit::SessionEnd;
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

void Statistics::onDefaultConnectionProxyChanged(const QUuid &AProxyId)
{
	FNetworkManager->setProxy(FConnectionManager->proxyById(AProxyId).proxy);
}

Q_EXPORT_PLUGIN2(plg_statistics, Statistics)
