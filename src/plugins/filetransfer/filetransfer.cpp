#include "filetransfer.h"

#include <QDir>
#include <QTimer>

#define ADR_STREAM_JID                Action::DR_StreamJid
#define ADR_CONTACT_JID               Action::DR_Parametr1
#define ADR_FILE_NAME                 Action::DR_Parametr2

#define SVN_AUTO_RECEIVE              "autoReceive"
#define SVN_HIDE_DIALOG_STARTED       "hideDialogWhenStarted"
#define SVN_REMOVE_TRANSFER_FINISHED  "removeTransferWhenFinished"

#define NOTIFICATOR_ID                "FileTransfer"

#define REMOVE_FINISHED_TIMEOUT       10000

FileTransfer::FileTransfer()
{
  FRosterPlugin = NULL;
  FDiscovery = NULL;
  FNotifications = NULL;
  FFileManager = NULL;
  FDataManager = NULL;
  FSettingsPlugin = NULL;

  FAutoReceive = false;
  FHideDialogWhenStarted = false;
  FRemoveTransferWhenFinished = false;
}

FileTransfer::~FileTransfer()
{

}

void FileTransfer::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Transfer files between two XMPP entities");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Files Transfer");
  APluginInfo->uid = FILETRANSFER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(FILESTREAMSMANAGER_UUID);
  APluginInfo->dependences.append(DATASTREAMSMANAGER_UUID);
}

bool FileTransfer::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IFileStreamsManager").value(0,NULL);
  if (plugin)
  {
    FFileManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
  }
  
  plugin = APluginManager->getPlugins("IDataStreamsManager").value(0,NULL);
  if (plugin)
  {
    FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin)
  {
    FNotifications = qobject_cast<INotifications *>(plugin->instance());
    if (FNotifications)
    {
      connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
      connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
    }
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

  return FFileManager!=NULL && FDataManager!=NULL;
}

bool FileTransfer::initObjects()
{
  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_SI_FILETRANSFER,this,DFO_DEFAULT);
  }
  if (FNotifications)
  {
    uchar kindMask = INotification::RosterIcon|INotification::PopupWindow|INotification::TrayIcon|INotification::TrayAction|INotification::PlaySound;
    FNotifications->insertNotificator(NOTIFICATOR_ID,tr("File Transfer"),kindMask,kindMask);
  }
  if (FFileManager)
  {
    FFileManager->insertStreamsHandler(this,FSHO_FILETRANSFER);
  }
  if (FSettingsPlugin)
  {
    FSettingsPlugin->insertOptionsHolder(this);
  }
  return true;
}

QWidget *FileTransfer::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_FILETRANSFER)
  {
    AOrder = OWO_FILETRANSFER;
    FileTransferOptions *widget = new FileTransferOptions(this);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

bool FileTransfer::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_SI_FILETRANSFER)
    return sendFile(AStreamJid,ADiscoInfo.contactJid)!=NULL;
  return false;
}

Action *FileTransfer::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (AFeature == NS_SI_FILETRANSFER)
  {
    if (isSupported(AStreamJid,ADiscoInfo.contactJid))
    {
      Action *action = new Action(AParent);
      action->setText(tr("Send File"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onShowSendFileDialogByAction(bool)));
      return action;
    }
  }
  return NULL;
}

bool FileTransfer::fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods)
{
  if (AOrder == FSHO_FILETRANSFER)
  {
    QDomElement fileElem = ARequest.firstElement("si",NS_STREAM_INITIATION).firstChildElement("file");
    while (!fileElem.isNull() && fileElem.namespaceURI()!=NS_SI_FILETRANSFER)
      fileElem = fileElem.nextSiblingElement("file");

    QString fileName = fileElem.attribute("name");
    qint64 fileSize = fileElem.attribute("size").toInt();
    if (!fileName.isEmpty() && fileSize>0)
    {
      IFileStream *stream = createStream(AStreamId,ARequest.to(),ARequest.from(),IFileStream::ReceiveFile);
      if (stream)
      {
        stream->setFileName(QDir(FFileManager->defaultDirectory(stream->contactJid())).absoluteFilePath(fileName));
        stream->setFileSize(fileSize);
        stream->setFileHash(fileElem.attribute("hash"));
        stream->setFileDate(DateTime(fileElem.attribute("date")).toLocal());
        stream->setFileDescription(fileElem.firstChildElement("desc").text());
        stream->setRangeSupported(!fileElem.firstChildElement("range").isNull());

        StreamDialog *dialog = createStreamDialog(stream);
        dialog->setSelectableMethods(AMethods);

        if (AMethods.contains(FFileManager->defaultStreamMethod()))
          autoStartStream(stream);

        notifyStream(stream, true);
        return true;
      }
    }
  }
  return false;
}

