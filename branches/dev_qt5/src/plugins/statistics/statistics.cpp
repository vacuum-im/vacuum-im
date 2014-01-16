#include "statistics.h"

#include <QDir>
#include <QUrlQuery>
#include <QWebFrame>
#include <QDataStream>
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QNetworkRequest>
#include <definitions/version.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/statisticsparams.h>
#include <utils/filecookiejar.h>
#include <utils/options.h>
#include <utils/logger.h>

#ifdef Q_OS_WIN32
#	include <windows.h>
#endif

#define MP_VER                       "1"
#define MP_ID                        "UA-11825394-10"
#define MP_URL                       "http://www.google-analytics.com/collect"

#define DIR_STATISTICS               "statistics"
#define FILE_COOKIES                 "cookies.dat"
#define RESEND_TIMEOUT               60*1000
#define SESSION_TIMEOUT              5*60*1000

QDataStream &operator>>(QDataStream &AStream, IStatisticsHit& AHit)
{
	AStream >> AHit.type;
	AStream >> AHit.session;
	AStream >> AHit.profile;
	AStream >> AHit.screen;
	AStream >> AHit.timestamp;

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
	AStream << AHit.profile;
	AStream << AHit.screen;
	AStream << AHit.timestamp;
	
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
	FOptionsManager = NULL;
	FConnectionManager = NULL;

	FSendHits = true;
	FDesktopWidget = new QDesktopWidget;

	FNetworkManager = new QNetworkAccessManager(this);
	connect(FNetworkManager,SIGNAL(proxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)),
		SLOT(onNetworkManagerProxyAuthenticationRequired(const QNetworkProxy &, QAuthenticator *)));
	connect(FNetworkManager,SIGNAL(finished(QNetworkReply *)),SLOT(onNetworkManagerFinished(QNetworkReply *)));

	FPendingTimer.setSingleShot(true);
	connect(&FPendingTimer,SIGNAL(timeout()),SLOT(onPendingTimerTimeout()));

	FSessionTimer.setSingleShot(false);
	FSessionTimer.setInterval(SESSION_TIMEOUT);
	connect(&FSessionTimer,SIGNAL(timeout()),SLOT(onSessionTimerTimeout()));

	connect(Logger::instance(),SIGNAL(viewReported(const QString &)),
		SLOT(onLoggerViewReported(const QString &)));
	connect(Logger::instance(),SIGNAL(errorReported(const QString &, const QString &, bool)),
		SLOT(onLoggerErrorReported(const QString &, const QString &, bool)));
	connect(Logger::instance(),SIGNAL(eventReported(const QString &, const QString &, const QString &, const QString &, qint64)),
		SLOT(onLoggerEventReported(const QString &, const QString &, const QString &, const QString &, qint64)));
	connect(Logger::instance(),SIGNAL(timingReported(const QString &, const QString &, const QString &, const QString &, qint64)),
		SLOT(onLoggerTimingReported(const QString &, const QString &, const QString &, const QString &, qint64)));
}

Statistics::~Statistics()
{
	if (!FPendingHits.isEmpty())
		LOG_WARNING(QString("Failed to send pending statistics hints, count=%1").arg(FPendingHits.count()));
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

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));
	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return true;
}

bool Statistics::initObjects()
{
	FUserAgent = userAgent();
	LOG_DEBUG(QString("Statistics User-Aget header - %1").arg(FUserAgent));

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}

	return true;
}

bool Statistics::initSettings()
{
	Options::setDefaultValue(OPV_MISC_STATISTICTS_ENABLED,true);
	return true;
}

