#include "databaseworker.h"

#include <QSqlQuery>
#include <QMetaObject>
#include <QMutexLocker>
#include <QCoreApplication>
#include <definitions/internalerrors.h>
#include <definitions/filearchivedatabaseproperties.h>
#include <utils/datetime.h>

#define DATABASE_STRUCTURE_VERSION     1
#define DATABASE_COMPATIBLE_VERSION    1

// DatabaseTask
quint32 DatabaseTask::FTaskCount = 0;
DatabaseTask::DatabaseTask(const Jid &AStreamJid, Type AType)	
{
	FAsync = true;
	FFinished = false;

	FType = AType;
	FStreamJid = AStreamJid;
	FTaskId = QString("FileArchiveDatabaseTask_%1").arg(++FTaskCount);
}

DatabaseTask::~DatabaseTask()
{

}

DatabaseTask::Type DatabaseTask::type() const
{
	return FType;
}

Jid DatabaseTask::streamJid() const
{
	return FStreamJid;
}

QString DatabaseTask::taskId() const
{
	return FTaskId;
}

bool DatabaseTask::isFinished() const
{
	return FFinished;
}

bool DatabaseTask::isFailed() const
{
	return !FError.isNull();
}

XmppError DatabaseTask::error() const
{
	return FError;
}

QString DatabaseTask::databaseConnection() const
{
	return QString("FileArchiveDatabase-%1").arg(FStreamJid.pBare());
}

void DatabaseTask::setSQLError(const QSqlError &AError)
{
	QString text = !AError.databaseText().isEmpty() ? AError.databaseText() : AError.driverText();
	FError = XmppError(IERR_FILEARCHIVE_DATABASE_EXEC_FAILED,text);
}

void DatabaseTask::addBindQueryValue(QSqlQuery &AQuery, const QVariant &AValue) const
{
	if (AValue.isNull())
		AQuery.addBindValue(QString(""));
	else
		AQuery.addBindValue(AValue);
}

void DatabaseTask::bindQueryValue(QSqlQuery &AQuery, const QString &APlaceHolder, const QVariant &AValue) const
{
	if (AValue.isNull())
		AQuery.bindValue(APlaceHolder,QString(""));
	else
		AQuery.bindValue(APlaceHolder,AValue);
}

// DatabaseTaskOpenDatabase
DatabaseTaskOpenDatabase::DatabaseTaskOpenDatabase(const Jid &AStreamJid, const QString &ADatabasePath) : DatabaseTask(AStreamJid,OpenDatabase)
{
	FDatabasePath = ADatabasePath;
}

QMap<QString,QString> DatabaseTaskOpenDatabase::databaseProperties() const
{
	return FProperties;
}

void DatabaseTaskOpenDatabase::run()
{
	QString connection = databaseConnection();
	if (!QSqlDatabase::contains(connection))
	{
		bool initialized = false;

		// QSqlDatabase should be destroyed before removing connection
		{
			QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",connection);
			db.setDatabaseName(FDatabasePath);

			if (!db.isValid())
				FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_CREATED,db.lastError().driverText());
			else if (!db.open())
				FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED,db.lastError().driverText());
			else if (!initializeDatabase(db))
				db.close();
			else
				initialized = true;
		}

		if (!initialized)
			QSqlDatabase::removeDatabase(connection);
	}
}

