#include "filetransfer.h"

#include <QDir>
#include <QTimer>
#include <QFileInfo>

#define ADR_STREAM_JID                Action::DR_StreamJid
#define ADR_CONTACT_JID               Action::DR_Parametr1
#define ADR_FILE_NAME                 Action::DR_Parametr2
#define ADR_FILE_DESC                 Action::DR_Parametr3

#define REMOVE_FINISHED_TIMEOUT       10000

FileTransfer::FileTransfer()
{
	FRosterPlugin = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FFileManager = NULL;
	FDataManager = NULL;
	FMessageWidgets = NULL;
	FMessageArchiver = NULL;
	FOptionsManager = NULL;
	FRostersViewPlugin = NULL;
}

FileTransfer::~FileTransfer()
{

}

void FileTransfer::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("File Transfer");
	APluginInfo->description = tr("Allows to send a file to another contact");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(FILESTREAMSMANAGER_UUID);
	APluginInfo->dependences.append(DATASTREAMSMANAGER_UUID);
}

bool FileTransfer::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
	if (plugin)
	{
		FFileManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
	if (plugin)
	{
		FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
			connect(FDiscovery->instance(),SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoRemoved(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)),SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)),SLOT(onNotificationRemoved(int)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(), SIGNAL(toolBarWidgetCreated(IToolBarWidget *)), SLOT(onToolBarWidgetCreated(IToolBarWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageArchiver").value(0,NULL);
	if (plugin)
	{
		FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FFileManager!=NULL && FDataManager!=NULL;
}

bool FileTransfer::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SENDFILE, tr("Send file"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SENDFILE, tr("Send file"), QKeySequence::UnknownKey, Shortcuts::WidgetShortcut);

	if (FDiscovery)
	{
		registerDiscoFeatures();
		FDiscovery->insertFeatureHandler(NS_SI_FILETRANSFER,this,DFO_DEFAULT);
	}
	if (FNotifications)
	{
		INotificationType notifyType;
		notifyType.order = NTO_FILETRANSFER_NOTIFY;
		notifyType.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_FILETRANSFER_RECEIVE);
		notifyType.title = tr("When receiving a prompt to accept the file");
		notifyType.kindMask = INotification::RosterNotify|INotification::PopupWindow|INotification::TrayNotify|INotification::TrayAction|INotification::SoundPlay|INotification::AlertWidget|INotification::ShowMinimized|INotification::AutoActivate;
		notifyType.kindDefs = notifyType.kindMask & ~(INotification::AutoActivate);
		FNotifications->registerNotificationType(NNT_FILETRANSFER,notifyType);
	}
	if (FFileManager)
	{
		FFileManager->insertStreamsHandler(this,FSHO_FILETRANSFER);
	}
	if (FRostersViewPlugin)
	{
		FRostersViewPlugin->rostersView()->insertDragDropHandler(this);
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SENDFILE,FRostersViewPlugin->rostersView()->instance());
	}
	if (FMessageWidgets)
	{
		FMessageWidgets->insertViewDropHandler(this);
	}
	return true;
}

bool FileTransfer::initSettings()
{
	Options::setDefaultValue(OPV_FILETRANSFER_AUTORECEIVE,false);
	Options::setDefaultValue(OPV_FILETRANSFER_HIDEONSTART,false);
	Options::setDefaultValue(OPV_FILETRANSFER_REMOVEONFINISH,false);

	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> FileTransfer::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_FILETRANSFER)
	{
		widgets.insertMulti(OWO_FILETRANSFER,FOptionsManager->optionsNodeWidget(Options::node(OPV_FILETRANSFER_AUTORECEIVE),tr("Automatically receive files from contacts in roster"),AParent));
		widgets.insertMulti(OWO_FILETRANSFER,FOptionsManager->optionsNodeWidget(Options::node(OPV_FILETRANSFER_HIDEONSTART),tr("Hide dialog after transfer started"),AParent));
		widgets.insertMulti(OWO_FILETRANSFER,FOptionsManager->optionsNodeWidget(Options::node(OPV_FILETRANSFER_REMOVEONFINISH),tr("Automatically remove finished transfers"),AParent));
	}
	return widgets;
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

Qt::DropActions FileTransfer::rosterDragStart(const QMouseEvent *AEvent, IRosterIndex *AIndex, QDrag *ADrag)
{
	Q_UNUSED(AEvent); Q_UNUSED(AIndex); Q_UNUSED(ADrag);
	return Qt::IgnoreAction;
}

