#include "filetransfer.h"

#include <QMimeData>
#include <QDir>
#include <QUrl>
#include <QTimer>
#include <QFileInfo>
#include <QFileDialog>
#include <definitions/namespaces.h>
#include <definitions/menuicons.h>
#include <definitions/soundfiles.h>
#include <definitions/resources.h>
#include <definitions/shortcuts.h>
#include <definitions/toolbargroups.h>
#include <definitions/fshandlerorders.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/discofeaturehandlerorders.h>
#include <definitions/notificationtypes.h>
#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypeorders.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/internalerrors.h>
#include <definitions/publicdatastreamparameters.h>
#include <definitions/publicdatastreamhandlerorders.h>
#include <definitions/messageviewurlhandlerorders.h>
#include <definitions/messagewriterorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/shortcuts.h>
#include <utils/datetime.h>
#include <utils/options.h>
#include <utils/action.h>
#include <utils/logger.h>
#include <utils/jid.h>

#define ADR_STREAM_JID                Action::DR_StreamJid
#define ADR_CONTACT_JID               Action::DR_Parametr1
#define ADR_FILE_NAME                 Action::DR_Parametr2

#define XMPP_URI_RECV                 "recvfile"

FileTransfer::FileTransfer()
{
	FRosterManager = NULL;
	FDiscovery = NULL;
	FNotifications = NULL;
	FFileManager = NULL;
	FDataManager = NULL;
	FDataPublisher = NULL;
	FMessageWidgets = NULL;
	FOptionsManager = NULL;
	FRostersViewPlugin = NULL;
	FMessageProcessor = NULL;
	FXmppUriQueries = NULL;
	FMultiChatManager = NULL;
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

bool FileTransfer::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
	if (plugin)
	{
		FFileManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
	if (plugin)
	{
		FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
		if (FDataManager)
		{
			connect(FDataManager->instance(),SIGNAL(streamInitStarted(const IDataStream &)),SLOT(onDataStreamInitStarted(const IDataStream &)));
			connect(FDataManager->instance(),SIGNAL(streamInitFinished(const IDataStream &, const XmppError &)),SLOT(onDataStreamInitFinished(const IDataStream &, const XmppError &)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataStreamsPublisher").value(0,NULL);
	if (plugin)
	{
		FDataPublisher = qobject_cast<IDataStreamsPublisher *>(plugin->instance());
		if (FDataPublisher)
		{
			connect(FDataPublisher->instance(),SIGNAL(streamStartAccepted(const QString &, const QString &)),
				SLOT(onPublicStreamStartAccepted(const QString &, const QString &)));
			connect(FDataPublisher->instance(),SIGNAL(streamStartRejected(const QString &, const XmppError &)),
				SLOT(onPublicStreamStartRejected(const QString &, const XmppError &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
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
			connect(FMessageWidgets->instance(), SIGNAL(toolBarWidgetCreated(IMessageToolBarWidget *)), SLOT(onToolBarWidgetCreated(IMessageToolBarWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
	if (plugin)
	{
		FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMultiUserChatManager").value(0,NULL);
	if (plugin)
	{
		FMultiChatManager = qobject_cast<IMultiUserChatManager *>(plugin->instance());
	}

	return FFileManager!=NULL && FDataManager!=NULL;
}

bool FileTransfer::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SENDFILE, tr("Send file"), tr("Ctrl+S","Send file"));

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILETRANSFER_TRANSFER_NOT_STARTED,tr("Failed to start file transfer"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_FILETRANSFER_TRANSFER_TERMINATED,tr("Data transmission terminated"));

	if (FDiscovery)
	{
		IDiscoFeature sift;
		sift.var = NS_SI_FILETRANSFER;
		sift.active = true;
		sift.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_FILETRANSFER_SEND);
		sift.name = tr("File Transfer");
		sift.description = tr("Supports the sending of the file to another contact");
		FDiscovery->insertDiscoFeature(sift);

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
		FFileManager->insertStreamsHandler(FSHO_FILETRANSFER,this);
	}
	if (FDataPublisher)
	{
		FDataPublisher->insertStreamHandler(PDSHO_DEFAULT, this);
	}
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	if (FRostersViewPlugin)
	{
		FRostersViewPlugin->rostersView()->insertDragDropHandler(this);
	}
	if (FMessageWidgets)
	{
		FMessageWidgets->insertViewDropHandler(this);
		FMessageWidgets->insertViewUrlHandler(MVUHO_FILETRANSFER,this);
	}
	if (FMessageProcessor)
	{
		FMessageProcessor->insertMessageWriter(MWO_FILETRANSFER,this);
	}
	if (FXmppUriQueries)
	{
		FXmppUriQueries->insertUriHandler(XUHO_DEFAULT,this);
	}
	return true;
}

bool FileTransfer::initSettings()
{
	Options::setDefaultValue(OPV_FILETRANSFER_AUTORECEIVE,false);
	Options::setDefaultValue(OPV_FILETRANSFER_HIDEONSTART,false);
	return true;
}

QMultiMap<int, IOptionsDialogWidget *> FileTransfer::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_DATATRANSFER)
	{
		widgets.insertMulti(OWO_DATATRANSFER_AUTORECEIVE,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_FILETRANSFER_AUTORECEIVE),tr("Automatically receive files from authorized contacts"),AParent));
		widgets.insertMulti(OWO_DATATRANSFER_HIDEONSTART,FOptionsManager->newOptionsDialogWidget(Options::node(OPV_FILETRANSFER_HIDEONSTART),tr("Hide file transfer dialog after transfer started"),AParent));
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
	if (AFeature==NS_SI_FILETRANSFER && isSupported(AStreamJid,ADiscoInfo.contactJid))
	{
		Action *action = new Action(AParent);
		action->setText(tr("Send File"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
		action->setData(ADR_STREAM_JID,AStreamJid.full());
		action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
		connect(action,SIGNAL(triggered(bool)),SLOT(onSendFileByAction(bool)));
		return action;
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
		foreach(const QUrl &url, AEvent->mimeData()->urls())
			if (!QFileInfo(url.toLocalFile()).isFile())
				return false;
		return true;
	}
	return false;
}

bool FileTransfer::rosterDragMove(const QDragMoveEvent *AEvent, IRosterIndex *AHover)
{
	if (AHover->data(RDR_KIND).toInt()==RIK_MUC_ITEM && FDataPublisher!=NULL && FMessageProcessor!=NULL)
		return AHover->data(RDR_SHOW).toInt()!=IPresence::Offline && AHover->data(RDR_SHOW).toInt()!=IPresence::Error;
	else if (AHover->data(RDR_KIND).toInt()!=RIK_STREAM_ROOT && AEvent->mimeData()->urls().count()==1)
		return isSupported(AHover->data(RDR_STREAM_JID).toString(),AHover->data(RDR_FULL_JID).toString());
	return false;
}

void FileTransfer::rosterDragLeave(const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AEvent);
}

bool FileTransfer::rosterDropAction(const QDropEvent *AEvent, IRosterIndex *AIndex, Menu *AMenu)
{
	if (AEvent->dropAction() != Qt::IgnoreAction)
	{
		QStringList filesList;
		foreach(const QUrl &url, AEvent->mimeData()->urls())
			filesList.append(url.toLocalFile());

		if (AIndex->data(RDR_KIND).toInt() == RIK_MUC_ITEM)
		{
			Jid contactJid = AIndex->data(RDR_FULL_JID).toString();
			contactJid.setResource(AIndex->data(RDR_MUC_NICK).toString());

			Action *publishAction = new Action(AMenu);
			publishAction->setText(tr("Send File"));
			publishAction->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
			publishAction->setData(ADR_STREAM_JID,AIndex->data(RDR_STREAM_JID).toString());
			publishAction->setData(ADR_CONTACT_JID,contactJid.full());
			publishAction->setData(ADR_FILE_NAME,filesList);
			connect(publishAction,SIGNAL(triggered(bool)),SLOT(onPublishFilesByAction(bool)));
			AMenu->addAction(publishAction,AG_DEFAULT,true);
			AMenu->setDefaultAction(publishAction);
			return true;
		}
		else if (AIndex->data(RDR_KIND).toInt() != RIK_STREAM_ROOT)
		{
			Action *sendAction = new Action(AMenu);
			sendAction->setText(tr("Send File"));
			sendAction->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
			sendAction->setData(ADR_STREAM_JID,AIndex->data(RDR_STREAM_JID).toString());
			sendAction->setData(ADR_CONTACT_JID,AIndex->data(RDR_FULL_JID).toString());
			sendAction->setData(ADR_FILE_NAME,filesList.value(0));
			connect(sendAction,SIGNAL(triggered(bool)),SLOT(onSendFileByAction(bool)));
			AMenu->addAction(sendAction,AG_DEFAULT,true);
			AMenu->setDefaultAction(sendAction);
			return true;
		}
	}
	return false;
}

bool FileTransfer::messageViewDragEnter(IMessageViewWidget *AWidget, const QDragEnterEvent *AEvent)
{
	if (AEvent->mimeData()->hasUrls())
	{
		foreach(const QUrl &url, AEvent->mimeData()->urls())
			if (!QFileInfo(url.toLocalFile()).isFile())
				return false;

		IMultiUserChatWindow *mucWindow = qobject_cast<IMultiUserChatWindow *>(AWidget->messageWindow()->instance());
		if (mucWindow != NULL)
			return mucWindow->multiUserChat()->isOpen();
		else if (AEvent->mimeData()->urls().count() == 1)
			return isSupported(AWidget->messageWindow()->streamJid(),AWidget->messageWindow()->contactJid());
	}
	return false;
}

bool FileTransfer::messageViewDragMove(IMessageViewWidget *AWidget, const QDragMoveEvent *AEvent)
{
	Q_UNUSED(AWidget); Q_UNUSED(AEvent);
	return true;
}

void FileTransfer::messageViewDragLeave(IMessageViewWidget *AWidget, const QDragLeaveEvent *AEvent)
{
	Q_UNUSED(AWidget); Q_UNUSED(AEvent);
}

bool FileTransfer::messageViewDropAction(IMessageViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu)
{
	if (AEvent->dropAction() != Qt::IgnoreAction)
	{
		QStringList filesList;
		foreach(const QUrl &url, AEvent->mimeData()->urls())
			filesList.append(url.toLocalFile());

		IMultiUserChatWindow *mucWindow = qobject_cast<IMultiUserChatWindow *>(AWidget->messageWindow()->instance());
		if (mucWindow != NULL)
		{
			Jid contactJid = mucWindow->contactJid();
			contactJid.setResource(mucWindow->multiUserChat()->nickname());

			Action *publishAction = new Action(AMenu);
			publishAction->setText(tr("Send File"));
			publishAction->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
			publishAction->setData(ADR_STREAM_JID,mucWindow->streamJid().full());
			publishAction->setData(ADR_CONTACT_JID,contactJid.full());
			publishAction->setData(ADR_FILE_NAME,filesList);
			connect(publishAction,SIGNAL(triggered(bool)),SLOT(onPublishFilesByAction(bool)));
			AMenu->addAction(publishAction,AG_DEFAULT,true);
			AMenu->setDefaultAction(publishAction);
			return true;
		}
		else
		{
			Action *sendAction = new Action(AMenu);
			sendAction->setText(tr("Send File"));
			sendAction->setIcon(RSR_STORAGE_MENUICONS,MNI_FILETRANSFER_SEND);
			sendAction->setData(ADR_STREAM_JID,AWidget->messageWindow()->streamJid().full());
			sendAction->setData(ADR_CONTACT_JID,AWidget->messageWindow()->contactJid().full());
			sendAction->setData(ADR_FILE_NAME,filesList.value(0));
			connect(sendAction,SIGNAL(triggered(bool)),SLOT(onSendFileByAction(bool)));
			AMenu->addAction(sendAction,AG_DEFAULT,true);
			AMenu->setDefaultAction(sendAction);
			return true;
		}
	}
	return false;
}

bool FileTransfer::messageViewUrlOpen(int AOrder, IMessageViewWidget *AWidget, const QUrl &AUrl)
{
	if (AOrder==MVUHO_FILETRANSFER && AUrl.scheme()==XMPP_URI_SCHEME && FXmppUriQueries!=NULL)
	{
		Jid contactJid;
		QString action;
		QMultiMap<QString,QString> params;
		if (FXmppUriQueries->parseXmppUri(AUrl,contactJid,action,params))
		{
			if (action==XMPP_URI_RECV && !params.value("sid").isEmpty())
			{
				QString reqId = receivePublicFile(AWidget->messageWindow()->streamJid(),contactJid,params.value("sid"));
				if (!reqId.isEmpty())
				{
					FPublicRequestView.insert(reqId,AWidget);
					connect(AWidget->instance(),SIGNAL(destroyed(QObject *)),SLOT(onMessageViewWidgetDestroyed(QObject *)),Qt::UniqueConnection);
				}
				else
				{
					showStatusEvent(AWidget,tr("Failed to send request for file '%1'").arg(params.value("name").toHtmlEscaped()));
				}
				return true;
			}
		}
	}
	return false;
}

bool FileTransfer::writeMessageHasText(int AOrder, Message &AMessage, const QString &ALang)
{
	Q_UNUSED(ALang);
	if (AOrder==MWO_FILETRANSFER && FDataPublisher!=NULL && FXmppUriQueries!=NULL)
		return !readPublicFiles(AMessage.stanza().element()).isEmpty();
	return false;
}

bool FileTransfer::writeMessageToText(int AOrder, Message &AMessage, QTextDocument *ADocument, const QString &ALang)
{
	Q_UNUSED(ALang);
	bool changed = false;
	if (AOrder==MWO_FILETRANSFER && FDataPublisher!=NULL && FXmppUriQueries!=NULL)
	{
		QStringList publishedNames;
		QList<IPublicFile> publicFiles;
		QList<IPublicFile> publishedFiles;
		foreach(const IPublicFile &file, readPublicFiles(AMessage.stanza().element()))
		{
			if (FDataPublisher->streams().contains(file.id))
			{
				publishedFiles.append(file);
				publishedNames.append(file.name);
			}
			else
			{
				publicFiles.append(file);
			}
		}

		QTextCursor cursor(ADocument);
		cursor.movePosition(QTextCursor::End);

		if (!publishedFiles.isEmpty())
		{
			if (!cursor.atStart())
				cursor.insertHtml("<br>");
			cursor.insertText(tr("/me share %n file(s): %1",0,publishedFiles.count()).arg(publishedNames.join(", ")));
			changed = true;
		}

		if (!publicFiles.isEmpty())
		{
			QStringList urlList;
			foreach(const IPublicFile &file, publicFiles)
			{
				QMultiMap<QString, QString> params;
				params.insert("sid",file.id);
				params.insert("name",file.name);
				params.insert("size",QString::number(file.size));
				if (!file.mimeType.isEmpty())
					params.insert("mime-type",file.mimeType);
				urlList.append(QString("<a href='%1'>%2</a>").arg(FXmppUriQueries->makeXmppUri(file.ownerJid,XMPP_URI_RECV,params), file.name.toHtmlEscaped()));
			}

			if (!cursor.atStart())
				cursor.insertHtml("<br>");
			cursor.insertHtml(tr("/me share %n file(s): %1",0,publicFiles.count()).arg(urlList.join(", ")));
			changed = true;
		}
	}
	return changed;
}

bool FileTransfer::writeTextToMessage(int AOrder, QTextDocument *ADocument, Message &AMessage, const QString &ALang)
{
	Q_UNUSED(AOrder); Q_UNUSED(ADocument); Q_UNUSED(AMessage); Q_UNUSED(ALang);
	return false;
}

bool FileTransfer::publicDataStreamCanStart(const IPublicDataStream &AStream) const
{
	return AStream.profile==NS_SI_FILETRANSFER && QFile::exists(AStream.params.value(PDSP_FILETRANSFER_NAME).toString());
}

bool FileTransfer::publicDataStreamStart(const Jid &AStreamJid, const Jid AContactJid, const QString &ASessionId, const IPublicDataStream &AStream)
{
	if (publicDataStreamCanStart(AStream))
	{
		IFileStream *stream = createStream(AStreamJid,AContactJid,IFileStream::SendFile,ASessionId);
		if (stream)
		{
			FPublicStreams.append(stream);
			stream->setFileName(AStream.params.value(PDSP_FILETRANSFER_NAME).toString());
			stream->setFileDescription(AStream.params.value(PDSP_FILETRANSFER_DESC).toString());
			stream->setAcceptableMethods(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList());
			if (stream->initStream(stream->acceptableMethods()))
			{
				LOG_STRM_INFO(AStreamJid,QString("Public file stream started, to=%1, sid=%2, id=%3").arg(AContactJid.full(),ASessionId,AStream.id));
				emit publicFileSendStarted(AStream.id, stream);
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to start public file stream, to=%1, id=%2: Stream not initialized").arg(AContactJid.full(),AStream.id));
				stream->instance()->deleteLater();
			}
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to start public file stream, to=%1, id=%2: Stream not created").arg(AContactJid.full(),AStream.id));
		}
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed to start public file stream, to=%1, id=%2: File not found").arg(AContactJid.full(),AStream.id));
	}
	return false;
}

bool FileTransfer::publicDataStreamRead(IPublicDataStream &AStream, const QDomElement &ASiPub) const
{
	QDomElement fileElem = Stanza::findElement(ASiPub,"file",NS_SI_FILETRANSFER);
	if (!fileElem.isNull() && fileElem.hasAttribute("name") && fileElem.hasAttribute("size"))
	{
		AStream.params.insert(PDSP_FILETRANSFER_NAME,fileElem.attribute("name"));
		AStream.params.insert(PDSP_FILETRANSFER_SIZE,fileElem.attribute("size").toLongLong());
		if (!fileElem.firstChildElement("desc").isNull())
			AStream.params.insert(PDSP_FILETRANSFER_DESC,fileElem.firstChildElement("desc").text());
		if (fileElem.hasAttribute("hash"))
			AStream.params.insert(PDSP_FILETRANSFER_HASH,fileElem.attribute("hash"));
		if (fileElem.hasAttribute("date"))
			AStream.params.insert(PDSP_FILETRANSFER_DATE,DateTime(fileElem.attribute("date")).toLocal());
		return true;
	}
	return false;
}

bool FileTransfer::publicDataStreamWrite(const IPublicDataStream &AStream, QDomElement &ASiPub) const
{
	if (AStream.profile==NS_SI_FILETRANSFER && AStream.params.contains(PDSP_FILETRANSFER_NAME) && AStream.params.contains(PDSP_FILETRANSFER_SIZE))
	{
		QDomElement fileElem = ASiPub.ownerDocument().createElementNS(NS_SI_FILETRANSFER,"file");
		ASiPub.appendChild(fileElem);

		fileElem.setAttribute("name", AStream.params.value(PDSP_FILETRANSFER_NAME).toString().split("/").last());
		fileElem.setAttribute("size",AStream.params.value(PDSP_FILETRANSFER_SIZE).toLongLong());

		if (AStream.params.contains(PDSP_FILETRANSFER_DESC))
		{
			QDomElement descElem = ASiPub.ownerDocument().createElement("desc");
			descElem.appendChild(ASiPub.ownerDocument().createTextNode((AStream.params.value(PDSP_FILETRANSFER_DESC).toString())));
			fileElem.appendChild(descElem);
		}

		if (AStream.params.contains(PDSP_FILETRANSFER_HASH))
			fileElem.setAttribute("hash", AStream.params.value(PDSP_FILETRANSFER_HASH).toString());

		if (AStream.params.contains(PDSP_FILETRANSFER_DATE))
			fileElem.setAttribute("date", DateTime(AStream.params.value(PDSP_FILETRANSFER_DATE).toDateTime()).toX85Date());

		return true;
	}
	return false;
}

bool FileTransfer::fileStreamProcessRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods)
{
	if (AOrder == FSHO_FILETRANSFER)
	{
		QDomElement fileElem = ARequest.firstElement("si",NS_STREAM_INITIATION).firstChildElement("file");
		while (!fileElem.isNull() && fileElem.namespaceURI()!=NS_SI_FILETRANSFER)
			fileElem = fileElem.nextSiblingElement("file");

		QString fileName = fileElem.attribute("name");
		qint64 fileSize = fileElem.attribute("size").toLongLong();
		if (!fileName.isEmpty() && fileSize>0)
		{
			IFileStream *stream = createStream(ARequest.to(),ARequest.from(),IFileStream::ReceiveFile,AStreamId);
			if (stream)
			{
				if (FPublicReceiveSessions.contains(AStreamId))
				{
					FPublicStreams.append(stream);
					LOG_STRM_INFO(ARequest.to(),QString("Receive public file stream created, from=%1, sid=%2").arg(ARequest.from(),AStreamId));
				}
				else
				{
					LOG_STRM_INFO(ARequest.to(),QString("Receive file stream created, from=%1, sid=%2").arg(ARequest.from(),AStreamId));
				}

				QList<QString> methods = AMethods.toSet().intersect(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList().toSet()).toList();

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

				autoStartStream(stream);
				notifyStream(stream);
				return true;
			}
			else
			{
				LOG_STRM_ERROR(ARequest.to(),QString("Failed to process file transfer request, sid=%1: Stream not created").arg(AStreamId));
			}
		}
		else
		{
			LOG_STRM_WARNING(ARequest.to(),QString("Failed to process file transfer request, sid=%1: Invalid request").arg(AStreamId));
		}
	}
	return false;
}

bool FileTransfer::fileStreamProcessResponse(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS)
{
	if (FFileManager!=NULL && FFileManager->findStreamHandler(AStreamId)==this)
	{
		IFileStream *stream = FFileManager->findStream(AStreamId);
		if (stream)
		{
			QDomElement rangeElem = AResponce.firstElement("si",NS_STREAM_INITIATION).firstChildElement("file").firstChildElement("range");
			if (!rangeElem.isNull())
			{
				if (rangeElem.hasAttribute("offset"))
					stream->setRangeOffset(rangeElem.attribute("offset").toLongLong());
				if (rangeElem.hasAttribute("length"))
					stream->setRangeLength(rangeElem.attribute("length").toLongLong());
			}

			if (stream->startStream(AMethodNS))
			{
				LOG_STRM_INFO(AResponce.to(),QString("Started file transfer to=%1, sid=%2, method=%3").arg(AResponce.from(),AStreamId,AMethodNS));
				return true;
			}
			else
			{
				LOG_STRM_WARNING(AResponce.to(),QString("Failed to start file transfer, sid=%1: Stream not started").arg(AStreamId));
				stream->abortStream(XmppError(IERR_FILETRANSFER_TRANSFER_NOT_STARTED));
			}
		}
		else
		{
			LOG_STRM_ERROR(AResponce.to(),QString("Failed to process file transfer response, sid=%1: Stream not found"));
		}
	}
	else if (FFileManager)
	{
		LOG_STRM_ERROR(AResponce.to(),QString("Failed to process file transfer response, sid=%1: Invalid stream handler"));
	}
	return false;
}

bool FileTransfer::fileStreamShowDialog(const QString &AStreamId)
{
	IFileStream *stream = FFileManager!=NULL ? FFileManager->findStream(AStreamId) : NULL;
	if (stream!=NULL && FFileManager->findStreamHandler(AStreamId)==this)
	{
		WidgetManager::showActivateRaiseWindow(getStreamDialog(stream));
		return true;
	}
	else if (stream)
	{
		LOG_STRM_ERROR(stream->streamJid(),QString("Failed to show file transfer dialog, sid=%1: Invalid handler").arg(AStreamId));
	}
	else if (!AStreamId.isEmpty())
	{
		LOG_ERROR(QString("Failed to show file transfer dialog, sid=%1: Stream not found").arg(AStreamId));
	}
	return false;
}

bool FileTransfer::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
	if (AAction == XMPP_URI_RECV)
	{
		QString streamId = AParams.value("sid");
		if (!streamId.isEmpty())
		{
			receivePublicFile(AStreamJid,AContactJid,streamId);
			return true;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to start public file receive by XMPP URI, from=%1: Public stream ID is empty").arg(AContactJid.full()));
		}
	}
	return false;
}

bool FileTransfer::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
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
		IFileStream *stream = createStream(AStreamJid,AContactJid,IFileStream::SendFile,QUuid::createUuid().toString());
		if (stream)
		{
			LOG_STRM_INFO(AStreamJid,QString("Send file stream created, to=%1, sid=%2").arg(AContactJid.full(),stream->streamId()));
			stream->setFileName(AFileName);
			stream->setFileDescription(AFileDesc);

			StreamDialog *dialog = getStreamDialog(stream);
			dialog->setSelectableMethods(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList());
			dialog->show();

			return stream;
		}
		else
		{
			LOG_STRM_ERROR(AStreamJid,QString("Failed to send file to=%1: Stream not created").arg(AContactJid.full()));
		}
	}
	else
	{
		LOG_STRM_ERROR(AStreamJid,QString("Failed to send file to=%1: Not supported").arg(AContactJid.full()));
	}
	return NULL;
}

IPublicFile FileTransfer::findPublicFile(const QString &AFileId) const
{
	return FDataPublisher!=NULL ? publicFileFromStream(FDataPublisher->findStream(AFileId)) : IPublicFile();
}

QList<IPublicFile> FileTransfer::findPublicFiles(const Jid &AOwnerJid, const QString &AFileName) const
{
	QList<IPublicFile> files;
	if (FDataPublisher)
	{
		foreach(const QString &streamId, FDataPublisher->streams())
		{
			IPublicFile file = publicFileFromStream(FDataPublisher->findStream(streamId));
			if (file.isValid())
			{
				if (AOwnerJid.isEmpty() || AOwnerJid.pBare()==file.ownerJid.pBare())
				{
					if (AFileName.isEmpty() || AFileName==file.name)
						files.append(file);
				}
			}
		}
	}
	return files;
}

QString FileTransfer::registerPublicFile(const Jid &AOwnerJid, const QString &AFileName, const QString &AFileDesc)
{
	if (FDataPublisher)
	{
		QFileInfo file(AFileName);
		if (file.exists() && file.isFile())
		{
			QList<IPublicFile> files = findPublicFiles(AOwnerJid,AFileName);
			if (files.isEmpty())
			{
				IPublicDataStream stream;
				stream.id = QUuid::createUuid().toString();
				stream.ownerJid = AOwnerJid;
				stream.profile = NS_SI_FILETRANSFER;
				stream.params.insert(PDSP_FILETRANSFER_NAME, file.absoluteFilePath());
				if (!AFileDesc.isEmpty())
					stream.params.insert(PDSP_FILETRANSFER_DESC, AFileDesc);
				stream.params.insert(PDSP_FILETRANSFER_SIZE, file.size());
				stream.params.insert(PDSP_FILETRANSFER_DATE, file.lastModified());

				if (FDataPublisher->publishStream(stream))
				{
					LOG_INFO(QString("Registered public file=%1, owner=%2, id=%3").arg(AFileName,AOwnerJid.full(),stream.id));
					return stream.id;
				}
				else
				{
					LOG_WARNING(QString("Failed to register public file=%1, owner=%2: Stream not registered").arg(AFileName,AOwnerJid.full()));
				}
			}
			else
			{
				return files.value(0).id;
			}
		}
		else
		{
			LOG_WARNING(QString("Failed to register public file=%1, owner=%2: File not found").arg(AFileName,AOwnerJid.full()));
		}
	}
	return QString::null;
}

void FileTransfer::removePublicFile(const QString &AFileId)
{
	if (FDataPublisher && FDataPublisher->streams().contains(AFileId))
	{
		FDataPublisher->removeStream(AFileId);
		LOG_INFO(QString("Removed public file, id=%1").arg(AFileId));
	}
	else
	{
		LOG_WARNING(QString("Failed to remove public file, id=%1: File not found").arg(AFileId));
	}
}

QList<IPublicFile> FileTransfer::readPublicFiles(const QDomElement &AParent) const
{
	QList<IPublicFile> files;
	if (FDataPublisher)
	{
		foreach(const IPublicDataStream &stream, FDataPublisher->readStreams(AParent))
		{
			IPublicFile file = publicFileFromStream(stream);
			if (file.isValid())
				files.append(file);
		}
	}
	return files;
}

bool FileTransfer::writePublicFile(const QString &AFileId, QDomElement &AParent) const
{
	return FDataPublisher!=NULL ? FDataPublisher->writeStream(AFileId,AParent) : false;
}

QString FileTransfer::receivePublicFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileId)
{
	if (FDataPublisher && FDataPublisher->isSupported(AStreamJid,AContactJid))
	{
		QString reqId = FDataPublisher->startStream(AStreamJid,AContactJid,AFileId);
		if (!reqId.isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,QString("Start public file receive request sent to=%1, file=%2, id=%3").arg(AContactJid.full(),AFileId,reqId));
			FPublicReceiveRequests.append(reqId);
			return reqId;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed send start receive public file request to=%1, file=%2: Stream not started").arg(AContactJid.full(),AFileId));
		}
	}
	else if (FDataPublisher)
	{
		LOG_STRM_WARNING(AStreamJid,QString("Failed send start receive public file request to=%1, id=%2: Not supported").arg(AContactJid.full(),AFileId));
	}
	return QString::null;
}

void FileTransfer::notifyStream(IFileStream *AStream)
{
	QString streamFile = !AStream->fileName().isEmpty() ? AStream->fileName().split("/").last() : QString::null;

	IMessageChatWindow *chatWindow = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStream->streamJid(),AStream->contactJid()) : NULL;
	IMultiUserChatWindow *mucWindow = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(AStream->streamJid(),AStream->contactJid()) : NULL;

	QString contactName = AStream->contactJid().uFull();
	if (mucWindow)
	{
		if (AStream->contactJid().hasResource())
			contactName = QString("[%1]").arg(AStream->contactJid().resource());
	}
	else if (chatWindow)
	{
		if (FNotifications)
			contactName = FNotifications->contactName(AStream->streamJid(),AStream->contactJid());
	}

	if (FNotifications && (!FPublicStreams.contains(AStream) || AStream->streamKind()==IFileStream::ReceiveFile))
	{
		StreamDialog *dialog = getStreamDialog(AStream);
		if (dialog==NULL || !dialog->isActiveWindow())
		{
			INotification notify;
			notify.kinds = FNotifications->enabledTypeNotificationKinds(NNT_FILETRANSFER);
			notify.typeId = NNT_FILETRANSFER;
			notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(AStream->streamKind()==IFileStream::SendFile ? MNI_FILETRANSFER_SEND : MNI_FILETRANSFER_RECEIVE));
			notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(AStream->streamJid(),AStream->contactJid()));
			notify.data.insert(NDR_STREAM_JID,AStream->streamJid().full());
			notify.data.insert(NDR_CONTACT_JID,AStream->contactJid().full());
			notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(AStream->contactJid()));
			notify.data.insert(NDR_POPUP_CAPTION, tr("File transfer"));
			notify.data.insert(NDR_ALERT_WIDGET,(qint64)dialog);
			notify.data.insert(NDR_SHOWMINIMIZED_WIDGET,(qint64)dialog);

			switch (AStream->streamState())
			{
			case IFileStream::Creating:
				if (AStream->streamKind() == IFileStream::ReceiveFile)
				{
					notify.data.insert(NDR_TOOLTIP,tr("File transfer request from %1").arg(contactName));
					notify.data.insert(NDR_POPUP_TEXT,tr("%1 wants to send you a file '%2'").arg(contactName,streamFile));
					notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_INCOMING);
				}
				break;
			case IFileStream::Negotiating:
			case IFileStream::Connecting:
			case IFileStream::Transfering:
				notify.kinds &= ~INotification::TrayNotify;
				if (AStream->streamKind() == IFileStream::SendFile)
					notify.data.insert(NDR_POPUP_TEXT,tr("Sending file '%1' to %2 started automatically").arg(streamFile,contactName));
				else
					notify.data.insert(NDR_POPUP_TEXT,tr("Receiving file '%1' from %2 started automatically").arg(streamFile,contactName));
				notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_INCOMING);
				break;
			case IFileStream::Finished:
				notify.data.insert(NDR_TOOLTIP,tr("File transfer completed"));
				if (AStream->streamKind() == IFileStream::SendFile)
					notify.data.insert(NDR_POPUP_TEXT,tr("File '%1' successfully sent to %2").arg(streamFile,contactName));
				else
					notify.data.insert(NDR_POPUP_TEXT,tr("File '%1' successfully received from %2").arg(streamFile,contactName));
				notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_COMPLETE);
				break;
			case IFileStream::Aborted:
				notify.data.insert(NDR_TOOLTIP,tr("File transfer failed"));
				if (AStream->streamKind() == IFileStream::SendFile)
					notify.data.insert(NDR_POPUP_TEXT,tr("Failed to send file '%1' to %2: %3").arg(streamFile,contactName,AStream->stateString()));
				else
					notify.data.insert(NDR_POPUP_TEXT,tr("Failed to receive file '%1' from %2: %3").arg(streamFile,contactName,AStream->stateString()));
				notify.data.insert(NDR_SOUND_FILE,SDF_FILETRANSFER_CANCELED);
				break;
			default:
				notify.kinds = 0;
			}

			if (notify.kinds > 0)
			{
				if (FStreamNotify.contains(AStream->streamId()))
					FNotifications->removeNotification(FStreamNotify.take(AStream->streamId()));
				FStreamNotify.insert(AStream->streamId(),FNotifications->appendNotification(notify));
			}
		}
	}

	QString note;
	if (AStream->streamState() == IFileStream::Finished)
	{
		if (AStream->streamKind() == IFileStream::SendFile)
			note = tr("File '%1' successfully sent to %2").arg(streamFile,contactName);
		else
			note = tr("File '%1' successfully received from %2").arg(streamFile,contactName);
	}
	else if (AStream->streamState() == IFileStream::Aborted)
	{
		if (AStream->streamKind() == IFileStream::SendFile)
			note = tr("Failed to send file '%1' to %2: %3").arg(streamFile,contactName,AStream->stateString());
		else
			note = tr("Failed to receive file '%1' from %2: %3").arg(streamFile,contactName,AStream->stateString());
	}

	if (!note.isEmpty())
	{
		if (mucWindow && FPublicStreams.contains(AStream))
			showStatusEvent(mucWindow->viewWidget(),note.toHtmlEscaped());
		else if (chatWindow)
			showStatusEvent(chatWindow->viewWidget(),note.toHtmlEscaped());
	}
}

