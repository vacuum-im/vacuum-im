#ifndef COMMANDS_H
#define COMMANDS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/icommands.h>
#include <interfaces/idataforms.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/ixmppuriqueries.h>
#include "commanddialog.h"

class Commands :
	public QObject,
	public IPlugin,
	public ICommands,
	public IStanzaHandler,
	public IStanzaRequestOwner,
	public IXmppUriHandler,
	public IDiscoHandler,
	public IDiscoFeatureHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin ICommands IStanzaHandler IStanzaRequestOwner IXmppUriHandler IDiscoHandler IDiscoFeatureHandler);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.ICommands");
public:
	Commands();
	~Commands();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return COMMANDS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IDiscoHandler
	virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
	virtual void fillDiscoItems(IDiscoItems &ADiscoItems);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//ICommands
	virtual QList<QString> commandNodes() const;
	virtual ICommandServer *commandServer(const QString &ANode) const;
	virtual void insertServer(const QString &ANode, ICommandServer *AServer);
	virtual void removeServer(const QString &ANode);
	virtual void insertClient(ICommandClient *AClient);
	virtual void removeClient(ICommandClient *AClient);
	virtual QString sendCommandRequest(const ICommandRequest &ARequest);
	virtual bool sendCommandResult(const ICommandResult &AResult);
	virtual QList<ICommand> contactCommands(const Jid &AStreamJid, const Jid &AContactJid) const;
	virtual bool executeCommand(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode);
	virtual ICommandResult prepareResult(const ICommandRequest&) const;
signals:
	void serverInserted(const QString &ANode, ICommandServer *AServer);
	void serverRemoved(const QString &ANode);
	void clientInserted(ICommandClient *AClient);
	void clientRemoved(ICommandClient *AClient);
	void commandsUpdated(const Jid &AstreamJid, const Jid &AContactJid, const QList<ICommand> &ACommands);
protected:
	void registerDiscoFeatures();
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
	void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
	void onDiscoItemsReceived(const IDiscoItems &AItems);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onExecuteActionTriggered(bool);
	void onRequestActionTriggered(bool);
private:
	IDataForms *FDataForms;
	IXmppStreamManager *FXmppStreamManager;
	IStanzaProcessor *FStanzaProcessor;
	IServiceDiscovery *FDiscovery;
	IPresenceManager *FPresenceManager;
	IXmppUriQueries *FXmppUriQueries;
private:
	QList<QString> FRequests;
	QMap<Jid, int> FSHICommands;
private:
	QList<ICommandClient *> FClients;
	QMap<QString, ICommandServer *> FServers;
private:
	QMap<Jid, QList<Jid> > FOnlineAgents;
	QMap<Jid, QMap<Jid, QList<ICommand> > > FCommands;
};

#endif // COMMANDS_H
