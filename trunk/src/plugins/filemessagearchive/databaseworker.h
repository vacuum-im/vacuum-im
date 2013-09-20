#ifndef DATABASEWORKER_H
#define DATABASEWORKER_H

#include <QMutex>
#include <QQueue>
#include <QThread>
#include <QRunnable>
#include <QSqlError>
#include <QSqlDatabase>
#include <QWaitCondition>
#include <interfaces/ifilemessagearchive.h>
#include <utils/xmpperror.h>

struct DatabaseArchiveHeader : 
	public IArchiveHeader
{
	QString gateway;
	QDateTime timestamp;
};

class DatabaseTask :
	public QRunnable
{
	friend class DatabaseWorker;
public:
	enum Type {
		OpenDatabase,
		CloseDatabase,
		SetProperty,
		LoadHeaders,
		InsertHeaders,
		UpdateHeaders,
		RemoveHeaders,
		LoadModifications
	};
public:
	DatabaseTask(const Jid &AStreamJid, Type AType);
	virtual ~DatabaseTask();
	Type type() const;
	Jid streamJid() const;
	QString taskId() const;
	bool isFinished() const;
	bool isFailed() const;
	XmppError error() const;
protected:
	QString databaseConnection() const;
	void setSQLError(const QSqlError &AError);
	void addBindQueryValue(QSqlQuery &AQuery, const QVariant &AValue) const;
	void bindQueryValue(QSqlQuery &AQuery, const QString &APlaceHolder, const QVariant &AValue) const;
protected:
	bool FAsync;
	bool FFinished;
	XmppError FError;
private:
	Type FType;
	Jid FStreamJid;
	QString FTaskId;
private:
	static quint32 FTaskCount;
};

class DatabaseTaskOpenDatabase :
	public DatabaseTask
{
public:
	DatabaseTaskOpenDatabase(const Jid &AStreamJid, const QString &ADatabasePath);
	QMap<QString,QString> databaseProperties() const;
protected:
	void run();
private:
	bool initializeDatabase(QSqlDatabase &ADatabase);
private:
	QString FDatabasePath;
	QMap<QString,QString> FProperties;
};

class DatabaseTaskCloseDatabase :
	public DatabaseTask
{
public:
	DatabaseTaskCloseDatabase(const Jid &AStreamJid);
protected:
	void run();
};

class DatabaseTaskSetProperty :
	public DatabaseTask
{
public:
	DatabaseTaskSetProperty(const Jid &AStreamJid, const QString &AProperty, const QString &AValue);
	QString property() const;
	QString value() const;
protected:
	void run();
private:
	QString FValue;	
	QString FProperty;
};

class DatabaseTaskLoadHeaders :
	public DatabaseTask
{
public:
	DatabaseTaskLoadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &AGateType);
	IArchiveRequest request() const;
	QList<DatabaseArchiveHeader> headers() const;
protected:
	void run();
private:
	QString FGateType;
	IArchiveRequest FRequest;
	QList<DatabaseArchiveHeader> FHeaders;
};

class DatabaseTaskInsertHeaders :
	public DatabaseTask
{
public:
	DatabaseTaskInsertHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders, const QString &AGateType);
	QList<IArchiveHeader> headers() const;
protected:
	void run();
private:
	QString FGateType;
	QList<IArchiveHeader> FHeaders;
};

class DatabaseTaskUpdateHeaders :
	public DatabaseTask
{
public:
	DatabaseTaskUpdateHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders, bool AInsertIfNotExist=false, const QString &AGateType=QString::null);
	QList<IArchiveHeader> headers() const;
protected:
	void run();
private:
	QString FGateType;
	bool FInsertIfNotExist;
	QList<IArchiveHeader> FHeaders;
};

class DatabaseTaskRemoveHeaders :
	public DatabaseTask
{
public:
	DatabaseTaskRemoveHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders);
	QList<IArchiveHeader> headers() const;
protected:
	void run();
private:
	QList<IArchiveHeader> FHeaders;
};

class DatabaseTaskLoadModifications :
	public DatabaseTask
{
public:
	DatabaseTaskLoadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount);
	IArchiveModifications modifications() const;
protected:
	void run();
private:
	int FCount;
	QDateTime FStart;
	IArchiveModifications FModifications;
};

class DatabaseWorker :
	public QThread
{
	Q_OBJECT;
public:
	DatabaseWorker(QObject *AParent);
	~DatabaseWorker();
	bool execTask(DatabaseTask *ATask);
	bool startTask(DatabaseTask *ATask);
signals:
	void taskFinished(DatabaseTask *ATask);
protected:
	void run();
	void quit();
private:
	bool FQuit;
	QMutex FMutex;
	QWaitCondition FTaskReady;
	QWaitCondition FTaskFinish;
	QQueue<DatabaseTask *> FTasks;
};

#endif // DATABASEWORKER_H