bool FileTransfer::rosterDragEnter(const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasUrls())
	{
		QList<QUrl> urlList = AEvent->mimeData()->urls();
		if (urlList.count()==1 && QFileInfo(urlList.first().toLocalFile()).isFile())
			return true;
	}
	return false;
}

bool FileTransfer::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	Q_UNUSED(AEvent);
	return AHover->data(RDR_TYPE).toInt()!=RIT_STREAM_ROOT && isSupported(AHover->data(RDR_STREAM_JID).toString(), AHover->data(RDR_FULL_JID).toString());
}

void FileTransfer::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool FileTransfer::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu)
{
	if (AEvent->dropAction()!=Qt::IgnoreAction && AIndex->data(RDR_TYPE).toInt()!=RIT_STREAM_ROOT && 
		isSupported(AIndex->data(RDR_STREAM_JID).toString(),AIndex->data(RDR_FULL_JID).toString()))
	{
		Action *action = new Action(AMenu);
		action->setText(tr("Send File"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
		action->setData(ADR_STREAM_JID,AIndex->data(RDR_STREAM_JID).toString());
		action->setData(ADR_CONTACT_JID,AIndex->data(RDR_FULL_JID).toString());
		action->setData(ADR_FILE_NAME, AEvent->mimeData()->urls().value(0).toLocalFile());
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowSendFileDialogByAction(bool)));
		AMenu->addAction(action, AG_DEFAULT, true);
		AMenu->setDefaultAction(action);
		return true;
	}
	return false;
}

bool FileTransfer::viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent)
{
	if (isSupported(AWidget->streamJid(), AWidget->contactJid()) && AEvent->mimeData()->hasUrls())
	{
		QList<QUrl> urlList = AEvent->mimeData()->urls();
		if (urlList.count()==1 && QFileInfo(urlList.first().toLocalFile()).isFile())
			return true;
	}
	return false;
}

bool FileTransfer::viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
	return true;
}

void FileTransfer::viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AWidget);
	Q_UNUSED(AEvent);
}

bool FileTransfer::viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu)
{
	if (AEvent->dropAction() != Qt::IgnoreAction)
	{
		Action *action = new Action(AMenu);
		action->setText(tr("Send File"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
		action->setData(ADR_STREAM_JID,AWidget->streamJid().full());
		action->setData(ADR_CONTACT_JID,AWidget->contactJid().full());
		action->setData(ADR_FILE_NAME, AEvent->mimeData()->urls().first().toLocalFile());
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowSendFileDialogByAction(bool)));
		AMenu->addAction(action, AG_DEFAULT, true);
		AMenu->setDefaultAction(action);
		return true;
	}
	return false;
}

bool FileTransfer::fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods)
{
	if (AOrder == FSHO_FILETRANSFER)
	{
		QDomElement fileElem = ARequest.firstElement("si",NS_STREAM_INITIATION).firstChildElement("file");
		while (!fileElem.isNull() && fileElem.namespaceURI()!=NS_SI_FILETRANSFER)
			fileElem = fileElem.nextSiblingElement("file");

		QString fileName = fileElem.attribute("name");
		qint64 fileSize = fileElem.attribute("size").toLongLong();

		QList<QString> methods = AMethods.toSet().intersect(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList().toSet()).toList();
		if (!fileName.isEmpty() && fileSize>0)
		{
			IFileStream *stream = createStream(AStreamId,ARequest.to(),ARequest.from(),IFileStream::ReceiveFile);
			if (stream)
			{
				QString dirName = Options::node(OPV_FILESTREAMS_DEFAULTDIR).value().toString();
				if (Options::node(OPV_FILESTREAMS_GROUPBYSENDER).value().toBool())
				{
					QString userDir = dirNameByUserName(FNotifications!=NULL ? FNotifications->contactName(stream->streamJid(),stream->contactJid()) : stream->contactJid().uNode());
					if (!userDir.isEmpty())
						dirName += "/" + userDir;
				}

				stream->setFileName(QDir(dirName).absoluteFilePath(fileName));
				stream->setFileSize(fileSize);
				stream->setFileHash(fileElem.attribute("hash"));
				stream->setFileDate(DateTime(fileElem.attribute("date")).toLocal());
				stream->setFileDescription(fileElem.firstChildElement("desc").text());
				stream->setRangeSupported(!fileElem.firstChildElement("range").isNull());
				stream->setAcceptableMethods(methods);

				StreamDialog *dialog = getStreamDialog(stream);
				dialog->setSelectableMethods(methods);

				if (methods.contains(Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString()))
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
				stream->setRangeOffset(rangeElem.attribute("offset").toLongLong());
			if (rangeElem.hasAttribute("length"))
				stream->setRangeLength(rangeElem.attribute("length").toLongLong());
		}
		if (!stream->startStream(AMethodNS))
			stream->abortStream(tr("Failed to start file transfer"));
		else
			return true;
	}
	return false;
}

bool FileTransfer::fileStreamShowDialog(const QString &AStreamId)
{
	IFileStream *stream = FFileManager!=NULL ? FFileManager->streamById(AStreamId) : NULL;
	if (stream && FFileManager->streamHandler(AStreamId)==this)
	{
		StreamDialog *dialog = getStreamDialog(stream);
		WidgetManager::showActivateRaiseWindow(dialog);
		return true;
	}
	return false;
}

bool FileTransfer::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
	Q_UNUSED(AStreamJid);

	if (!FFileManager || !FDataManager)
		return false;

	if (Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList().isEmpty())
		return false;

	return FDiscovery==NULL || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_SI_FILETRANSFER);
}

