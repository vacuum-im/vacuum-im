#include "remotecontrol.h"

#define COMMAND_NODE_ROOT               "http://jabber.org/protocol/rc"
#define COMMAND_NODE_PING               COMMAND_NODE_ROOT"#ping"
#define COMMAND_NODE_SET_STATUS         COMMAND_NODE_ROOT"#set-status"
#define COMMAND_NODE_SET_MAIN_STATUS    COMMAND_NODE_ROOT"#set-main-status"
#define COMMAND_NODE_LEAVE_MUC          COMMAND_NODE_ROOT"#leave-groupchats"

#define FIELD_STATUS                    "status"
#define FIELD_GROUPCHATS                "groupchats"

RemoteControl::RemoteControl()
{
	FCommands = NULL;
	FStatusChanger = NULL;
	FMUCPlugin = NULL;
	FDataForms = NULL;
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
	}
	plugin = APluginManager->pluginInterface("IStatusChanger").value(0,NULL);
	if (plugin)
	{
		FStatusChanger = qobject_cast<IStatusChanger *>(plugin->instance());
	}
	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMUCPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
	}
	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}
	return (FCommands!=NULL && FDataForms!=NULL);
}

bool RemoteControl::initObjects()
{
	if (FCommands != NULL)
	{
		FCommands->insertServer(COMMAND_NODE_PING, this);
		if (FStatusChanger != NULL)
		{
			FCommands->insertServer(COMMAND_NODE_SET_STATUS, this);
			FCommands->insertServer(COMMAND_NODE_SET_MAIN_STATUS, this);
		}
		if (FMUCPlugin != NULL)
		{
			FCommands->insertServer(COMMAND_NODE_LEAVE_MUC, this);
		}
	}
	if (FDataForms != NULL)
	{
		FDataForms->insertLocalizer(this, DATA_FORM_REMOTECONTROL);
	}
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
	return QString::null;
}

bool RemoteControl::receiveCommandRequest(const ICommandRequest &ARequest)
{
	if (isCommandPermitted(ARequest.streamJid, ARequest.contactJid, ARequest.node))
	{
		if (ARequest.node == COMMAND_NODE_PING)
			return processPing(ARequest);

		if (ARequest.node == COMMAND_NODE_SET_STATUS && FStatusChanger != NULL)
			return processSetStatus(ARequest);

		if (ARequest.node == COMMAND_NODE_SET_MAIN_STATUS && FStatusChanger != NULL)
			return processSetStatus(ARequest);

		if (ARequest.node == COMMAND_NODE_LEAVE_MUC && FMUCPlugin != NULL)
			return processLeaveMUC(ARequest);
	}
	return false;
}

bool RemoteControl::processPing(const ICommandRequest &ARequest)
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
	return false;
}

bool RemoteControl::processLeaveMUC(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->prepareResult(ARequest);
	if (ARequest.action == COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
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
		field.required = true;

		IDataOption opt;
		foreach(IMultiUserChat* muc, FMUCPlugin->multiUserChats())
		{
			if (muc->isOpen() && muc->streamJid()==ARequest.streamJid)
			{
				opt.label = tr("%1 on %2").arg(muc->nickName()).arg(muc->roomJid().bare());
				opt.value = muc->roomJid().full();
				field.options.append(opt);
			}
		}

		if (field.options.isEmpty())
		{
			ICommandNote note;
			note.type = COMMAND_NOTE_INFO;
			note.message = tr("This entity is not joined to any conferences");
			result.notes.append(note);
			result.status = COMMAND_STATUS_COMPLETED;
			result.form = IDataForm();
		}
		else
		{
			result.form.fields.append(field);
			result.status = COMMAND_STATUS_EXECUTING;
			result.actions.append(COMMAND_ACTION_COMPLETE);
		}
		return FCommands->sendCommandResult(result);
	}
	else if (ARequest.action == COMMAND_ACTION_COMPLETE || ARequest.action == COMMAND_ACTION_EXECUTE)
	{
		int index = FDataForms!=NULL ? FDataForms->fieldIndex(FIELD_GROUPCHATS,ARequest.form.fields) : -1;
		if (index>=0)
		{
			foreach(QString roomJid, ARequest.form.fields.value(index).value.toStringList())
			{
				IMultiUserChatWindow *window = FMUCPlugin->multiChatWindow(ARequest.streamJid, roomJid);
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
	return false;
}

bool RemoteControl::processSetStatus(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->prepareResult(ARequest);
	bool isMainStatus = ARequest.node == COMMAND_NODE_SET_MAIN_STATUS;
	if (ARequest.action == COMMAND_ACTION_EXECUTE && ARequest.form.fields.isEmpty())
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
				opt.label = tr("%1 (%2)")
					.arg(FStatusChanger->nameByShow(FStatusChanger->statusItemShow(status)))
					.arg(FStatusChanger->statusItemName(status));
				opt.value = QString::number(status);
				field.options.append(opt);
			}
		}
		result.form.fields.append(field);
		result.actions.append(COMMAND_ACTION_COMPLETE);
		return FCommands->sendCommandResult(result);
	}
	else if (ARequest.action == COMMAND_ACTION_COMPLETE || ARequest.action == COMMAND_ACTION_EXECUTE)
	{
		int index = FDataForms!=NULL ? FDataForms->fieldIndex(FIELD_STATUS, ARequest.form.fields) : -1;
		int statusId = index>=0 ? ARequest.form.fields.value(index).value.toInt() : STATUS_NULL_ID;
		if (statusId!=STATUS_NULL_ID && statusId>=(isMainStatus ? STATUS_MAIN_ID : STATUS_NULL_ID) && FStatusChanger->statusItems().contains(statusId))
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
	return false;
}

IDataFormLocale RemoteControl::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_REMOTECONTROL)
	{
		locale.fields["auto-auth"].label = tr("Whether to automatically authorize subscription requests");
		locale.fields["auto-files"].label = tr("Whether to automatically accept file transfers");
		locale.fields["auto-msg"].label = tr("Whether to automatically open new messages");
		locale.fields["auto-offline"].label = tr("Whether to automatically go offline when idle");
		locale.fields["sounds"].label = tr("Whether to play sounds");
		locale.fields["files"].label = tr("A list of pending file transfers");
		locale.fields["groupchats"].label = tr("A list of joined conferences");
		locale.fields["status"].label = tr("A presence or availability status");
		locale.fields["status-message"].label = tr("The status message text");
		locale.fields["status-priority"].label = tr("The new priority for the client");
		locale.fields["status"].options["chat"].label = tr("Chat");
		locale.fields["status"].options["online"].label = tr("Online");
		locale.fields["status"].options["away"].label = tr("Away");
		locale.fields["status"].options["xa"].label = tr("Extended Away");
		locale.fields["status"].options["dnd"].label = tr("Do Not Disturb");
		locale.fields["status"].options["invisible"].label = tr("Invisible");
		locale.fields["status"].options["offline"].label = tr("Offline");
	}
	return locale;
}

Q_EXPORT_PLUGIN2(plg_remotecontrol, RemoteControl)
