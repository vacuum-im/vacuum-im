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
signals:
	void transferProgress(qint64 ABytes);
protected:
	void run();
private:
	bool FAbort;
	int FKind;
	qint64 FBytesToTransfer;
	QFile *FFile;
	IDataStreamSocket *FSocket;
};

#endif // TRANSFERTHREAD_H
