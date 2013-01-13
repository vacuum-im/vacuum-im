#include "filestream.h"

#include <QDir>
#include <QTimer>
#include <QFileInfo>

#define CONNECTION_TIMEOUT 60000

FileStream::FileStream(IDataStreamsManager *ADataManager, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent) : QObject(AParent)
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
	FSpeedIndex = 0;
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
		FThread = NULL;
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
	return FSocket!=NULL ? FSocket->methodNS() : QString::null;
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
			FFileName = AFileName;
			updateFileInfo();
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

QUuid FileStream::settingsProfile() const
{
	return FProfileId;
}

void FileStream::setSettingsProfile(const QUuid &AProfileId)
{
	if (FProfileId != AProfileId)
	{
		FProfileId = AProfileId;
		emit propertiesChanged();
	}
}

bool FileStream::initStream(const QList<QString> &AMethods)
{
	if (FStreamState==Creating && FStreamKind==SendFile)
	{
		if (updateFileInfo() && !FFileName.isEmpty() && FFileSize>0)
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
				stremMethod->loadMethodSettings(FSocket,FDataManager->settingsProfileNode(FProfileId, AMethodNS));
				connect(FSocket->instance(),SIGNAL(stateChanged(int)),SLOT(onSocketStateChanged(int)));
				if (!FSocket->open(QIODevice::WriteOnly))
				{
					delete FSocket->instance();
					FSocket = NULL;
				}
				else
				{
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
					stremMethod->loadMethodSettings(FSocket,FDataManager->settingsProfileNode(FProfileId, AMethodNS));
					connect(FSocket->instance(),SIGNAL(stateChanged(int)),SLOT(onSocketStateChanged(int)));
					if (!FSocket->open(QIODevice::ReadOnly))
					{
						delete FSocket->instance();
						FSocket = NULL;
					}
					else
					{
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
	if (updateFileInfo() && !FFileName.isEmpty() && FFileSize>0)
	{
		QFileInfo finfo(FFileName);
		if (FStreamKind==IFileStream::ReceiveFile ? QDir::root().mkpath(finfo.absolutePath()) : true)
		{
			FFile.setFileName(FFileName);
			QIODevice::OpenMode mode = QIODevice::ReadOnly;
			if (FStreamKind==IFileStream::ReceiveFile)
			{
				mode = QIODevice::WriteOnly;
				mode |= FRangeOffset > 0 ? QIODevice::Append : QIODevice::Truncate;
			}
			if (FFile.open(mode))
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

bool FileStream::updateFileInfo()
{
	if (FStreamKind == SendFile)
	{
		QFileInfo finfo(FFileName);
		if (FFileSize != finfo.size())
		{
			if (FStreamState == Creating)
			{
				FFileSize = finfo.size();
				FFileDate = finfo.lastModified();
				emit propertiesChanged();
			}
			else
			{
				abortStream(tr("File size unexpectedly changed"));
				return false;
			}
		}
	}
	return true;
}

void FileStream::setStreamState(int AState, const QString &AMessage)
{
	if (FStreamState != AState)
	{
		if (AState == Connecting)
		{
			QTimer::singleShot(CONNECTION_TIMEOUT,this,SLOT(onConnectionTimeout()));
		}
		else if (AState == Transfering)
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

QStringList FileStream::acceptableMethods() const
{
	return FAcceptableMethods;
}

void FileStream::setAcceptableMethods(const QStringList &AMethods)
{
	FAcceptableMethods = AMethods;
}

void FileStream::onSocketStateChanged(int AState)
{
	if (AState == IDataStreamSocket::Opening)
	{
		setStreamState(Connecting,tr("Connecting"));
	}
	else if (AState == IDataStreamSocket::Opened)
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
				abortStream(tr("Data transmission terminated by remote user"));
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

void FileStream::onConnectionTimeout()
{
	if (FStreamState == Connecting)
	{
		abortStream(tr("Connection timed out"));
	}
}
