#include "inbandstream.h"

#include <QTime>

#define BUFFER_INCREMENT_SIZE     1024
#define MAX_WRITE_BUFFER_SIZE     8192

#define OPEN_TIMEOUT              60000
#define CLOSE_TIMEOUT             5000

#define SHC_INBAND_OPEN           "/iq[@type='set']/open[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_CLOSE          "/iq[@type='set']/close[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_IQ        "/iq[@type='set']/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_MESSAGE   "/message/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"

InBandStream::InBandStream(IStanzaProcessor *AProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent) 
  : QIODevice(AParent), FReadBuffer(BUFFER_INCREMENT_SIZE), FWriteBuffer(BUFFER_INCREMENT_SIZE,MAX_WRITE_BUFFER_SIZE)
{
  FStreamId = AStreamId;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FStreamKind = AKind;

  FSHIOpen = -1;
  FSHIClose = -1;
  FSHIData = -1;

  FStreamState = IDataStreamSocket::Closed;

  FStanzaProcessor = AProcessor;
}

InBandStream::~InBandStream()
{
  if (FStanzaProcessor)
  {
    removeStanzaHandle(FSHIOpen);
    removeStanzaHandle(FSHIClose);
    removeStanzaHandle(FSHIData);
  }
}

bool InBandStream::stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AHandleId); Q_UNUSED(AStreamJid); Q_UNUSED(AStanza); Q_UNUSED(AAccept);
  return false;
}

bool InBandStream::stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  QDomElement elem = AStanza.firstElement(QString::null,NS_INBAND_BYTESTREAMS);
  if (AHandleId==FSHIData && elem.attribute("sid")==FStreamId)
  {
    if (AStanza.type()=="result" || FStanzaTag=="message")
    {
      QByteArray data =  QByteArray::fromBase64(elem.text().toUtf8());
      if (FSeqIn==elem.attribute("seq").toInt() && data.size()>0 && data.size()<=FBlockSize)
      {
        if (AStanza.tagName() == "iq")
        {
          Stanza result("iq");
          result.setType("result").setId(AStanza.id()).setTo(AStanza.from());
          FStanzaProcessor->sendStanzaOut(AStreamJid,result);
        }
        FSeqIn = FSeqIn<65535 ? FSeqIn+1 : 0;
        FReadBuffer.write(data);
        emit readyRead();
      }
      else
      {
        setErrorString(tr("Malformed data packet"));
        Stanza error = AStanza.replyError("bad-request",EHN_DEFAULT,ErrorHandler::BAD_REQUEST,errorString());
        error.detach();
        QDomElement dataElem = error.firstElement("data",NS_INBAND_BYTESTREAMS);
        dataElem.removeChild(dataElem.firstChild());
        FStanzaProcessor->sendStanzaOut(AStreamJid,error);
        close();
      }
    }
    else
    {
      setErrorString(ErrorHandler(AStanza.element()).message());
      setStreamState(IDataStreamSocket::Closed);
    }
  }
  else if (AHandleId==FSHIOpen && elem.attribute("sid")==FStreamId)
  {
    AAccept = true;
    removeStanzaHandle(FSHIOpen);
    if (FStreamState==IDataStreamSocket::Opening)
    {
      QDomElement openElem = AStanza.firstElement("open");
      FBlockSize = openElem.attribute("block-size").toInt();
      if (FBlockSize>0 && FBlockSize<=FMaxBlockSize)
      {
        FStanzaTag = openElem.attribute("stanza");
        FSHIData = insertStanzaHandle(FStanzaTag=="message" ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
        FSHIClose = insertStanzaHandle(SHC_INBAND_CLOSE);
        if (FSHIData>0 && FSHIClose>0)
        {
          Stanza reply("iq");
          reply.setType("result").setTo(AStanza.from()).setId(AStanza.id());
          if (FStanzaProcessor->sendStanzaOut(AStreamJid,reply))
          {
            setStreamState(IDataStreamSocket::Opened);
          }
          else
          {
            setErrorString(tr("Failed to open stream"));
            setStreamState(IDataStreamSocket::Closed);
          }
        }
        else
        {
          Stanza error = AStanza.replyError("not-acceptable");
          FStanzaProcessor->sendStanzaOut(AStreamJid,error);
          setErrorString(tr("Failed to open stream"));
          setStreamState(IDataStreamSocket::Closed);
        }
      }
      else
      {
        Stanza error = AStanza.replyError("resource-constraint",EHN_DEFAULT,ErrorHandler::UNKNOWNCODE,tr("Block size is not acceptable"));
        FStanzaProcessor->sendStanzaOut(AStreamJid,error);
        setErrorString(tr("Block size is not acceptable"));
        setStreamState(IDataStreamSocket::Closed);
      }
    }
    else
    {
      Stanza error = AStanza.replyError("not-acceptable");
      FStanzaProcessor->sendStanzaOut(AStreamJid,error);
    }
  }
  else if (AHandleId==FSHIClose && elem.attribute("sid")==FStreamId)
  {
    AAccept = true;
    Stanza reply("iq");
    reply.setType("result").setTo(AStanza.from()).setId(AStanza.id());
    FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    setStreamState(IDataStreamSocket::Closed);
  }
  return false;
}

