#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <QSet>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QMultiMap>
#include <definitions/version.h>
#include <definitions/namespaces.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterclickhookerorders.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/serviceicons.h>
#include <definitions/shortcuts.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/xmppurihandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
#include <interfaces/istatusicons.h>
#include <interfaces/ixmppuriqueries.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/shortcuts.h>
#include "discoinfowindow.h"
#include "discoitemswindow.h"

struct DiscoveryRequest {
	Jid streamJid;
	Jid contactJid;
	QString node;
	bool operator==(const DiscoveryRequest &AOther) const {
		return streamJid==AOther.streamJid && contactJid==AOther.contactJid && node==AOther.node;
	}
};

struct EntityCapabilities {
	Jid streamJid;
	Jid entityJid;
	QString node;
	QString ver;
	QString hash;
};

class ServiceDiscovery :
			public QObject,
			public IPlugin,
			public IServiceDiscovery,
			public IStanzaHandler,
			public IStanzaRequestOwner,
			public IXmppUriHandler,
			public IDiscoHandler,
			public IRostersClickHooker
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IServiceDiscovery IStanzaHandler IStanzaRequestOwner IXmppUriHandler IDiscoHandler IRostersClickHooker);
public:
	ServiceDiscovery();
	~ServiceDiscovery();
	virtual QObject *instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return SERVICEDISCOVERY_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin() { return true; }
	//IStanzaHandler
	virtual bool stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IDiscoHandler
	virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
	virtual void fillDiscoItems(IDiscoItems &ADiscoItems);
	//IRostersClickHooker
	virtual bool rosterIndexSingleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	virtual bool rosterIndexDoubleClicked(int AOrder, IRosterIndex *AIndex, const QMouseEvent *AEvent);
	//IServiceDiscovery
	virtual IPluginManager *pluginManager() const { return FPluginManager; }
	virtual IDiscoInfo selfDiscoInfo(const Jid &AStreamJid, const QString &ANode = QString::null) const;
	virtual void showDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL);
	virtual void showDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL);
	virtual bool checkDiscoFeature(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault = true);
	virtual QList<IDiscoInfo> findDiscoInfo(const Jid &AStreamJid, const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const;
	virtual QIcon identityIcon(const QList<IDiscoIdentity> &AIdentity) const;
	virtual QIcon serviceIcon(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANode) const;
	virtual void updateSelfEntityCapabilities();
	//DiscoHandler
	virtual void insertDiscoHandler(IDiscoHandler *AHandler);
	virtual void removeDiscoHandler(IDiscoHandler *AHandler);
	//FeatureHandler
	virtual bool hasFeatureHandler(const QString &AFeature) const;
	virtual void insertFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler, int AOrder);
	virtual bool execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
	virtual QList<Action *> createFeatureActions(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
	virtual void removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler);
	//DiscoFeatures
	virtual void insertDiscoFeature(const IDiscoFeature &AFeature);
	virtual QList<QString> discoFeatures() const;
	virtual IDiscoFeature discoFeature(const QString &AFeatureVar) const;
	virtual void removeDiscoFeature(const QString &AFeatureVar);
	//DiscoInfo
	virtual bool hasDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) const;
	virtual IDiscoInfo discoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null) const;
	virtual bool requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null);
	virtual void removeDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null);
	virtual int findIdentity(const QList<IDiscoIdentity> &AIdentity, const QString &ACategory, const QString &AType) const;
	//DiscoItems
	virtual bool requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = QString::null);
signals:
	void discoItemsWindowCreated(IDiscoItemsWindow *AWindow);
	void discoItemsWindowDestroyed(IDiscoItemsWindow *AWindow);
	void discoHandlerInserted(IDiscoHandler *AHandler);
	void discoHandlerRemoved(IDiscoHandler *AHandler);
	void featureHandlerInserted(const QString &AFeature, IDiscoFeatureHandler *AHandler);
	void featureHandlerRemoved(const QString &AFeature, IDiscoFeatureHandler *AHandler);
	void discoFeatureInserted(const IDiscoFeature &AFeature);
	void discoFeatureRemoved(const IDiscoFeature &AFeature);
	void discoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void discoInfoRemoved(const IDiscoInfo &ADiscoInfo);
	void discoItemsReceived(const IDiscoItems &ADiscoItems);
