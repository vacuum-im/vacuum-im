#ifndef LOGGER_H
#define LOGGER_H

#include <QMutex>
#include <QObject>
#include <QStringList>
#include "utilsexport.h"

class UTILS_EXPORT Logger : 
	public QObject
{
	Q_OBJECT;
	struct LoggerData;
public:
	enum LogType {
		Fatal         = 0x0001,
		Error         = 0x0002,
		Warning       = 0x0004,
		Info          = 0x0008,
		View          = 0x0010,
		Event         = 0x0020,
		Timing        = 0x0040,
		Debug         = 0x0080,
		Stanza        = 0x0100
	};
public:
	static Logger *instance();
	static QString logFileName();
	static void openLog(const QString &APath);
	static void closeLog(bool ARemove = false);
public:
	static quint32 loggedTypes();
	static quint32 enabledTypes();
	static void setEnabledTypes(quint32 ATypes);
	static void writeLog(quint32 AType, const QString &AClass, const QString &AMessage);
public:
	static void startTiming(const QString &AVariable, const QString &AContext = QString::null);
	static qint64 checkTiming(const QString &AVariable, const QString &AContext = QString::null);
	static qint64 finishTiming(const QString &AVariable, const QString &AContext = QString::null);
public:
	static void reportView(const QString &AClass);
	static void reportError(const QString &AClass, const QString &AMessage, bool AFatal);
	static void reportEvent(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue);
	static void reportTiming(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime);
signals:
	void viewReported(const QString &AClass);
	void errorReported(const QString &AClass, const QString &AMessage, bool AFatal);
	void eventReported(const QString &AClass, const QString &ACategory, const QString &AAction, const QString &ALabel, qint64 AValue);
	void timingReported(const QString &AClass, const QString &ACategory, const QString &AVariable, const QString &ALabel, qint64 ATime);
private:
	Logger();
	~Logger();
	LoggerData *d;
	static QMutex FMutex;
};

#define LOG_TYPE(type,msg)                   Logger::writeLog(type,staticMetaObject.className(),msg)
#define LOG_FATAL(msg)                       LOG_TYPE(Logger::Fatal,msg)
#define LOG_ERROR(msg)                       LOG_TYPE(Logger::Error,msg)
#define LOG_WARNING(msg)                     LOG_TYPE(Logger::Warning,msg)
#define LOG_INFO(msg)                        LOG_TYPE(Logger::Info,msg)
#define LOG_DEBUG(msg)                       LOG_TYPE(Logger::Debug,msg)

#define LOG_STRM_TYPE(type,strm,msg)         LOG_TYPE(type,QString("[%1] %2").arg(Jid(strm).pBare(),msg))
#define LOG_STRM_FATAL(strm,msg)             LOG_STRM_TYPE(Logger::Fatal,strm,msg)
#define LOG_STRM_ERROR(strm,msg)             LOG_STRM_TYPE(Logger::Error,strm,msg)
#define LOG_STRM_WARNING(strm,msg)           LOG_STRM_TYPE(Logger::Warning,strm,msg)
#define LOG_STRM_INFO(strm,msg)              LOG_STRM_TYPE(Logger::Info,strm,msg)
#define LOG_STRM_DEBUG(strm,msg)             LOG_STRM_TYPE(Logger::Debug,strm,msg)

#define REPORT_VIEW                          Logger::reportView(staticMetaObject.className())
#define REPORT_FATAL(msg)                    Logger::reportError(staticMetaObject.className(),msg,true)
#define REPORT_ERROR(msg)                    Logger::reportError(staticMetaObject.className(),msg,false)
#define REPORT_EVENT(prm,val)                { QStringList prms=QString(prm).split("|"); Logger::reportEvent(staticMetaObject.className(),prms.value(0),prms.value(0)+"-"+prms.value(1),prms.value(2),val); }
#define REPORT_TIMING(prm,val)               { QStringList prms=QString(prm).split("|"); Logger::reportTiming(staticMetaObject.className(),prms.value(0),prms.value(0)+"-"+prms.value(1),prms.value(2),val); }

#endif // LOGGER_H