IFileStream *FileTransfer::sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName, const QString &AFileDesc)
{
	if (isSupported(AStreamJid,AContactJid))
	{
		IFileStream *stream = createStream(QUuid::createUuid().toString(),AStreamJid,AContactJid,IFileStream::SendFile);
		if (stream)
		{
			stream->setFileName(AFileName);
			stream->setFileDescription(AFileDesc);
			StreamDialog *dialog = getStreamDialog(stream);
			dialog->setSelectableMethods(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList());
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
	dfeature.description = tr("Supports the sending of the file to another contact");
	FDiscovery->insertDiscoFeature(dfeature);
}

void FileTransfer::notifyStream(IFileStream *AStream, bool ANewStream)
{
	if (FNotifications)
	{
		StreamDialog *dialog = getStreamDialog(AStream);
		if (dialog==NULL || !dialog->isActiveWindow())
		{
			QString file = !AStream->fileName().isEmpty() ? AStream->fileName().split("/").last() : QString::null;

			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_FILETRANSFER);
			notify.typeId = NNT_FILETRANSFER;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AStream->streamKind()==IFileStream::SendFile ? MNI_FILETRANSFER_SEND : MNI_FILETRANSFER_RECEIVE));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AStream->streamJid(),AStream->contactJid()));
			notify.data.insert(NDR_STREAM_JID,AStream->streamJid().full());
			notify.data.insert(NDR_CONTACT_JID,AStream->contactJid().full());
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AStream->contactJid()));
			notify.data.insert(NDR_POPUP_CAPTION, file);

			switch (AStream->streamState())
			{
			case IFileStream::Creating:
				if (AStream->streamKind() == IFileStream::ReceiveFile)
				{
					notify.data.insert(NDR_TOOLTIP,tr("Requested file transfer: %1").arg(file));
					notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("You received a request to transfer the file")));
					notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_INCOMING);
				}
				break;
			case IFileStream::Negotiating:
			case IFileStream::Connecting:
			case IFileStream::Transfering:
				if (ANewStream)
				{
					notify.kinds &= ~INotification::TrayNotify;
					if (AStream->streamKind() == IFileStream::SendFile)
					{
						notify.data.insert(NDR_TOOLTIP,tr("Auto sending file: %1").arg(file));
						notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("File sending is started automatically")));
					}
					else
					{
						notify.data.insert(NDR_TOOLTIP,tr("Auto receiving file: %1").arg(file));
						notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("File receiving is started automatically")));
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
				notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("File transfer completed")));
				notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_COMPLETE);
				break;
			case IFileStream::Aborted:
				notify.data.insert(NDR_TOOLTIP,tr("Canceled transferring file: %1").arg(file));
				notify.data.insert(NDR_POPUP_HTML,Qt::escape(tr("File transfer canceled: %1").arg(AStream->stateString())));
				notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_CANCELED);
				break;
			default:
				notify.kinds = 0;
			}

			if (notify.kinds > 0)
			{
				notify.data.insert(NDR_ALERT_WIDGET,(qint64)dialog);
				notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)dialog);
				if (FStreamNotify.contains(AStream->streamId()))
					FNotifications->removeNotification(FStreamNotify.value(AStream->streamId()));
				int notifyId = FNotifications->appendNotification(notify);
				FStreamNotify.insert(AStream->streamId(),notifyId);
			}
		}
	}

	if (AStream->streamState() == IFileStream::Finished)
	{
		QString note = AStream->streamKind()==IFileStream::SendFile ? tr("File '%1' successfully sent.") : tr("File '%1' successfully received.");
		note = note.arg(!AStream->fileName().isEmpty() ? AStream->fileName().split("/").last() : QString::null);
		if (FMessageWidgets)
		{
			IChatWindow *window = FMessageWidgets->findChatWindow(AStream->streamJid(),AStream->contactJid());
			if (window)
			{
				IMessageContentOptions options;
				options.kind = IMessageContentOptions::KindStatus;
				options.type |= IMessageContentOptions::TypeEvent;
				options.direction = IMessageContentOptions::DirectionIn;
				options.time = QDateTime::currentDateTime();
				window->viewWidget()->appendText(note,options);
			}
		}
		if (FMessageArchiver)
		{
			FMessageArchiver->saveNote(AStream->streamJid(),AStream->contactJid(),note);
		}
	}
}

