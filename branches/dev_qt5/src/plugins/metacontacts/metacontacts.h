#ifndef METACONTACTS_H
#define METACONTACTS_H

#include <interfaces/ipluginmanager.h>
#include <interfaces/imetacontacts.h>
#include <interfaces/iprivatestorage.h>

class MetaContacts : 
	public QObject,
	public IPlugin,
	public IMetaContacts
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMetaContacts);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.MetaContacts");
public:
	MetaContacts();
	~MetaContacts();
	virtual QObject *instance() { return this; }
	// IPlugin
	virtual QUuid pluginUuid() const { return METACONTACTS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	// IMetaContacts
	virtual bool isReady(const Jid &AStreamJid) const;
protected:
	bool processMetaContact(const Jid &AStreamJid, const IMetaContact &AContact);
	void processMetaContacts(const Jid &AStreamJid, const QList<IMetaContact> &AContacts);
protected:
	bool saveContactsToStorage(const Jid &AStreamJid) const;
protected:
	QString metaContactsFileName(const Jid &AStreamJid) const;
	QList<IMetaContact> loadMetaContactsFromXML(const QDomElement &AElement) const;
	void saveMetaContactsToXML(QDomElement &AElement, const QList<IMetaContact> &AContacts) const;
	QList<IMetaContact> loadMetaContactsFromFile(const QString &AFileName) const;
	void saveMetaContactsToFile(const QString &AFileName, const QList<IMetaContact> &AContacts) const;
protected slots:
	void onPrivateStorageOpened(const Jid &AStreamJid);
	void onPrivateStorageDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateStorageDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onPrivateStorageNotifyAboutToClose(const Jid &AStreamJid);
private:
	IPluginManager *FPluginManager;
	IPrivateStorage *FPrivateStorage;
private:
	QMap<Jid, QMap<QUuid, IMetaContact > > FMetaContacts;
};

#endif // METACONTACTS_H