void InBandStream::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
  Q_UNUSED(AStreamJid);
  if (AStanza.id() == FDataRequestId)
  {
    if (AStanza.type() == "result")
    {

    }
    else
    {
      setErrorString(ErrorHandler(AStanza.element()).message());
      setStreamState(IDataStreamSocket::Closed);
    }
  }
  else if (AStanza.id() == FOpenRequestId)
  {
    if (AStanza.type() == "result")
    {
      FSHIData = insertStanzaHandle(FStanzaTag=="message" ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
      FSHIClose = insertStanzaHandle(SHC_INBAND_CLOSE);
      if (FSHIData>0 && FSHIClose>0)
      {
        setStreamState(IDataStreamSocket::Opened);
      }
      else
      {
        setErrorString(tr("Failed to open stream"));
        setStreamState(IDataStreamSocket::Closed);
      }
    }
    else
    {
      setErrorString(ErrorHandler(AStanza.element()).message());
      setStreamState(IDataStreamSocket::Closed);
    }
  }
  else if (AStanza.id() == FCloseRequestId)
  {
    setStreamState(IDataStreamSocket::Closed);
  }
}

void InBandStream::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
  Q_UNUSED(AStreamJid);
  if (AStanzaId == FOpenRequestId)
  {
    setErrorString(ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
    setStreamState(IDataStreamSocket::Closed);
  }
  else if (AStanzaId == FCloseRequestId)
  {
    setStreamState(IDataStreamSocket::Closed);
  }
}

bool InBandStream::isSequential() const
{
  return true;
}

qint64 InBandStream::bytesAvailable() const
{
  return FReadBuffer.size();
}

qint64 InBandStream::bytesToWrite() const
{
  return FWriteBuffer.size();
}

bool InBandStream::waitForBytesWritten(int AMsecs)
{
  return false;
}

bool InBandStream::waitForReadyRead(int AMsecs)
{
  QTime time;
  time.start();
  while (FReadBuffer.size()==0)
  {
    if (AMsecs>=0 && time.elapsed()>AMsecs)
      return false;
    if (FStreamState == IDataStreamSocket::Closed)
      return false;
  }
  return true;
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
  return FStreamState==IDataStreamSocket::Opened;
}

bool InBandStream::open(QIODevice::OpenMode AMode)
{
  if (FStanzaProcessor && FStreamState == IDataStreamSocket::Closed)
  {
    FStanzaTag = "iq";
    FBlockSize = 4096;
    FMaxBlockSize = 10240;
    if (FStreamKind == IDataStreamSocket::Initiator)
    {
      Stanza openRequest("iq");
      openRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.eFull());
      QDomElement elem = openRequest.addElement("open",NS_INBAND_BYTESTREAMS);
      elem.setAttribute("sid",FStreamId);
      elem.setAttribute("block-size",FBlockSize);
      elem.setAttribute("stanza",FStanzaTag);
      if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,openRequest,OPEN_TIMEOUT))
      {
        FOpenRequestId = openRequest.id();
        setOpenMode(AMode);
        setStreamState(IDataStreamSocket::Opening);
        return true;
      }
      else
      {
        setErrorString(tr("Failed to initiate stream"));
      }
    }
    else
    {
      FSHIOpen = insertStanzaHandle(SHC_INBAND_OPEN);
      if (FSHIOpen != -1)
      {
        setOpenMode(AMode);
        setStreamState(IDataStreamSocket::Opening);
        return true;
      }
      else
      {
        setErrorString(tr("Failed to initiate stream"));
      }
    }
  }
  return false;
}

bool InBandStream::flush()
{
  return false;
}

void InBandStream::abort(const QString &AError)
{
  if (FStreamState != IDataStreamSocket::Closed)
  {
    setErrorString(AError);
    close();
    setStreamState(IDataStreamSocket::Closed);
  }
}

void InBandStream::close()
{
  if (FStreamState==IDataStreamSocket::Opened || FStreamState==IDataStreamSocket::Opening)
  {
    emit aboutToClose();
    Stanza closeRequest("iq");
    closeRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.eFull());
    closeRequest.addElement("close",NS_INBAND_BYTESTREAMS);
    if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,FStreamJid,closeRequest,CLOSE_TIMEOUT))
    {
      FCloseRequestId = closeRequest.id();
      setStreamState(IDataStreamSocket::Closing);
    }
    else
    {
      setErrorString(tr("Failed to finish stream"));
      setStreamState(IDataStreamSocket::Closed);
    }
  }
}

qint64 InBandStream::readData(char *AData, qint64 AMaxSize)
{
  return FReadBuffer.read(AData,AMaxSize);
}

qint64 InBandStream::writeData(const char *AData, qint64 AMaxSize)
{
  return FWriteBuffer.write(AData,AMaxSize);
}

void InBandStream::setErrorString(const QString &AError)
{
  if (errorString().isEmpty() || AError.isEmpty())
    QIODevice::setErrorString(AError);
}

void InBandStream::setStreamState(int AState)
{
  if (FStreamState != AState)
  {
    if (AState == IDataStreamSocket::Opening)
    {
      setErrorString(QString::null);
    }
    else if (AState == IDataStreamSocket::Opened)
    {
      FSeqIn = 0;
      FSeqOut = 0;
    }
    else if (AState == IDataStreamSocket::Closed)
    {
      removeStanzaHandle(FSHIOpen);
      removeStanzaHandle(FSHIClose);
      removeStanzaHandle(FSHIData);
      emit readChannelFinished();
      QIODevice::close();
      FReadBuffer.clear();
      FWriteBuffer.clear();
    }
    FStreamState = AState;
    emit stateChanged(AState);
  }
}

int InBandStream::insertStanzaHandle(const QString &ACondition)
{
  if (FStanzaProcessor)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.priority = SHP_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.streamJid = FStreamJid;
    shandle.conditions.append(ACondition);
    return FStanzaProcessor->insertStanzaHandle(shandle);
  }
  return -1;
}

void InBandStream::removeStanzaHandle(int &AHandleId)
{
  if (FStanzaProcessor && AHandleId>0)
  {
    FStanzaProcessor->removeStanzaHandle(AHandleId);
    AHandleId = -1;
  }
}
