#include "filestream.h"

#include <QDir>
#include <QTimer>
#include <QFileInfo>

FileStream::FileStream(IDataStreamsManager *ADataManager, const QString &AStreamId, const Jid &AStreamJid, 
                       const Jid &AContactJid, int AKind, QObject *AParent) : QObject(AParent)
{
  FStreamId = AStreamId;
  FStreamJid = AStreamJid;
  FContactJid= AContactJid;
  FStreamKind = AKind;
  FDataManager = ADataManager;

  FThread = NULL;
  FSocket = NULL;

  FAborted = false;
  FProgress = 0;
  FFileSize = 0;
  FRangeOffset = 0;
  FRangeLength = 0;
  FRangeSupported = AKind==IFileStream::SendFile;
  FStreamState = IFileStream::Creating;
}

FileStream::~FileStream()
{
  if (FThread)
  {
    FThread->abort();
    FThread->wait();
    delete FThread;
  }
  if (FSocket)
  {
    delete FSocket->instance();
  }
  emit streamDestroyed();
}

QString FileStream::streamId() const
{
  return FStreamId;
}

Jid FileStream::streamJid() const
{
  return FStreamJid;
}

Jid FileStream::contactJid() const
{
  return FContactJid;
}

int FileStream::streamKind() const
{
  return FStreamKind;
}

int FileStream::streamState() const
{
  return FStreamState;
}

QString FileStream::methodNS() const
{
  return FMethodNS;
}

qint64 FileStream::speed() const
{
  if (FStreamState == Transfering)
  {
    qreal secs = (SPEED_INTERVAL * (SPEED_POINTS - 1)) / 1000;
    qreal bytes = 0;
    for (int i=0; i<SPEED_POINTS; i++)
      if (i!=FSpeedIndex)
        bytes += FSpeed[i];
    return qRound64(bytes/secs);
  }
  return 0;
}

qint64 FileStream::progress() const
{
  return FProgress;
}

QString FileStream::stateString() const
{
  return FStateString;
}

bool FileStream::isRangeSupported() const
{
  return FRangeSupported;
}

void FileStream::setRangeSupported(bool ASupported)
{
  if (FStreamState==Creating)
  {
    if (FRangeSupported!=ASupported)
    {
      FRangeSupported = ASupported;
      emit propertiesChanged();
    }
  }
}

qint64 FileStream::rangeOffset() const
{
  return FRangeOffset;
}

void FileStream::setRangeOffset(qint64 AOffset)
{
  if (FStreamState==Creating || FStreamState==Negotiating)
  {
    if (AOffset>=0 && FRangeOffset!=AOffset)
    {
      FRangeOffset = AOffset;
      emit propertiesChanged();
    }
  }
}

qint64 FileStream::rangeLength() const
{
  return FRangeLength;
}

void FileStream::setRangeLength(qint64 ALength)
{
  if (FStreamState==Creating || FStreamState==Negotiating)
  {
    if (ALength>=0 && FRangeLength!=ALength)
    {
      FRangeLength = ALength;
      emit propertiesChanged();
    }
  }
}

QString FileStream::fileName() const
{
  return FFileName;
}

void FileStream::setFileName(const QString &AFileName)
{
  if (FStreamState == Creating)
  {
    if (FFileName != AFileName)
    {
      if (FStreamKind == SendFile)
      {
        QFileInfo info(AFileName);
        FFileSize = info.size();
        FFileDate = info.lastModified();
      }
      FFileName = AFileName;
      emit propertiesChanged();
    }
  }
}

qint64 FileStream::fileSize() const
{
  return FFileSize;
}

void FileStream::setFileSize(qint64 AFileSize)
{
  if (FStreamState == Creating)
  {
    if (FFileSize != AFileSize)
    {
      if (FStreamKind == ReceiveFile)
      {
        FFileSize = AFileSize;
        emit propertiesChanged();
      }
    }
  }
}

