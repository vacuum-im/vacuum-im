#include "remotecontrol.h"

#include <QMap>
#include <definitions/namespaces.h>
#include <definitions/optionvalues.h>
#include <definitions/dataformtypes.h>
#include <definitions/messagedataroles.h>
#include <definitions/stanzahandlerorders.h>
#include <utils/options.h>
#include <utils/logger.h>

#define COMMAND_NODE_ROOT               "http://jabber.org/protocol/rc"
#define COMMAND_NODE_PING               COMMAND_NODE_ROOT"#ping"
#define COMMAND_NODE_SET_STATUS         COMMAND_NODE_ROOT"#set-status"
#define COMMAND_NODE_SET_MAIN_STATUS    COMMAND_NODE_ROOT"#set-main-status"
#define COMMAND_NODE_LEAVE_MUC          COMMAND_NODE_ROOT"#leave-groupchats"
#define COMMAND_NODE_ACCEPT_FILES       COMMAND_NODE_ROOT"#accept-files"
#define COMMAND_NODE_SET_OPTIONS        COMMAND_NODE_ROOT"#set-options"
#define COMMAND_NODE_FORWARD_MESSAGES   COMMAND_NODE_ROOT"#forward"

#define FIELD_STATUS                    "status"
#define FIELD_STATUS_MESSAGE            "status-message"
#define FIELD_STATUS_PRIORITY           "status-priority"
#define FIELD_GROUPCHATS                "groupchats"
#define FIELD_FILES                     "files"
#define FIELD_MESSAGES                  "messages"
#define FIELD_SOUNDS                    "sounds"
#define FIELD_AUTO_MSG                  "auto-msg"
#define FIELD_AUTO_FILES                "auto-files"
#define FIELD_AUTO_AUTH                 "auto-auth"
#define FIELD_AUTO_OFFLINE              "auto-offline"

#define SHC_MESSAGE_ADDRESS             "/message/addresses[@xmlns='" NS_ADDRESS "']/address[@type='ofrom']"

struct OptionsFormItem {
	OptionsFormItem(QString ANode = QString::null, QString ALabel = QString::null) {
		node = ANode;
		label = ALabel;
	}
	QString node;
	QString label;
};

QMap<QString, OptionsFormItem> optionItems;

RemoteControl::RemoteControl()
{
	FCommands = NULL;
	FStatusChanger = NULL;
	FMultiUserChatPlugin = NULL;
	FDataForms = NULL;
	FFileStreamManager = NULL;
	FMessageProcessor = NULL;
	FNotifications = NULL;
	FStanzaProcessor = NULL;

	FSHIMessageForward = -1;
}

RemoteControl::~RemoteControl()
{

}

void RemoteControl::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Remote Control");
	APluginInfo->description = tr("Allows to remotely control the client");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Maxim Ignatenko";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(COMMANDS_UUID);
	APluginInfo->dependences.append(DATAFORMS_UUID);
}

bool RemoteControl::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("ICommands").value(0,NULL);
	if (plugin)
	{
		FCommands = qobject_cast<ICommands *>(plugin->instance());
		if (FCommands == NULL)
			LOG_WARNING("Failed to load required interface: ICommands");
	}
	
	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
		if (FDataForms == NULL)
			LOG_WARNING("Failed to load required interface: IDataForms");
	}

	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}
	
	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
	}
	
	plugin = APluginManager->pluginInterface("IFileStreamsManager").value(0,NULL);
	if (plugin)
	{
		FFileStreamManager = qobject_cast<IFileStreamsManager *>(plugin->instance());
	}
	
	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	return FCommands!=NULL && FDataForms!=NULL;
}

