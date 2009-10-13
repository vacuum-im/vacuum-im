#include "filestreamsmanager.h"

#include <QSet>
#include <QDir>

#define SVN_DEFAULT_DIRECTORY           "defaultDirectory"
#define SVN_SEPARATE_DIRECTORIES        "separateDirectories"
#define SVN_DEFAULT_STREAM_METHOD       "defaultStreamMethod"

FileStreamsManager::FileStreamsManager()
{
  FDataManager = NULL;
  FSettingsPlugin = NULL;
  FTrayManager = NULL;
  FMainWindowPlugin = NULL;

  FSeparateDirectories = false;
  FDefaultDirectory = QDir::homePath()+"/"+tr("Downloads");
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

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ITrayManager").value(0,NULL);
  if (plugin)
  {
     FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return FDataManager!=NULL;
}

bool FileStreamsManager::initObjects()
{
  if (FDataManager)
  {
    FDataManager->insertProfile(this);
  }
  if (FTrayManager || FMainWindowPlugin)
  {
    Action *action = new Action;
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILESTREAMSMANAGER);
    action->setText(tr("File Transfers"));
    connect(action,SIGNAL(triggered(bool)),SLOT(onShowFileStreamsWindow(bool)));
    if (FMainWindowPlugin)
      FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action,AG_MMENU_FILESTREAMSMANAGER,true);
    if (FTrayManager)
      FTrayManager->addAction(action, AG_TMTM_FILESTREAMSMANAGER, true);
  }
  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
    FSettingsPlugin->openOptionsNode(ON_FILETRANSFER,tr("File Transfer"),tr("Common options for file transfer"),MNI_FILESTREAMSMANAGER,ONO_FILETRANSFER);
  }
  return true;
}

QWidget *FileStreamsManager::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_FILETRANSFER)
  {
    AOrder = OWO_FILESTREAMSMANAGER;
    if (FDataManager)
    {
      FileStreamsOptions *widget = new FileStreamsOptions(FDataManager, this);
      connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
      return widget;
    }
  }
  return NULL;
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
    stream->abortStream(AError);
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

QString FileStreamsManager::defaultDirectory() const
{
  return FDefaultDirectory;
}

QString FileStreamsManager::defaultDirectory(const Jid &AContactJid) const
{
  QString dir = FDefaultDirectory;
  if (FSeparateDirectories && !AContactJid.domain().isEmpty())
    dir += "/" + AContactJid.encode(AContactJid.pBare());
  return dir;
}

void FileStreamsManager::setDefaultDirectory(const QString &ADirectory)
{
  FDefaultDirectory = ADirectory;
}

bool FileStreamsManager::separateDirectories() const
{
  return FSeparateDirectories;
}

void FileStreamsManager::setSeparateDirectories(bool ASeparate)
{
  FSeparateDirectories = ASeparate;
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

void FileStreamsManager::onShowFileStreamsWindow(bool)
{
  if (FFileStreamsWindow.isNull())
    FFileStreamsWindow = new FileStreamsWindow(this, FSettingsPlugin!=NULL ? FSettingsPlugin->settingsForPlugin(pluginUuid()) : NULL, NULL);
  FFileStreamsWindow->show();
  FFileStreamsWindow->activateWindow();
  FFileStreamsWindow->raise();
}

void FileStreamsManager::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(FILESTREAMSMANAGER_UUID);
  FDefaultDirectory = settings->value(SVN_DEFAULT_DIRECTORY, FSettingsPlugin->homeDir().path()+"/"+tr("Downloads")).toString();
  FSeparateDirectories = settings->value(SVN_SEPARATE_DIRECTORIES, false).toBool();
  setDefaultStreamMethod(settings->value(SVN_DEFAULT_STREAM_METHOD).toString());
}

void FileStreamsManager::onSettingsClosed()
{
  if (!FFileStreamsWindow.isNull())
    delete FFileStreamsWindow;

  foreach(IFileStream *stream, FStreams.values())
    delete stream->instance();

  ISettings *settings = FSettingsPlugin->settingsForPlugin(FILESTREAMSMANAGER_UUID);
  settings->setValue(SVN_DEFAULT_DIRECTORY, FDefaultDirectory);
  settings->setValue(SVN_SEPARATE_DIRECTORIES, FSeparateDirectories);
  settings->setValue(SVN_DEFAULT_STREAM_METHOD, FDefaultMethod);
}

Q_EXPORT_PLUGIN2(FileStreamsManagerPlugin, FileStreamsManager);
