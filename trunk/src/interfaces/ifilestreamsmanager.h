#ifndef IFILESTREAMSMANAGER_H
#define IFILESTREAMSMANAGER_H

#include <QWidget>
#include <QDateTime>

class Jid;
class Stanza;

#define FILESTREAMSMANAGER_UUID       "{ea9ea27a-5ad7-40e3-82b3-db8ac3bdc288}"

class IFileStream
{
public:
  enum StreamKind {
    SendFile,
    ReceiveFile
  };
  enum StreamState {
    Creating,
    Negotiating,
    Connecting,
    Transfering,
    Disconnecting,
    Finished,
    Aborted
  };
public:
  virtual QObject *instance() =0;
  virtual QString streamId() const =0;
  virtual Jid streamJid() const =0;
  virtual Jid contactJid() const =0;
  virtual int streamKind() const =0;
  virtual int streamState() const =0;
  virtual qint64 speed() const =0;
  virtual qint64 progress() const =0;
  virtual QString stateString() const =0;
  virtual bool isRangeSupported() const =0;
  virtual void setRangeSupported(bool ASupported) =0;
  virtual qint64 rangeOffset() const =0;
  virtual void setRangeOffset(qint64 AOffset) =0;
  virtual qint64 rangeLength() const =0;
  virtual void setRangeLength(qint64 ALength) =0;
  virtual QString fileName () const =0;
  virtual void setFileName(const QString &AFileName) =0;
  virtual qint64 fileSize() const =0;
  virtual void setFileSize(qint64 AFileSize) =0;
  virtual QString fileHash() const =0;
  virtual void setFileHash(const QString &AFileHash) =0;
  virtual QDateTime fileDate() const =0;
  virtual void setFileDate(const QDateTime &ADate) =0;
  virtual QString fileDescription() const =0;
  virtual void setFileDescription(const QString &AFileDesc) =0;
  virtual bool initStream(const QList<QString> &AMethods) =0;
  virtual bool startStream(const QString &AMethodNS, const QString &ASettingsNS) =0;
  virtual void abortStream(const QString &AError) =0;
signals:
  virtual void stateChanged() =0;
  virtual void speedUpdated() =0;
  virtual void progressChanged() =0;
  virtual void propertiesChanged() =0;
  virtual void streamDestroyed() =0;
};

class IFileStreamsHandler
{
public:
  virtual bool fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods) =0;
  virtual bool fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS) =0;
  virtual bool fileStreamShowDialog(const QString &AStreamId) =0;
};

class IFileStreamsManager
{
public:
  virtual QObject *instance() =0;
  virtual QList<IFileStream *> streams() const =0;
  virtual IFileStream *streamById(const QString &AStreamId) const =0;
  virtual IFileStream *createStream(IFileStreamsHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid, 
    const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent = NULL) =0;
  virtual QString defaultStreamMethod() const =0;
  virtual void setDefaultStreamMethod(const QString &AMethodNS) =0;
  virtual QList<QString> streamMethods() const =0;
  virtual void insertStreamMethod(const QString &AMethodNS) =0;
  virtual void removeStreamMethod(const QString &AMethodNS) =0;
  virtual IFileStreamsHandler *streamHandler(const QString &AStreamId) const =0;
  virtual void insertStreamsHandler(IFileStreamsHandler *AHandler, int AOrder) =0;
  virtual void removeStreamsHandler(IFileStreamsHandler *AHandler, int AOrder) =0;
signals:
  virtual void streamCreated(IFileStream *AStream) =0;
  virtual void streamDestroyed(IFileStream *AStream) =0;
};

Q_DECLARE_INTERFACE(IFileStream,"Vacuum.Plugin.IFileStream/1.0")
Q_DECLARE_INTERFACE(IFileStreamsHandler,"Vacuum.Plugin.IFileStreamsHandler/1.0")
Q_DECLARE_INTERFACE(IFileStreamsManager,"Vacuum.Plugin.IFileStreamsManager/1.0")

#endif // IFILESTREAMSMANAGER_H
