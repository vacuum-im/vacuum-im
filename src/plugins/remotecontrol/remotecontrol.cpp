#include "remotecontrol.h"

#include <QtDebug>

#define REMOTECONTROL_NODE "http://jabber.org/protocol/rc"
#define DATA_FORM_REMOTECONTROL "http://jabber.org/protocol/rc"

RemoteControl::RemoteControl()
{
	FPluginManager = NULL;
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
	APluginInfo->description = tr("Allows remote entities execute local commands (XEP-0146)");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Maxim Ignatenko";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(COMMANDS_UUID);
}

bool RemoteControl::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	FPluginManager = APluginManager;
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

	return (FCommands != NULL);
}

bool RemoteControl::initObjects()
{
	if (FCommands != NULL)
	{
		FCommands->insertServer("ping", this);
		if (FStatusChanger != NULL)
		{
			FCommands->insertServer(REMOTECONTROL_NODE"#set-status", this);
			FCommands->insertServer("set-main-status", this);
		}
		if (FMUCPlugin != NULL)
		{
			FCommands->insertServer(REMOTECONTROL_NODE"#leave-groupchats", this);
		}
		return true;
	}
	if (FDataForms != NULL)
	{
		FDataForms->insertLocalizer(this, DATA_FORM_REMOTECONTROL);
	}
	return false;
}

bool RemoteControl::initSettings()
{
	return true;
}

bool RemoteControl::startPlugin()
{
	return true;
}

QString RemoteControl::commandName(const QString &ANode) const
{
	if (ANode == "ping") return tr("Ping");
	if (ANode == REMOTECONTROL_NODE"#set-status") return tr("Change status");
	if (ANode == REMOTECONTROL_NODE"#leave-groupchats") return tr("Leave groupchats");
	if (ANode == "set-main-status") return tr("Change main status");
	return "";
}

bool RemoteControl::receiveCommandRequest(const ICommandRequest &ARequest)
{
	if (ARequest.node == "ping" && ARequest.action == COMMAND_ACTION_EXECUTE)
		return processPing(ARequest);

	if (ARequest.node == REMOTECONTROL_NODE"#set-status" && FStatusChanger != NULL)
		return processSetStatus(ARequest);

	if (ARequest.node == "set-main-status" && FStatusChanger != NULL)
		return processSetMainStatus(ARequest);

	if (ARequest.node == REMOTECONTROL_NODE"#leave-groupchats" && FMUCPlugin != NULL)
		return processLeaveMUC(ARequest);

	return false;
}

bool RemoteControl::processPing(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->makeResult(ARequest);
	result.status = COMMAND_STATUS_COMPLETED;
	ICommandNote pong;
	pong.type = COMMAND_NOTE_INFO;
	pong.message = tr("Pong!");
	result.notes.append(pong);
	FCommands->sendCommandResult(result);
	return true;
}

bool RemoteControl::processLeaveMUC(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->makeResult(ARequest);
	if (ARequest.action == COMMAND_ACTION_EXECUTE)
	{
		result.status = COMMAND_STATUS_EXECUTING;
		result.sessionId = QUuid::createUuid().toString();

		result.form.type = DATAFORM_TYPE_FORM;
		result.form.title = tr("Leave groupchats");
		result.form.instructions.append(tr("Choose groupchats you want to leave"));

		IDataField field;
		field.type = DATAFIELD_TYPE_HIDDEN;
		field.var = "FORM_TYPE";
		field.value = DATA_FORM_REMOTECONTROL;
		result.form.fields.append(field);

		field.type = DATAFIELD_TYPE_LISTMULTI;
		field.var = "groupchats";
		field.label = tr("Groupchats");
		field.required = true;
		IDataOption opt;
		QList<IMultiUserChat *> mucList = FMUCPlugin->multiUserChats();
		foreach(IMultiUserChat* muc, mucList)
		{
			if (muc->streamJid() == ARequest.streamJid)
			{
				opt.label = tr("%1 on %2").arg(muc->nickName(), muc->roomJid().bare());
				opt.value = muc->roomJid().full();
				field.options.append(opt);
			}
		}
		result.form.fields.append(field);
		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == "" || ARequest.action == COMMAND_ACTION_COMPLETE)
	{
		result.status = COMMAND_STATUS_COMPLETED;
		foreach(IDataField field, ARequest.form.fields)
		{
			if (field.var == "groupchats")
			{
				foreach(QString roomJid, field.value.toStringList())
				{
					IMultiUserChatWindow *w = FMUCPlugin->multiChatWindow(ARequest.streamJid, Jid(roomJid));
					if (w != NULL) w->exitAndDestroy(tr("Remote request from \"%1\"").arg(ARequest.commandJid.full()));
				}
			}
		}
		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == COMMAND_ACTION_CANCEL)
	{
		result.status = COMMAND_STATUS_CANCELED;
		FCommands->sendCommandResult(result);
		return true;
	}
	return false;
}

