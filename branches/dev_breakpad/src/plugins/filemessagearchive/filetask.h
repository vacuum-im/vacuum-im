#ifndef FILETASK_H
#define FILETASK_H

#include <QRunnable>
#include <definitions/internalerrors.h>
#include <interfaces/ifilemessagearchive.h>
#include <utils/xmpperror.h>

class FileTask : 
	public QRunnable
{
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
	Type type() const;
	QString taskId() const;
	bool hasError() const;
	XmppError error() const;
protected:
	void emitFinished();
protected:
	Type FType;
	QString FTaskId;
	Jid FStreamJid;
	XmppError FError;
	IFileMessageArchive *FFileArchive;
private:
	static quint32 FTaskCount;
};


class FileTaskSaveCollection :
	public FileTask
{
public:
	FileTaskSaveCollection(IFileMessageArchive *AArchive, const Jid &AStreamJid, const IArchiveCollection &ACollection, const QString &ASaveMode);
	IArchiveHeader archiveHeader() const;
protected:
	void run();
private:
	QString FSaveMode;
	IArchiveCollection FCollection;
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
	FileTaskLoadModifications(IFileMessageArchive *AArchive, const Jid &AStreamJid, const QDateTime &AStart, int ACount);
	IArchiveModifications archiveModifications() const;
protected:
	void run();
private:
	int FCount;
	QDateTime FStart;
	IArchiveModifications FModifications;
};

#endif // FILETASK_H
