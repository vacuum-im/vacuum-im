#include "systemmanager.h"

#include <QDir>
#include <QSysInfo>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>
#include <QStringList>
#include <thirdparty/idle/idle.h>

#if defined(Q_OS_UNIX)
#	include <sys/utsname.h>
#elif defined(Q_WS_HAIKU)
#	include <Path.h>
#	include <AppFileInfo.h>
#	include <FindDirectory.h>
#endif

struct SystemManager::SystemManagerData
{
	SystemManagerData() {
		idle = new Idle;
		idleSeconds = 0;
	}
	~SystemManagerData() {
		delete idle;
	}
	Idle *idle;
	int idleSeconds;
};

SystemManager::SystemManager()
{
	d = new SystemManagerData;
	connect(d->idle,SIGNAL(secondsIdle(int)),SLOT(onIdleChanged(int)));
}

SystemManager::~SystemManager()
{
	delete d;
}

SystemManager *SystemManager::instance()
{
	static SystemManager *inst = new SystemManager;
	return inst;
}

int SystemManager::systemIdle()
{
	return instance()->d->idleSeconds;
}

bool SystemManager::isSystemIdleActive()
{
	return instance()->d->idle->isActive();
}

void SystemManager::startSystemIdle()
{
	SystemManagerData *q = instance()->d;
	if (!q->idle->isActive())
		q->idle->start();
}

void SystemManager::stopSystemIdle()
{
	SystemManagerData *q = instance()->d;
	if (q->idle->isActive())
		q->idle->stop();
}

QString SystemManager::osVersion()
{
	static QString osver;
	if (osver.isEmpty())
	{
#if defined(Q_WS_MAC)
		switch (QSysInfo::MacintoshVersion)
		{
# if QT_VERSION >= 0x040803
		case QSysInfo::MV_MOUNTAINLION:
			osver = "OS X 10.8 Mountain Lion";
			break;
# endif
		case QSysInfo::MV_LION:
			osver = "OS X 10.7 Lion";
			break;
		case QSysInfo::MV_SNOWLEOPARD:
			osver = "Mac OS X 10.6 Snow Leopard)";
			break;
		case QSysInfo::MV_LEOPARD:
			osver = "Mac OS X 10.5 Leopard";
			break;
		case QSysInfo::MV_TIGER:
			osver = "Mac OS X 10.4 Tiger";
			break;
		case QSysInfo::MV_PANTHER:
			osver = "Mac OS X 10.3 Panther";
			break;
		case QSysInfo::MV_JAGUAR:
			osver = "Mac OS X 10.2 Jaguar";
			break;
		case QSysInfo::MV_PUMA:
			osver = "Mac OS X 10.1 Puma";
			break;
		case QSysInfo::MV_CHEETAH:
			osver = "Mac OS X 10.0 Cheetah";
			break;
		case QSysInfo::MV_9:
			osver = "Mac OS 9";
			break;
		case QSysInfo::MV_Unknown:
		default:
			osver = "MacOS (unknown)";
			break;
		}
#elif defined(Q_WS_X11)
		QStringList path;
		foreach(const QString &env, QProcess::systemEnvironment())
			if (env.startsWith("PATH="))
				path = env.split('=').value(1).split(':');

		QString found;
		foreach(const QString &dirname, path)
		{
			QDir dir(dirname);
			QFileInfo cand(dir.filePath("lsb_release"));
			if (cand.isExecutable())
			{
				found = cand.absoluteFilePath();
				break;
			}
		}

		if (!found.isEmpty())
		{
			QProcess process;
			process.start(found, QStringList()<<"--description"<<"--short", QIODevice::ReadOnly);
			if (process.waitForStarted())
			{
				QTextStream stream(&process);
				while (process.waitForReadyRead())
					osver += stream.readAll();
				process.close();
				osver = osver.trimmed();
			}
		}

		if (osver.isEmpty())
		{
			utsname buf;
			if (uname(&buf) != -1)
			{
				osver.append(buf.release).append(QLatin1Char(' '));
				osver.append(buf.sysname).append(QLatin1Char(' '));
				osver.append(buf.machine).append(QLatin1Char(' '));
				osver.append(QLatin1String(" (")).append(buf.machine).append(QLatin1Char(')'));
			}
			else
			{
				osver = QLatin1String("Linux/Unix (unknown)");
			}
		}
#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)
		switch (QSysInfo::WindowsVersion)
		{
		case QSysInfo::WV_CE_6:
			osver = "Windows CE 6.x";
			break;
		case QSysInfo::WV_CE_5:
			osver = "Windows CE 5.x";
			break;
		case QSysInfo::WV_CENET:
			osver = "Windows CE .NET";
			break;
		case QSysInfo::WV_CE:
			osver = "Windows CE";
			break;
# if QT_VERSION >= 0x040803
		case QSysInfo::WV_WINDOWS8:
			osver = "Windows 8";
			break;
# endif
		case QSysInfo::WV_WINDOWS7:
			osver = "Windows 7";
			break;
		case QSysInfo::WV_VISTA:
			osver = "Windows Vista";
			break;
		case QSysInfo::WV_2003:
			osver = "Windows Server 2003";
			break;
		case QSysInfo::WV_XP:
			osver = "Windows XP";
			break;
		case QSysInfo::WV_2000:
			osver = "Windows 2000";
			break;
		case QSysInfo::WV_NT:
			osver = "Windows NT";
			break;
		case QSysInfo::WV_Me:
			osver = "Windows Me";
			break;
		case QSysInfo::WV_98:
			osver = "Windows 98";
			break;
		case QSysInfo::WV_95:
			osver = "Windows 95";
			break;
		case QSysInfo::WV_32s:
			osver = "Windows 3.1 with Win32s";
			break;
		default:
			osver = "Windows (unknown)";
			break;
		}
#elif defined(Q_WS_HAIKU)
		BPath path;
		QString strVersion("Haiku");
		if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) 
		{
			path.Append("libbe.so");

			BAppFileInfo appFileInfo;
			version_info versionInfo;
			BFile file;
			if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
				&& appFileInfo.SetTo(&file) == B_OK
				&& appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK
				&& versionInfo.short_info[0] != '\0')
			{
				strVersion = versionInfo.short_info;
			}
		}

		utsname uname_info;
		if (uname(&uname_info) == 0) 
		{
			osver = uname_info.sysname;
			long revision = 0;
			if (sscanf(uname_info.version, "r%10ld", &revision) == 1)
			{
				char version[16];
				snprintf(version, sizeof(version), "%ld", revision);
				osver += " ( " + strVersion + " Rev. ";
				osver += version;
				osver += ")";
			}
		}
#else
		osver = "Unknown OS";
#endif
	}
	return osver;
}

void SystemManager::onIdleChanged(int ASeconds)
{
	d->idleSeconds = ASeconds;
	emit systemIdleChanged(ASeconds);
}