bool RemoteControl::processSetStatus(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->makeResult(ARequest);
	if (ARequest.action == COMMAND_ACTION_EXECUTE)
	{
		result.status = COMMAND_STATUS_EXECUTING;
		result.sessionId = QUuid::createUuid().toString();
		result.form.type = DATAFORM_TYPE_FORM;
		result.form.title = tr("Change status");

		IDataField field;
		field.type = DATAFIELD_TYPE_HIDDEN;
		field.var = "FORM_TYPE";
		field.value = DATA_FORM_REMOTECONTROL;
		result.form.fields.append(field);

		field.type = DATAFIELD_TYPE_LISTSINGLE;
		field.var = "status";
		field.label = tr("Change status");
		field.required = true;

		IDataOption opt;
		opt.label = tr("Main status");
		opt.value = "-1";
		field.options.append(opt);
		foreach(int status, FStatusChanger->statusItems())
		{
			if (status < 0) continue;
			opt.label = tr("%1 (%2)").arg(FStatusChanger->nameByShow(FStatusChanger->statusItemShow(status)),
										  FStatusChanger->statusItemName(status));
			opt.value = QString::number(status);
			field.options.append(opt);
		}
		result.form.fields.append(field);
		field.options.clear();

		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == "" || ARequest.action == COMMAND_ACTION_COMPLETE)
	{
		// Следующий код считает что STATUS_MAIN_ID - наименьший допустимый статус
		result.status = COMMAND_STATUS_COMPLETED;
		int statusId = STATUS_MAIN_ID - 1;
		foreach(IDataField field, ARequest.form.fields)
		{
			if (field.var == "status")
			{
				bool ok = true;
				statusId = field.value.toInt(&ok);
				if (!ok) statusId = STATUS_MAIN_ID - 1;
			}
		}
		if (statusId >= STATUS_MAIN_ID)
			FStatusChanger->setStreamStatus(ARequest.streamJid,statusId);

		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == COMMAND_ACTION_CANCEL)
	{
		result.status = COMMAND_STATUS_CANCELED;
		FCommands->sendCommandResult(result);
		return true;
	}
	return false;
}

bool RemoteControl::processSetMainStatus(const ICommandRequest &ARequest)
{
	ICommandResult result = FCommands->makeResult(ARequest);
	if (ARequest.action == COMMAND_ACTION_EXECUTE)
	{
		result.status = COMMAND_STATUS_EXECUTING;
		result.sessionId = QUuid::createUuid().toString();
		result.form.type = DATAFORM_TYPE_FORM;
		result.form.title = tr("Change main status");

		IDataField field;
		field.type = DATAFIELD_TYPE_HIDDEN;
		field.var = "FORM_TYPE";
		field.value = DATA_FORM_REMOTECONTROL;
		result.form.fields.append(field);

		field.type = DATAFIELD_TYPE_LISTSINGLE;
		field.var = "status";
		field.label = tr("Change main status");
		field.required = true;

		IDataOption opt;
		foreach(int status, FStatusChanger->statusItems())
		{
			if (status < 0) continue;
			opt.label = tr("%1 (%2)").arg(FStatusChanger->nameByShow(FStatusChanger->statusItemShow(status)),
										  FStatusChanger->statusItemName(status));
			opt.value = QString::number(status);
			field.options.append(opt);
		}
		result.form.fields.append(field);
		field.options.clear();

		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == "" || ARequest.action == COMMAND_ACTION_COMPLETE)
	{
		// Следующий код считает что все допустимые статусы неотрицательные
		result.status = COMMAND_STATUS_COMPLETED;
		int statusId = -1;
		foreach(IDataField field, ARequest.form.fields)
		{
			if (field.var == "status")
			{
				bool ok = true;
				statusId = field.value.toInt(&ok);
				if (!ok) statusId = -1;
			}
		}
		if (statusId >= 0)
			FStatusChanger->setMainStatus(statusId);

		FCommands->sendCommandResult(result);
		return true;
	}
	else if (ARequest.action == COMMAND_ACTION_CANCEL)
	{
		result.status = COMMAND_STATUS_CANCELED;
		FCommands->sendCommandResult(result);
		return true;
	}
	return false;
}


bool RemoteControl::receiveCommandError(const ICommandError &AError)
{
	return true;
}

IDataFormLocale RemoteControl::dataFormLocale(const QString &AFormType)
{
	IDataFormLocale locale;
	if (AFormType == DATA_FORM_REMOTECONTROL)
	{
		locale.fields["auto-auth"].label = tr("Wheter to automatically authorize subscription requests");
		locale.fields["auto-files"].label = tr("Whether to automatically accept file transfers");
		locale.fields["auto-msg"].label = tr("Whether to automatically open new messages");
		locale.fields["auto-offline"].label = tr("Whether to automatically go offline when idle");
		locale.fields["sounds"].label = tr("Whether to play sounds");
		locale.fields["files"].label = tr("A list of pending file transfers");
		locale.fields["groupchats"].label = tr("A list of joined groupchat rooms");
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
