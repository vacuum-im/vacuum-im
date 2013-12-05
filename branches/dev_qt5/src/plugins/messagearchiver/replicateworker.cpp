#include "replicateworker.h"

#include <QtDebug>

#include <QSqlQuery>
#include <QMetaObject>
#include <QMutexLocker>
#include <utils/datetime.h>

#define DATABASE_STRUCTURE_VERSION      1
#define DATABASE_COMPATIBLE_VERSION     1

#define DELETED_HEADER_VERSION          -1
#define DELETED_HEADER_VERSION_STR      "-1"

// ReplicateTask
quint32 ReplicateTask::FTaskCount = 0;
ReplicateTask::ReplicateTask(Type AType)
{
	FType = AType;
	FFailed = false;
	FTaskId = QString("ArchiveReplicateTask_%1").arg(++FTaskCount);
}

ReplicateTask::~ReplicateTask()
{

}

ReplicateTask::Type ReplicateTask::type() const
{
	return FType;
}

QString ReplicateTask::taskId() const
{
	return FTaskId;
}

bool ReplicateTask::isFailed() const
{
	return FFailed;
}

void ReplicateTask::setSQLError(const QSqlError &AError)
{
	Q_UNUSED(AError);
	FFailed = true;

	qDebug() << AError.databaseText();
}

// ReplicateTaskLoadState
ReplicateTaskLoadState::ReplicateTaskLoadState(const QUuid &AEngineId) : ReplicateTask(LoadState)
{
	FEngineId = AEngineId;
}

QUuid ReplicateTaskLoadState::engineId() const
{
	return FEngineId;
}

QString ReplicateTaskLoadState::nextRef() const
{
	return FNextRef;
}

QDateTime ReplicateTaskLoadState::startTime() const
{
	return FStartTime;
}

void ReplicateTaskLoadState::run(QSqlDatabase &ADatabase)
{
	if (ADatabase.isOpen())
	{
		QSqlQuery loadStateQuery(ADatabase);
		if (!loadStateQuery.prepare("SELECT modif_start, modif_next FROM archives WHERE engine_id=:engine_id"))
		{
			setSQLError(loadStateQuery.lastError());
		}
		else
		{
			loadStateQuery.bindValue(":engine_id",FEngineId.toString());

			if (!loadStateQuery.exec())
			{
				setSQLError(loadStateQuery.lastError());
			}
			else if (!loadStateQuery.next())
			{
				QSqlQuery insertStateQuery(ADatabase);
				if (!insertStateQuery.prepare("INSERT INTO archives (engine_id, modif_start, modif_next) VALUES (:engine_id, :modif_start, :modif_next)"))
				{
					setSQLError(insertStateQuery.lastError());
				}
				else
				{
					FNextRef = QString::null;
					FStartTime = QDateTime(QDate(1970,1,1),QTime(0,0),Qt::UTC);

					insertStateQuery.bindValue(":engine_id",FEngineId.toString());
					insertStateQuery.bindValue(":modif_start",DateTime(FStartTime).toX85UTC());
					insertStateQuery.bindValue(":modif_next",FNextRef);

					if (!insertStateQuery.exec())
						setSQLError(insertStateQuery.lastError());
				}
			}
			else
			{
				FStartTime = DateTime(loadStateQuery.value(0).toString()).toLocal();
				FNextRef = loadStateQuery.value(1).toString();
			}
		}
	}
	else
	{
		FFailed = true;
	}
}

// ReplicateTaskSaveModifications
ReplicateTaskSaveModifications::ReplicateTaskSaveModifications(const QUuid &AEngineId, const IArchiveModifications &AModifications, bool ACompleted) : ReplicateTask(SaveModifications)
{
	FEngineId = AEngineId;
	FCompleted = ACompleted;
	FModifications = AModifications;
}

QUuid ReplicateTaskSaveModifications::engineId() const
{
	return FEngineId;
}

bool ReplicateTaskSaveModifications::isCompleted() const
{
	return FCompleted;
}

IArchiveModifications ReplicateTaskSaveModifications::modifications() const
{
	return FModifications;
}