bool FileTransfer::autoStartStream(IFileStream *AStream) const
{
	if (Options::node(OPV_FILETRANSFER_AUTORECEIVE).value().toBool() && AStream->streamKind()==IFileStream::ReceiveFile)
	{
		if (!QFile::exists(AStream->fileName()))
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStream->streamJid()) : NULL;
			IRosterItem ritem = roster!=NULL ? roster->findItem(AStream->contactJid()) : IRosterItem();
			if (ritem.subscription==SUBSCRIPTION_BOTH || ritem.subscription==SUBSCRIPTION_FROM)
			{
				QString method = Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString();
				if (AStream->acceptableMethods().contains(method))
					return AStream->startStream(method);
			}
		}
		else
		{
			LOG_STRM_WARNING(AStream->streamJid(),QString("Failed to auto start file transfer, sid=%1: File already exists").arg(AStream->streamId()));
		}
	}
	return false;
}

void FileTransfer::updateToolBarAction(IMessageToolBarWidget *AWidget)
{
	Action *fileAction = FToolBarActions.value(AWidget);
	IMessageChatWindow *chatWindow = qobject_cast<IMessageChatWindow *>(AWidget->messageWindow()->instance());
	IMultiUserChatWindow *mucWindow = qobject_cast<IMultiUserChatWindow *>(AWidget->messageWindow()->instance());
	if (chatWindow != NULL)
	{
		if (fileAction == NULL)
		{
			fileAction = new Action(AWidget->toolBarChanger()->toolBar());
			fileAction->setIcon(RSR_STORAGE_MENUICONS, MNI_FILETRANSFER_SEND);
			fileAction->setText(tr("Send File"));
			fileAction->setShortcutId(SCT_MESSAGEWINDOWS_SENDFILE);
			connect(fileAction,SIGNAL(triggered(bool)),SLOT(onSendFileByAction(bool)));
			AWidget->toolBarChanger()->insertAction(fileAction,TBG_MWTBW_FILETRANSFER);
			FToolBarActions.insert(AWidget,fileAction);
		}
		fileAction->setEnabled(isSupported(chatWindow->streamJid(),chatWindow->contactJid()));
	}
	else if (FDataPublisher!=NULL && FMessageProcessor!=NULL && mucWindow!=NULL)
	{
		if (fileAction == NULL)
		{
			fileAction = new Action(AWidget->toolBarChanger()->toolBar());
			fileAction->setIcon(RSR_STORAGE_MENUICONS, MNI_FILETRANSFER_SEND);
			fileAction->setText(tr("Send File"));
			fileAction->setShortcutId(SCT_MESSAGEWINDOWS_SENDFILE);
			connect(fileAction,SIGNAL(triggered(bool)),SLOT(onPublishFilesByAction(bool)));
			AWidget->toolBarChanger()->insertAction(fileAction,TBG_MWTBW_FILETRANSFER);
			FToolBarActions.insert(AWidget,fileAction);
		}
		fileAction->setEnabled(FDataPublisher!=NULL && mucWindow->multiUserChat()->isOpen());
	}
}

