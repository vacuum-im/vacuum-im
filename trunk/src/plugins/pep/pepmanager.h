#ifndef PEPMANAGER_H
#define PEPMANAGER_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/ipep.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>

class PEPManager : public QObject,
                   public IPlugin,
                   public IPEPManager,
                   public IStanzaHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPEPManager IStanzaHandler);
public:
	PEPManager();
	~PEPManager();
	// IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return PEP_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// IPEPManager
	virtual bool publishItem(const QString &ANode, const QDomElement &AItem);
	virtual bool publishItem(const QString &ANode, const QDomElement &AItem, const Jid& streamJid);
	virtual int insertNodeHandler(const QString &ANode, IPEPHandler *AHandle);
	virtual bool removeNodeHandler(int AHandleId);
	virtual IPEPHandler* nodeHandler(int AHandleId);
	// IStanzaHandler
	virtual bool stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	virtual bool stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
private slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onPEPHandlerDestroyed(QObject *);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
private:
	IServiceDiscovery *FDisco;
	IStanzaProcessor *FStanzaProcessor;
	QList<Jid> pepCapableStreams;
	QList<Jid> unverifiedStreams;
	QMap<int, IPEPHandler*> handlersById;
	QMultiMap<QString, int> handlersByNode;
	QMap<Jid, int> stanzaHandles;
};

#endif // PEPMANAGER_H