bool DatabaseTaskOpenDatabase::initializeDatabase(QSqlDatabase &ADatabase)
{
	QSqlQuery query(ADatabase);

	if (ADatabase.tables().contains("properties"))
	{
		if (query.exec("SELECT property, value FROM properties"))
		{
			while (query.next())
				FProperties.insert(query.value(0).toString(),query.value(1).toString());
		}
		else
		{
			setSQLError(query.lastError());
			return false;
		}
	}

	int structureVersion = FProperties.value(FADP_STRUCTURE_VERSION).toInt();
	int compatibleVersion = FProperties.value(FADP_COMPATIBLE_VERSION).toInt();
	if (structureVersion < DATABASE_STRUCTURE_VERSION)
	{
		static const struct { QString createQuery; int compatible; } databaseUpdates[] = 
		{
			{
				"CREATE TABLE properties ("
				"  property         TEXT NOT NULL,"
				"  value            TEXT"
				");"

				"CREATE TABLE headers ("
				"  with_node        TEXT,"
				"  with_domain      TEXT NOT NULL,"
				"  with_resource    TEXT,"
				"  start            DATETIME NOT NULL,"
				"  subject          TEXT,"
				"  thread           TEXT,"
				"  version          INTEGER NOT NULL,"
				"  gateway          TEXT,"
				"  timestamp        DATETIME NOT NULL"
				");"

				"CREATE TABLE modifications ("
				"  id               INTEGER PRIMARY KEY,"
				"  timestamp        DATETIME NOT NULL,"
				"  action           INTEGER NOT NULL,"
				"  with             TEXT NOT NULL,"
				"  start            DATETIME NOT NULL,"
				"  version          INTEGER NOT NULL"
				");"

				"CREATE UNIQUE INDEX properties_property ON properties ("
				"  property         ASC"
				");"

				"CREATE UNIQUE INDEX headers_with_start ON headers ("
				"  with_node        ASC,"
				"  with_domain      ASC,"
				"  with_resource    ASC,"
				"  start            DESC"
				");"

				"CREATE INDEX headers_start ON headers ("
				"  start            DESC"
				");"

				"CREATE INDEX modifications_timestamp ON modifications ("
				"  timestamp        ASC"
				");"

				"CREATE UNIQUE INDEX modifications_start_with ON modifications ("
				"  start            ASC,"
				"  with             ASC"
				");"

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
					setSQLError(createQuery.lastError());
					ADatabase.rollback();
					return false;
				}
			}
		}
		ADatabase.commit();

		FProperties.insert(FADP_STRUCTURE_VERSION,QString::number(DATABASE_STRUCTURE_VERSION));
		FProperties.insert(FADP_COMPATIBLE_VERSION,QString::number(databaseUpdates[DATABASE_STRUCTURE_VERSION-1].compatible));
	}
	else if (compatibleVersion > DATABASE_STRUCTURE_VERSION)
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_COMPATIBLE);
		return false;
	}

	return true;
}

// DatabaseTaskCloseDatabase
DatabaseTaskCloseDatabase::DatabaseTaskCloseDatabase(const Jid &AStreamJid) : DatabaseTask(AStreamJid,CloseDatabase)
{

}

void DatabaseTaskCloseDatabase::run()
{
	QString connection = databaseConnection();
	if (QSqlDatabase::contains(connection))
		QSqlDatabase::removeDatabase(connection);
}

// DatabaseTaskSetProperty
DatabaseTaskSetProperty::DatabaseTaskSetProperty(const Jid &AStreamJid, const QString &AProperty, const QString &AValue) : DatabaseTask(AStreamJid,SetProperty)
{
	FValue = AValue;
	FProperty = AProperty;
}

QString DatabaseTaskSetProperty::property() const
{
	return FProperty;
}

QString DatabaseTaskSetProperty::value() const
{
	return FValue;
}

void DatabaseTaskSetProperty::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		QSqlQuery updateQuery(db);
		if (!updateQuery.prepare("UPDATE properties SET value=:value WHERE property=:property"))
		{
			setSQLError(updateQuery.lastError());
		}
		else
		{
			bindQueryValue(updateQuery,":property",FProperty);
			bindQueryValue(updateQuery,":value",FValue);

			if (!updateQuery.exec())
			{
				setSQLError(updateQuery.lastError());
			}
			else if (updateQuery.numRowsAffected() <= 0)
			{
				QSqlQuery insertQuery(db);
				if (!insertQuery.prepare("INSERT INTO properties (property, value) VALUES (:property, :value)"))
				{
					setSQLError(insertQuery.lastError());
				}
				else
				{
					bindQueryValue(insertQuery,":property",FProperty);
					bindQueryValue(insertQuery,":value",FValue);

					if (!insertQuery.exec())
						setSQLError(insertQuery.lastError());
				}
			}
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseTaskLoadHeaders
DatabaseTaskLoadHeaders::DatabaseTaskLoadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest, const QString &AGateType) : DatabaseTask(AStreamJid,LoadHeaders)
{
	FRequest = ARequest;
	FGateType = AGateType;
}

