#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <definitions/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/icommands.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/idataforms.h>

#define REMOTECONTROL_UUID "{152A3172-9A38-11DF-A3E4-001CBF2EDCFC}"

class RemoteControl :
			public QObject,
			public IPlugin,
			public ICommandServer,
			public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ICommandServer IDataLocalizer);
public:
	RemoteControl();
	~RemoteControl();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return REMOTECONTROL_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// ICommandServer
	virtual QString commandName(const QString &ANode) const;
	virtual bool receiveCommandRequest(const ICommandRequest &ARequest);
	virtual bool receiveCommandError(const ICommandError &AError);
	// IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
private:
	bool processPing(const ICommandRequest &ARequest);
	bool processLeaveMUC(const ICommandRequest &ARequest);
	bool processSetStatus(const ICommandRequest &ARequest);
	bool processSetMainStatus(const ICommandRequest &ARequest);
	IPluginManager *FPluginManager;
	ICommands *FCommands;
	IStatusChanger *FStatusChanger;
	IMultiUserChatPlugin *FMUCPlugin;
	IDataForms *FDataForms;
};

#endif // REMOTECONTROL_H