bool RemoteControl::initObjects()
{
	if (FCommands)
	{
		FCommands->insertServer(COMMAND_NODE_PING, this);
		FCommands->insertServer(COMMAND_NODE_SET_OPTIONS, this);
		if (FDataForms && FStatusChanger)
		{
			FCommands->insertServer(COMMAND_NODE_SET_STATUS, this);
			FCommands->insertServer(COMMAND_NODE_SET_MAIN_STATUS, this);
		}
		if (FDataForms && FMultiUserChatPlugin)
		{
			FCommands->insertServer(COMMAND_NODE_LEAVE_MUC, this);
		}
		if (FDataForms && FFileStreamManager)
		{
			FCommands->insertServer(COMMAND_NODE_ACCEPT_FILES, this);
		}
		if (FDataForms && FStanzaProcessor && FMessageProcessor)
		{
			FCommands->insertServer(COMMAND_NODE_FORWARD_MESSAGES, this);
		}
	}

	if (FDataForms)
	{
		FDataForms->insertLocalizer(this, DATA_FORM_REMOTECONTROL);
	}

	if (FStanzaProcessor)
	{
		IStanzaHandle handle;
		handle.order = SHO_MI_REMOTECONTROL;
		handle.direction = IStanzaHandle::DirectionIn;
		handle.handler = this;
		handle.conditions.append(SHC_MESSAGE_ADDRESS);
		FSHIMessageForward = FStanzaProcessor->insertStanzaHandle(handle);
	}

	optionItems.clear();
	optionItems[FIELD_SOUNDS] = OptionsFormItem(QString(OPV_NOTIFICATIONS_KINDENABLED_ITEM"[%1]").arg(INotification::SoundPlay), tr("Play sounds"));
	optionItems[FIELD_AUTO_MSG] = OptionsFormItem(QString(OPV_NOTIFICATIONS_KINDENABLED_ITEM"[%1]").arg(INotification::AutoActivate), tr("Automatically Open New Messages"));
	optionItems[FIELD_AUTO_FILES] = OptionsFormItem(OPV_FILETRANSFER_AUTORECEIVE, tr("Automatically Accept File Transfers"));
	optionItems[FIELD_AUTO_AUTH] = OptionsFormItem(OPV_ROSTER_AUTOSUBSCRIBE, tr("Automatically Authorize Contacts"));

	return true;
}

bool RemoteControl::initSettings()
{
	return true;
}

bool RemoteControl::startPlugin()
{
	return true;
}

bool RemoteControl::stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	Q_UNUSED(AAccept);
	if (AHandleId == FSHIMessageForward)
	{
		if (AStreamJid.pBare() == Jid(AStanza.from()).pBare())
		{
			QDomElement addressElem = AStanza.firstElement("addresses",NS_ADDRESS).firstChildElement("address");
			while(!addressElem.isNull() && addressElem.attribute("type")!="ofrom")
				addressElem = addressElem.nextSiblingElement("address");
			if (!addressElem.isNull() && addressElem.hasAttribute("jid"))
				AStanza.setFrom(addressElem.attribute("jid"));
		}
	}
	return false;
}

