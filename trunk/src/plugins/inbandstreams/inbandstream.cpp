#include "inbandstream.h"

#include <QTime>
#include <QEvent>
#include <QReadLocker>
#include <QWriteLocker>
#include <QCoreApplication>

#define BUFFER_INCREMENT_SIZE     1024
#define MAX_WRITE_BUFFER_SIZE     8192

#define DATA_TIMEOUT              60000
#define OPEN_TIMEOUT              30000
#define CLOSE_TIMEOUT             10000

#define SHC_INBAND_OPEN           "/iq[@type='set']/open[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_CLOSE          "/iq[@type='set']/close[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_IQ        "/iq[@type='set']/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"
#define SHC_INBAND_DATA_MESSAGE   "/message/data[@xmlns='" NS_INBAND_BYTESTREAMS "']"

#define MINIMUM_BLOCK_SIZE        128

class SendDataEvent : 
  public QEvent
{
public:
  SendDataEvent(bool AFlush) : QEvent(FEventType) { FFlush = AFlush; }
  inline bool flush() { return FFlush; }
  static int registeredType() { return FEventType; }
private:
  bool FFlush;
  static QEvent::Type FEventType;
};

QEvent::Type SendDataEvent::FEventType = static_cast<QEvent::Type>(QEvent::registerEventType());


InBandStream::InBandStream(IStanzaProcessor *AProcessor, const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, int AKind, QObject *AParent) 
  : QIODevice(AParent), FReadBuffer(BUFFER_INCREMENT_SIZE), FWriteBuffer(BUFFER_INCREMENT_SIZE,MAX_WRITE_BUFFER_SIZE)
{
  FStanzaProcessor = AProcessor;

  FStreamId = AStreamId;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FStreamKind = AKind;

  FSHIOpen = -1;
  FSHIClose = -1;
  FSHIData = -1;

  FBlockSize = 4096;
  FMaxBlockSize = 10240;
  FStanzaType = StanzaIq;
  FStreamState = IDataStreamSocket::Closed;
}

