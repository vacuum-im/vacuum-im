#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <definitions/namespaces.h>
#include <definitions/dataformtypes.h>
#include <definitions/stanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/icommands.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/idataforms.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/imessageprocessor.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/inotifications.h>

#define REMOTECONTROL_UUID "{152A3172-9A38-11DF-A3E4-001CBF2EDCFC}"

class RemoteControl :
			public QObject,
			public IPlugin,
			public ICommandServer,
			public IStanzaHandler,
			public IDataLocalizer
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ICommandServer IStanzaHandler IDataLocalizer);
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
	// IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	// ICommandServer
	virtual bool isCommandPermitted(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode) const;
	virtual QString commandName(const QString &ANode) const;
	virtual bool receiveCommandRequest(const ICommandRequest &ARequest);
	// IDataLocalizer
	virtual IDataFormLocale dataFormLocale(const QString &AFormType);
private:
	bool processPing(const ICommandRequest &ARequest);
	bool processLeaveMUC(const ICommandRequest &ARequest);
	bool processSetStatus(const ICommandRequest &ARequest);
	bool processFileTransfers(const ICommandRequest &ARequest);
	bool processSetOptions(const ICommandRequest &ARequest);
	bool processForwardMessages(const ICommandRequest &ARequest);
private:
	ICommands *FCommands;
	IStatusChanger *FStatusChanger;
	IMultiUserChatPlugin *FMUCPlugin;
	IDataForms *FDataForms;
	IFileStreamsManager *FFileStreamManager;
	IMessageProcessor *FMessageProcessor;
	IStanzaProcessor *FStanzaProcessor;
	INotifications *FNotifications;
private:
	int FSHIMessageForward;
};

#endif // REMOTECONTROL_H