#define SET_ERROR_AND_EXIT(query) { setSQLError(query.lastError()); ADatabase.rollback(); return; }
void ReplicateTaskSaveModifications::run(QSqlDatabase &ADatabase)
{
	if (ADatabase.isOpen())
	{
		ADatabase.transaction();

		if (!FModifications.items.isEmpty())
		{
			QSqlQuery insertHeaderQuery(ADatabase);
			if (!insertHeaderQuery.prepare("INSERT OR IGNORE INTO headers (with, start) VALUES (:with, :start)"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			QSqlQuery getArchiveParamsQuery(ADatabase);
			if (!getArchiveParamsQuery.prepare("SELECT id, modif_finish FROM archives WHERE engine_id==:engine_id"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			QSqlQuery getHeaderParamsQuery(ADatabase);
			if (!getHeaderParamsQuery.prepare("SELECT id, modification FROM headers WHERE with==:with AND start==:start"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			QSqlQuery insertVersionQuery(ADatabase);
			if (!insertVersionQuery.prepare("INSERT OR IGNORE INTO versions (header_id, archive_id, version, modification) VALUES (:header_id, :archive_id, :version, :modification)"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			QSqlQuery updateVersionQuery(ADatabase);
			if (!updateVersionQuery.prepare("UPDATE versions SET version=:version1, modification=:modification WHERE archive_id==:archive_id AND header_id==:header_id AND version!=:version2 AND (version<:version3 OR :version4=="DELETED_HEADER_VERSION_STR")"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			QSqlQuery updateHeaderQuery(ADatabase);
			if (!updateHeaderQuery.prepare("UPDATE headers SET modification=:modification WHERE id==:header_id"))
				SET_ERROR_AND_EXIT(insertHeaderQuery);

			foreach(const IArchiveModification &modification, FModifications.items)
			{
				QString headerWith = modification.header.with.pFull();
				QString headerStart = DateTime(modification.header.start).toX85UTC();
				qint64 headerVersion = modification.action!=IArchiveModification::Removed ? (qint64)modification.header.version : DELETED_HEADER_VERSION;

				// Get archive params
				quint32 archiveId;
				bool archiveReady;
				getArchiveParamsQuery.bindValue(":engine_id",FEngineId.toString());
				if (getArchiveParamsQuery.exec() && getArchiveParamsQuery.next())
				{
					archiveId = getArchiveParamsQuery.value(0).toULongLong();
					archiveReady = !getArchiveParamsQuery.value(1).isNull();
				}
				else
				{
					SET_ERROR_AND_EXIT(getArchiveParamsQuery);
				}

				// Create new header record if not exists
				insertHeaderQuery.bindValue(":with",headerWith);
				insertHeaderQuery.bindValue(":start",headerStart);
				if (!insertHeaderQuery.exec())
					SET_ERROR_AND_EXIT(insertHeaderQuery);
				bool isNewHeader = insertHeaderQuery.numRowsAffected()>0;

				// Get header params
				quint32 headerId;
				quint32 headerMod;
				getHeaderParamsQuery.bindValue(":with",headerWith);
				getHeaderParamsQuery.bindValue(":start",headerStart);
				if (getHeaderParamsQuery.exec() && getHeaderParamsQuery.next())
				{
					headerId = getHeaderParamsQuery.value(0).toULongLong();
					headerMod = getHeaderParamsQuery.value(1).toULongLong();
				}
				else
				{
					SET_ERROR_AND_EXIT(getHeaderParamsQuery);
				}

				if (isNewHeader || !archiveReady)
				{
					// Create new archive header version record if not exists
					insertVersionQuery.bindValue(":archive_id",archiveId);
					insertVersionQuery.bindValue(":header_id",headerId);
					insertVersionQuery.bindValue(":version",headerVersion);
					insertVersionQuery.bindValue(":modification",headerMod);
					if (!insertVersionQuery.exec())
						SET_ERROR_AND_EXIT(insertVersionQuery);
				}
				else
				{
					quint32 newHeaderMod = headerMod + 1;

					// Create new archive header version record if not exists
					insertVersionQuery.bindValue(":archive_id",archiveId);
					insertVersionQuery.bindValue(":header_id",headerId);
					insertVersionQuery.bindValue(":version",headerVersion);
					insertVersionQuery.bindValue(":modification",newHeaderMod);
					if (!insertVersionQuery.exec())
						SET_ERROR_AND_EXIT(insertVersionQuery);

					// If this is not first archive header modification then update existing modification params
					if (insertVersionQuery.numRowsAffected() == 0)
					{
						updateVersionQuery.bindValue(":archive_id",archiveId);
						updateVersionQuery.bindValue(":header_id",headerId);
						updateVersionQuery.bindValue(":version1",headerVersion);
						updateVersionQuery.bindValue(":version2",headerVersion);
						updateVersionQuery.bindValue(":version3",headerVersion);
						updateVersionQuery.bindValue(":version4",headerVersion);
						updateVersionQuery.bindValue(":modification",newHeaderMod);
						if (!updateVersionQuery.exec())
							SET_ERROR_AND_EXIT(updateVersionQuery);

					}

					// If archive header modification changed then update global header modification
					if (insertVersionQuery.numRowsAffected()>0 || updateVersionQuery.numRowsAffected()>0)
					{
						updateHeaderQuery.bindValue(":header_id",headerId);
						updateHeaderQuery.bindValue(":modification",newHeaderMod);
						if (!updateHeaderQuery.exec())
							SET_ERROR_AND_EXIT(updateHeaderQuery);
					}
				}
			}
		}

		// Update last modifications state
		QSqlQuery updateArchivesQuery(ADatabase);
		if (updateArchivesQuery.prepare("UPDATE archives SET modif_start=:modif_start, modif_next=:modif_next WHERE engine_id=:engine_id"))
		{
			updateArchivesQuery.bindValue(":engine_id",FEngineId.toString());
			updateArchivesQuery.bindValue(":modif_start",DateTime(FModifications.start).toX85UTC());
			updateArchivesQuery.bindValue(":modif_next",FModifications.next);

			if (!updateArchivesQuery.exec())
				SET_ERROR_AND_EXIT(updateArchivesQuery);
		}
		else
		{
			SET_ERROR_AND_EXIT(updateArchivesQuery);
		}

		// Save timestamp when all modifications was loaded
		if (FCompleted)
		{
			QSqlQuery finishModificationsQuery(ADatabase);
			if (finishModificationsQuery.prepare("UPDATE archives SET modif_finish=:modif_finish WHERE engine_id==:engine_id"))
			{
				finishModificationsQuery.bindValue(":engine_id",FEngineId.toString());
				finishModificationsQuery.bindValue(":modif_finish",DateTime(QDateTime::currentDateTime()).toX85UTC());

				if (!finishModificationsQuery.exec())
					SET_ERROR_AND_EXIT(finishModificationsQuery);
			}
			else
			{
				SET_ERROR_AND_EXIT(finishModificationsQuery);
			}
		}

		ADatabase.commit();
	}
	else
	{
		FFailed = true;
	}
}

// ReplicateTaskLoadUpdates
ReplicateTaskLoadModifications::ReplicateTaskLoadModifications(const QList<QUuid> &AEngines) : ReplicateTask(LoadModifications)
{
	FEngines = AEngines;
}

QList<ReplicateModification> ReplicateTaskLoadModifications::modifications() const
{
	return FModifications;
}

void ReplicateTaskLoadModifications::run(QSqlDatabase &ADatabase)
{
	if (ADatabase.isOpen())
	{
		QString enginesPlaceholders;
		for (int i=0; i<FEngines.count()-1; i++)
			enginesPlaceholders.append("?,");
		enginesPlaceholders.append("?");

		QSqlQuery loadModificationsQuery(ADatabase);
		if (!loadModificationsQuery.prepare(QString(
			"SELECT header_peers.with, header_peers.start, header_seeds.modification, header_seeds.version, header_seeds.engines, group_concat(header_peers.engine_id,',') "
			"FROM header_peers JOIN header_seeds ON header_peers.header_id==header_seeds.header_id "
			"WHERE (header_seeds.version!="DELETED_HEADER_VERSION_STR" OR (header_peers.version IS NOT NULL AND header_seeds.version!=header_peers.version)) AND header_peers.engine_id IN (%1) "
			"GROUP BY header_peers.header_id "
			"ORDER BY header_peers.start DESC").arg(enginesPlaceholders)))
		{
			setSQLError(loadModificationsQuery.lastError());
		}
		else
		{
			foreach(const QUuid &engine, FEngines)
				loadModificationsQuery.addBindValue(engine.toString());

			if (!loadModificationsQuery.exec())
			{
				setSQLError(loadModificationsQuery.lastError());
			}
			else while (loadModificationsQuery.next())
			{
				ReplicateModification modification;
				modification.header.with = loadModificationsQuery.value(0).toString();
				modification.header.start = DateTime(loadModificationsQuery.value(1).toString()).toLocal();
				modification.number = loadModificationsQuery.value(2).toULongLong();
				qint64 version = loadModificationsQuery.value(3).toLongLong();
				modification.action = version!=DELETED_HEADER_VERSION ? IArchiveModification::Changed : IArchiveModification::Removed;
				foreach(const QString &engine, loadModificationsQuery.value(4).toString().split(",",QString::SkipEmptyParts))
					modification.sources.append(engine);
				foreach(const QString &engine, loadModificationsQuery.value(5).toString().split(",",QString::SkipEmptyParts))
					modification.destinations.append(engine);
				FModifications.append(modification);
			}
		}
	}
	else
	{
		FFailed = true;
	}
}

// ReplicateTaskUpdateVersion
ReplicateTaskUpdateVersion::ReplicateTaskUpdateVersion(const QUuid &AEngineId, const ReplicateModification &AModification, quint32 AVersion) : ReplicateTask(ReplicateTask::UpdateVersion)
{
	FEngineId = AEngineId;
	FVersion = AVersion;
	FModification = AModification;
}

QUuid ReplicateTaskUpdateVersion::engineId() const
{
	return FEngineId;
}

void ReplicateTaskUpdateVersion::run(QSqlDatabase &ADatabase)
{
	if (ADatabase.isOpen())
	{
		QSqlQuery getHeaderParamsQuery(ADatabase);
		if (!getHeaderParamsQuery.prepare("SELECT hid, aid FROM (SELECT id AS hid FROM headers WHERE with==:with AND start==:start) JOIN (SELECT id AS aid FROM archives WHERE engine_id==:engine_id)"))
		{
			setSQLError(getHeaderParamsQuery.lastError());
		}
		else
		{
			getHeaderParamsQuery.bindValue(":engine_id",FEngineId.toString());
			getHeaderParamsQuery.bindValue(":with",FModification.header.with.pFull());
			getHeaderParamsQuery.bindValue(":start",DateTime(FModification.header.start).toX85UTC());

			if (!getHeaderParamsQuery.exec() || !getHeaderParamsQuery.next())
			{
				setSQLError(getHeaderParamsQuery.lastError());
			}
			else
			{
				QSqlQuery updateVersionQuery(ADatabase);
				if (!updateVersionQuery.prepare("INSERT OR REPLACE INTO versions (header_id, archive_id, version, modification) VALUES (:header_id, :archive_id, :version, :modification)"))
				{
					setSQLError(updateVersionQuery.lastError());
				}
				else
				{
					updateVersionQuery.bindValue(":header_id",getHeaderParamsQuery.value(0));
					updateVersionQuery.bindValue(":archive_id",getHeaderParamsQuery.value(1));
					updateVersionQuery.bindValue(":version",FModification.action!=IArchiveModification::Removed ? (qint64)FVersion : DELETED_HEADER_VERSION);
					updateVersionQuery.bindValue(":modification",FModification.number);

					if (!updateVersionQuery.exec())
						setSQLError(updateVersionQuery.lastError());
				}
			}
		}
	}
	else
	{
		FFailed = true;
	}
}

// ReplicateWorker
ReplicateWorker::ReplicateWorker(const QString &AConnection, const QString &ADatabaseFilePath, QObject *AParent) : QThread(AParent)
{
	FQuit = false;
	FConnection = AConnection;
	FDatabaseFilePath = ADatabaseFilePath;
	qRegisterMetaType<ReplicateTask *>("ReplicateTask *");
}

ReplicateWorker::~ReplicateWorker()
{
	quit();
	wait();
}

void ReplicateWorker::quit()
{
	QMutexLocker locker(&FMutex);
	FQuit = true;
	FTaskReady.wakeAll();
}

bool ReplicateWorker::startTask(ReplicateTask *ATask)
{
	QMutexLocker locker(&FMutex);
	if (!FQuit)
	{
		FTasks.enqueue(ATask);
		FTaskReady.wakeAll();
		return true;
	}
	delete ATask;
	return false;
}

void ReplicateWorker::run()
{
	QMutexLocker locker(&FMutex);
	if (!QSqlDatabase::contains(FConnection))
	{
		// QSqlDatabase should be destroyed before removing connection
		{
			QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",FConnection);
			db.setDatabaseName(FDatabaseFilePath);

			if (db.isValid() && db.open() && initializeDatabase(db))
			{
				QMetaObject::invokeMethod(this,"ready",Qt::QueuedConnection);
				while (!FQuit || !FTasks.isEmpty())
				{
					ReplicateTask *task = !FTasks.isEmpty() ? FTasks.dequeue() : NULL;
					if (task)
					{
						locker.unlock();
						task->run(db);
						QMetaObject::invokeMethod(this,"taskFinished",Qt::QueuedConnection,Q_ARG(ReplicateTask *,task));
						locker.relock();
					}
					else
					{
						FTaskReady.wait(locker.mutex());
					}
				}
			}

			db.close();
		}
		QSqlDatabase::removeDatabase(FConnection);
	}
}

bool ReplicateWorker::initializeDatabase(QSqlDatabase &ADatabase)
{
	QSqlQuery query(ADatabase);

	QMap<QString,QString> properties;
	if (ADatabase.tables().contains("properties"))
	{
		if (query.exec("SELECT property, value FROM properties"))
		{
			while (query.next())
				properties.insert(query.value(0).toString(),query.value(1).toString());
		}
		else
		{
			return false;
		}
	}

	int structureVersion = properties.value("StructureVersion").toInt();
	int compatibleVersion = properties.value("CompatibleVersion").toInt();
	if (structureVersion < DATABASE_STRUCTURE_VERSION)
	{
		static const struct { QString createQuery; int compatible; } databaseUpdates[] = 
		{
			{
				"CREATE TABLE properties ("
				"  property         TEXT PRIMARY KEY,"
				"  value            TEXT"
				");"

				"CREATE TABLE headers ("
				"  id               INTEGER PRIMARY KEY,"
				"  with             TEXT NOT NULL,"
				"  start            DATETIME NOT NULL,"
				"  modification     INTEGER DEFAULT 0"
				");"

				"CREATE TABLE archives ("
				"  id               INTEGER PRIMARY KEY,"
				"  engine_id        TEXT NOT NULL,"
				"  modif_start      DATETIME NOT NULL,"
				"  modif_next       TEXT,"
				"  modif_finish     DATETIME"
				");"

				"CREATE TABLE versions ("
				"  archive_id       INTEGER NOT NULL,"
				"  header_id        INTEGER NOT NULL,"
				"  version          INTEGER NOT NULL,"
				"  modification     INTEGER DEFAULT 0,"
				"  PRIMARY KEY      (header_id, archive_id)"
				");"

				"CREATE UNIQUE INDEX headers_with_start ON headers ("
				"  with             ASC,"
				"  start            ASC"
				");"

				"CREATE UNIQUE INDEX archives_engineid ON archives ("
				"  engine_id        ASC"
				");"

				"CREATE VIEW header_seeds AS"
				"  SELECT headers.id AS header_id, headers.modification AS modification, versions.version AS version, group_concat(archives.engine_id,',') AS engines"
				"  FROM headers JOIN versions ON headers.id==versions.header_id JOIN archives ON versions.archive_id==archives.id"
				"  WHERE versions.modification==headers.modification"
				"  GROUP BY headers.id"
				";"

				"CREATE VIEW header_peers AS"
				"  SELECT archives.id AS archive_id, archives.engine_id AS engine_id, headers.id AS header_id, headers.with AS with, headers.start AS start, versions.version AS version, versions.modification AS modification"
				"  FROM headers JOIN archives LEFT JOIN versions ON versions.archive_id==archives.id AND versions.header_id==headers.id"
				"  WHERE versions.modification IS NULL OR versions.modification<headers.modification"
				";"

				"INSERT INTO properties(property,value) VALUES('StructureVersion','1');"
				"INSERT INTO properties(property,value) VALUES('CompatibleVersion','1');"
				, 1
			} 
		};

		ADatabase.transaction();
		QSqlQuery createQuery(ADatabase);
		for (int i=structureVersion; i<DATABASE_STRUCTURE_VERSION; i++)
		{
			QStringList commands = databaseUpdates[i].createQuery.split(';',QString::SkipEmptyParts);
			foreach(const QString &command, commands)
			{
				if (!createQuery.exec(command))
				{
					qDebug() << createQuery.lastError().databaseText();
					ADatabase.rollback();
					return false;
				}
			}
		}
		ADatabase.commit();
	}
	else if (compatibleVersion > DATABASE_STRUCTURE_VERSION)
	{
		return false;
	}
	return true;
}
