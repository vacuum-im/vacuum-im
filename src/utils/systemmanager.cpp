#include "systemmanager.h"

#include <QDir>
#include <QSysInfo>
#include <QProcess>
#include <QFileInfo>
#include <QSettings>
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
#if defined(Q_OS_WIN) || defined(Q_OS_CYGWIN)
		QSettings settings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", QSettings::NativeFormat);
		osver = settings.value("ProductName").toString();
#elif defined(Q_OS_MAC)
		switch (QSysInfo::MacintoshVersion)
		{
        case QSysInfo::MV_SIERRA:
            osver = "macOS 10.12 Sierra";
            break;
        case QSysInfo::MV_ELCAPITAN:
            osver = "OS X 10.11 El Capitan";
            break;
		case QSysInfo::MV_YOSEMITE:
			osver = "OS X 10.10 Yosemite";
			break;
		case QSysInfo::MV_MAVERICKS:
			osver = "OS X 10.9 Mavericks";
			break;
		case QSysInfo::MV_MOUNTAINLION:
			osver = "OS X 10.8 Mountain Lion";
			break;
		case QSysInfo::MV_LION:
			osver = "OS X 10.7 Lion";
			break;
		case QSysInfo::MV_SNOWLEOPARD:
			osver = "Mac OS X 10.6 Snow Leopard";
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
#elif defined(Q_OS_UNIX)
		if (QFile::exists(QLatin1String("/etc/os-release")))
		{
			QSettings s(QLatin1String("/etc/os-release"), QSettings::IniFormat);
			s.setIniCodec("UTF-8");
			QString pretty = s.value(QLatin1String("PRETTY_NAME")).toString();
			QString name = s.value(QLatin1String("NAME")).toString();
			QString version = s.value(QLatin1String("VERSION")).toString();
			osver = !pretty.isEmpty () ? pretty : QString(name + " " + version);
		}
		else
		{
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
