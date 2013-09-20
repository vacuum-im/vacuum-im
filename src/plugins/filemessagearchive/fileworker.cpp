#include "fileworker.h"

#include <QMetaObject>
#include <definitions/internalerrors.h>

#define THREAD_WAIT_TIME   10000

// FileTask
quint32 FileTask::FTaskCount = 0;
FileTask::FileTask(IFileMessageArchive *AArchive, const Jid &AStreamJid, Type AType)
{
	FType = AType;
	FFileArchive = AArchive;
	FStreamJid = AStreamJid;
	FTaskId = QString("FileTask_%1").arg(++FTaskCount);
	setAutoDelete(false);
}

FileTask::~FileTask()
{

}

FileTask::Type FileTask::type() const
{
	return FType;
}

QString FileTask::taskId() const
{
	return FTaskId;
}

bool FileTask::isFailed() const
{
	return !FError.isNull();
}

XmppError FileTask::error() const
{
	return FError;
}

// FileTaskSaveCollection
FileTaskSaveCollection::FileTaskSaveCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode) : FileTask(AArchive,AStreamJid,SaveCollection)
{
	FSaveMode = ASaveMode;
	FCollection = ACollection;
}

IArchiveHeader FileTaskSaveCollection::archiveHeader() const
{
	return FCollection.header;
}

void FileTaskSaveCollection::run()
{
	if (!FFileArchive->saveFileCollection(FStreamJid,FCollection,FSaveMode))
		FError = XmppError(IERR_HISTORY_CONVERSATION_SAVE_ERROR);
}

// FileTaskLoadHeaders
FileTaskLoadHeaders::FileTaskLoadHeaders(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveRequest &ARequest) : FileTask(AArchive,AStreamJid,LoadHeaders)
{
	FRequest = ARequest;
}

QList<IArchiveHeader> FileTaskLoadHeaders::archiveHeaders() const
{
	return FHeaders;
}

void FileTaskLoadHeaders::run()
{
	if (FFileArchive->isDatabaseReady(FStreamJid))
		FHeaders = FFileArchive->loadDatabaseHeaders(FStreamJid,FRequest);
	else
		FHeaders = FFileArchive->loadFileHeaders(FStreamJid,FRequest);
}

// FileTaskLoadCollection
FileTaskLoadCollection::FileTaskLoadCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveHeader &AHeader) : FileTask(AArchive,AStreamJid,LoadCollection)
{
	FHeader = AHeader;
}

IArchiveCollection FileTaskLoadCollection::archiveCollection() const
{
	return FCollection;
}

void FileTaskLoadCollection::run()
{
	QString filePath = FFileArchive->collectionFilePath(FStreamJid,FHeader.with,FHeader.start);
	FCollection = FFileArchive->loadFileCollection(filePath);
	if (!FCollection.header.with.isValid() || !FCollection.header.start.isValid())
		FError = XmppError(IERR_HISTORY_CONVERSATION_LOAD_ERROR);
}

// FileTaskRemoveCollection
FileTaskRemoveCollection::FileTaskRemoveCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveRequest &ARequest) : FileTask(AArchive,AStreamJid,RemoveCollections)
{
	FRequest = ARequest;
}

IArchiveRequest FileTaskRemoveCollection::archiveRequest() const
{
	return FRequest;
}

void FileTaskRemoveCollection::run()
{
	FRequest.end = !FRequest.end.isValid() ? FRequest.start : FRequest.end;

	QList<IArchiveHeader> headers;
	if (FFileArchive->isDatabaseReady(FStreamJid))
		headers = FFileArchive->loadDatabaseHeaders(FStreamJid,FRequest); 
	else
		headers = FFileArchive->loadFileHeaders(FStreamJid,FRequest);

	foreach(const IArchiveHeader &header, headers)
	{
		if (!FFileArchive->removeFileCollection(FStreamJid,header.with,header.start))
			FError = XmppError(IERR_HISTORY_CONVERSATION_REMOVE_ERROR);
	}
}

// FileTaskLoadModifications
FileTaskLoadModifications::FileTaskLoadModifications(IFileMessageArchive *AArchive, const Jid &AStreamJid, const QDateTime &AStart, int ACount) : FileTask(AArchive,AStreamJid,LoadModifications)
{
	FCount = ACount;
	FStart = AStart;
}

IArchiveModifications FileTaskLoadModifications::archiveModifications() const
{
	return FModifications;
}

void FileTaskLoadModifications::run()
{
	if (FFileArchive->isDatabaseReady(FStreamJid))
		FModifications = FFileArchive->loadDatabaseModifications(FStreamJid,FStart,FCount);
	else
		FError = XmppError(IERR_HISTORY_MODIFICATIONS_LOAD_ERROR);
}

// FileWorker
FileWorker::FileWorker(QObject *AParent) : QThread(AParent)
{
	FQuit = false;
}

FileWorker::~FileWorker()
{
	quit();
	wait();
}

bool FileWorker::startTask(FileTask *ATask)
{
	QMutexLocker locker(&FMutex);
	if (!FQuit)
	{
		FTasks.enqueue(ATask);
		FTaskReady.wakeAll();
		start();
		return true;
	}
	return false;
}

void FileWorker::run()
{
	QMutexLocker locker(&FMutex);
	while (!FQuit && !FTasks.isEmpty())
	{
		FileTask *task = FTasks.dequeue();

		locker.unlock();
		task->run();
		QMetaObject::invokeMethod(this,"taskFinished",Qt::QueuedConnection,Q_ARG(FileTask *,task));
		locker.relock();

		if (FTasks.isEmpty())
			FTaskReady.wait(locker.mutex(),THREAD_WAIT_TIME);
	}
}

void FileWorker::quit()
{
	QMutexLocker locker(&FMutex);
	FQuit = true;
}