QString FileStream::fileHash() const
{
  return FFileHash;
}

void FileStream::setFileHash(const QString &AFileHash)
{
  if (FStreamState == Creating)
  {
    if (FFileHash != AFileHash)
    {
      if (FStreamKind == ReceiveFile)
      {
        FFileHash = AFileHash;
        emit propertiesChanged();
      }
    }
  }
}
QDateTime FileStream::fileDate() const
{
  return FFileDate;
}

void FileStream::setFileDate(const QDateTime &ADate)
{
  if (FStreamState == Creating)
  {
    if (FFileDate != ADate)
    {
      if (FStreamKind == ReceiveFile)
      {
        FFileDate = ADate;
        emit propertiesChanged();
      }
    }
  }
}

QString FileStream::fileDescription() const
{
  return FFileDesc;
}

void FileStream::setFileDescription(const QString &AFileDesc)
{
  if (FFileDesc != AFileDesc)
  {
    FFileDesc = AFileDesc;
    emit propertiesChanged();
  }
}

QString FileStream::methodSettings() const
{
  return FMethodSettings;
}

void FileStream::setMethodSettings(const QString &ASettingsNS)
{
  if (FStreamState == Creating)
  {
    if (FMethodSettings != ASettingsNS)
    {
      FMethodSettings = ASettingsNS;
      emit propertiesChanged();
    }
  }
}

bool FileStream::initStream(const QList<QString> &AMethods)
{
  if (FStreamState==Creating && FStreamKind==SendFile)
  {
    if (!FFileName.isEmpty() && FFileSize>0)
    {
      if (FDataManager->initStream(FStreamJid,FContactJid,FStreamId,NS_SI_FILETRANSFER,AMethods))
      {
        setStreamState(Negotiating,tr("Waiting for a response to send a file request"));
        return true;
      }
    }
  }
  return false;
}

bool FileStream::startStream(const QString &AMethodNS)
{
  if (FStreamKind==SendFile && FStreamState==Negotiating)
  {
    if (openFile())
    {
      IDataStreamMethod *stremMethod = FDataManager->method(AMethodNS);
      FSocket = stremMethod!=NULL ? stremMethod->dataStreamSocket(FStreamId,FStreamJid,FContactJid,IDataStreamSocket::Initiator,this) : NULL;
      if (FSocket)
      {
        stremMethod->loadSettings(FSocket,FMethodSettings);
        setStreamState(Connecting,tr("Connecting"));
        connect(FSocket->instance(),SIGNAL(stateChanged(int)),SLOT(onSocketStateChanged(int)));
        if (FSocket->open(QIODevice::WriteOnly))
        {
          FMethodNS = AMethodNS;
          return true;
        }
      }
      FFile.close();
    }
  }
  else if (FStreamKind==ReceiveFile && FStreamState==Creating)
  {
    if (openFile())
    {
      if (FDataManager->acceptStream(FStreamId,AMethodNS))
      {
        IDataStreamMethod *stremMethod = FDataManager->method(AMethodNS);
        FSocket = stremMethod!=NULL ? stremMethod->dataStreamSocket(FStreamId,FStreamJid,FContactJid,IDataStreamSocket::Target,this) : NULL;
        if (FSocket)
        {
          stremMethod->loadSettings(FSocket,FMethodSettings);
          setStreamState(Connecting,tr("Connecting"));
          connect(FSocket->instance(),SIGNAL(stateChanged(int)),SLOT(onSocketStateChanged(int)));
          if (FSocket->open(QIODevice::ReadOnly))
          {
            FMethodNS = AMethodNS;
            return true;
          }
        }
      }
      FFile.close();
    }
  }
  return false;
}

