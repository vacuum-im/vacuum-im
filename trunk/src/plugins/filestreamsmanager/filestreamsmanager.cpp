#include "filestreamsmanager.h"

#include <QSet>

FileStreamsManager::FileStreamsManager()
{
  FDataManager = NULL;
}

FileStreamsManager::~FileStreamsManager()
{

}

void FileStreamsManager::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing custom file streams");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("File Streams Manager");
  APluginInfo->uid = FILESTREAMSMANAGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(DATASTREAMSMANAGER_UUID);
}

bool FileStreamsManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IDataStreamsManager").value(0,NULL);
  if (plugin)
  {
    FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
  }

  return FDataManager!=NULL;
}

bool FileStreamsManager::initObjects()
{
  if (FDataManager)
  {
    FDataManager->insertProfile(this);
  }
  return true;
}

QString FileStreamsManager::profileNS() const
{
  return NS_SI_FILETRANSFER;
}

bool FileStreamsManager::requestDataStream(const QString &AStreamId, Stanza &ARequest) const
{
  IFileStream *stream = streamById(AStreamId);
  if (stream && stream->streamKind()==IFileStream::SendFile)
  {
    if (!stream->fileName().isEmpty() && stream->fileSize()>0)
    {
      QDomElement siElem = ARequest.firstElement("si",NS_STREAM_INITIATION);
      if (!siElem.isNull())
      {
        siElem.setAttribute("mime-type","text/plain");
        QDomElement fileElem = siElem.appendChild(ARequest.createElement("file",NS_SI_FILETRANSFER)).toElement();
        fileElem.setAttribute("name",stream->fileName().split("/").last());
        fileElem.setAttribute("size",stream->fileSize());
        if (!stream->fileHash().isEmpty())
          fileElem.setAttribute("hash",stream->fileHash());
        if (!stream->fileDate().isValid())
          fileElem.setAttribute("date",DateTime(stream->fileDate()).toX85Date());
        if (!stream->fileDescription().isEmpty())
          fileElem.appendChild(ARequest.createElement("desc")).appendChild(ARequest.createTextNode(stream->fileDescription()));
        if (stream->isRangeSupported())
          fileElem.appendChild(ARequest.createElement("range"));
        return true;
      }
    }
  }
  return false;
}

bool FileStreamsManager::responceDataStream(const QString &AStreamId, Stanza &AResponce) const
{
  IFileStream *stream = streamById(AStreamId);
  if (stream && stream->streamKind()==IFileStream::ReceiveFile)
  {
    if (stream->isRangeSupported() && (stream->rangeOffset()>0 || stream->rangeLength()>0))
    {
      QDomElement siElem = AResponce.firstElement("si",NS_STREAM_INITIATION);
      if (!siElem.isNull())
      {
        QDomElement fileElem = siElem.appendChild(AResponce.createElement("file",NS_SI_FILETRANSFER)).toElement();
        QDomElement rangeElem = fileElem.appendChild(AResponce.createElement("range")).toElement();
        if (stream->rangeOffset()>0)
          rangeElem.setAttribute("offset",stream->rangeOffset());
        if (stream->rangeLength()>0)
          rangeElem.setAttribute("length",stream->rangeLength());
      }
    }
    return true;
  }
  return false;
}

bool FileStreamsManager::dataStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods)
{
  if (!FStreams.contains(AStreamId))
  {
    QList<QString> methods = AMethods.toSet().intersect(FMethods.toSet()).toList();
    if (!methods.isEmpty())
    {
      for (QMultiMap<int, IFileStreamsHandler *>::const_iterator it = FHandlers.constBegin(); it!=FHandlers.constEnd(); it++)
      {
        IFileStreamsHandler *handler = it.value();
        if (handler->fileStreamRequest(it.key(),AStreamId,ARequest,methods))
        {
          return true;
        }
      }
    }
  }
  return false;
}

bool FileStreamsManager::dataStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethod)
{
  IFileStreamsHandler *handler = streamHandler(AStreamId);
  return handler!=NULL ? handler->fileStreamResponce(AStreamId,AResponce,AMethod) : false;
}

bool FileStreamsManager::dataStreamError(const QString &AStreamId, const QString &AError)
{
  IFileStream *stream = streamById(AStreamId);
  if (stream)
  {
    stream->cancelStream(AError);
    return true;
  }
  return false;
}

QList<IFileStream *> FileStreamsManager::streams() const
{
  return FStreams.values();
}

IFileStream *FileStreamsManager::streamById(const QString &AStreamId) const
{
  return FStreams.value(AStreamId, NULL);
}

IFileStream *FileStreamsManager::createStream(IFileStreamsHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid, 
                                                  const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent)
{
  if (FDataManager && AHandler && !AStreamId.isEmpty() && !FStreams.contains(AStreamId))
  {
    IFileStream *stream = new FileStream(FDataManager,AStreamId,AStreamJid,AContactJid,AKind,AParent);
    connect(stream->instance(),SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));
    FStreams.insert(AStreamId,stream);
    FStreamHandler.insert(AStreamId,AHandler);
    emit streamCreated(stream);
    return stream;
  }
  return NULL;
}

QString FileStreamsManager::defaultStreamMethod() const
{
  return FDefaultMethod;
}

void FileStreamsManager::setDefaultStreamMethod(const QString &AMethodNS)
{
  if (FMethods.contains(AMethodNS))
    FDefaultMethod = AMethodNS;
}

QList<QString> FileStreamsManager::streamMethods() const
{
  return FMethods;
}

void FileStreamsManager::insertStreamMethod(const QString &AMethodNS)
{
  if (!FMethods.contains(AMethodNS))
    FMethods.append(AMethodNS);
}

void FileStreamsManager::removeStreamMethod(const QString &AMethodNS)
{
  FMethods.removeAt(FMethods.indexOf(AMethodNS));
}

IFileStreamsHandler *FileStreamsManager::streamHandler(const QString &AStreamId) const
{
  return FStreamHandler.value(AStreamId);
}

void FileStreamsManager::insertStreamsHandler(IFileStreamsHandler *AHandler, int AOrder)
{
  if (AHandler!=NULL)
    FHandlers.insertMulti(AOrder,AHandler);
}

void FileStreamsManager::removeStreamsHandler(IFileStreamsHandler *AHandler, int AOrder)
{
  FHandlers.remove(AOrder,AHandler);
}

void FileStreamsManager::onStreamDestroyed()
{
  IFileStream *stream = qobject_cast<IFileStream *>(sender());
  if (stream)
  {
    FStreams.remove(stream->streamId());
    FStreamHandler.remove(stream->streamId());
    emit streamDestroyed(stream);
  }
}

Q_EXPORT_PLUGIN2(FileStreamsManagerPlugin, FileStreamsManager);