IArchiveRequest DatabaseTaskLoadHeaders::request() const
{
	return FRequest;
}

QList<DatabaseArchiveHeader> DatabaseTaskLoadHeaders::headers() const
{
	return FHeaders;
}

void DatabaseTaskLoadHeaders::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		if (!FRequest.openOnly)
		{
			QString command = "SELECT with_node, with_domain, with_resource, start, subject, thread, version, gateway, timestamp FROM headers";

			QStringList conditions;
			QVariantList bindValues;

			if (!FRequest.with.node().isEmpty())
			{
				conditions.append("(with_node = ?)");
				bindValues.append(FRequest.with.pNode());
			}
			else if (FRequest.exactmatch)
			{
				conditions.append("(with_node = '')");
			}
			if (!FGateType.isEmpty())
			{
				conditions.append("(gateway = ?)");
				bindValues.append(FGateType);
			}
			else if (!FRequest.with.domain().isEmpty())
			{
				conditions.append("(with_domain = ?)");
				bindValues.append(FRequest.with.pDomain());
			}
			else if (FRequest.exactmatch)
			{
				conditions.append("(with_domain = '')");
			}
			if (!FRequest.with.resource().isEmpty())
			{
				conditions.append("(with_resource = ?)");
				bindValues.append(FRequest.with.pResource());
			}
			else if (FRequest.exactmatch)
			{
				conditions.append("(with_resource = '')");
			}

			if (FRequest.start.isValid())
			{
				conditions.append("(start >= ?)");
				bindValues.append(DateTime(FRequest.start).toX85UTC());
			}
			if (FRequest.end.isValid())
			{
				conditions.append("(start <= ?)");
				bindValues.append(DateTime(FRequest.end).toX85UTC());
			}

			if (!FRequest.threadId.isEmpty())
			{
				conditions.append("(thread = ?)");
				bindValues.append(FRequest.threadId);
			}

			if (!conditions.isEmpty())
			{
				command += QString (" WHERE %1").arg(conditions.join(" AND "));
			}

			command += QString(" ORDER BY start %1").arg(FRequest.order==Qt::AscendingOrder ? "ASC" : "DESC");

			command += QString(" LIMIT %1").arg(FRequest.maxItems);

			QSqlQuery selectQuery(db);
			if (!selectQuery.prepare(command))
			{
				setSQLError(selectQuery.lastError());
			}
			else
			{
				foreach(const QVariant &value, bindValues)
					addBindQueryValue(selectQuery,value);

				if (!selectQuery.exec())
				{
					setSQLError(selectQuery.lastError());
				}
				else while (selectQuery.next())
				{
					DatabaseArchiveHeader header;
					header.engineId = FILEMESSAGEARCHIVE_UUID;
					header.with = Jid(selectQuery.value(0).toString(),selectQuery.value(1).toString(),selectQuery.value(2).toString());
					header.start = DateTime(selectQuery.value(3).toString()).toLocal();
					header.subject = selectQuery.value(4).toString();
					header.threadId = selectQuery.value(5).toString();
					header.version = selectQuery.value(6).toInt();
					header.gateway = selectQuery.value(7).toString();
					header.timestamp = DateTime(selectQuery.value(8).toString()).toLocal();
					FHeaders.append(header);
				}
			}
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseTaskInsertHeaders
DatabaseTaskInsertHeaders::DatabaseTaskInsertHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders, const QString &AGateType) : DatabaseTask(AStreamJid,InsertHeaders)
{
	FHeaders = AHeaders;
	FGateType = AGateType;
}

QList<IArchiveHeader> DatabaseTaskInsertHeaders::headers() const
{
	return FHeaders;
}

