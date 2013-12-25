#ifndef PRIVATESTORAGE_H
#define PRIVATESTORAGE_H

#include <QSet>
#include <QMap>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/ipresence.h>

class PrivateStorage :
	public QObject,
	public IPlugin,
	public IPrivateStorage,
	public IStanzaHandler,
	public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPrivateStorage IStanzaHandler IStanzaRequestOwner);
public:
	PrivateStorage();
	~PrivateStorage();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRIVATESTORAGE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IPrivateStorage
	virtual bool isOpen(const Jid &AStreamJid) const;
	virtual bool isLoaded(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	virtual QDomElement getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	virtual QString saveData(const Jid &AStreamJid, const QDomElement &AElement);
	virtual QString loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	virtual QString removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
signals:
	void storageOpened(const Jid &AStreamJid);
	void dataError(const QString &AId, const XmppError &AError);
	void dataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void storageNotifyAboutToClose(const Jid &AStreamJid);
	void storageAboutToClose(const Jid &AStreamJid);
	void storageClosed(const Jid &AStreamJid);
protected:
	void notifyDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	QDomElement insertElement(const Jid &AStreamJid, const QDomElement &AElement);
	void removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void saveOptionsElement(const Jid &AStreamJid, const QDomElement &AElement) const;
	QDomElement loadOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	void removeOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamAboutToClose(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onPresenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
private:
	IXmppStreams *FXmppStreams;
	IPresencePlugin *FPresencePlugin;
	IStanzaProcessor *FStanzaProcessor;
private:
	int FSHINotifyDataChanged;
	QMap<QString, QDomElement> FSaveRequests;
	QMap<QString, QDomElement> FLoadRequests;
	QMap<QString, QDomElement> FRemoveRequests;
private:
	QDomDocument FStorage;
	QSet<Jid> FPreClosedStreams;
	QMap<Jid, QDomElement> FStreamElements;
};

#endif // PRIVATESTORAGE_H