InBandStream::~InBandStream()
{
  abort(tr("Stream destroyed"));
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
    AAccept = true;
    if (AStanza.firstElement("error").isNull())
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
        
        FThreadLock.lockForWrite();
        FReadBuffer.write(data);
        FThreadLock.unlock();

        FSeqIn = FSeqIn<USHRT_MAX ? FSeqIn+1 : 0;
        emit readyRead();
        FReadyReadCondition.wakeAll();
      }
      else
      {
        abort(tr("Malformed data packet"));
      }
    }
    else
    {
      abort(ErrorHandler(AStanza.element()).message());
    }
  }
  else if (AHandleId==FSHIOpen && elem.attribute("sid")==FStreamId)
  {
    AAccept = true;
    removeStanzaHandle(FSHIOpen);
    if (FStreamState == IDataStreamSocket::Opening)
    {
      QDomElement openElem = AStanza.firstElement("open");
      FBlockSize = openElem.attribute("block-size").toInt();
      if (FBlockSize>MINIMUM_BLOCK_SIZE && FBlockSize<=FMaxBlockSize)
      {
        FStanzaType = openElem.attribute("stanza","message") == "message" ? StanzaMessage : StanzaIq;
        FSHIData = insertStanzaHandle(FStanzaType==StanzaMessage ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
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
            abort(tr("Failed to open stream"));
          }
        }
        else
        {
          Stanza error = AStanza.replyError("not-acceptable");
          FStanzaProcessor->sendStanzaOut(AStreamJid,error);
          abort(tr("Failed to open stream"));
        }
      }
      else
      {
        Stanza error = AStanza.replyError("resource-constraint",EHN_DEFAULT,ErrorHandler::UNKNOWNCODE,tr("Block size is not acceptable"));
        FStanzaProcessor->sendStanzaOut(AStreamJid,error);
        abort(tr("Block size is not acceptable"));
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
  if (AStanza.id() == FDataIqRequestId)
  {
    if (AStanza.type() == "result")
    {
      FDataIqRequestId.clear();
      sendNextPaket();
    }
    else
    {
      abort(ErrorHandler(AStanza.element()).message());
    }
  }
  else if (AStanza.id() == FOpenRequestId)
  {
    if (AStanza.type() == "result")
    {
      FSHIData = insertStanzaHandle(FStanzaType==StanzaMessage ? SHC_INBAND_DATA_MESSAGE : SHC_INBAND_DATA_IQ);
      FSHIClose = insertStanzaHandle(SHC_INBAND_CLOSE);
      if (FSHIData>0 && FSHIClose>0)
      {
        setStreamState(IDataStreamSocket::Opened);
      }
      else
      {
        abort(tr("Failed to open stream"));
      }
    }
    else
    {
      abort(ErrorHandler(AStanza.element()).message());
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
  if (AStanzaId == FDataIqRequestId)
  {
    abort(ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
  }
  else if (AStanzaId == FOpenRequestId)
  {
    abort(ErrorHandler(ErrorHandler::REQUEST_TIMEOUT).message());
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
  QReadLocker locker(&FThreadLock);
  return FReadBuffer.size();
}

qint64 InBandStream::bytesToWrite() const
{
  QReadLocker locker(&FThreadLock);
  return FWriteBuffer.size();
}

bool InBandStream::waitForBytesWritten(int AMsecs)
{
  bool isWritten = false;
  if (streamState() != IDataStreamSocket::Closed)
  {
    FThreadLock.lockForWrite();
    isWritten = FBytesWrittenCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
    FThreadLock.unlock();
  }
  return isWritten && isOpen();
}

bool InBandStream::waitForReadyRead(int AMsecs)
{
  if (streamState()!=IDataStreamSocket::Closed && bytesAvailable()==0)
  {
    FThreadLock.lockForWrite();
    FReadyReadCondition.wait(&FThreadLock, AMsecs>=0 ? (unsigned long)AMsecs : ULONG_MAX);
    FThreadLock.unlock();
  }
  return bytesAvailable()>0;
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
  QReadLocker locker(&FThreadLock);
  return FStreamState;
}

bool InBandStream::isOpen() const
{
  QReadLocker locker(&FThreadLock);
  return FStreamState==IDataStreamSocket::Opened;
}

bool InBandStream::open(QIODevice::OpenMode AMode)
{
  if (FStanzaProcessor && streamState()==IDataStreamSocket::Closed)
  {
    setStreamError(QString::null,NoError);
    if (streamKind() == IDataStreamSocket::Initiator)
    {
      Stanza openRequest("iq");
      openRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.eFull());
      QDomElement elem = openRequest.addElement("open",NS_INBAND_BYTESTREAMS);
      elem.setAttribute("sid",FStreamId);
      elem.setAttribute("block-size",FBlockSize);
      elem.setAttribute("stanza",FStanzaType==StanzaMessage ? "message" : "iq");
      if (FStanzaProcessor->sendStanzaRequest(this,FStreamJid,openRequest,OPEN_TIMEOUT))
      {
        FOpenRequestId = openRequest.id();
        setOpenMode(AMode);
        setStreamState(IDataStreamSocket::Opening);
        return true;
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
    }
  }
  return false;
}

bool InBandStream::flush()
{
  if (bytesToWrite() > 0)
  {
    SendDataEvent *dataEvent = new SendDataEvent(true);
    QCoreApplication::postEvent(this,dataEvent);
    return true;
  }
  return false;
}

void InBandStream::close()
{
  int state = streamState();
  if (state==IDataStreamSocket::Opened || state==IDataStreamSocket::Opening)
  {
    emit aboutToClose();
    Stanza closeRequest("iq");
    closeRequest.setType("set").setId(FStanzaProcessor->newId()).setTo(FContactJid.eFull());
    closeRequest.addElement("close",NS_INBAND_BYTESTREAMS).setAttribute("sid",FStreamId);
    if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,FStreamJid,closeRequest,CLOSE_TIMEOUT))
    {
      FCloseRequestId = closeRequest.id();
      setStreamState(IDataStreamSocket::Closing);
    }
    else
    {
      setStreamState(IDataStreamSocket::Closed);
    }
  }
}

void InBandStream::abort(const QString &AError, int ACode)
{
  if (streamState() != IDataStreamSocket::Closed)
  {
    setStreamError(AError, ACode);
    close();
    setStreamState(IDataStreamSocket::Closed);
  }
}

int InBandStream::errorCode() const
{
  QReadLocker locker(&FThreadLock);
  return FErrorCode;
}

QString InBandStream::errorString() const
{
  QReadLocker locker(&FThreadLock);
  return QIODevice::errorString();
}

int InBandStream::blockSize() const
{
  return FBlockSize;
}

void InBandStream::setBlockSize(int ASize)
{
  if (streamState()==IDataStreamSocket::Closed && ASize>=MINIMUM_BLOCK_SIZE && ASize<=maximumBlockSize())
  {
    FBlockSize = ASize;
    emit propertiesChanged();
  }
}

int InBandStream::maximumBlockSize()
{
  return FMaxBlockSize;
}

void InBandStream::setMaximumBlockSize(int ASize)
{
  if (ASize>=MINIMUM_BLOCK_SIZE && ASize<=USHRT_MAX)
  {
    FMaxBlockSize = ASize;
    emit propertiesChanged();
  }
}

int InBandStream::dataStanzaType() const
{
  return FStanzaType;
}

void InBandStream::setDataStanzaType(int AType)
{
  if (streamState() == IDataStreamSocket::Closed)
  {
    FStanzaType = AType;
    emit propertiesChanged();
  }
}

qint64 InBandStream::readData(char *AData, qint64 AMaxSize)
{
  QWriteLocker locker(&FThreadLock);
  return FReadBuffer.read(AData,AMaxSize);
}

qint64 InBandStream::writeData(const char *AData, qint64 AMaxSize)
{
  SendDataEvent *dataEvent = new SendDataEvent(false);
  QCoreApplication::postEvent(this,dataEvent);
  QWriteLocker locker(&FThreadLock);
  return FWriteBuffer.write(AData,AMaxSize);
}

void InBandStream::setOpenMode(OpenMode AMode)
{
  QWriteLocker locker(&FThreadLock);
  QIODevice::setOpenMode(AMode);
}

bool InBandStream::event(QEvent *AEvent)
{
  if (AEvent->type() == SendDataEvent::registeredType())
  {
    SendDataEvent *dataEvent = static_cast<SendDataEvent *>(AEvent);
    sendNextPaket(dataEvent->flush());
    return true;
  }
  return QIODevice::event(AEvent);
}

bool InBandStream::sendNextPaket(bool AFlush)
{
  bool sent = false;
  if (isOpen() && FDataIqRequestId.isEmpty() && (bytesToWrite()>=FBlockSize || AFlush))
  {
    FThreadLock.lockForWrite();
    QByteArray data = FWriteBuffer.read(FBlockSize);
    FThreadLock.unlock();
    if (!data.isEmpty())
    {
      Stanza paket(FStanzaType==StanzaMessage ? "message" : "iq");
      paket.setTo(FContactJid.eFull()).setId(FStanzaProcessor->newId());
      QDomElement dataElem = paket.addElement("data",NS_INBAND_BYTESTREAMS);
      dataElem.setAttribute("sid",FStreamId);
      dataElem.setAttribute("seq",FSeqOut);
      dataElem.appendChild(paket.createTextNode(QString::fromUtf8(data.toBase64())));

      if (FStanzaType == StanzaMessage)
      {
        QDomElement ampElem = paket.addElement("amp","http://jabber.org/protocol/amp");
        QDomElement ruleElem = ampElem.appendChild(paket.createElement("rule")).toElement();
        ruleElem.setAttribute("condition","deliver");
        ruleElem.setAttribute("value","stored");
        ruleElem.setAttribute("action","error");
        ruleElem = ampElem.appendChild(paket.createElement("rule")).toElement();
        ruleElem.setAttribute("condition","match-resource");
        ruleElem.setAttribute("value","exact");
        ruleElem.setAttribute("action","error");

        SendDataEvent *dataEvent = new SendDataEvent(AFlush);
        QCoreApplication::postEvent(this, dataEvent);

        sent = FStanzaProcessor && FStanzaProcessor->sendStanzaOut(FStreamJid,paket);
      }
      else
      {
        paket.setType("set");
        FDataIqRequestId = paket.id();
        sent = FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,FStreamJid,paket,DATA_TIMEOUT);
      }

      if (sent)
      {
        FSeqOut = FSeqOut<USHRT_MAX ? FSeqOut+1 : 0;
        emit bytesWritten(data.size());
        FBytesWrittenCondition.wakeAll();
      }
      else
      {
        abort(tr("Failed to send data"));
      }
    }
  }
  return sent;  
}

void InBandStream::setStreamState(int AState)
{
  if (streamState() != AState)
  {
    if (AState == IDataStreamSocket::Opened)
    {
      FSeqIn = 0;
      FSeqOut = 0;
      FDataIqRequestId.clear();
      FThreadLock.lockForWrite();
      QIODevice::open(openMode());
      FThreadLock.unlock();
    }
    else if (AState == IDataStreamSocket::Closed)
    {
      removeStanzaHandle(FSHIOpen);
      removeStanzaHandle(FSHIClose);
      removeStanzaHandle(FSHIData);
      emit readChannelFinished();

      FThreadLock.lockForWrite();
      FStreamState = AState;
      QString saveError = QIODevice::errorString();
      QIODevice::close();
      QIODevice::setErrorString(saveError);
      FReadBuffer.clear();
      FWriteBuffer.clear();
      FThreadLock.unlock();

      FReadyReadCondition.wakeAll();
      FBytesWrittenCondition.wakeAll();
    }

    FThreadLock.lockForWrite();
    FStreamState = AState;
    FThreadLock.unlock();

    emit stateChanged(AState);
  }
}

void InBandStream::setStreamError(const QString &AError, int ACode)
{
  if (ACode==NoError || errorCode()==NoError)
  {
    QWriteLocker locker(&FThreadLock);
    FErrorCode = ACode;
    setErrorString(AError);
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
