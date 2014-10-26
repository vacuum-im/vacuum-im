#ifndef FILEWORKER_H
#define FILEWORKER_H

#include <QQueue>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <interfaces/ifilemessagearchive.h>
#include <utils/xmpperror.h>

class FileTask
{
	friend class FileWorker;
public:
	enum Type {
		SaveCollection,
		LoadHeaders,
		LoadCollection,
		RemoveCollections,
		LoadModifications
	};
public:
	FileTask(IFileMessageArchive *AArchive, const Jid &AStreamJid, Type AType);
	virtual ~FileTask();
	Type type() const;
	Jid streamJid() const;
	QString taskId() const;
	bool isFailed() const;
	XmppError error() const;
protected:
	virtual void run() = 0;
protected:
	Type FType;
	QString FTaskId;
	Jid FStreamJid;
	XmppError FError;
	IFileMessageArchive *FFileArchive;
private:
	static quint32 FTaskCount;
};

class FileTaskLoadHeaders :
	public FileTask
{
public:
	FileTaskLoadHeaders(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveRequest &ARequest);
	QList<IArchiveHeader> archiveHeaders() const;
protected:
	void run();
private:
	IArchiveRequest FRequest;
	QList<IArchiveHeader> FHeaders;
};

class FileTaskSaveCollection :
	public FileTask
{
public:
	FileTaskSaveCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveCollection &ACollection);
	IArchiveCollection archiveCollection() const;
protected:
	void run();
private:
	IArchiveCollection FCollection;
};

class FileTaskLoadCollection :
	public FileTask
{
public:
	FileTaskLoadCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveHeader &AHeader);
	IArchiveCollection archiveCollection() const;
protected:
	void run();
private:
	IArchiveHeader FHeader;
	IArchiveCollection FCollection;
};

class FileTaskRemoveCollection :
	public FileTask
{
public:
	FileTaskRemoveCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveRequest &ARequest);
	IArchiveRequest archiveRequest() const;
protected:
	void run();
private:
	IArchiveRequest FRequest;
};

class FileTaskLoadModifications :
	public FileTask
{
public:
	FileTaskLoadModifications(IFileMessageArchive *AArchive, const Jid &AStreamJid, const QDateTime &AStart, int ACount, const QString &ANextRef);
	IArchiveModifications archiveModifications() const;
protected:
	void run();
private:
	int FCount;
	QDateTime FStart;
	QString FNextRef;
	IArchiveModifications FModifications;
};

class FileWorker :
	public QThread
{
	Q_OBJECT;
public:
	FileWorker(QObject *AParent);
	~FileWorker();
	void quit();
	bool startTask(FileTask *ATask);
signals:
	void taskFinished(FileTask *ATask);
protected:
	void run();
private:
	bool FQuit;
	QMutex FMutex;
	QWaitCondition FTaskReady;
	QQueue<FileTask *> FTasks;
};

#endif // FILEWORKER_H
