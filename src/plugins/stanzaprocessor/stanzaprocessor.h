#ifndef STANZAPROCESSOR_H
#define STANZAPROCESSOR_H

#include <QTimer>
#include <QMultiMap>
#include <QDomDocument>
#include <interfaces/ipluginmanager.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>

struct StanzaRequest {
	StanzaRequest() {
		timer=NULL;
		owner=NULL;
	}
	Jid streamJid;
	Jid contactJid;
	QTimer *timer;
	IStanzaRequestOwner *owner;
};

class StanzaProcessor :
	public QObject,
	public IPlugin,
	public IStanzaProcessor,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IStanzaProcessor IXmppStanzaHadler);
public:
	StanzaProcessor();
	~StanzaProcessor();
	//IPlugin
	virtual QObject* instance() { return this; }
	virtual QUuid pluginUuid() const { return STANZAPROCESSOR_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AXmppStream, Stanza &AStanza, int AOrder);
	//IStanzaProcessor
	virtual QString newId() const;
	virtual bool sendStanzaIn(const Jid &AStreamJid, Stanza &AStanza);
	virtual bool sendStanzaOut(const Jid &AStreamJid, Stanza &AStanza);
	virtual bool sendStanzaRequest(IStanzaRequestOwner *AIqOwner, const Jid &AStreamJid, Stanza &AStanza, int ATimeout);
	virtual Stanza makeReplyResult(const Stanza &AStanza) const;
	virtual Stanza makeReplyError(const Stanza &AStanza, const XmppStanzaError &AError) const;
	virtual bool checkStanza(const Stanza &AStanza, const QString &ACondition) const;
	virtual QList<int> stanzaHandles() const;
	virtual IStanzaHandle stanzaHandle(int AHandleId) const;
	virtual int insertStanzaHandle(const IStanzaHandle &AHandle);
	virtual void removeStanzaHandle(int AHandleId);
signals:
	void stanzaSent(const Jid &AStreamJid, const Stanza &AStanza);
	void stanzaReceived(const Jid &AStreamJid, const Stanza &AStanza);
	void stanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle);
	void stanzaHandleRemoved(int AHandleId, const IStanzaHandle &AHandle);
protected:
	bool checkCondition(const QDomElement &AElem, const QString &ACondition, int APos = 0) const;
	bool processStanza(const Jid &AStreamJid, Stanza &AStanza, int ADirection) const;
	bool processStanzaRequest(const Jid &AStreamJid, const Stanza &AStanza);
	void processRequestTimeout(const QString &AStanzaId) const;
	void removeStanzaRequest(const QString &AStanzaId);
	void insertErrorElement(Stanza &AStanza, const XmppStanzaError &AError) const;
protected slots:
	void onStreamCreated(IXmppStream *AXmppStream);
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onStreamDestroyed(IXmppStream *AXmppStream);
	void onStanzaRequestTimeout();
	void onStanzaRequestOwnerDestroyed(QObject *AOwner);
	void onStanzaHandlerDestroyed(QObject *AHandler);
private:
	IXmppStreams *FXmppStreams;
private:
	QMap<int, IStanzaHandle> FHandles;
	QMap<QString, StanzaRequest> FRequests;
	QMultiMap<int, int> FHandleIdByOrder;
};

#endif // STANZAPROCESSOR_H