QList<IMessageToolBarWidget *> FileTransfer::findToolBarWidgets(const Jid &AContactJid) const
{
	QList<IMessageToolBarWidget *> toolBars;
	foreach (IMessageToolBarWidget *widget, FToolBarActions.keys())
		if (widget->messageWindow()->contactJid()==AContactJid)
			toolBars.append(widget);
	return toolBars;
}

StreamDialog *FileTransfer::getStreamDialog(IFileStream *AStream)
{
	StreamDialog *dialog = FStreamDialog.value(AStream->streamId());
	if (dialog == NULL)
	{
		dialog = new StreamDialog(FDataManager,FFileManager,this,AStream,NULL);
		connect(dialog,SIGNAL(dialogDestroyed()),SLOT(onStreamDialogDestroyed()));

		if (AStream->streamKind() == IFileStream::SendFile)
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_SEND,0,0,"windowIcon");
		else
			IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(dialog,MNI_FILETRANSFER_RECEIVE,0,0,"windowIcon");

		if (FNotifications)
		{
			QString name = "<b>"+ FNotifications->contactName(AStream->streamJid(), AStream->contactJid()).toHtmlEscaped() +"</b>";
			if (AStream->contactJid().hasResource())
				name += "/" + AStream->contactJid().resource().toHtmlEscaped();
			dialog->setContactName(name);
			dialog->installEventFilter(this);
		}

		FStreamDialog.insert(AStream->streamId(),dialog);
	}
	return dialog;
}

