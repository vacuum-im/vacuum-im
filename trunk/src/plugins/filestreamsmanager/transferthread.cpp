#include "transferthread.h"

#include <QByteArray>

#define TRANSFER_BLOCK_SIZE     4096

TransferThread::TransferThread(IDataStreamSocket *ASocket, QFile *AFile, int AKind, qint64 ABytes, QObject *AParent) : QThread(AParent)
{
  FKind = AKind;
  FBytes = ABytes;
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
  qint64 bytes = 0;
  
  QIODevice *inDevice = FKind==IFileStream::SendFile ? FFile : FSocket->instance();
  QIODevice *outDevice =  FKind==IFileStream::SendFile ? FSocket->instance() : FFile;

  while (!FAbort && bytes<FBytes && FSocket->isOpen() && FFile->isOpen() && FFile->error()==QFile::NoError)
  {
    if (inDevice->bytesAvailable() > 0)
    {
      int writtenSize = 0;
      qint64 readSize = qMin(qint64(TRANSFER_BLOCK_SIZE),FBytes-bytes);
      QByteArray buffer = inDevice->read(readSize);
      while (!FAbort && writtenSize<buffer.size())
      {
        qint64 size = outDevice->write(buffer.constData()+writtenSize, buffer.size()-writtenSize);
        if (size > 0)
        {
          bytes += size;
          writtenSize += size;
          emit transferProgress(bytes);
        }
        else if (writtenSize == 0)
        {
          outDevice->waitForBytesWritten(100);
        }
        else
        {
          break;
        }
      }
    }
    else
    {
      inDevice->waitForReadyRead(100);
    }
    if (bytes >= FBytes)
    {
      if (FKind == IFileStream::SendFile)
        FSocket->flush();
      else
        FFile->flush();
    }
  }
  FFile->close();
}