void DatabaseTaskInsertHeaders::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		QSqlQuery insertQuery(db);
		QSqlQuery modifyQuery(db);
		if (!insertQuery.prepare(
			"INSERT INTO headers (with_node, with_domain, with_resource, start, subject, thread, version, gateway, timestamp) " 
			"VALUES (:with_n, :with_d, :with_r, :start, :subject, :thread, :version, :gateway, :timestamp)"))
		{
			setSQLError(insertQuery.lastError());
		}
		else if (!modifyQuery.prepare(
			"INSERT OR REPLACE INTO modifications (timestamp, action, with, start, version) "
			"VALUES (:timestamp, :action, :with, :start, :version)"))
		{
			setSQLError(modifyQuery.lastError());
		}
		else if (!FHeaders.isEmpty())
		{
			db.transaction();
			foreach(const IArchiveHeader &header, FHeaders)
			{
				QString timestamp = DateTime(QDateTime::currentDateTime()).toX85UTC();

				bindQueryValue(insertQuery,":with_n",header.with.pNode());
				bindQueryValue(insertQuery,":with_d",header.with.pDomain());
				bindQueryValue(insertQuery,":with_r",header.with.pResource());
				bindQueryValue(insertQuery,":start",DateTime(header.start).toX85UTC());
				bindQueryValue(insertQuery,":subject",header.subject);
				bindQueryValue(insertQuery,":thread",header.threadId);
				bindQueryValue(insertQuery,":version",header.version);
				bindQueryValue(insertQuery,":gateway",FGateType);
				bindQueryValue(insertQuery,":timestamp",timestamp);

				bindQueryValue(modifyQuery,":timestamp",timestamp);
				bindQueryValue(modifyQuery,":action",IArchiveModification::Changed);
				bindQueryValue(modifyQuery,":with",header.with.pFull());
				bindQueryValue(modifyQuery,":start",DateTime(header.start).toX85UTC());
				bindQueryValue(modifyQuery,":version",header.version);

				if (!insertQuery.exec())
				{
					setSQLError(insertQuery.lastError());
					db.rollback();
					return;
				}
				else if (!modifyQuery.exec())
				{
					setSQLError(modifyQuery.lastError());
					db.rollback();
					return;
				}
			}
			db.commit();
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseTaskUpdateHeaders
DatabaseTaskUpdateHeaders::DatabaseTaskUpdateHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders, bool AInsertIfNotExist, const QString &AGateType) : DatabaseTask(AStreamJid,UpdateHeaders)
{
	FHeaders = AHeaders;
	FGateType = AGateType;
	FInsertIfNotExist = AInsertIfNotExist;
}

QList<IArchiveHeader> DatabaseTaskUpdateHeaders::headers() const
{
	return FHeaders;
}