IFileStream *FileTransfer::createStream(const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind, const QString &AStreamId)
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

IPublicFile FileTransfer::publicFileFromStream(const IPublicDataStream &AStream) const
{
	IPublicFile file;
	if (AStream.isValid() && AStream.profile == NS_SI_FILETRANSFER)
	{
		file.id = AStream.id;
		file.ownerJid = AStream.ownerJid;
		file.mimeType = AStream.mimeType;
		file.name = AStream.params.value(PDSP_FILETRANSFER_NAME).toString();
		file.size = AStream.params.value(PDSP_FILETRANSFER_SIZE).toLongLong();
		file.hash = AStream.params.value(PDSP_FILETRANSFER_HASH).toString();
		file.date = AStream.params.value(PDSP_FILETRANSFER_DATE).toDateTime();
		file.description = AStream.params.value(PDSP_FILETRANSFER_DESC).toString();
		return file;
	}
	return file;
}

void FileTransfer::showStatusEvent(IMessageViewWidget *AView, const QString &AHtml) const
{
	if (AView!=NULL && !AHtml.isEmpty())
	{
		IMessageStyleContentOptions options;
		options.kind = IMessageStyleContentOptions::KindStatus;
		options.type |= IMessageStyleContentOptions::TypeEvent;
		options.direction = IMessageStyleContentOptions::DirectionIn;
		options.time = QDateTime::currentDateTime();
		AView->appendHtml(AHtml,options);
	}
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
			if (FPublicStreams.contains(stream) && stream->streamKind()==IFileStream::SendFile)
				stream->instance()->deleteLater();
			notifyStream(stream);
		}
		else if (stream->streamState() == IFileStream::Aborted)
		{
			if (FPublicStreams.contains(stream) && stream->streamKind()==IFileStream::SendFile)
				stream->instance()->deleteLater();
			notifyStream(stream);
		}
	}
}