void FileTransfer::autoStartStream(IFileStream *AStream)
{
	if (Options::node(OPV_FILETRANSFER_AUTORECEIVE).value().toBool() && AStream->streamKind()==IFileStream::ReceiveFile)
	{
		if (!QFile::exists(AStream->fileName()))
		{
			IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStream->streamJid()) : NULL;
			if (roster && roster->rosterItem(AStream->contactJid()).isValid)
				AStream->startStream(Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString());
		}
	}
}

void FileTransfer::insertToolBarAction(IToolBarWidget *AWidget)
{
	if (FToolBarActions.value(AWidget) == NULL)
	{
		Action *action = NULL;
		if (isSupported(AWidget->editWidget()->streamJid(),AWidget->editWidget()->contactJid()))
		{
			action = new Action(AWidget->toolBarChanger()->toolBar());
			action->setIcon(RSR_STORAGE_MENUICONS, MNI_FILETRANSFER_SEND);
			action->setText(tr("Send File"));
			action->setShortcutId(SCT_MESSAGEWINDOWS_SENDFILE);
			connect(action,SIGNAL(triggered(bool)),SLOT(onShowSendFileDialogByAction(bool)));
			AWidget->toolBarChanger()->insertAction(action,TBG_MWTBW_FILETRANSFER);
		}
		FToolBarActions.insert(AWidget, action);
	}
	else
	{
		FToolBarActions.value(AWidget)->setEnabled(true);
	}
}

void FileTransfer::removeToolBarAction(IToolBarWidget *AWidget)
{
	if (FToolBarActions.value(AWidget)!=NULL)
	{
		FToolBarActions.value(AWidget)->setEnabled(false);
	}
}

QList<IToolBarWidget *> FileTransfer::findToolBarWidgets(const Jid &AContactJid) const
{
	QList<IToolBarWidget *> toolBars;
	foreach (IToolBarWidget *widget, FToolBarActions.keys())
		if (widget->editWidget()->contactJid()==AContactJid)
			toolBars.append(widget);
	return toolBars;
}

StreamDialog *FileTransfer::getStreamDialog(IFileStream *AStream)
{
	StreamDialog *dialog = FStreamDialog.value(AStream->streamId());
	if (dialog == NULL)
	{
		dialog = new StreamDialog(FDataManager,FFileManager,this,AStream,NULL);
		if (AStream->streamKind() == IFileStream::SendFile)
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_SEND,0,0,"windowIcon");
		else
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_RECEIVE,0,0,"windowIcon");
		if (FNotifications)
		{
			QString name = "<b>"+ Qt::escape(FNotifications->contactName(AStream->streamJid(), AStream->contactJid())) +"</b>";
			if (!AStream->contactJid().resource().isEmpty())
				name += Qt::escape("/" + AStream->contactJid().resource());
			dialog->setContactName(name);
			dialog->installEventFilter(this);
		}
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

QString FileTransfer::dirNameByUserName(const QString &AUserName) const
{
	QString fileName;
	for (int i = 0; i < AUserName.length(); i++)
	{
		if (AUserName.at(i) == '.')
			fileName.append('.');
		else if (AUserName.at(i) == '_')
			fileName.append('_');
		else if (AUserName.at(i) == '-')
			fileName.append('-');
		else if (AUserName.at(i) == ' ')
			fileName.append(' ');
		else if (AUserName.at(i).isLetterOrNumber())
			fileName.append(AUserName.at(i));
	}
	return fileName.trimmed();
}

