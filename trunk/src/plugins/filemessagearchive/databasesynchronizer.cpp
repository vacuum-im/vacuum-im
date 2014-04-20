#include "databasesynchronizer.h"

#include <QMutexLocker>
#include <QDirIterator>
#include <definitions/statisticsparams.h>
#include <utils/logger.h>

DatabaseSynchronizer::DatabaseSynchronizer(IFileMessageArchive *AFileArchive, DatabaseWorker *ADatabaseWorker, QObject *AParent) : QThread(AParent)
{
	FQuit = false;
	FFileArchive = AFileArchive;
	FDatabaseWorker = ADatabaseWorker;
}

DatabaseSynchronizer::~DatabaseSynchronizer()
{
	quit();
	wait();
}

void DatabaseSynchronizer::startSync(const Jid &AStreamJid)
{
	QMutexLocker locker(&FMutex);
	if (!FStreams.contains(AStreamJid))
	{
		FStreams.enqueue(AStreamJid);
		start();
	}
}

void DatabaseSynchronizer::removeSync(const Jid &AStreamJid)
{
	QMutexLocker locker(&FMutex);
	FStreams.removeAll(AStreamJid);
}

void DatabaseSynchronizer::quit()
{
	QMutexLocker locker(&FMutex);
	FQuit = true;
}