void FileTransfer::onStreamDestroyed()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
	{
		LOG_STRM_INFO(stream->streamJid(),QString("File transfer stream destroyed, sid=%1").arg(stream->streamId()));
		if (FNotifications && FStreamNotify.contains(stream->streamId()))
			FNotifications->removeNotification(FStreamNotify.value(stream->streamId()));
		FPublicStreams.removeAll(stream);
	}
}

void FileTransfer::onStreamDialogDestroyed()
{
	StreamDialog *dialog = qobject_cast<StreamDialog *>(sender());
	if (dialog)
		FStreamDialog.remove(FStreamDialog.key(dialog));
}

void FileTransfer::onSendFileByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMessageToolBarWidget *widget = FToolBarActions.key(action);

		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		QString file = action->data(ADR_FILE_NAME).toString();

		if (file.isEmpty())
		{
			QWidget *parent = widget!=NULL ? widget->messageWindow()->instance() : NULL;
			file = QFileDialog::getOpenFileName(parent,tr("Select File"));
		}

		if (!file.isEmpty())
		{
			if (streamJid.isValid() && contactJid.isValid())
				sendFile(streamJid,contactJid,file);
			else if (widget != NULL)
				sendFile(widget->messageWindow()->streamJid(),widget->messageWindow()->contactJid(),file);
		}
	}
}

