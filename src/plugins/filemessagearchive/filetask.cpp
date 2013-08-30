#include "filetask.h"

#include <QMetaObject>

quint32 FileTask::FTaskCount = 0;

// FileTask
FileTask::FileTask(IFileMessageArchive *AArchive, const Jid &AStreamJid, Type AType)
{
	FType = AType;
	FFileArchive = AArchive;
	FStreamJid = AStreamJid;
	FTaskId = QString("task_%1").arg(++FTaskCount);

	setAutoDelete(false);
}

FileTask::Type FileTask::type() const
{
	return FType;
}

QString FileTask::taskId() const
{
	return FTaskId;
}

bool FileTask::hasError() const
{
	return !FError.isNull();
}

XmppError FileTask::error() const
{
	return FError;
}

void FileTask::emitFinished()
{
	QMetaObject::invokeMethod(FFileArchive->instance(),"onFileTaskFinished",Qt::QueuedConnection,Q_ARG(FileTask *,this));
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
	if (!FFileArchive->saveCollectionToFile(FStreamJid,FCollection,FSaveMode))
		FError = XmppError(IERR_HISTORY_CONVERSATION_SAVE_ERROR);
	emitFinished();
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
	foreach(QString file, FFileArchive->findCollectionFiles(FStreamJid,FRequest))
		FHeaders.append(FFileArchive->loadHeaderFromFile(file));
	emitFinished();
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
	QString file = FFileArchive->collectionFilePath(FStreamJid,FHeader.with,FHeader.start);
	FCollection = FFileArchive->loadCollectionFromFile(file);
	if (!FCollection.header.with.isValid() || !FCollection.header.start.isValid())
		FError = XmppError(IERR_HISTORY_CONVERSATION_LOAD_ERROR);
	emitFinished();
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
	foreach(QString file, FFileArchive->findCollectionFiles(FStreamJid,FRequest))
	{
		IArchiveHeader header = FFileArchive->loadHeaderFromFile(file);
		if (!FFileArchive->removeCollectionFile(FStreamJid,header.with,header.start))
			FError = XmppError(IERR_HISTORY_CONVERSATION_REMOVE_ERROR);
	}
	emitFinished();
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
	FModifications = FFileArchive->loadFileModifications(FStreamJid,FStart,FCount);
	emitFinished();
}