void DatabaseTaskUpdateHeaders::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		QSqlQuery updateQuery(db);
		QSqlQuery insertQuery(db);
		QSqlQuery modifyQuery(db);
		if (!updateQuery.prepare(
			"UPDATE headers SET subject=:subject, thread=:thread, version=:version, timestamp=:timestamp " 
			"WHERE with_node=:with_n AND with_domain=:with_d AND with_resource=:with_r AND start=:start"))
		{
			setSQLError(updateQuery.lastError());
		}
		else if (!insertQuery.prepare(
			"INSERT INTO headers (with_node, with_domain, with_resource, start, subject, thread, version, gateway, timestamp) " 
			"VALUES (:with_n, :with_d, :with_r, :start, :subject, :thread, :version, :gateway, :timestamp)"))
		{
			setSQLError(insertQuery.lastError());
		}
		else if (!modifyQuery.prepare(
			"INSERT OR REPLACE INTO modifications (timestamp, action, with, start, version) "
			"VALUES (:timestamp, :action, :with, :start, :version)"))
		{
			setSQLError(updateQuery.lastError());
		}
		else if (!FHeaders.isEmpty())
		{
			db.transaction();
			foreach(const IArchiveHeader &header, FHeaders)
			{
				QString timestamp = DateTime(QDateTime::currentDateTime()).toX85UTC();

				bindQueryValue(updateQuery,":subject",header.subject);
				bindQueryValue(updateQuery,":thread",header.threadId);
				bindQueryValue(updateQuery,":version",header.version);
				bindQueryValue(updateQuery,":timestamp",timestamp);
				bindQueryValue(updateQuery,":with_n",header.with.pNode());
				bindQueryValue(updateQuery,":with_d",header.with.pDomain());
				bindQueryValue(updateQuery,":with_r",header.with.pResource());
				bindQueryValue(updateQuery,":start",DateTime(header.start).toX85UTC());

				bindQueryValue(modifyQuery,":timestamp",timestamp);
				bindQueryValue(modifyQuery,":action",IArchiveModification::Changed);
				bindQueryValue(modifyQuery,":with",header.with.pFull());
				bindQueryValue(modifyQuery,":start",DateTime(header.start).toX85UTC());
				bindQueryValue(modifyQuery,":version",header.version);

				if (!updateQuery.exec())
				{
					setSQLError(updateQuery.lastError());
					db.rollback();
					return;
				}
				else if (updateQuery.numRowsAffected() <= 0)
				{
					if (FInsertIfNotExist)
					{
						bindQueryValue(insertQuery,":with_n",header.with.pNode());
						bindQueryValue(insertQuery,":with_d",header.with.pDomain());
						bindQueryValue(insertQuery,":with_r",header.with.pResource());
						bindQueryValue(insertQuery,":start",DateTime(header.start).toX85UTC());
						bindQueryValue(insertQuery,":subject",header.subject);
						bindQueryValue(insertQuery,":thread",header.threadId);
						bindQueryValue(insertQuery,":version",header.version);
						bindQueryValue(insertQuery,":gateway",FGateType);
						bindQueryValue(insertQuery,":timestamp",timestamp);

						if (!insertQuery.exec())
						{
							setSQLError(insertQuery.lastError());
							db.rollback();
							return;
						}
						else if (!modifyQuery.exec())
						{
							setSQLError(modifyQuery.lastError());
							db.rollback();
							return;
						}
					}	
					else
					{
						FError = XmppError(IERR_FILEARCHIVE_DATABASE_EXEC_FAILED);
						db.rollback();
						return;
					}
				}
				else if (!modifyQuery.exec())
				{
					setSQLError(modifyQuery.lastError());
					db.rollback();
					return;
				}
			}
			db.commit();
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseTaskRemoveHeaders
DatabaseTaskRemoveHeaders::DatabaseTaskRemoveHeaders(const Jid &AStreamJid, const QList<IArchiveHeader> &AHeaders) : DatabaseTask(AStreamJid,RemoveHeaders)
{
	FHeaders = AHeaders;
}

QList<IArchiveHeader> DatabaseTaskRemoveHeaders::headers() const
{
	return FHeaders;
}

void DatabaseTaskRemoveHeaders::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		QSqlQuery removeQuery(db);
		QSqlQuery modifyQuery(db);
		if (!removeQuery.prepare(
			"DELETE FROM headers WHERE with_node=:with_n AND with_domain=:with_d AND with_resource=:with_r AND start=:start"))
		{
			setSQLError(removeQuery.lastError());
		}
		else if (!modifyQuery.prepare(
			"INSERT OR REPLACE INTO modifications (timestamp, action, with, start, version) "
			"VALUES (:timestamp, :action, :with, :start, :version)"))
		{
			setSQLError(removeQuery.lastError());
		}
		else if (!FHeaders.isEmpty())
		{
			db.transaction();
			foreach(const IArchiveHeader &header, FHeaders)
			{
				bindQueryValue(removeQuery,":with_n",header.with.pNode());
				bindQueryValue(removeQuery,":with_d",header.with.pDomain());
				bindQueryValue(removeQuery,":with_r",header.with.pResource());
				bindQueryValue(removeQuery,":start",DateTime(header.start).toX85UTC());

				bindQueryValue(modifyQuery,":timestamp",DateTime(QDateTime::currentDateTime()).toX85UTC());
				bindQueryValue(modifyQuery,":action",IArchiveModification::Removed);
				bindQueryValue(modifyQuery,":with",header.with.pFull());
				bindQueryValue(modifyQuery,":start",DateTime(header.start).toX85UTC());
				bindQueryValue(modifyQuery,":version",header.version);

				if (!removeQuery.exec())
				{
					setSQLError(removeQuery.lastError());
					db.rollback();
					return;
				}
				else if (removeQuery.numRowsAffected()>0 && !modifyQuery.exec())
				{
					setSQLError(modifyQuery.lastError());
					db.rollback();
					return;
				}
			}
			db.commit();
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseTaskLoadModifications
DatabaseTaskLoadModifications::DatabaseTaskLoadModifications(const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef) : DatabaseTask(AStreamJid,LoadModifications)
{
	FStart = AStart;
	FCount = ACount;
	FNextRef = ANextRef;
}

IArchiveModifications DatabaseTaskLoadModifications::modifications() const
{
	return FModifications;
}

void DatabaseTaskLoadModifications::run()
{
	QSqlDatabase db = QSqlDatabase::database(databaseConnection());
	if (db.isOpen())
	{
		QSqlQuery loadQuery(db);
		if (!loadQuery.prepare("SELECT id, action, with, start, version FROM modifications WHERE id>? AND timestamp>? ORDER BY id LIMIT ?"))
		{
			setSQLError(loadQuery.lastError());
		}
		else
		{
			addBindQueryValue(loadQuery,FNextRef.isEmpty() ? 0 : FNextRef.toInt());
			addBindQueryValue(loadQuery,DateTime(FStart).toX85UTC());
			addBindQueryValue(loadQuery,FCount);

			QDateTime requestTime = QDateTime::currentDateTime();
			if (!loadQuery.exec())
			{
				setSQLError(loadQuery.lastError());
			}
			else while (loadQuery.next())
			{
				IArchiveModification item;
				item.action = (IArchiveModification::ModifyAction)loadQuery.value(1).toInt();
				item.header.engineId = FILEMESSAGEARCHIVE_UUID;
				item.header.with = loadQuery.value(2).toString();
				item.header.start = DateTime(loadQuery.value(3).toString()).toLocal();
				item.header.version = loadQuery.value(4).toInt();
				FModifications.items.append(item);

				FModifications.next = loadQuery.value(0).toString();
			}

			FModifications.isValid = !isFailed();
			FModifications.start = !FModifications.items.isEmpty() ? FStart : requestTime;
		}
	}
	else
	{
		FError = XmppError(IERR_FILEARCHIVE_DATABASE_NOT_OPENED);
	}
}

// DatabaseWorker
DatabaseWorker::DatabaseWorker(QObject *AParent) : QThread(AParent)
{
	FQuit = false;
	qRegisterMetaType<DatabaseTask *>("DatabaseTask *");
}

DatabaseWorker::~DatabaseWorker()
{
	quit();
	wait();
}

bool DatabaseWorker::execTask(DatabaseTask *ATask)
{
	QMutexLocker locker(&FMutex);
	if (!FQuit)
	{
		ATask->FAsync = false;
		FTasks.enqueue(ATask);
		FTaskReady.wakeAll();

		while (FTaskFinish.wait(locker.mutex()))
		{
			if (ATask->isFinished())
				return true;
		}
	}
	return false;
}

bool DatabaseWorker::startTask(DatabaseTask *ATask)
{
	QMutexLocker locker(&FMutex);
	if (!FQuit)
	{
		ATask->FAsync = true;
		FTasks.enqueue(ATask);
		FTaskReady.wakeAll();
		return true;
	}
	delete ATask;
	return false;
}

void DatabaseWorker::run()
{
	QMutexLocker locker(&FMutex);
	while (!FQuit || !FTasks.isEmpty())
	{
		DatabaseTask *task = !FTasks.isEmpty() ? FTasks.dequeue() : NULL;
		if (task)
		{
			locker.unlock();
			
			task->run();
			task->FFinished = true;
			if (!task->FAsync)
				FTaskFinish.wakeAll();
			else
				QMetaObject::invokeMethod(this,"taskFinished",Qt::QueuedConnection,Q_ARG(DatabaseTask *,task));

			locker.relock();
		}
		else
		{
			FTaskReady.wait(locker.mutex());
		}
	}
}

void DatabaseWorker::quit()
{
	QMutexLocker locker(&FMutex);
	FQuit = true;
	FTaskReady.wakeAll();
}
