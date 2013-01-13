#ifndef MESSAGECARBONS_H
#define MESSAGECARBONS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/imessagecarbons.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/imessageprocessor.h>
#include <utils/message.h>

class MessageCarbons : 
	public QObject,
	public IPlugin,
	public IMessageCarbons,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMessageCarbons IStanzaHandler IStanzaRequestOwner);
public:
	MessageCarbons();
	~MessageCarbons();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return MESSAGECARBONS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IMessageCarbons
	virtual bool isSupported(const Jid &AStreamJid) const;
	virtual bool isEnabled(const Jid &AStreamJid) const;
	virtual bool setEnabled(const Jid &AStreamJid, bool AEnabled);
signals:
	void enableChanged(const Jid &AStreamJid, bool AEnable);
	void messageSent(const Jid &AStreamJid, const Message &AMessage);
	void messageReceived(const Jid &AStreamJid, const Message &AMessage);
	void errorReceived(const Jid &AStreamJid, const XmppStanzaError &AError);
protected slots:
	void onXmppStreamOpened(IXmppStream *AXmppStream);
	void onXmppStreamClosed(IXmppStream *AXmppStream);
	void onDiscoInfoReceived(const IDiscoInfo &AInfo);
private:
	IXmppStreams *FXmppStreams;
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	IMessageProcessor *FMessageProcessor;
private:
	QMap<Jid,int> FSHIForwards;
	QList<QString> FEnableRequests;
	QList<QString> FDisableRequests;
private:
	QMap<Jid, bool> FEnabled;
};

#endif // MESSAGECARBONS_H
