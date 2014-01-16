#include "logger.h"

#include <QDir>
#include <QFile>
#include <QtDebug>
#include <QDateTime>
#include <QMutexLocker>
#include "datetime.h"

#define MAX_LOG_FILES  10

void qtMessagesHandler(QtMsgType AType, const char *AMessage)
{
	switch (AType)
	{
	case QtDebugMsg:
		Logger::writeLog(Logger::Debug,"Qt",AMessage);
		break;
	case QtWarningMsg:
		Logger::writeLog(Logger::Warning,"Qt",AMessage);
		break;
	case QtCriticalMsg:
		Logger::writeLog(Logger::Error,"Qt",AMessage);
		break;
	case QtFatalMsg:
		Logger::writeLog(Logger::Fatal,"Qt",AMessage);
		break;
	}
}

struct Logger::LoggerData {
	QFile logFile;
	quint32 loggedTypes;
	quint32 enabledTypes;
	QMap<QString,QMap<QString,QDateTime> > timings;
};

QMutex Logger::FMutex;
Logger::Logger()
{
	d = new LoggerData;
}

Logger::~Logger()
{
	delete d;
}

Logger * Logger::instance()
{
	static Logger *inst = new Logger;
	return inst;
}

QString Logger::logFileName()
{
	return instance()->d->logFile.fileName();
}

void Logger::openLog(const QString &APath)
{
	QMutexLocker locker(&FMutex);
	LoggerData *q = instance()->d;
	if (!q->logFile.isOpen() && !APath.isEmpty())
	{
		QDir logDir(APath);
		QStringList logFiles = logDir.entryList(QStringList()<<"*.log",QDir::Files,QDir::Name);
		while (logFiles.count() > MAX_LOG_FILES)
			QFile::remove(logDir.absoluteFilePath(logFiles.takeFirst()));

#ifndef DEBUG_MODE
		qInstallMsgHandler(qtMessagesHandler);
#endif
		q->logFile.setFileName(logDir.absoluteFilePath(DateTime(QDateTime::currentDateTime()).toX85DateTime().replace(":","-") +".log"));
		q->logFile.open(QFile::WriteOnly|QFile::Truncate);
	}
}

void Logger::closeLog(bool ARemove)
{
	LOG_INFO("Log closed");
	QMutexLocker locker(&FMutex);
	LoggerData *q = instance()->d;
	if (q->logFile.isOpen())
	{
		q->logFile.close();
		if (ARemove)
			QFile::remove(q->logFile.fileName());
	}
}

quint32 Logger::loggedTypes()
{
	return instance()->d->loggedTypes;
}

quint32 Logger::enabledTypes()
{
	return instance()->d->enabledTypes;
}

void Logger::setEnabledTypes(quint32 ATypes)
{
	QMutexLocker locker(&FMutex);
	instance()->d->enabledTypes = ATypes;
}

void Logger::writeLog(quint32 AType, const QString &AClass, const QString &AMessage)
{
	QMutexLocker locker(&FMutex);
	LoggerData *q = instance()->d;
	if ((q->enabledTypes & AType)>0 && q->logFile.isOpen())
	{
		static QDateTime lastLogTime = QDateTime::currentDateTime();

		QDateTime curDateTime = QDateTime::currentDateTime();

		QString typeName;
		switch(AType)
		{
		case Logger::Fatal:
			typeName = "!FTL";
			break;
		case Logger::Error:
			typeName = "!ERR";
			break;
		case Logger::Warning:
			typeName = "*WNG";
			break;
		case Logger::Info:
			typeName = " INF";
			break;
		case Logger::View:
			typeName = " VIW";
			break;
		case Logger::Event:
			typeName = " EVT";
			break;
		case Logger::Timing:
			typeName = " TMG";
			break;
		case Logger::Debug:
			typeName = " DBG";
			break;
		case Logger::Stanza:
			typeName = " STZ";
			break;
		default:
			typeName = QString(" T%1").arg(AType);
		}

		QString timestamp = curDateTime.toString("hh:mm:ss.zzz");
		int timeDelta = lastLogTime.msecsTo(curDateTime);
		QString logLine = QString("%1\t+%2\t%3\t[%4] %5").arg(timestamp).arg(timeDelta).arg(typeName,AClass,AMessage);
		q->logFile.write(logLine.toUtf8());
		q->logFile.write("\r\n");
		q->logFile.flush();

#if defined(DEBUG_MODE) && defined(DEBUG_LOGGER)
		qDebug() << logLine;
#endif

		q->loggedTypes |= AType;
		lastLogTime = curDateTime;
	}
}

QString Logger::startTiming(const QString &AVariable, const QString &AContext)
{
	if (!AVariable.isEmpty())
	{
		QMutexLocker locker(&FMutex);
		instance()->d->timings[AVariable][AContext] = QDateTime::currentDateTime();
	}
	return AContext;
}

qint64 Logger::checkTiming( const QString &AVariable, const QString &AContext)
{
	QMutexLocker locker(&FMutex);
	QDateTime startTime = instance()->d->timings.value(AVariable).value(AContext);
	return startTime.isValid() ? startTime.msecsTo(QDateTime::currentDateTime()) : -1;
}

qint64 Logger::finishTiming(const QString &AVariable, const QString &AContext)
{
	qint64 timing = -1;

	QMutexLocker locker(&FMutex);
	LoggerData *q = instance()->d;

	QMap<QString, QDateTime> &varMap = q->timings[AVariable];
		
	QDateTime startTime = varMap.take(AContext);
	if (startTime.isValid())
		timing = startTime.msecsTo(QDateTime::currentDateTime());

	if (varMap.isEmpty())
		q->timings.remove(AVariable);

	return timing;
}

void Logger::reportView(const QString &AClass)
{
	writeLog(Logger::View,AClass,"View");
	QMetaObject::invokeMethod(instance(),"viewReported",Qt::QueuedConnection,Q_ARG(QString,AClass));
}

void Logger::reportError(const QString &AClass, const QString &AMessage, bool AFatal)
{
	writeLog(AFatal ? Logger::Fatal : Logger::Error,AClass,AMessage);
	QMetaObject::invokeMethod(instance(),"errorReported",Qt::QueuedConnection,Q_ARG(QString,AClass),Q_ARG(QString,AMessage),Q_ARG(bool,AFatal));
}

void Logger::reportEvent(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue)
{
	writeLog(Logger::Event,AClass,QString("%1 (%2)").arg(!ALabel.isEmpty() ? ALabel : AAction).arg(AValue));
	QMetaObject::invokeMethod(instance(),"eventReported",Qt::QueuedConnection,Q_ARG(QString,AClass),Q_ARG(QString,ACategory),Q_ARG(QString,AAction),Q_ARG(QString,ALabel),Q_ARG(qint64,AValue));
}

void Logger::reportTiming(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime)
{
	writeLog(Logger::Timing,AClass,QString("%1: %2").arg(!ALabel.isEmpty() ? ALabel : AVariable).arg(ATime));
	QMetaObject::invokeMethod(instance(),"timingReported",Qt::QueuedConnection,Q_ARG(QString,AClass),Q_ARG(QString,ACategory),Q_ARG(QString,AVariable),Q_ARG(QString,ALabel),Q_ARG(qint64,ATime));
}