void FileTransfer::onPublishFilesByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action!=NULL && FDataPublisher!=NULL && FMessageProcessor!=NULL)
	{
		IMessageToolBarWidget *widget = FToolBarActions.key(action);

		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		QStringList files = action->data(ADR_FILE_NAME).toStringList();

		if (files.isEmpty())
		{
			QWidget *parent = widget!=NULL ? widget->messageWindow()->instance() : NULL;
			files = QFileDialog::getOpenFileNames(parent,tr("Select Files"));
		}

		IMessageChatWindow *chatWindow = NULL;
		IMultiUserChatWindow *mucWindow = NULL;
		if (widget != NULL)
		{
			chatWindow = qobject_cast<IMessageChatWindow *>(widget->messageWindow()->instance());
			mucWindow = qobject_cast<IMultiUserChatWindow *>(widget->messageWindow()->instance());
		}
		else if (streamJid.isValid() && contactJid.isValid())
		{
			chatWindow = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(streamJid,contactJid) : NULL;
			mucWindow = FMultiChatManager!=NULL ? FMultiChatManager->findMultiChatWindow(streamJid,contactJid) : NULL;
		}

		Jid ownerJid;
		Message message;
		if (chatWindow != NULL)
		{
			message.setType(Message::Chat);
			message.setFrom(chatWindow->streamJid().full()).setTo(chatWindow->contactJid().full());
			ownerJid = chatWindow->streamJid();
		}
		else if (mucWindow!=NULL && mucWindow->multiUserChat()->isOpen())
		{
			message.setType(Message::GroupChat);
			message.setFrom(mucWindow->streamJid().full()).setTo(mucWindow->contactJid().full());
			ownerJid = mucWindow->multiUserChat()->mainUser()->userJid();
		}

		if (ownerJid.isValid())
		{
			int registered = 0;
			foreach(const QString &file, files)
			{
				QString fileId = registerPublicFile(ownerJid,file);
				if (!fileId.isEmpty())
				{
					QDomElement messageElem = message.stanza().element();
					if (FDataPublisher->writeStream(fileId,messageElem))
						registered++;
					else
						LOG_ERROR(QString("Failed to write public file stream to message, file=%1").arg(fileId));
				}
			}

			if (registered > 0)
			{
				if (FMessageProcessor->sendMessage(message.from(),message,IMessageProcessor::DirectionOut))
					LOG_STRM_INFO(message.from(), QString("Sent %1 public file(s) to %2").arg(files.count()).arg(message.to()));
				else
					LOG_STRM_WARNING(message.from(), QString("Failed to send %1 public file(s) to %2").arg(files.count()).arg(message.to()));
			}
		}
	}
}

