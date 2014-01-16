#ifndef REPLICATEWORKER_H
#define REPLICATEWORKER_H

#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QSqlError>
#include <QSqlDatabase>
#include <QWaitCondition>
#include <interfaces/imessagearchiver.h>

struct ReplicateModification {
	quint32 number;
	IArchiveHeader header;
	QList<QUuid> sources;
	QList<QUuid> destinations;
	IArchiveModification::ModifyAction action;
};

class ReplicateTask
{
	friend class ReplicateWorker;
public:
	enum Type {
		LoadState,
		SaveModifications,
		LoadModifications,
		UpdateVersion
	};
public:
	ReplicateTask(Type AType);
	virtual ~ReplicateTask();
	Type type() const;
	QString taskId() const;
	bool isFailed() const;
	QSqlError error() const;
protected:
	void setSQLError(const QSqlError &AError);
protected:
	virtual void run(QSqlDatabase &ADatabase) =0;
protected:
	Type FType;
	bool FFailed;
	QString FTaskId;
	QSqlError FError;
private:
	static quint32 FTaskCount;
};

class ReplicateTaskLoadState :
	public ReplicateTask
{
public:
	ReplicateTaskLoadState(const QUuid &AEngineId);
	QUuid engineId() const;
	QString nextRef() const;
	QDateTime startTime() const;
protected:
	void run(QSqlDatabase &ADatabase);
private:
	QUuid FEngineId;
	QString FNextRef;
	QDateTime FStartTime;
};

class ReplicateTaskSaveModifications :
	public ReplicateTask
{
public:
	ReplicateTaskSaveModifications(const QUuid &AEngineId, const IArchiveModifications &AModifications, bool ACompleted);
	QUuid engineId() const;
	bool isCompleted() const;
	IArchiveModifications modifications() const;
protected:
	void run(QSqlDatabase &ADatabase);
private:
	QUuid FEngineId;
	bool FCompleted;
	IArchiveModifications FModifications;
};

class ReplicateTaskLoadModifications :
	public ReplicateTask
{
public:
	ReplicateTaskLoadModifications(const QList<QUuid> &AEngines);
	QList<ReplicateModification> modifications() const;
protected:
	void run(QSqlDatabase &ADatabase);
private:
	QList<QUuid> FEngines;
	QList<ReplicateModification> FModifications;
};

class ReplicateTaskUpdateVersion :
	public ReplicateTask
{
public:
	ReplicateTaskUpdateVersion(const QUuid &AEngineId, const ReplicateModification &AModification, quint32 AVersion);
	QUuid engineId() const;
protected:
	void run(QSqlDatabase &ADatabase);
private:
	QUuid FEngineId;
	quint32 FVersion;
	ReplicateModification FModification;
};

class ReplicateWorker :
	public QThread
{
	Q_OBJECT;
public:
	ReplicateWorker(const QString &AConnection, const QString &ADatabaseFilePath, QObject *AParent);
	~ReplicateWorker();
	void quit();
	bool startTask(ReplicateTask *ATask);
signals:
	void ready();
	void taskFinished(ReplicateTask *ATask);
protected:
	void run();
	bool initializeDatabase(QSqlDatabase &ADatabase);
private:
	QMutex FMutex;
	QWaitCondition FTaskReady;
	QQueue<ReplicateTask *> FTasks;
private:
	bool FQuit;
	QString FConnection;
	QString FDatabaseFilePath;
};

#endif // REPLICATEWORKER_H
