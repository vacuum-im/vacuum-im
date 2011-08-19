#ifndef PRIVATESTORAGE_H
#define PRIVATESTORAGE_H

#include <QMap>
#include <definitions/namespaces.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/ixmppstreams.h>
#include <utils/stanza.h>
#include <utils/options.h>

class PrivateStorage :
			public QObject,
			public IPlugin,
			public IPrivateStorage,
			public IStanzaRequestOwner
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPrivateStorage IStanzaRequestOwner);
public:
	PrivateStorage();
	~PrivateStorage();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRIVATESTORAGE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
	//IPrivateStorage
	virtual bool isOpen(const Jid &AStreamJid) const;
	virtual bool isLoaded(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	virtual QDomElement getData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	virtual QString saveData(const Jid &AStreamJid, const QDomElement &AElement);
	virtual QString loadData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	virtual QString removeData(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
signals:
	void storageOpened(const Jid &AStreamJid);
	void dataSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataRemoved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void dataError(const QString &AId, const QString &AError);
	void storageAboutToClose(const Jid &AStreamJid);
	void storageClosed(const Jid &AStreamJid);
protected:
	QDomElement insertElement(const Jid &AStreamJid, const QDomElement &AElement);
	void removeElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void saveOptionsElement(const Jid &AStreamJid, const QDomElement &AElement) const;
	QDomElement loadOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
	void removeOptionsElement(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamAboutToClose(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
private:
	IStanzaProcessor *FStanzaProcessor;
private:
	QDomDocument FStorage;
	QMap<Jid, QDomElement> FStreamElements;
	QMap<QString, QDomElement> FSaveRequests;
	QMap<QString, QDomElement> FLoadRequests;
	QMap<QString, QDomElement> FRemoveRequests;
};

#endif // PRIVATESTORAGE_H
