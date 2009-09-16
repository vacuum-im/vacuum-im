#include "transferthread.h"

#include <QByteArray>

#define TRANSFER_BUFFER_SIZE      4096

TransferThread::TransferThread(IDataStreamSocket *ASocket, QFile *AFile, int AKind, qint64 ABytes, QObject *AParent) : QThread(AParent)
{
  FKind = AKind;
  FBytesToTransfer = ABytes;
  FFile = AFile;
  FSocket = ASocket;

  FAbort = false;
}

TransferThread::~TransferThread()
{
  FAbort = true;
  wait();
}

void TransferThread::abort()
{
  FAbort = true;
}

void TransferThread::run()
{
  qint64 transferedBytes = 0;
  char buffer[TRANSFER_BUFFER_SIZE];
  
  QIODevice *inDevice = FKind==IFileStream::SendFile ? FFile : FSocket->instance();
  QIODevice *outDevice =  FKind==IFileStream::SendFile ? FSocket->instance() : FFile;
  
  // QFile в Qt 4.5.1 не генерирует сигнала byteswritten(), приходится лепить костыль
  if (FKind == IFileStream::SendFile)
    connect(outDevice,SIGNAL(bytesWritten(qint64)),this,SIGNAL(transferProgress(qint64)));

  while (!FAbort && transferedBytes<FBytesToTransfer)
  {
    qint64 writtenBytes = 0;
    qint64 readedBytes = inDevice->read(buffer,qMin(qint64(TRANSFER_BUFFER_SIZE),FBytesToTransfer-transferedBytes));
    if (readedBytes > 0)
    {
      while (!FAbort && writtenBytes<readedBytes)
      {
        qint64 bytes = outDevice->write(buffer+writtenBytes, readedBytes-writtenBytes);
        if (bytes > 0)
        {
          transferedBytes += bytes;
          writtenBytes += bytes;
          if (FKind == IFileStream::ReceiveFile)
            emit transferProgress(bytes);
        }
        else if (bytes == 0)
        {
          outDevice->waitForBytesWritten(100);
        }
        else
        {
          break;
        }
      }
      if (writtenBytes < readedBytes)
        break;
    }
    else if (readedBytes == 0)
    {
      inDevice->waitForReadyRead(100);
    }
    else
    {
      break;
    }

  }

  if (FKind == IFileStream::SendFile)
  {
    while (!FAbort && outDevice->isOpen() && outDevice->bytesToWrite()>0)
    {
      FSocket->flush();
      outDevice->waitForBytesWritten(100);
    }
  }
  else 
  {
    FFile->flush();
  }

  FFile->close();
}
