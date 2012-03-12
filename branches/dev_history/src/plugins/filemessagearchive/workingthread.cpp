#include "workingthread.h"

#include <QMutexLocker>

uint WorkingThread::FWorkIndex = 0;

WorkingThread::WorkingThread(IFileMessageArchive *AFileArchive, IMessageArchiver *AMessageArchiver, QObject *AParent) : QThread(AParent)
{
	FHasError = false;
	FFileArchive = AFileArchive;
	FArchiver = AMessageArchiver;
	FWorkId = QString("work_%1").arg(++FWorkIndex);
}

WorkingThread::~WorkingThread()
{

}

QString WorkingThread::workId() const
{
	return FWorkId;
}

int WorkingThread::workAction() const
{
	return FAction;
}

bool WorkingThread::hasError() const
{
	return FHasError;
}

QString WorkingThread::errorString() const
{
	return FErrorString;
}

void WorkingThread::setErrorString(const QString &AError)
{
	FHasError = !AError.isNull();
	FErrorString = AError;
}

Jid WorkingThread::streamJid() const
{
	return FStreamJid;
}

void WorkingThread::setStreamJid(const Jid &AStreamJid)
{
	FStreamJid = AStreamJid;
}

IArchiveHeader WorkingThread::archiveHeader() const
{
	return FHeader;
}

void WorkingThread::setArchiveHeader(const IArchiveHeader &AHeader)
{
	FHeader = AHeader;
}

QList<IArchiveHeader> WorkingThread::archiveHeaders() const
{
	return FHeaders;
}

void WorkingThread::setArchiveHeaders(const QList<IArchiveHeader> &AHeaders)
{
	FHeaders = AHeaders;
}

IArchiveCollection WorkingThread::archiveCollection() const
{
	return FCollection;
}

void WorkingThread::setArchiveCollection(const IArchiveCollection &ACollection)
{
	FCollection = ACollection;
}

IArchiveRequest WorkingThread::archiveRequest() const
{
	return FRequest;
}

void WorkingThread::setArchiveRequest(const IArchiveRequest &ARequest)
{
	FRequest = ARequest;
}

int WorkingThread::modificationsCount() const
{
	return FModificationsCount;
}

void WorkingThread::setModificationsCount(int ACount)
{
	FModificationsCount = ACount;
}

QDateTime WorkingThread::modificationsStart() const
{
	return FModificationsStart;
}

void WorkingThread::setModificationsStart(const QDateTime &AStart)
{
	FModificationsStart = AStart;
}

IArchiveModifications WorkingThread::archiveModifications() const
{
	return FModifications;
}

void WorkingThread::setArchiveModifications(const IArchiveModifications &AModifs)
{
	FModifications = AModifs;
}

QString WorkingThread::executeAction(int AAction)
{
	if (!isRunning())
	{
		FAction = AAction;
		if (FAction == SaveCollection)
			FItemPrefs = FArchiver->archiveItemPrefs(FStreamJid,FCollection.header.with,FCollection.header.threadId);
		start();
		return workId();
	}
	return QString::null;
}

void WorkingThread::run()
{
	if (FAction == SaveCollection)
	{
		if (!FFileArchive->saveCollectionToFile(FStreamJid,FCollection,FItemPrefs.save))
			setErrorString(tr("Failed to save archive collection"));
	}
	else if (FAction == RemoveCollection)
	{
		foreach(QString file, FFileArchive->findCollectionFiles(FStreamJid,FRequest))
		{
			IArchiveHeader header = FFileArchive->loadHeaderFromFile(file);
			if (!FFileArchive->removeCollectionFile(FStreamJid,header.with,header.start))
				setErrorString(tr("Failed to remove collection file"));
		}
	}
	else if (FAction == LoadHeaders)
	{
		FHeaders.clear();
		foreach(QString file, FFileArchive->findCollectionFiles(FStreamJid,FRequest))
			FHeaders.append(FFileArchive->loadHeaderFromFile(file));
	}
	else if (FAction == LoadCollection)
	{
		QString file = FFileArchive->collectionFilePath(FStreamJid,FHeader.with,FHeader.start);
		FCollection = FFileArchive->loadCollectionFromFile(file);
		if (!FCollection.header.with.isValid() || !FCollection.header.start.isValid())
			setErrorString(tr("Failed to load collection from file"));
	}
	else if (FAction == LoadModifications)
	{
		FModifications = FFileArchive->loadFileModifications(FStreamJid,FModificationsStart,FModificationsCount);
	}
	else
	{
		setErrorString(tr("Internal error"));
	}
}