protected:
	void discoInfoToElem(const IDiscoInfo &AInfo, QDomElement &AElem) const;
	void discoInfoFromElem(const QDomElement &AElem, IDiscoInfo &AInfo) const;
	IDiscoInfo parseDiscoInfo(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const;
	IDiscoItems parseDiscoItems(const Stanza &AStanza, const DiscoveryRequest &ADiscoRequest) const;
	void registerFeatures();
	void appendQueuedRequest(const QDateTime &ATimeStart, const DiscoveryRequest &ARequest);
	void removeQueuedRequest(const DiscoveryRequest &ARequest);
	bool hasEntityCaps(const EntityCapabilities &ACaps) const;
	QString capsFileName(const EntityCapabilities &ACaps, bool AForJid) const;
	IDiscoInfo loadEntityCaps(const EntityCapabilities &ACaps) const;
	bool saveEntityCaps(const IDiscoInfo &AInfo) const;
	QString calcCapsHash(const IDiscoInfo &AInfo, const QString &AHash) const;
	bool compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const;
	bool compareFeatures(const QStringList &AFeatures, const QStringList &AWith) const;
	void insertStreamMenu(const Jid &AStreamJid);
	void removeStreamMenu(const Jid &AStreamJid);
	Action *createDiscoInfoAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QObject *AParent) const;
	Action *createDiscoItemsAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QObject *AParent) const;
protected slots:
	void onStreamOpened(IXmppStream *AXmppStream);
	void onStreamClosed(IXmppStream *AXmppStream);
	void onPresenceItemReceived(IPresence *APresence, const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onMultiUserPresence(IMultiUser *AUser, int AShow, const QString &AStatus);
	void onMultiUserChatCreated(IMultiUserChat *AMultiChat);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMultiMap<int,QString> &AToolTips);
	void onShowDiscoInfoByAction(bool);
	void onShowDiscoItemsByAction(bool);
	void onDiscoInfoWindowDestroyed(QObject *AObject);
	void onDiscoItemsWindowDestroyed(IDiscoItemsWindow *AWindow);
	void onQueueTimerTimeout();
	void onSelfCapsChanged();
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IRosterPlugin *FRosterPlugin;
	IPresencePlugin *FPresencePlugin;
	IStanzaProcessor *FStanzaProcessor;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	ITrayManager *FTrayManager;
	IMainWindowPlugin *FMainWindowPlugin;
	IStatusIcons *FStatusIcons;
	IDataForms *FDataForms;
	IXmppUriQueries *FXmppUriQueries;
private:
	QTimer FQueueTimer;
	QMap<Jid ,int> FSHIInfo;
	QMap<Jid ,int> FSHIItems;
	QMap<Jid, int> FSHIPresenceIn;
	QMap<Jid, int> FSHIPresenceOut;
	QMap<QString, DiscoveryRequest > FInfoRequestsId;
	QMap<QString, DiscoveryRequest > FItemsRequestsId;
	QMultiMap<QDateTime, DiscoveryRequest> FQueuedRequests;
private:
	bool FUpdateSelfCapsStarted;
	QMap<Jid, EntityCapabilities> FSelfCaps;
	QMap<Jid, QHash<Jid, EntityCapabilities> > FEntityCaps;
	QMap<Jid, QHash<Jid, QMap<QString, IDiscoInfo> > > FDiscoInfo;
private:
	Menu *FDiscoMenu;
	QList<IDiscoHandler *> FDiscoHandlers;
	QMap<QString, IDiscoFeature> FDiscoFeatures;
	QList<DiscoItemsWindow *> FDiscoItemsWindows;
	QMap<Jid, DiscoInfoWindow *> FDiscoInfoWindows;
	QMap<QString, QMultiMap<int, IDiscoFeatureHandler *> > FFeatureHandlers;
};

#endif // SERVICEDISCOVERY_H