bool FileTransfer::fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS)
{
  if (FFileManager!=NULL && FFileManager->streamHandler(AStreamId)==this)
  {
    IFileStream *stream = FFileManager->streamById(AStreamId);
    QDomElement rangeElem = AResponce.firstElement("si",NS_STREAM_INITIATION).firstChildElement("file").firstChildElement("range");
    if (!rangeElem.isNull())
    {
      if (rangeElem.hasAttribute("offset"))
        stream->setRangeOffset(rangeElem.attribute("offset").toInt());
      if (rangeElem.hasAttribute("length"))
        stream->setRangeLength(rangeElem.attribute("length").toInt());
    }
    return stream->startStream(AMethodNS);
  }
  return false;
}

bool FileTransfer::fileStreamShowDialog(const QString &AStreamId)
{
  IFileStream *stream = FFileManager!=NULL ? FFileManager->streamById(AStreamId) : NULL;
  if (stream && FFileManager->streamHandler(AStreamId)==this)
  {
    StreamDialog *dialog = createStreamDialog(stream);
    dialog->show();
    dialog->activateWindow();
    dialog->raise();
    return true;
  }
  return false;
}

bool FileTransfer::autoReceive() const
{
  return FAutoReceive;
}

void FileTransfer::setAutoReceive(bool AAuto)
{
  FAutoReceive = AAuto;
}

bool FileTransfer::hideDialogWhenStarted() const
{
  return FHideDialogWhenStarted;
}

void FileTransfer::setHideDialogWhenStarted(bool AHide)
{
  FHideDialogWhenStarted = AHide;
}

bool FileTransfer::removeTransferWhenFinished() const
{
  return FRemoveTransferWhenFinished;
}

void FileTransfer::setRemoveTransferWhenFinished(bool ARemove)
{
  FRemoveTransferWhenFinished = ARemove;
}

bool FileTransfer::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
  Q_UNUSED(AStreamJid);

  if (!FFileManager || !FDataManager)
    return false;

  if (FFileManager->streamMethods().isEmpty())
    return false;

  if (FDiscovery && FDiscovery->hasDiscoInfo(AContactJid))
  {
    IDiscoInfo info = FDiscovery->discoInfo(AContactJid);
    if (!info.features.contains(NS_SI_FILETRANSFER))
      return false;
  }

  return true;
}

IFileStream *FileTransfer::sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName)
{
  if (isSupported(AStreamJid,AContactJid))
  {
    IFileStream *stream = createStream(QUuid::createUuid().toString(),AStreamJid,AContactJid,IFileStream::SendFile);
    if (stream)
    {
      stream->setFileName(AFileName);
      StreamDialog *dialog = createStreamDialog(stream);
      dialog->setSelectableMethods(FFileManager->streamMethods());
      dialog->show();
      return stream;
    }
  }
  return NULL;
}

void FileTransfer::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.var = NS_SI_FILETRANSFER;
  dfeature.active = true;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_FILETRANSFER_SEND);
  dfeature.name = tr("File Transfer");
  dfeature.description = tr("Transfer files between two XMPP entities");
  FDiscovery->insertDiscoFeature(dfeature);
}

void FileTransfer::notifyStream(IFileStream *AStream, bool ANewStream)
{
  if (FNotifications)
  {
    StreamDialog *dialog = FStreamDialog.value(AStream->streamId());
    if (dialog==NULL || !dialog->isActiveWindow())
    {
      QString file = !AStream->fileName().isEmpty() ? AStream->fileName().split("/").last() : QString::null;

      INotification notify;
      notify.kinds = FNotifications->notificatorKinds(NOTIFICATOR_ID);
      notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AStream->streamKind()==IFileStream::SendFile ? MNI_FILETRANSFER_SEND : MNI_FILETRANSFER_RECEIVE));
      notify.data.insert(NDR_WINDOW_TITLE,FNotifications->contactName(AStream->streamJid(),AStream->contactJid()));
      notify.data.insert(NDR_WINDOW_IMAGE,FNotifications->contactAvatar(AStream->contactJid()));
      notify.data.insert(NDR_WINDOW_CAPTION, file);

      switch (AStream->streamState())
      {
      case IFileStream::Creating:
        if (AStream->streamKind() == IFileStream::ReceiveFile)
        {
          notify.data.insert(NDR_TOOLTIP,tr("Requested file transfer: %1").arg(file));
          notify.data.insert(NDR_WINDOW_TEXT, tr("You received a request to transfer the file"));
          notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_INCOMING);
        }
        break;
      case IFileStream::Negotiating:
      case IFileStream::Connecting:
      case IFileStream::Transfering:
        if (ANewStream)
        {
          notify.kinds &= ~INotification::TrayIcon;
          if (AStream->streamKind() == IFileStream::SendFile)
          {
            notify.data.insert(NDR_TOOLTIP,tr("Auto sending file: %1").arg(file));
            notify.data.insert(NDR_WINDOW_TEXT, tr("File sending is started automatically"));
          }
          else
          {
            notify.data.insert(NDR_TOOLTIP,tr("Auto receiving file: %1").arg(file));
            notify.data.insert(NDR_WINDOW_TEXT, tr("File receiving is started automatically"));
          }
          notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_INCOMING);
        }
        else
        {
          notify.kinds = 0;
        }
        break;
      case IFileStream::Finished:
        notify.data.insert(NDR_TOOLTIP,tr("Completed transferring file: %1").arg(file));
        notify.data.insert(NDR_WINDOW_TEXT, tr("File transfer completed"));
        notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_COMPLETE);
        break;
      case IFileStream::Aborted:
        notify.data.insert(NDR_TOOLTIP,tr("Canceled transferring file: %1").arg(file));
        notify.data.insert(NDR_WINDOW_TEXT, tr("File transfer canceled: %1").arg(Qt::escape(AStream->stateString())));
        notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_CANCELED);
        break;
      default:
        notify.kinds = 0;
      }

      if (notify.kinds > 0)
      {
        if (FStreamNotify.contains(AStream->streamId()))
          FNotifications->removeNotification(FStreamNotify.value(AStream->streamId()));
        int notifyId = FNotifications->appendNotification(notify);
        FStreamNotify.insert(AStream->streamId(),notifyId);
      }
    }
  }
}

