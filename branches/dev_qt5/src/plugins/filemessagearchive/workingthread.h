#ifndef WORKINGTHREAD_H
#define WORKINGTHREAD_H

#include <QThread>
#include <definitions/internalerrors.h>
#include <interfaces/ifilemessagearchive.h>
#include <utils/xmpperror.h>

class WorkingThread : 
	public QThread
{
	Q_OBJECT;
public:
	enum Actions {
		NoAction,
		SaveCollection,
		RemoveCollection,
		LoadHeaders,
		LoadCollection,
		LoadModifications
	};
public:
	WorkingThread(IFileMessageArchive *AFileArchive, IMessageArchiver *AMessageArchiver, QObject *AParent);
	~WorkingThread();
	QString workId() const;
	int workAction() const;
	bool hasError() const;
	XmppError error() const;
	void setError(const XmppError &AError);
	Jid streamJid() const;
	void setStreamJid(const Jid &AStreamJid);
	IArchiveHeader archiveHeader() const;
	void setArchiveHeader(const IArchiveHeader &AHeader);
	QList<IArchiveHeader> archiveHeaders() const;
	void setArchiveHeaders(const QList<IArchiveHeader> &AHeaders);
	IArchiveCollection archiveCollection() const;
	void setArchiveCollection(const IArchiveCollection &ACollection);
	IArchiveRequest archiveRequest() const;
	void setArchiveRequest(const IArchiveRequest &ARequest);
	int modificationsCount() const;
	void setModificationsCount(int ACount);
	QDateTime modificationsStart() const;
	void setModificationsStart(const QDateTime &AStart);
	IArchiveModifications archiveModifications() const;
	void setArchiveModifications(const IArchiveModifications &AModifs);
public:
	QString executeAction(int AAction);
protected:
	virtual void run();
private:
	IMessageArchiver *FArchiver;
	IFileMessageArchive *FFileArchive;
private:
	int FAction;
	QString FWorkId;
	XmppError FError;
	IArchiveItemPrefs FItemPrefs;
private:
	Jid FStreamJid;
	IArchiveHeader FHeader;
	IArchiveCollection FCollection;
	IArchiveRequest FRequest;
	QList<IArchiveHeader> FHeaders;
	int FModificationsCount;
	QDateTime FModificationsStart;
	IArchiveModifications FModifications;
private:
	static uint FWorkIndex;
};

#endif // WORKINGTHREAD_H