QMultiMap<int, IOptionsWidget *> Statistics::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_MISC)
	{
		widgets.insertMulti(OWO_MISC_STATISTICS,FOptionsManager->optionsNodeWidget(Options::node(OPV_MISC_STATISTICTS_ENABLED),tr("Send anonymous statistics information to developer"),AParent));
	}
	return widgets;
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
		if (AHit.screen.isEmpty())
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
	if (FSendHits && isValidHit(AHit))
	{
		if (!FProfileId.isNull() || !AHit.profile.isNull())
		{
			QNetworkRequest request(buildHitUrl(AHit));
			request.setRawHeader("User-Agent",FUserAgent.toUtf8());
			QNetworkReply *reply = FNetworkManager->get(request);
			if (!reply->isFinished())
			{
				FReplyHits.insert(reply,AHit);
				FPluginManager->delayShutdown();
			}
		}
		else
		{
			FPendingHits.append(AHit);
			FPendingTimer.start(RESEND_TIMEOUT);
		}
		return true;
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

	QList< QPair<QString,QString> > query;
	query.append(qMakePair<QString,QString>("v",QUrl::toPercentEncoding(MP_VER)));
	query.append(qMakePair<QString,QString>("tid",QUrl::toPercentEncoding(MP_ID)));

	QString cid = !AHit.profile.isNull() ? AHit.profile.toString() : FProfileId.toString();
	cid.remove(0,1); cid.chop(1);
	query.append(qMakePair<QString,QString>("cid",QUrl::toPercentEncoding(cid)));

	query.append(qMakePair<QString,QString>("fl",QUrl::toPercentEncoding(qVersion())));

	qint64 qt = AHit.timestamp.msecsTo(QDateTime::currentDateTime());
	if (qt > 0)
		query.append(qMakePair<QString,QString>("qt",QUrl::toPercentEncoding(QString::number(qt))));

	if (!AHit.screen.isEmpty())
		query.append(qMakePair<QString,QString>("cd",QUrl::toPercentEncoding(AHit.screen)));

	if (AHit.session == IStatisticsHit::SessionStart)
		query.append(qMakePair<QString,QString>("sc",QUrl::toPercentEncoding("start")));
	else if (AHit.session == IStatisticsHit::SessionEnd)
		query.append(qMakePair<QString,QString>("sc",QUrl::toPercentEncoding("end")));

	query.append(qMakePair<QString,QString>("an",QUrl::toPercentEncoding(CLIENT_NAME)));
	query.append(qMakePair<QString,QString>("av",QUrl::toPercentEncoding(QString("%1.%2").arg(FPluginManager->version(),FPluginManager->revision()))));

	query.append(qMakePair<QString,QString>("ul",QUrl::toPercentEncoding(QLocale().name())));

	QRect sr = FDesktopWidget->screenGeometry();
	query.append(qMakePair<QString,QString>("sr",QUrl::toPercentEncoding(QString("%1.%2").arg(sr.width()).arg(sr.height()))));

	if (AHit.type == IStatisticsHit::HitView)
	{
		query.append(qMakePair<QString,QString>("t",QUrl::toPercentEncoding("appview")));
	}
	else if (AHit.type == IStatisticsHit::HitEvent)
	{
		query.append(qMakePair<QString,QString>("t",QUrl::toPercentEncoding("event")));

		query.append(qMakePair<QString,QString>("ec",QUrl::toPercentEncoding(AHit.event.category)));
		query.append(qMakePair<QString,QString>("ea",QUrl::toPercentEncoding(AHit.event.action)));

		if (!AHit.event.label.isEmpty())
			query.append(qMakePair<QString,QString>("el",QUrl::toPercentEncoding(AHit.event.label)));

		if (AHit.event.value >= 0)
			query.append(qMakePair<QString,QString>("ev",QUrl::toPercentEncoding(QString::number(AHit.event.value))));
	}
	else if (AHit.type == IStatisticsHit::HitTiming)
	{
		query.append(qMakePair<QString,QString>("t",QUrl::toPercentEncoding("timing")));
		query.append(qMakePair<QString,QString>("utc",QUrl::toPercentEncoding(AHit.timing.category)));
		query.append(qMakePair<QString,QString>("utv",QUrl::toPercentEncoding(AHit.timing.variable)));
		query.append(qMakePair<QString,QString>("utt",QUrl::toPercentEncoding(QString::number(AHit.timing.time))));

		if (!AHit.timing.label.isEmpty())
			query.append(qMakePair<QString,QString>("utl",QUrl::toPercentEncoding(AHit.timing.label)));
	}
	else if (AHit.type == IStatisticsHit::HitException)
	{
		query.append(qMakePair<QString,QString>("t",QUrl::toPercentEncoding("exception")));
		query.append(qMakePair<QString,QString>("exd",QUrl::toPercentEncoding(AHit.exception.descr)));
		query.append(qMakePair<QString,QString>("exf",QUrl::toPercentEncoding(AHit.exception.fatal ? "1" : "0")));
	}

	query.append(qMakePair<QString,QString>("z",QUrl::toPercentEncoding(QString::number(qrand()))));

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

void Statistics::onNetworkManagerFinished(QNetworkReply *AReply)
{
	AReply->deleteLater();
	if (FReplyHits.contains(AReply))
	{
		IStatisticsHit hit = FReplyHits.take(AReply);
		if (AReply->error() != QNetworkReply::NoError)
		{
			hit.profile = FProfileId;
			FPendingHits.append(hit);
			FPendingTimer.start(RESEND_TIMEOUT);
			LOG_WARNING(QString("Failed to send statistics hit: %1").arg(AReply->errorString()));
		}
		else 
		{
			if (!FPendingHits.isEmpty())
				FPendingTimer.start(0);
			LOG_DEBUG(QString("Statistics hit sent, type=%1, screen=%2").arg(hit.type).arg(hit.screen));
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
	FSendHits = Options::node(OPV_MISC_STATISTICTS_ENABLED).value().toBool();

	FProfileId = Options::node(OPV_STATISTICS_PROFILEID).value().toString();
	if (FProfileId.isNull())
	{
		FProfileId = QUuid::createUuid();
		Options::node(OPV_STATISTICS_PROFILEID).setValue(FProfileId.toString());
	}

	if (FNetworkManager->cookieJar() != NULL)
		FNetworkManager->cookieJar()->deleteLater();
	FNetworkManager->setCookieJar(new FileCookieJar(getStatisticsFilePath(FILE_COOKIES)));

	REPORT_EVENT(SEVP_SESSION_STARTED,1);
	FSessionTimer.start();
}

void Statistics::onOptionsClosed()
{
	REPORT_EVENT(SEVP_SESSION_FINISHED,1);
	FSessionTimer.stop();
}

void Statistics::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_MISC_STATISTICTS_ENABLED)
	{
		if (ANode.value().toBool())
		{
			FSendHits = true;
			onLoggerEventReported("Statistics","statistics","statistics-enabled","Statistics Enabled",1); // SEVP_STATISTICS_ENABLED
		}
		else
		{
			onLoggerEventReported("Statistics","statistics","statistics-disabled","Statistics Disabled",1); // SEVP_STATISTICS_DISABLED
			FSendHits = false;
		}
	}
}

void Statistics::onPendingTimerTimeout()
{
	bool sent = false;
	while (!FPendingHits.isEmpty() && !sent)
	{
		IStatisticsHit hit = FPendingHits.takeFirst();
		sent = sendStatisticsHit(hit);
	}
}

void Statistics::onSessionTimerTimeout()
{
	REPORT_EVENT(SEVP_SESSION_CONTINUED,1);
}

void Statistics::onDefaultConnectionProxyChanged(const QUuid &AProxyId)
{
	FNetworkManager->setProxy(FConnectionManager->proxyById(AProxyId).proxy);
}

void Statistics::onLoggerViewReported(const QString &AClass)
{
	if (!AClass.isEmpty())
	{
		IStatisticsHit hit;
		hit.type = IStatisticsHit::HitView;
		hit.screen = AClass;
		sendStatisticsHit(hit);
	}
}

void Statistics::onLoggerErrorReported(const QString &AClass, const QString &AMessage, bool AFatal)
{
	if (!AClass.isEmpty() && !AMessage.isEmpty() && !FReportedErrors.contains(AClass,AMessage))
	{
		IStatisticsHit hit;
		hit.type = IStatisticsHit::HitException;
		hit.screen = AClass;
		hit.exception.fatal = AFatal;
		hit.exception.descr = AMessage;
		sendStatisticsHit(hit);
		FReportedErrors.insertMulti(AClass,AMessage);
	}
}

void Statistics::onLoggerEventReported(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue)
{
	if (!ACategory.isEmpty() && !AAction.isEmpty())
	{
		IStatisticsHit hit;
		hit.type = IStatisticsHit::HitEvent;
		hit.screen = AClass;
		hit.event.category = ACategory;
		hit.event.action = AAction;
		hit.event.label = ALabel;
		hit.event.value = AValue;
		sendStatisticsHit(hit);
	}
}

void Statistics::onLoggerTimingReported(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime)
{
	if (!ACategory.isEmpty() && !AVariable.isEmpty() && ATime>=0)
	{
		IStatisticsHit hit;
		hit.type = IStatisticsHit::HitTiming;
		hit.screen = AClass;
		hit.timing.category = ACategory;
		hit.timing.variable = AVariable;
		hit.timing.label = ALabel;
		hit.timing.time = ATime;
		sendStatisticsHit(hit);
	}
}