void FileTransfer::autoStartStream(IFileStream *AStream)
{
  if (FAutoReceive && AStream->streamKind()==IFileStream::ReceiveFile)
  {
    if (!QFile::exists(AStream->fileName()))
    {
      IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStream->streamJid()) : NULL;
      if (roster && roster->rosterItem(AStream->contactJid()).isValid)
        AStream->startStream(FFileManager->defaultStreamMethod());
    }
  }
}

StreamDialog *FileTransfer::createStreamDialog(IFileStream *AStream)
{
  StreamDialog *dialog = FStreamDialog.value(AStream->streamId());
  if (dialog == NULL)
  {
    dialog = new StreamDialog(FDataManager,FFileManager,AStream,NULL);
    if (AStream->streamKind() == IFileStream::SendFile)
      IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_SEND,0,0,"windowIcon");
    else
      IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_RECEIVE,0,0,"windowIcon");
    connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onStreamDialogDestroyed()));
    FStreamDialog.insert(AStream->streamId(),dialog);
  }
  return dialog;
}

IFileStream *FileTransfer::createStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind)
{
  IFileStream *stream = FFileManager!=NULL ? FFileManager->createStream(this,AStreamId,AStreamJid,AContactJid,AStreamKind,this) : NULL;
  if (stream)
  {
    connect(stream->instance(),SIGNAL(stateChanged()),SLOT(onStreamStateChanged()));
    connect(stream->instance(),SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));
  }
  return stream;
}

void FileTransfer::onStreamStateChanged()
{
  IFileStream *stream = qobject_cast<IFileStream *>(sender());
  if (stream)
  {
    if (stream->streamState() == IFileStream::Transfering)
    {
      if (FHideDialogWhenStarted && FStreamDialog.contains(stream->streamId()))
        FStreamDialog.value(stream->streamId())->close();
    }
    else if (stream->streamState() == IFileStream::Finished)
    {
      if (FRemoveTransferWhenFinished)
        QTimer::singleShot(REMOVE_FINISHED_TIMEOUT, stream->instance(), SLOT(deleteLater()));
    }
    notifyStream(stream);
  }
}

void FileTransfer::onStreamDestroyed()
{
  IFileStream *stream = qobject_cast<IFileStream *>(sender());
  if (stream)
  {
    if (FNotifications && FStreamNotify.contains(stream->streamId()))
      FNotifications->removeNotification(FStreamNotify.value(stream->streamId()));
  }
}

void FileTransfer::onStreamDialogDestroyed()
{
  StreamDialog *dialog = qobject_cast<StreamDialog *>(sender());
  if (dialog)
    FStreamDialog.remove(FStreamDialog.key(dialog));
}

void FileTransfer::onShowSendFileDialogByAction(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    QString file = action->data(ADR_FILE_NAME).toString();
    sendFile(streamJid,contactJid,file);
  }
}

void FileTransfer::onNotificationActivated(int ANotifyId)
{
  if (fileStreamShowDialog(FStreamNotify.key(ANotifyId)))
    FNotifications->removeNotification(ANotifyId);
}

void FileTransfer::onNotificationRemoved(int ANotifyId)
{
  FStreamNotify.remove(FStreamNotify.key(ANotifyId));
}

void FileTransfer::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(FILETRANSFER_UUID);
  FAutoReceive = settings->value(SVN_AUTO_RECEIVE, false).toBool();
  FHideDialogWhenStarted = settings->value(SVN_HIDE_DIALOG_STARTED, false).toBool();
  FRemoveTransferWhenFinished = settings->value(SVN_REMOVE_TRANSFER_FINISHED, false).toBool();
}

void FileTransfer::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(FILETRANSFER_UUID);
  settings->setValue(SVN_AUTO_RECEIVE,FAutoReceive);
  settings->setValue(SVN_HIDE_DIALOG_STARTED,FHideDialogWhenStarted);
  settings->setValue(SVN_REMOVE_TRANSFER_FINISHED,FRemoveTransferWhenFinished);
}


Q_EXPORT_PLUGIN2(FileTransferPlugin, FileTransfer);