void FileStream::abortStream(const QString &AError)
{
  if (FStreamState != Aborted)
  {
    if (!FAborted)
    {
      FAborted = true;
      FAbortString = AError;
    }
    if (FThread && FThread->isRunning())
    {
      FThread->abort();
    }
    else if (FSocket && FSocket->streamState()!=IDataStreamSocket::Closed)
    {
      FSocket->close();
    }
    else
    {
      if (FStreamKind==ReceiveFile && FStreamState==Creating)
        FDataManager->rejectStream(FStreamId,AError);
      setStreamState(Aborted,AError);
    }
  }
}

bool FileStream::openFile()
{
  if (!FFileName.isEmpty() && FFileSize>0)
  {
    QFileInfo finfo(FFileName);
    if (FStreamKind==IFileStream::ReceiveFile ? QDir::root().mkpath(finfo.absolutePath()) : true)
    {
      FFile.setFileName(FFileName);
      if (FFile.open(FStreamKind==IFileStream::SendFile ? QIODevice::ReadOnly : QIODevice::WriteOnly))
      {
        if (FRangeOffset==0 || FFile.seek(FRangeOffset))
          return true;
        if (FStreamKind == IFileStream::ReceiveFile)
          FFile.remove();
        FFile.close();
      }
    }
  }
  return false;
}

void FileStream::setStreamState(int AState, const QString &AMessage)
{
  if (FStreamState!=AState)
  {
    if (AState == Transfering)
    {
      FSpeedIndex = 0;
      memset(&FSpeed,0,sizeof(FSpeed));
      QTimer::singleShot(SPEED_INTERVAL,this,SLOT(onIncrementSpeedIndex()));
    }
    FStreamState = AState;
    FStateString = AMessage;
    emit stateChanged();
  }
}

void FileStream::onSocketStateChanged(int AState)
{
  if (AState == IDataStreamSocket::Opened)
  {
    if (FThread == NULL)
    {
      qint64 bytesForTransfer = FRangeLength>0 ? FRangeLength : FFileSize-FRangeOffset;
      FThread = new TransferThread(FSocket,&FFile,FStreamKind,bytesForTransfer,this);
      connect(FThread,SIGNAL(transferProgress(qint64)),SLOT(onTransferThreadProgress(qint64)));
      connect(FThread,SIGNAL(finished()),SLOT(onTransferThreadFinished()));
      setStreamState(Transfering,tr("Data transmission"));
      FThread->start();
    }
  }
  else if (AState == IDataStreamSocket::Closed)
  {
    if (FThread)
    {
      FThread->abort();
      FThread->wait();
    }
    if (!FAborted)
    {
      qint64 bytesForTransfer = FRangeLength>0 ? FRangeLength : FFileSize-FRangeOffset;
      if (FFile.error() != QFile::NoError)
      {
        abortStream(FFile.errorString());
      }
      else if (FSocket->errorCode() != IDataStreamSocket::NoError)
      {
        abortStream(FSocket->errorString());
      }
      else if (FProgress == bytesForTransfer)
      {
        setStreamState(Finished,tr("Data transmission finished"));
      }
      else
      {
        abortStream(tr("Data transmission terminated by user"));
      }
    }
    else
    {
      abortStream(FAbortString);
    }
    FSocket->instance()->deleteLater();
    FSocket = NULL;
  }
}

void FileStream::onTransferThreadProgress(qint64 ABytes)
{
  FProgress += ABytes;
  FSpeed[FSpeedIndex] += ABytes;
  emit progressChanged();
}

void FileStream::onTransferThreadFinished()
{
  if (FSocket && FSocket->isOpen())
  {
    setStreamState(Disconnecting,tr("Disconnecting"));
    FSocket->close();
  }
  FThread->deleteLater();
  FThread = NULL;
}

void FileStream::onIncrementSpeedIndex()
{
  if (FStreamState == Transfering)
    QTimer::singleShot(SPEED_INTERVAL,this,SLOT(onIncrementSpeedIndex()));
  FSpeedIndex = (FSpeedIndex+1) % SPEED_POINTS;
  FSpeed[FSpeedIndex] = 0;
  emit speedChanged();
}