bool RemoteControl::isCommandPermitted(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const
{
	Q_UNUSED(ANode);
	return AStreamJid.pBare() == AContactJid.pBare();
}

QString RemoteControl::commandName(const QString &ANode) const
{
	if (ANode == COMMAND_NODE_PING) 
		return tr("Ping");
	if (ANode == COMMAND_NODE_SET_STATUS)
		return tr("Change connection status");
	if (ANode == COMMAND_NODE_SET_MAIN_STATUS)
		return tr("Change main status");
	if (ANode == COMMAND_NODE_LEAVE_MUC) 
		return tr("Leave conferences");
	if (ANode == COMMAND_NODE_ACCEPT_FILES)
		return tr("Accept pending file transfers");
	if (ANode == COMMAND_NODE_SET_OPTIONS)
		return tr("Set options");
	if (ANode == COMMAND_NODE_FORWARD_MESSAGES)
		return tr("Forward unread messages");
	return QString::null;
}

bool RemoteControl::receiveCommandRequest(const ICommandRequest &ARequest)
{
	if (isCommandPermitted(ARequest.streamJid, ARequest.contactJid, ARequest.node))
	{
		LOG_STRM_INFO(ARequest.streamJid,QString("Received command request from=%1, node=%2, action=%3, sid=%4").arg(ARequest.contactJid.full(),ARequest.node,ARequest.action,ARequest.sessionId));
		if (ARequest.node == COMMAND_NODE_PING)
			return processPing(ARequest);
		else if (ARequest.node == COMMAND_NODE_SET_STATUS)
			return processSetStatus(ARequest);
		else if (ARequest.node == COMMAND_NODE_SET_MAIN_STATUS)
			return processSetStatus(ARequest);
		else if (ARequest.node == COMMAND_NODE_LEAVE_MUC)
			return processLeaveMUC(ARequest);
		else if (ARequest.node == COMMAND_NODE_ACCEPT_FILES)
			return processFileTransfers(ARequest);
		else if (ARequest.node == COMMAND_NODE_SET_OPTIONS)
			return processSetOptions(ARequest);
		else if (ARequest.node == COMMAND_NODE_FORWARD_MESSAGES)
			return processForwardMessages(ARequest);
		else
			LOG_STRM_ERROR(ARequest.streamJid,QString("Failed to process command request from=%1, node=%2: Unexpected request").arg(ARequest.contactJid.full(),ARequest.node));
	}
	else
	{
		LOG_STRM_WARNING(ARequest.streamJid,QString("Failed to process command request from=%1, node=%2: Permission denied").arg(ARequest.contactJid.full(),ARequest.node));
	}
	return false;
}

IDataFormLocale RemoteControl::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_REMOTECONTROL)
	{
		locale.fields[FIELD_AUTO_AUTH].label = tr("Whether to automatically authorize subscription requests");
		locale.fields[FIELD_AUTO_FILES].label = tr("Whether to automatically accept file transfers");
		locale.fields[FIELD_AUTO_MSG].label = tr("Whether to automatically open new messages");
		locale.fields[FIELD_AUTO_OFFLINE].label = tr("Whether to automatically go offline when idle");
		locale.fields[FIELD_SOUNDS].label = tr("Whether to play sounds");
		locale.fields[FIELD_FILES].label = tr("A list of pending file transfers");
		locale.fields[FIELD_MESSAGES].label = tr("A list of unread messages");
		locale.fields[FIELD_GROUPCHATS].label = tr("A list of joined conferences");
		locale.fields[FIELD_STATUS].label = tr("A presence or availability status");
		locale.fields[FIELD_STATUS_MESSAGE].label = tr("The status message text");
		locale.fields[FIELD_STATUS_PRIORITY].label = tr("The new priority for the client");

		if (FStatusChanger)
		{
			locale.fields[FIELD_STATUS].options["online"].label = FStatusChanger->nameByShow(IPresence::Online);
			locale.fields[FIELD_STATUS].options["chat"].label = FStatusChanger->nameByShow(IPresence::Chat);
			locale.fields[FIELD_STATUS].options["away"].label = FStatusChanger->nameByShow(IPresence::Away);
			locale.fields[FIELD_STATUS].options["xa"].label = FStatusChanger->nameByShow(IPresence::ExtendedAway);
			locale.fields[FIELD_STATUS].options["dnd"].label = FStatusChanger->nameByShow(IPresence::DoNotDisturb);
			locale.fields[FIELD_STATUS].options["invisible"].label = FStatusChanger->nameByShow(IPresence::Invisible);
			locale.fields[FIELD_STATUS].options["offline"].label = FStatusChanger->nameByShow(IPresence::Offline);
		}
	}
	return locale;
}