void DatabaseSynchronizer::run()
{
	QMutexLocker locker(&FMutex);
	while (!FQuit && !FStreams.isEmpty())
	{
		Jid streamJid = FStreams.dequeue();
		locker.unlock();

		Logger::startTiming(STMP_HISTORY_FILE_DATABASE_SYNC);

		int loadHeaders = 0;
		int fileHeaders = 0;
		int readHeaders = 0;
		int checkHeaders = 0;
		int corruptHeaders = 0;
		int invalidHeaders = 0;
		int insertHeaders = 0;
		int updateHeaders = 0;
		int removeHeaders = 0;

		bool syncFailed = false;
		QDateTime syncTime = QDateTime::currentDateTime();

		QString archivePath = FFileArchive->fileArchivePath(streamJid);
		if (!archivePath.isEmpty())
		{
			IArchiveRequest loadRequest;
			QHash<Jid, QList<QString> > databaseHeadersMap;
			QHash<QString, DatabaseArchiveHeader> databaseFileHeaders;
			DatabaseTaskLoadHeaders *loadTask = new DatabaseTaskLoadHeaders(streamJid,loadRequest,QString::null);
			if (FDatabaseWorker->execTask(loadTask) && !loadTask->isFailed())
			{
				foreach(const DatabaseArchiveHeader &header, loadTask->headers())
				{
					if (header.timestamp < syncTime)
					{
						QString fileName = (FFileArchive->collectionDirName(header.with)+"/"+FFileArchive->collectionFileName(header.start)).toLower();
						databaseHeadersMap[header.with].append(fileName);
						databaseFileHeaders.insert(fileName,header);
					}
					loadHeaders++;
				}
			}
			else
			{
				syncFailed = true;
				REPORT_ERROR(QString("Failed to synchronize file archive database: Database headers not loaded - %1").arg(loadTask->error().errorMessage()));
			}
			delete loadTask;


			QHash<Jid, QList<IArchiveHeader> > fileHeadersMap;
			QDirIterator bareIt(archivePath,QDir::Dirs|QDir::NoDotAndDotDot);
			while (!FQuit && !syncFailed && bareIt.hasNext())
			{
				QDirIterator filesIt(bareIt.next(), QDir::Files, QDirIterator::Subdirectories);
				
				Jid bareWith = Jid::decode(bareIt.fileName());
				bool isGated = bareWith.pDomain().endsWith(".gateway");
				int pathLength = bareIt.filePath().length()-bareIt.fileName().length();

				while (filesIt.hasNext())
				{
					filesIt.next();
					QDateTime fileLastModified = filesIt.fileInfo().lastModified();
					if (fileLastModified < syncTime)
					{
						QString fileName = filesIt.filePath().mid(pathLength).toLower();
						QHash<QString, DatabaseArchiveHeader>::iterator dbHeaderIt = databaseFileHeaders.find(fileName);
						if (dbHeaderIt==databaseFileHeaders.end() || dbHeaderIt->timestamp<fileLastModified)
						{
							IArchiveHeader header = FFileArchive->loadFileHeader(filesIt.filePath());
							if (header.with.isValid() && header.start.isValid())
							{
								if (!isGated && header.with.pBare()==bareWith.pBare())
									fileHeadersMap[header.with].append(header);
								else if (isGated && header.with.pNode()==bareWith.pNode())
									fileHeadersMap[header.with].append(header);
								else
									invalidHeaders++;
							}
							else
							{
								corruptHeaders++;
							}
							readHeaders++;
						}
						else
						{
							databaseFileHeaders.erase(dbHeaderIt);
						}
					}
					fileHeaders++;
				}
			}


			for (QHash<Jid, QList<IArchiveHeader> >::iterator it=fileHeadersMap.begin(); !FQuit && !syncFailed && it!=fileHeadersMap.end(); ++it)
			{
				Jid with = it.key();

				QList<IArchiveHeader> newHeaders;
				QList<IArchiveHeader> difHeaders;
				QList<IArchiveHeader> oldHeaders;

				QList<IArchiveHeader> &fileHeaders = it.value();
				qSort(fileHeaders.begin(),fileHeaders.end());
				checkHeaders += fileHeaders.count();

				QList<IArchiveHeader> databaseHeaders;
				foreach(const QString &fileName, databaseHeadersMap.take(with))
				{
					QHash<QString, DatabaseArchiveHeader>::const_iterator dbHeaderIt = databaseFileHeaders.constFind(fileName);
					if (dbHeaderIt != databaseFileHeaders.constEnd())
						databaseHeaders.append(dbHeaderIt.value());
				}
				qSort(databaseHeaders.begin(),databaseHeaders.end());

				if (databaseHeaders.isEmpty())
				{
					newHeaders = fileHeaders;
				}
				else if (fileHeaders.isEmpty())
				{
					oldHeaders = databaseHeaders;
				}
				else while (!fileHeaders.isEmpty() || !databaseHeaders.isEmpty())
				{
					if (fileHeaders.isEmpty())
					{
						oldHeaders += databaseHeaders.takeFirst();
					}
					else if (databaseHeaders.isEmpty())
					{
						newHeaders += fileHeaders.takeFirst();
					}
					else if (fileHeaders.first() < databaseHeaders.first())
					{
						newHeaders += fileHeaders.takeFirst();
					}
					else if (databaseHeaders.first() < fileHeaders.first())
					{
						oldHeaders += databaseHeaders.takeFirst();
					}
					else if (fileHeaders.first().version != databaseHeaders.first().version)
					{
						difHeaders += fileHeaders.takeFirst();
						databaseHeaders.removeFirst();
					}
					else
					{
						fileHeaders.removeFirst();
						databaseHeaders.removeFirst();
					}
				}

				if (!syncFailed && !newHeaders.isEmpty())
				{
					QString gateType = !with.node().isEmpty() ? FFileArchive->contactGateType(with) : QString::null;
					DatabaseTaskInsertHeaders *insertTask = new DatabaseTaskInsertHeaders(streamJid,newHeaders,gateType);
					if (FDatabaseWorker->execTask(insertTask) & !insertTask->isFailed())
					{
						insertHeaders += newHeaders.count();
					}
					else
					{
						syncFailed = true;
						REPORT_ERROR(QString("Failed to synchronize file archive database: New headers not inserted - %1").arg(insertTask->error().errorMessage()));
					}
					delete insertTask;
				}

				if (!syncFailed && !difHeaders.isEmpty())
				{
					DatabaseTaskUpdateHeaders *updateTask = new DatabaseTaskUpdateHeaders(streamJid,difHeaders);
					if (FDatabaseWorker->execTask(updateTask) && !updateTask->isFailed())
					{
						updateHeaders += difHeaders.count();
					}
					else
					{
						syncFailed = true;
						REPORT_ERROR(QString("Failed to synchronize file archive database: Changed headers not updated - %1").arg(updateTask->error().errorMessage()));
					}
					delete updateTask;
				}

				if (!syncFailed && !oldHeaders.isEmpty())
				{
					DatabaseTaskRemoveHeaders *removeTask = new DatabaseTaskRemoveHeaders(streamJid,oldHeaders);
					if (FDatabaseWorker->execTask(removeTask) && !removeTask->isFailed())
					{
						removeHeaders += oldHeaders.count();
					}
					else
					{
						syncFailed = true;
						REPORT_ERROR(QString("Failed to synchronize file archive database: Old headers not removed - %1").arg(removeTask->error().errorMessage()));
					}
					delete removeTask;
				}
			}


			for (QHash<Jid, QList<QString> >::const_iterator it=databaseHeadersMap.constBegin(); !FQuit && !syncFailed && it!=databaseHeadersMap.constEnd(); ++it)
			{
				QList<IArchiveHeader> oldHeaders;
				foreach(const QString &fileName, it.value())
				{
					QHash<QString, DatabaseArchiveHeader>::const_iterator dbHeaderIt = databaseFileHeaders.constFind(fileName);
					if (dbHeaderIt != databaseFileHeaders.constEnd())
						oldHeaders.append(dbHeaderIt.value());
				}

				if (!oldHeaders.isEmpty())
				{
					DatabaseTaskRemoveHeaders *removeTask = new DatabaseTaskRemoveHeaders(streamJid,oldHeaders);
					if (FDatabaseWorker->execTask(removeTask) && !removeTask->isFailed())
					{
						removeHeaders += oldHeaders.count();
					}
					else
					{
						syncFailed = true;
						REPORT_ERROR(QString("Failed to synchronize file archive database: Old headers not removed - %1").arg(removeTask->error().errorMessage()));
					}
					delete removeTask;
				}
			}
		}
		else
		{
			syncFailed = true;
			REPORT_ERROR("Failed to synchronize file archive database: Archive path is empty");
		}

		if (!FQuit)
			QMetaObject::invokeMethod(this,"syncFinished",Qt::QueuedConnection,Q_ARG(const Jid &,streamJid),Q_ARG(bool,syncFailed));

		if (!syncFailed)
			REPORT_TIMING(STMP_HISTORY_FILE_DATABASE_SYNC,Logger::finishTiming(STMP_HISTORY_FILE_DATABASE_SYNC));

		locker.relock();
	}
}
