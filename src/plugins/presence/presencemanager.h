#ifndef PRESENCEMANAGER_H
#define PRESENCEMANAGER_H

#include <QSet>
#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresencemanager.h>
#include "presence.h"

class PresenceManager :
	public QObject,
	public IPlugin,
	public IPresenceManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPresenceManager);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.PresenceManager");
public:
	PresenceManager();
	~PresenceManager();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRESENCE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IPresenceManager
	virtual QList<IPresence *> presences() const;
	virtual IPresence *findPresence(const Jid &AStreamJid) const;
	virtual IPresence *createPresence(IXmppStream *AXmppStream);
	virtual void destroyPresence(IPresence *APresence);
	virtual bool isPresenceActive(IPresence *APresence) const;
	virtual QList<Jid> onlineContacts() const;
	virtual bool isOnlineContact(const Jid &AContactJid) const;
	virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const;
	virtual QList<IPresenceItem> sortPresenceItems(const QList<IPresenceItem> &AItems) const;
signals:
	void presenceCreated(IPresence *APresence);
	void presenceOpened(IPresence *APresence);
	void presenceClosed(IPresence *APresence);
	void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriotity);
	void presenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void presenceDirectSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity);
	void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
	void presenceActiveChanged(IPresence *APresence, bool AActive);
	void presenceDestroyed(IPresence *APresence);
	void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
protected slots:
	void onPresenceOpened();
	void onPresenceClosed();
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceDirectSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void onPresenceDestroyed();
protected slots:
	void onXmppStreamActiveChanged(IXmppStream *AXmppStream, bool AActive);
private:
	IStanzaProcessor *FStanzaProcessor;
	IXmppStreamManager *FXmppStreamManager;
private:
	QList<IPresence *> FPresences;
	QObjectCleanupHandler FCleanupHandler;
	QHash<Jid, QSet<IPresence *> > FContactPresences;
};

#endif // PRESENCEMANAGER_H
