#ifndef INBANDSTREAM_H
#define INBANDSTREAM_H

#include "../../definations/namespaces.h"
#include "../../interfaces/iinbandstreams.h"
#include "../../interfaces/idatastreamsmanager.h"
#include "../../utils/jid.h"

class InBandStream : 
  public QIODevice,
  public IInBandStream
{
  Q_OBJECT;
  Q_INTERFACES(IInBandStream);
public:
  InBandStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent=NULL);
  ~InBandStream();
  virtual QIODevice *instance() { return this; }
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
signals:
  virtual void stateChanged(int AState) =0;
protected:
  virtual qint64 readData(char *AData, qint64 AMaxSize);
  virtual qint64 writeData(const char *AData, qint64 AMaxSize);
private:
  Jid FStreamJid;
  Jid FContactJid;
  int FStreamKind;
  int FStreamState;
  QString FStreamId;
};

#endif // INBANDSTREAM_H
