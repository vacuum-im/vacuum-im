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
	virtual IPresence *addPresence(IXmppStream *AXmppStream);
	virtual IPresence *getPresence(const Jid &AStreamJid) const;
	virtual bool isContactOnline(const Jid &AContactJid) const { return FContactPresences.contains(AContactJid); }
	virtual QList<Jid> contactsOnline() const { return FContactPresences.keys(); }
	virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const { return FContactPresences.value(AContactJid).toList(); }
	virtual void removePresence(IXmppStream *AXmppStream);
signals:
	void streamStateChanged(const Jid &AStreamJid, bool AStateOnline);
	void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void presenceAdded(IPresence *APresence);
	void presenceOpened(IPresence *APresence);
	void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriotity);
	void presenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
	void presenceSent(IPresence *APresence, const Jid &AContactJid, int AShow, const QString &AStatus, int APriotity);
	void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
	void presenceClosed(IPresence *APresence);
	void presenceRemoved(IPresence *APresence);
protected slots:
	void onPresenceOpened();
	void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
	void onPresenceReceived(const IPresenceItem &APresenceItem);
	void onPresenceSent(const Jid &AContactJid, int AShow, const QString &AStatus, int APriority);
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
