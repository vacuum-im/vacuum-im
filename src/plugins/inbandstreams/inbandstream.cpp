#include "inbandstream.h"

InBandStream::InBandStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent) : QIODevice(AParent)
{
  FStreamId = AStreamId;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FStreamKind = AKind;
}

InBandStream::~InBandStream()
{

}

bool InBandStream::isSequential() const
{
  return true;
}

qint64 InBandStream::bytesAvailable() const
{
  return 0;
}

qint64 InBandStream::bytesToWrite() const
{
  return 0;
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
  return false;
}

bool InBandStream::open(QIODevice::OpenMode AMode)
{
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
  return 0;
}

qint64 InBandStream::writeData(const char *AData, qint64 AMaxSize)
{
  return 0;
}
