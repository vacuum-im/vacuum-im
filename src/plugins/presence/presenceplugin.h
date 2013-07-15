#ifndef PRESENCEPLUGIN_H
#define PRESENCEPLUGIN_H

#include <QSet>
#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include "presence.h"

class PresencePlugin :
	public QObject,
	public IPlugin,
	public IPresencePlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IPresencePlugin);
#ifdef HAVE_QT5
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IPresencePlugin");
#endif
public:
	PresencePlugin();
	~PresencePlugin();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return PRESENCE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IPresencePlugin
	virtual IPresence *getPresence(IXmppStream *AXmppStream);
	virtual IPresence *findPresence(const Jid &AStreamJid) const;
	virtual void removePresence(IXmppStream *AXmppStream);
	virtual QList<Jid> onlineContacts() const;
	virtual bool isOnlineContact(const Jid &AContactJid) const;
	virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const;
	virtual QList<IPresenceItem> sortPresenceItems(const QList<IPresenceItem> &AItems) const;
signals:
	void streamStateChanged(const Jid &AStreamJid, bool AStateOnline);
	void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void presenceAdded(IPresence *APresence);
	void presenceOpened(IPresence *APresence);
	void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriotity);
	void presenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void presenceDirectSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity);
	void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
	void presenceClosed(IPresence *APresence);
	void presenceRemoved(IPresence *APresence);
protected slots:
	void onPresenceOpened();
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onPresenceDirectSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
	void onPresenceAboutToClose(int AShow, const QString &AStatus);
	void onPresenceClosed();
	void onPresenceDestroyed(QObject *AObject);
	void onStreamAdded(IXmppStream *AXmppStream);
	void onStreamRemoved(IXmppStream *AXmppStream);
private:
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
private:
	QList<IPresence *> FPresences;
	QObjectCleanupHandler FCleanupHandler;
	QHash<Jid, QSet<IPresence *> > FContactPresences;
};

#endif // PRESENCEPLUGIN_H
