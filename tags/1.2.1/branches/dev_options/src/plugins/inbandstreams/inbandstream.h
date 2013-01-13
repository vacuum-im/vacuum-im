#ifndef INBANDSTREAM_H
#define INBANDSTREAM_H

#include <QReadWriteLock>
#include <QWaitCondition>
#include <definations/namespaces.h>
#include <interfaces/iinbandstreams.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/errorhandler.h>
#include <utils/ringbuffer.h>
#include <utils/jid.h>

#define MINIMUM_BLOCK_SIZE        128

#define DEFAULT_BLOCK_SIZE        4096
#define DEFAULT_MAX_BLOCK_SIZE    10240
#define DEFAULT_DATA_STANZA_TYPE  IInBandStream::StanzaIq

class InBandStream : 
  public QIODevice,
  public IInBandStream,
  public IStanzaHandler,
  public IStanzaRequestOwner
{
  Q_OBJECT;
  Q_INTERFACES(IInBandStream IDataStreamSocket IStanzaHandler IStanzaRequestOwner);
public:
  InBandStream(IStanzaProcessor *AProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent=NULL);
  ~InBandStream();
  virtual QIODevice *instance() { return this; }
  //IStanzaHandler
  virtual bool stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //QIODevice
  virtual bool isSequential() const;
  virtual qint64 bytesAvailable() const;
  virtual qint64 bytesToWrite() const;
  virtual bool waitForBytesWritten(int AMsecs);
  virtual bool waitForReadyRead(int AMsecs);
  //IDataStreamSocket
  virtual QString methodNS() const;
  virtual QString streamId() const;
  virtual Jid streamJid() const;
  virtual Jid contactJid() const;
  virtual int streamKind() const;
  virtual int streamState() const;
  virtual bool isOpen() const;
  virtual bool open(QIODevice::OpenMode AMode);
  virtual bool flush();
  virtual void close();
  virtual void abort(const QString &AError, int ACode = UnknownError);
  virtual int errorCode() const;
  virtual QString errorString() const;
  //IInBandStream
  virtual int blockSize() const;
  virtual void setBlockSize(int ASize);
  virtual int maximumBlockSize() const;
  virtual void setMaximumBlockSize(int ASize);
  virtual int dataStanzaType() const;
  virtual void setDataStanzaType(int AType);
signals:
  void stateChanged(int AState);
  void propertiesChanged();
protected:
  virtual qint64 readData(char *AData, qint64 AMaxSize);
  virtual qint64 writeData(const char *AData, qint64 AMaxSize);
  void setOpenMode(OpenMode AMode);
  virtual bool event(QEvent *AEvent);
protected:
  bool sendNextPaket(bool AFlush = false);
  void setStreamState(int AState);
  void setStreamError(const QString &AError, int ACode);
  int insertStanzaHandle(const QString &ACondition);
  void removeStanzaHandle(int &AHandleId);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  int FErrorCode;
  Jid FStreamJid;
  Jid FContactJid;
  int FStreamKind;
  int FStreamState;
  QString FStreamId;
private:
  int FSHIOpen;
  int FSHIClose;
  int FSHIData;
  QString FOpenRequestId;
  QString FCloseRequestId;
  QString FDataIqRequestId;
private:
  int FBlockSize;
  int FMaxBlockSize;
  int FStanzaType;
  quint16 FSeqIn;
  quint16 FSeqOut;
private:
  RingBuffer FReadBuffer;
  RingBuffer FWriteBuffer;
  mutable QReadWriteLock FThreadLock;
  QWaitCondition FReadyReadCondition;
  QWaitCondition FBytesWrittenCondition;
};

#endif // INBANDSTREAM_H