void FileTransfer::onPublicStreamStartAccepted(const QString &ARequestId, const QString &ASessionId)
{
	if (FPublicReceiveRequests.contains(ARequestId))
	{
		LOG_INFO(QString("Start public file receive request accepted, id=%1, sid=%2").arg(ARequestId,ASessionId));
		FPublicRequestView.remove(ARequestId);
		FPublicReceiveRequests.removeAll(ARequestId);
		FPublicReceiveSessions.insert(ASessionId,ARequestId);
	}
}

void FileTransfer::onPublicStreamStartRejected(const QString &ARequestId, const XmppError &AError)
{
	if (FPublicReceiveRequests.contains(ARequestId))
	{
		LOG_INFO(QString("Start public file receive request rejected, id=%1: %2").arg(ARequestId,AError.condition()));
		if (FPublicRequestView.contains(ARequestId))
			showStatusEvent(FPublicRequestView.take(ARequestId),tr("File request rejected: %1").arg(AError.errorMessage().toHtmlEscaped()));
		FPublicReceiveRequests.removeAll(ARequestId);
		emit publicFileReceiveRejected(ARequestId,AError);
	}
}

void FileTransfer::onDataStreamInitStarted(const IDataStream &AStream)
{
	if (FPublicReceiveSessions.contains(AStream.streamId))
	{
		QString reqId = FPublicReceiveSessions.take(AStream.streamId);
		IFileStream *stream = FFileManager!=NULL ? FFileManager->findStream(AStream.streamId) : NULL;
		if (stream)
		{
			getStreamDialog(stream)->show();
			LOG_STRM_INFO(AStream.streamJid,QString("Public file receive started, id=%1, sid=%2").arg(reqId,stream->streamId()));
			emit publicFileReceiveStarted(reqId,stream);
		}
		else
		{
			LOG_STRM_ERROR(AStream.streamJid,QString("Failed to start public file receive, id=%1: Stream not found").arg(reqId));
			emit publicFileReceiveRejected(reqId,XmppStanzaError(XmppStanzaError::EC_ITEM_NOT_FOUND));
		}
	}
}

