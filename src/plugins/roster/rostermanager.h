#ifndef ROSTERMANAGER_H
#define ROSTERMANAGER_H

#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresencemanager.h>
#include "roster.h"

class RosterManager :
	public QObject,
	public IPlugin,
	public IRosterManager
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IRosterManager);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IRosterManager");
public:
	RosterManager();
	~RosterManager();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return ROSTER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IRosterManager
	virtual QList<IRoster *> rosters() const;
	virtual IRoster *findRoster(const Jid &AStreamJid) const;
	virtual IRoster *createRoster(IXmppStream *AXmppStream);
	virtual void destroyRoster(IRoster *ARoster);
	virtual bool isRosterActive(IRoster *ARoster) const;
	virtual QString rosterFileName(const Jid &AStreamJid) const;
signals:
	void rosterCreated(IRoster *ARoster);
	void rosterOpened(IRoster *ARoster);
	void rosterClosed(IRoster *ARoster);
	void rosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void rosterSubscriptionSent(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void rosterSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void rosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
	void rosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
	void rosterActiveChanged(IRoster *ARoster, bool AActive);
	void rosterDestroyed(IRoster *ARoster);
protected slots:
	void onRosterOpened();
	void onRosterClosed();
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void onRosterSubscriptionSent(const Jid &AItemJid, int ASubsType, const QString &AText);
	void onRosterSubscriptionReceived(const Jid &AItemJid, int ASubsType, const QString &AText);
	void onRosterStreamJidAboutToBeChanged(const Jid &AAfter);
	void onRosterStreamJidChanged(const Jid &ABefore);
	void onRosterDestroyed();
protected slots:
	void onXmppStreamActiveChanged(IXmppStream *AStream, bool AActive);
private:
	IPluginManager *FPluginManager;
	IStanzaProcessor *FStanzaProcessor;
	IXmppStreamManager *FXmppStreamManager;
private:
	QList<IRoster *> FRosters;
	QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERMANAGER_H