bool FileTransfer::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (AEvent->type() == QEvent::WindowActivate)
	{
		if (FNotifications)
		{
			QString streamId = FStreamDialog.key(qobject_cast<StreamDialog *>(AObject));
			FNotifications->removeNotification(FStreamNotify.value(streamId));
		}
	}
	return QObject::eventFilter(AObject,AEvent);
}

void FileTransfer::onStreamStateChanged()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
	{
		if (stream->streamState() == IFileStream::Transfering)
		{
			if (Options::node(OPV_FILETRANSFER_HIDEONSTART).value().toBool() && FStreamDialog.contains(stream->streamId()))
				FStreamDialog.value(stream->streamId())->close();
		}
		else if (stream->streamState() == IFileStream::Finished)
		{
			if (Options::node(OPV_FILETRANSFER_REMOVEONFINISH).value().toBool())
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
		IToolBarWidget *widget = FToolBarActions.key(action);
		if (!widget)
		{
			Jid streamJid = action->data(ADR_STREAM_JID).toString();
			Jid contactJid = action->data(ADR_CONTACT_JID).toString();
			QString file = action->data(ADR_FILE_NAME).toString();
			QString desc = action->data(ADR_FILE_DESC).toString();
			sendFile(streamJid,contactJid,file,desc);
		}
		else if (widget->editWidget() != NULL)
			sendFile(widget->editWidget()->streamJid(), widget->editWidget()->contactJid());
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

void FileTransfer::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	foreach(IToolBarWidget *widget, findToolBarWidgets(AInfo.contactJid))
	{
		if (isSupported(widget->editWidget()->streamJid(),widget->editWidget()->contactJid()))
			insertToolBarAction(widget);
		else
			removeToolBarAction(widget);
	}
}

void FileTransfer::onDiscoInfoRemoved(const IDiscoInfo &AInfo)
{
	foreach(IToolBarWidget *widget, findToolBarWidgets(AInfo.contactJid))
		removeToolBarAction(widget);
}

void FileTransfer::onToolBarWidgetCreated(IToolBarWidget *AWidget)
{
	if (AWidget->editWidget() != NULL)
	{
		insertToolBarAction(AWidget);
		connect(AWidget->instance(),SIGNAL(destroyed(QObject *)),SLOT(onToolBarWidgetDestroyed(QObject *)));
		connect(AWidget->editWidget()->instance(),SIGNAL(contactJidChanged(const Jid &)), SLOT(onEditWidgetContactJidChanged(const Jid &)));
	}
}

void FileTransfer::onEditWidgetContactJidChanged(const Jid &ABefore)
{
	Q_UNUSED(ABefore);
	IEditWidget *editWidget = qobject_cast<IEditWidget *>(sender());
	if (editWidget)
	{
		foreach(IToolBarWidget *widget, findToolBarWidgets(editWidget->contactJid()))
		{
			if (isSupported(widget->editWidget()->streamJid(),widget->editWidget()->contactJid()))
				insertToolBarAction(widget);
			else
				removeToolBarAction(widget);
		}
	}
}

void FileTransfer::onToolBarWidgetDestroyed(QObject *AObject)
{
	foreach(IToolBarWidget *widget, FToolBarActions.keys())
		if (qobject_cast<QObject *>(widget->instance()) == AObject)
			FToolBarActions.remove(widget);
}

void FileTransfer::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersViewPlugin && AWidget==FRostersViewPlugin->rostersView()->instance() && !FRostersViewPlugin->rostersView()->hasMultiSelection())
	{
		if (AId == SCT_ROSTERVIEW_SENDFILE)
		{
			IRosterIndex *index = !FRostersViewPlugin->rostersView()->hasMultiSelection() ? FRostersViewPlugin->rostersView()->selectedRosterIndexes().value(0) : NULL;
			int indexType = index!=NULL ? index->data(RDR_TYPE).toInt() : -1;
			if (indexType==RIT_CONTACT || indexType==RIT_AGENT)
				sendFile(index->data(RDR_STREAM_JID).toString(),index->data(RDR_FULL_JID).toString());
		}
	}
}

Q_EXPORT_PLUGIN2(plg_filetransfer, FileTransfer);
