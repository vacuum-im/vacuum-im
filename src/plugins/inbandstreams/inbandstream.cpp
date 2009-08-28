#include "inbandstream.h"

#define BUFFER_INCREMENT_SIZE     1024
#define MAX_WRITE_BUFFER_SIZE     8192

InBandStream::InBandStream(IStanzaProcessor *AProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent) 
  : QIODevice(AParent), readBuffer(BUFFER_INCREMENT_SIZE), writeBuffer(BUFFER_INCREMENT_SIZE,MAX_WRITE_BUFFER_SIZE)
{
  FStreamId = AStreamId;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FStreamKind = AKind;
  FStreamState = IDataStreamSocket::Closed;

  FStanzaProcessor = AProcessor;
}

InBandStream::~InBandStream()
{

}

bool InBandStream::stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AHandleId); Q_UNUSED(AStreamJid); Q_UNUSED(AStanza); Q_UNUSED(AAccept);
  return false;
}

bool InBandStream::stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  return false;
}

void InBandStream::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{

}

void InBandStream::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{

}

bool InBandStream::isSequential() const
{
  return true;
}

qint64 InBandStream::bytesAvailable() const
{
  return readBuffer.size();
}

qint64 InBandStream::bytesToWrite() const
{
  return writeBuffer.size();
}

bool InBandStream::waitForBytesWritten(int AMsecs)
{
  return false;
}

bool InBandStream::waitForReadyRead(int AMsecs)
{
  return false;
}

QString InBandStream::methodNS() const
{
  return NS_INBAND_BYTESTREAMS;
}

QString InBandStream::streamId() const
{
  return FStreamId;
}

Jid InBandStream::streamJid() const
{
  return FStreamJid;
}

Jid InBandStream::contactJid() const
{
  return FContactJid;
}

int InBandStream::streamKind() const
{
  return FStreamKind;
}

int InBandStream::streamState() const
{
  return FStreamState;
}

bool InBandStream::isOpen() const
{
  return QIODevice::isOpen() && FStreamState==IDataStreamSocket::Opened;
}

bool InBandStream::open(QIODevice::OpenMode AMode)
{
  if (FStreamState == IDataStreamSocket::Closed)
  {
    
  }
  return false;
}

bool InBandStream::flush()
{
  return false;
}

void InBandStream::close()
{

}

qint64 InBandStream::readData(char *AData, qint64 AMaxSize)
{
  return readBuffer.read(AData,AMaxSize);
}

qint64 InBandStream::writeData(const char *AData, qint64 AMaxSize)
{
  return writeBuffer.write(AData,AMaxSize);
}

