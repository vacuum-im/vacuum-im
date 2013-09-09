#ifndef TRANSFERTHREAD_H
#define TRANSFERTHREAD_H

#include <QFile>
#include <QThread>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>

class TransferThread :
			public QThread
{
	Q_OBJECT;
public:
	TransferThread(IDataStreamSocket *ASocket, QFile *AFile, int AKind, qint64 ABytes, QObject *AParent);
	~TransferThread();
	void abort();
	bool isAborted() const;
signals:
	void transferProgress(qint64 ABytes);
protected:
	void run();
private:
	int FKind;
	QFile *FFile;
	qint64 FBytesToTransfer;
	IDataStreamSocket *FSocket;
private:
	volatile bool FAborted;
};

#endif // TRANSFERTHREAD_H
