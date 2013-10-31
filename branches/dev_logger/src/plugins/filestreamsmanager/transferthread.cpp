#include "transferthread.h"

#include <QByteArray>

#define TRANSFER_BUFFER_SIZE      51200

TransferThread::TransferThread(IDataStreamSocket *ASocket, QFile *AFile, int AKind, qint64 ABytes, QObject *AParent) : QThread(AParent)
{
	FKind = AKind;
	FFile = AFile;
	FSocket = ASocket;
	FBytesToTransfer = ABytes;

	FAborted = false;
}

TransferThread::~TransferThread()
{
	abort();
	wait();
}

void TransferThread::abort()
{
	FAborted = true;
}

bool TransferThread::isAborted() const
{
	return FAborted;
}

void TransferThread::run()
{
	qint64 transferedBytes = 0;
	char buffer[TRANSFER_BUFFER_SIZE];

	QIODevice *inDevice = FKind==IFileStream::SendFile ? FFile : FSocket->instance();
	QIODevice *outDevice = FKind==IFileStream::SendFile ? FSocket->instance() : FFile;

	while (!FAborted && transferedBytes<FBytesToTransfer)
	{
		qint64 readedBytes = inDevice->read(buffer,qMin(qint64(TRANSFER_BUFFER_SIZE),FBytesToTransfer-transferedBytes));
		if (readedBytes > 0)
		{
			qint64 writtenBytes = 0;
			while (!FAborted && writtenBytes<readedBytes)
			{
				qint64 bytes = outDevice->write(buffer+writtenBytes, readedBytes-writtenBytes);
				if (bytes > 0)
				{
					transferedBytes += bytes;
					writtenBytes += bytes;
					emit transferProgress(bytes);
				}
				else if (bytes == 0)
				{
					outDevice->waitForBytesWritten(100);
				}
				else
				{
					abort();
				}
			}
		}
		else if (readedBytes == 0)
		{
			inDevice->waitForReadyRead(100);
		}
		else
		{
			abort();
		}
	}

	while (FKind==IFileStream::SendFile && !FAborted && FSocket->flush())
		outDevice->waitForBytesWritten(100);

	FFile->close();
}