void FileTransfer::onDataStreamInitFinished(const IDataStream &AStream, const XmppError &AError)
{
	if (FPublicReceiveSessions.contains(AStream.streamId))
	{
		QString reqId = FPublicReceiveSessions.take(AStream.streamId);
		if (!AError.isNull())
		{
			LOG_STRM_ERROR(AStream.streamJid,QString("Failed to start public file receive, id=%1: %2").arg(reqId,AError.condition()));
			emit publicFileReceiveRejected(reqId,AError);
		}
		else
		{
			REPORT_ERROR("Receive public file stream initiation not handled on start");
		}
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

void FileTransfer::onMultiUserChatStateChanged(int AState)
{
	Q_UNUSED(AState);
	IMultiUserChat *multiChat = qobject_cast<IMultiUserChat *>(sender());
	if (multiChat)
	{
		foreach(IMessageToolBarWidget *widget, findToolBarWidgets(multiChat->roomJid()))
			updateToolBarAction(widget);
	}
}

void FileTransfer::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	foreach(IMessageToolBarWidget *widget, findToolBarWidgets(AInfo.contactJid))
		updateToolBarAction(widget);
}

void FileTransfer::onDiscoInfoRemoved(const IDiscoInfo &AInfo)
{
	foreach(IMessageToolBarWidget *widget, findToolBarWidgets(AInfo.contactJid))
		updateToolBarAction(widget);
}

void FileTransfer::onToolBarWidgetCreated(IMessageToolBarWidget *AWidget)
{
	IMessageChatWindow *chatWindow = qobject_cast<IMessageChatWindow *>(AWidget->messageWindow()->instance());
	IMultiUserChatWindow *mucWindow = qobject_cast<IMultiUserChatWindow *>(AWidget->messageWindow()->instance());

	if (mucWindow)
		connect(mucWindow->multiUserChat()->instance(),SIGNAL(stateChanged(int)),SLOT(onMultiUserChatStateChanged(int)));
	if (chatWindow)
		connect(AWidget->messageWindow()->address()->instance(),SIGNAL(addressChanged(const Jid &,const Jid &)),SLOT(onToolBarWidgetAddressChanged(const Jid &,const Jid &)));
	connect(AWidget->instance(),SIGNAL(destroyed(QObject *)),SLOT(onToolBarWidgetDestroyed(QObject *)));

	updateToolBarAction(AWidget);
}

void FileTransfer::onToolBarWidgetAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore)
{
	Q_UNUSED(AStreamBefore); Q_UNUSED(AContactBefore);
	IMessageAddress *address = qobject_cast<IMessageAddress *>(sender());
	if (address)
	{
		foreach(IMessageToolBarWidget *widget, FToolBarActions.keys())
			if (widget->messageWindow()->address() == address)
				updateToolBarAction(widget);
	}
}

void FileTransfer::onToolBarWidgetDestroyed(QObject *AObject)
{
	foreach(IMessageToolBarWidget *widget, FToolBarActions.keys())
		if (qobject_cast<QObject *>(widget->instance()) == AObject)
			FToolBarActions.remove(widget);
}

void FileTransfer::onMessageViewWidgetDestroyed(QObject *AObject)
{
	for (QMap<QString, IMessageViewWidget *>::iterator it=FPublicRequestView.begin(); it!=FPublicRequestView.end(); )
	{
		if (qobject_cast<QObject *>(it.value()->instance()) == AObject)
			it = FPublicRequestView.erase(it);
		else
			++it;
	}
}