bool RemoteControl::processPing(const ICommandRequest &ARequest)
{
	if (FCommands)
	{
		if (ARequest.action == COMMAND_ACTION_EXECUTE)
		{
			ICommandResult result = FCommands->prepareResult(ARequest);
			result.status = COMMAND_STATUS_COMPLETED;

			ICommandNote pong;
			pong.type = COMMAND_NOTE_INFO;
			pong.message = tr("Pong!");
			result.notes.append(pong);

			return FCommands->sendCommandResult(result);
		}
	}
	return false;
}

bool RemoteControl::processSetStatus(const ICommandRequest &ARequest)
{
	if (FCommands && FDataForms && FStatusChanger)
	{
		ICommandResult result = FCommands->prepareResult(ARequest);
		bool isMainStatus = ARequest.node == COMMAND_NODE_SET_MAIN_STATUS;
		if (ARequest.action==COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
		{
			result.status = COMMAND_STATUS_EXECUTING;
			result.sessionId = QUuid::createUuid().toString();
			result.form.type = DATAFORM_TYPE_FORM;
			result.form.title = commandName(ARequest.node);

			IDataField field;
			field.type = DATAFIELD_TYPE_HIDDEN;
			field.var = "FORM_TYPE";
			field.value = DATA_FORM_REMOTECONTROL;
			field.required = false;
			result.form.fields.append(field);

			field.type = DATAFIELD_TYPE_LISTSINGLE;
			field.var = FIELD_STATUS;
			field.label = tr("A presence or availability status");
			field.value = QString::number(isMainStatus ? FStatusChanger->mainStatus() : FStatusChanger->streamStatus(ARequest.streamJid));
			field.required = true;

			IDataOption opt;
			if (!isMainStatus)
			{
				opt.label = tr("Main status");
				opt.value = QString::number(STATUS_MAIN_ID);
				field.options.append(opt);
			}
			foreach(int status, FStatusChanger->statusItems())
			{
				if (status > STATUS_NULL_ID)
				{
					opt.label = tr("%1 (%2)").arg(FStatusChanger->nameByShow(FStatusChanger->statusItemShow(status))).arg(FStatusChanger->statusItemName(status));
					opt.value = QString::number(status);
					field.options.append(opt);
				}
			}
			result.form.fields.append(field);
			result.actions.append(COMMAND_ACTION_COMPLETE);
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action==COMMAND_ACTION_COMPLETE || ARequest.action==COMMAND_ACTION_EXECUTE)
		{
			int index = FDataForms->fieldIndex(FIELD_STATUS, ARequest.form.fields);
			int statusId = index>=0 ? ARequest.form.fields.value(index).value.toInt() : STATUS_NULL_ID;
			if ((statusId>STATUS_NULL_ID || statusId==STATUS_MAIN_ID) && FStatusChanger->statusItems().contains(statusId))
			{
				if (isMainStatus)
					FStatusChanger->setMainStatus(statusId);
				else
					FStatusChanger->setStreamStatus(ARequest.streamJid,statusId);
				result.status = COMMAND_STATUS_COMPLETED;
			}
			else
			{
				ICommandNote note;
				note.type = COMMAND_NOTE_ERROR;
				note.message = tr("Requested status is not acceptable");
				result.notes.append(note);
				result.status = COMMAND_STATUS_CANCELED;
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action == COMMAND_ACTION_CANCEL)
		{
			result.status = COMMAND_STATUS_CANCELED;
			return FCommands->sendCommandResult(result);
		}
	}
	return false;
}

bool RemoteControl::processLeaveMUC(const ICommandRequest &ARequest)
{
	if (FCommands && FDataForms && FMultiUserChatPlugin)
	{
		ICommandResult result = FCommands->prepareResult(ARequest);
		if (ARequest.action==COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
		{
			result.sessionId = QUuid::createUuid().toString();
			result.form.type = DATAFORM_TYPE_FORM;
			result.form.title = commandName(ARequest.node);

			IDataField field;
			field.type = DATAFIELD_TYPE_HIDDEN;
			field.var = "FORM_TYPE";
			field.value = DATA_FORM_REMOTECONTROL;
			field.required = false;
			result.form.fields.append(field);

			field.type = DATAFIELD_TYPE_LISTMULTI;
			field.var = FIELD_GROUPCHATS;
			field.label = tr("A list of joined conferences");
			field.value = QVariant();
			field.required = true;

			IDataOption opt;
			foreach(IMultiUserChat* muc, FMultiUserChatPlugin->multiUserChats())
			{
				if (muc->isConnected() && muc->streamJid()==ARequest.streamJid)
				{
					opt.label = tr("%1 on %2").arg(muc->nickName()).arg(muc->roomJid().uBare());
					opt.value = muc->roomJid().bare();
					field.options.append(opt);
				}
			}

			if (field.options.isEmpty())
			{
				result.status = COMMAND_STATUS_COMPLETED;
				result.form = IDataForm();

				ICommandNote note;
				note.type = COMMAND_NOTE_INFO;
				note.message = tr("This entity is not joined to any conferences");
				result.notes.append(note);
			}
			else
			{
				result.status = COMMAND_STATUS_EXECUTING;
				result.form.fields.append(field);
				result.actions.append(COMMAND_ACTION_COMPLETE);
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action==COMMAND_ACTION_COMPLETE || ARequest.action==COMMAND_ACTION_EXECUTE)
		{
			int index = FDataForms->fieldIndex(FIELD_GROUPCHATS,ARequest.form.fields);
			if (index>=0)
			{
				foreach(const QString &roomJid, ARequest.form.fields.value(index).value.toStringList())
				{
					IMultiUserChatWindow *window = FMultiUserChatPlugin->findMultiChatWindow(ARequest.streamJid, roomJid);
					if (window != NULL)
						window->exitAndDestroy(tr("Remote command to leave"));
				}
				result.status = COMMAND_STATUS_COMPLETED;
			}
			else
			{
				result.status = COMMAND_STATUS_CANCELED;
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action == COMMAND_ACTION_CANCEL)
		{
			result.status = COMMAND_STATUS_CANCELED;
			return FCommands->sendCommandResult(result);;
		}
	}
	return false;
}

bool RemoteControl::processFileTransfers(const ICommandRequest &ARequest)
{
	if (FCommands && FDataForms && FFileStreamManager)
	{
		ICommandResult result = FCommands->prepareResult(ARequest);
		if (ARequest.action==COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
		{
			result.sessionId = QUuid::createUuid().toString();
			result.form.type = DATAFORM_TYPE_FORM;
			result.form.title = commandName(ARequest.node);

			IDataField field;
			field.type = DATAFIELD_TYPE_HIDDEN;
			field.var = "FORM_TYPE";
			field.value = DATA_FORM_REMOTECONTROL;
			field.required = false;
			result.form.fields.append(field);

			field.type = DATAFIELD_TYPE_LISTMULTI;
			field.var = FIELD_FILES;
			field.label = tr("Pending file transfers");
			field.value = QVariant();
			field.required = true;

			IDataOption opt;
			foreach(IFileStream *stream, FFileStreamManager->streams())
			{
				if (stream->streamKind() == IFileStream::ReceiveFile &&
					stream->streamState() == IFileStream::Creating)
				{
					QString name = FNotifications!=NULL ? FNotifications->contactName(stream->streamJid(),stream->contactJid()) : stream->contactJid().uBare();
					opt.label = tr("%1 (%2 bytes) from '%3'").arg(stream->fileName()).arg(stream->fileSize()).arg(name);
					opt.value = stream->streamId();
					field.options.append(opt);
				}
			}

			if (field.options.isEmpty())
			{
				result.status = COMMAND_STATUS_COMPLETED;
				result.form = IDataForm();

				ICommandNote note;
				note.type = COMMAND_NOTE_INFO;
				note.message = tr("There are no pending file transfers");
				result.notes.append(note);
			}
			else
			{
				result.status = COMMAND_STATUS_EXECUTING;
				result.form.fields.append(field);
				result.actions.append(COMMAND_ACTION_COMPLETE);
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action==COMMAND_ACTION_COMPLETE || ARequest.action==COMMAND_ACTION_EXECUTE)
		{
			int index = FDataForms->fieldIndex(FIELD_FILES, ARequest.form.fields);
			if (index >= 0)
			{
				foreach(const QString &streamId, ARequest.form.fields.value(index).value.toStringList())
				{
					IFileStream *stream = FFileStreamManager->streamById(streamId);
					QString defaultMethod = Options::node(OPV_FILESTREAMS_DEFAULTMETHOD).value().toString();
					if (stream->acceptableMethods().contains(defaultMethod))
					{
						stream->startStream(defaultMethod);
					}
					else if (!stream->acceptableMethods().isEmpty())
					{
						stream->startStream(stream->acceptableMethods().at(0));
					}
				}
				result.status = COMMAND_STATUS_COMPLETED;
			}
			else
			{
				result.status = COMMAND_STATUS_CANCELED;
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action == COMMAND_ACTION_CANCEL)
		{
			result.status = COMMAND_STATUS_CANCELED;
			return FCommands->sendCommandResult(result);
		}
	}
	return false;
}

bool RemoteControl::processSetOptions(const ICommandRequest &ARequest)
{
	if (FCommands)
	{
		ICommandResult result = FCommands->prepareResult(ARequest);
		if (ARequest.action==COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
		{
			result.status = COMMAND_STATUS_EXECUTING;
			result.sessionId = QUuid::createUuid().toString();
			result.form.type = DATAFORM_TYPE_FORM;
			result.form.title = commandName(ARequest.node);

			IDataField field;
			field.type = DATAFIELD_TYPE_HIDDEN;
			field.var = "FORM_TYPE";
			field.value = DATA_FORM_REMOTECONTROL;
			field.required = false;
			result.form.fields.append(field);

			field.type = DATAFIELD_TYPE_BOOLEAN;
			foreach(const QString &fieldName, optionItems.keys())
			{
				field.var = fieldName;
				field.label = optionItems[fieldName].label;
				field.value = Options::node(optionItems[fieldName].node).value().toBool();
				result.form.fields.append(field);
			}

			result.actions.append(COMMAND_ACTION_COMPLETE);
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action==COMMAND_ACTION_COMPLETE || ARequest.action==COMMAND_ACTION_EXECUTE)
		{
			foreach(const IDataField &field, ARequest.form.fields)
			{
				if (optionItems.contains(field.var) && Options::node(optionItems[field.var].node).value().toBool() != field.value.toBool())
					Options::node(optionItems[field.var].node).setValue(field.value.toBool());
			}
			result.status = COMMAND_STATUS_COMPLETED;
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action == COMMAND_ACTION_CANCEL)
		{
			result.status = COMMAND_STATUS_CANCELED;
			return FCommands->sendCommandResult(result);
		}
	}
	return false;
}

bool RemoteControl::processForwardMessages(const ICommandRequest &ARequest)
{
	if (FDataForms && FStanzaProcessor && FMessageProcessor)
	{
		ICommandResult result = FCommands->prepareResult(ARequest);
		if (ARequest.action==COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
		{
			result.status = COMMAND_STATUS_EXECUTING;
			result.sessionId = QUuid::createUuid().toString();
			result.form.type = DATAFORM_TYPE_FORM;
			result.form.title = commandName(ARequest.node);

			IDataField field;
			field.type = DATAFIELD_TYPE_HIDDEN;
			field.var = "FORM_TYPE";
			field.value = DATA_FORM_REMOTECONTROL;
			field.required = false;
			result.form.fields.append(field);

			field.type = DATAFIELD_TYPE_LISTMULTI;
			field.var = FIELD_MESSAGES;
			field.label = tr("List of unread messages");
			field.value = QVariant();
			field.required = true;

			QMap<Jid, int> unread;
			foreach(const Message &message, notifiedMessages(ARequest.streamJid))
			{
				if (ARequest.contactJid != message.from())
					unread[message.from()]++;
			}

			for (QMap<Jid, int>::const_iterator it=unread.constBegin(); it!=unread.constEnd(); ++it)
			{
				IDataOption opt;

				QString name = FNotifications!=NULL ? FNotifications->contactName(ARequest.streamJid,it.key()) : it.key().uBare();
				if (!it.key().resource().isEmpty())
					name += "/" + it.key().resource();

				opt.label = tr("%n message(s) from '%1'","",it.value()).arg(name);
				opt.value = it.key().full();
				field.options.append(opt);
			}

			if (field.options.isEmpty())
			{
				result.status = COMMAND_STATUS_COMPLETED;
				result.form = IDataForm();

				ICommandNote note;
				note.type = COMMAND_NOTE_INFO;
				note.message = tr("There are no unread messages");
				result.notes.append(note);
			}
			else
			{
				result.status = COMMAND_STATUS_EXECUTING;
				result.form.fields.append(field);
				result.actions.append(COMMAND_ACTION_COMPLETE);
			}

			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action==COMMAND_ACTION_COMPLETE || ARequest.action==COMMAND_ACTION_EXECUTE)
		{
			int index = (FDataForms != NULL) ? FDataForms->fieldIndex(FIELD_MESSAGES, ARequest.form.fields) : -1;
			if (index >= 0)
			{
				foreach(const QString &senderJid, ARequest.form.fields.value(index).value.toStringList())
				{
					foreach(Message message, notifiedMessages(ARequest.streamJid,senderJid))
					{
						message.detach();
						message.setFrom(QString::null);
						message.setTo(ARequest.contactJid.full());
						message.setDateTime(message.dateTime(),true);

						QDomElement addresses = message.stanza().firstElement("addresses",NS_ADDRESS);
						if (!addresses.isNull())
							addresses.parentNode().removeChild(addresses);
						addresses = message.stanza().addElement("addresses",NS_ADDRESS);
						QDomElement address = addresses.appendChild(message.stanza().createElement("address")).toElement();
						address.setAttribute("type","ofrom");
						address.setAttribute("jid",senderJid);

						FStanzaProcessor->sendStanzaOut(ARequest.streamJid,message.stanza());
					}
				}
				result.status = COMMAND_STATUS_COMPLETED;
			}
			else
			{
				result.status = COMMAND_STATUS_CANCELED;
			}
			return FCommands->sendCommandResult(result);
		}
		else if (ARequest.action == COMMAND_ACTION_CANCEL)
		{
			result.status = COMMAND_STATUS_CANCELED;
			return FCommands->sendCommandResult(result);
		}
	}
	return false;
}

QList<Message> RemoteControl::notifiedMessages(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QList<Message> messages;
	if (FMessageProcessor)
	{
		foreach(int messageId, FMessageProcessor->notifiedMessages())
		{
			Message message = FMessageProcessor->notifiedMessage(messageId);
			if(AStreamJid==message.to() && message.data(MDR_MESSAGE_DIRECTION).toInt()==IMessageProcessor::MessageIn)
			{
				if (message.type()!=Message::Error && !message.body().isEmpty())
				{
					if (FMultiUserChatPlugin==NULL || FMultiUserChatPlugin->findMultiUserChat(AStreamJid,Jid(message.from()).bare())==NULL)
					{
						if (AContactJid.isEmpty() || AContactJid==message.from())
							messages.append(message);
					}
				}
			}
		}
	}
	return messages;
}

Q_EXPORT_PLUGIN2(plg_remotecontrol, RemoteControl)
