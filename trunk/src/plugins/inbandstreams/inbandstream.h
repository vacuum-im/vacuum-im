#ifndef INBANDSTREAM_H
#define INBANDSTREAM_H

#include "../../definations/namespaces.h"
#include "../../interfaces/iinbandstreams.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../utils/errorhandler.h"
#include "../../utils/ringbuffer.h"
#include "../../utils/jid.h"

class InBandStream : 
  public QIODevice,
  public IDataStreamSocket,
  public IStanzaHandler,
  public IStanzaRequestOwner
{
  Q_OBJECT;
  Q_INTERFACES(IStanzaHandler IStanzaRequestOwner IDataStreamSocket);
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
  virtual void abort(const QString &AError);
  virtual void close();
signals:
  virtual void stateChanged(int AState);
protected:
  virtual qint64 readData(char *AData, qint64 AMaxSize);
  virtual qint64 writeData(const char *AData, qint64 AMaxSize);
  void setErrorString(const QString &AError);
protected:
  void setStreamState(int AState);
  int insertStanzaHandle(const QString &ACondition);
  void removeStanzaHandle(int &AHandleId);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
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
  QString FDataRequestId;
private:
  int FBlockSize;
  int FMaxBlockSize;
  quint16 FSeqIn;
  quint16 FSeqOut;
  QString FStanzaTag;
  RingBuffer FReadBuffer;
  RingBuffer FWriteBuffer;
};

#endif // INBANDSTREAM_H
