#ifndef GATEWAYS_H
#define GATEWAYS_H

#include <QSet>
#include <QTimer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/irostermanager.h>
#include <interfaces/ipresencemanager.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/irostersview.h>
#include <interfaces/ivcardmanager.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iregistraton.h>
#include "addlegacycontactdialog.h"

class Gateways :
	public QObject,
	public IPlugin,
	public IGateways,
	public IStanzaRequestOwner,
	public IDiscoFeatureHandler
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IGateways IStanzaRequestOwner IDiscoFeatureHandler);
	Q_PLUGIN_METADATA(IID "org.jrudevels.vacuum.IGateways");
public:
	Gateways();
	~Gateways();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return GATEWAYS_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IDiscoFeatureHandler
	virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	//IGateways
	virtual void resolveNickName(const Jid &AStreamJid, const Jid &AContactJid);
	virtual void sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn);
	virtual QList<Jid> keepConnections(const Jid &AStreamJid) const;
	virtual void setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled);
	virtual QList<Jid> streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity = IDiscoIdentity()) const;
	virtual QList<Jid> serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const;
	virtual bool removeService(const Jid &AStreamJid, const Jid &AService, bool AWithContacts = true);
	virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe);
	virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid);
	virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID);
	virtual QDialog *showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
	void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt);
	void userJidReceived(const QString &AId, const Jid &AUserJid);
	void errorReceived(const QString &AId, const XmppError &AError);
protected:
	void registerDiscoFeatures();
	void savePrivateStorageKeep(const Jid &AStreamJid);
	void savePrivateStorageSubscribe(const Jid &AStreamJid);
	bool isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const;
protected slots:
	void onAddLegacyUserActionTriggered(bool);
	void onLogActionTriggered(bool);
	void onResolveActionTriggered(bool);
	void onKeepActionTriggered(bool);
	void onChangeActionTriggered(bool);
	void onRemoveActionTriggered(bool);
	void onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onPresenceOpened(IPresence *APresence);
	void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
	void onPresenceClosed(IPresence *APresence);
	void onPresenceActiveChanged(IPresence *APresence, bool AActive);
	void onRosterOpened(IRoster *ARoster);
	void onRosterSubscriptionReceived(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
	void onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
	void onPrivateStorateOpened(const Jid &AStreamJid);
	void onPrivateDataLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
	void onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace);
	void onKeepTimerTimeout();
	void onVCardReceived(const Jid &AContactJid);
	void onVCardError(const Jid &AContactJid, const XmppError &AError);
	void onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow);
	void onDiscoItemContextMenu(const QModelIndex &AIndex, Menu *AMenu);
	void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
	void onRegisterError(const QString &AId, const XmppError &AError);
private:
	IServiceDiscovery *FDiscovery;
	IStanzaProcessor *FStanzaProcessor;
	IRosterManager *FRosterManager;
	IPresenceManager *FPresenceManager;
	IRosterChanger *FRosterChanger;
	IRostersViewPlugin *FRostersViewPlugin;
	IVCardManager *FVCardManager;
	IPrivateStorage *FPrivateStorage;
	IStatusIcons *FStatusIcons;
	IRegistration *FRegistration;
private:
	QTimer FKeepTimer;
	QMultiMap<Jid, Jid> FKeepConnections;
	QMap<Jid, QSet<Jid> > FPrivateStorageKeep;
private:
	QList<QString> FPromptRequests;
	QList<QString> FUserJidRequests;
	QMultiMap<Jid, Jid> FResolveNicks;
	QMultiMap<Jid, Jid> FSubscribeServices;
	QMap<QString, Jid> FShowRegisterRequests;
};

#endif // GATEWAYS_H
