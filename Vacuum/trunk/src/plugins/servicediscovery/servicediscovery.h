#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <QPair>
#include <QSet>
#include <QHash>
#include <QTimer>
#include <QMultiMap>
#include "../../definations/version.h"
#include "../../definations/namespaces.h"
#include "../../definations/initorders.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterdataholderorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/rosterclickhookerorders.h"
#include "../../definations/actiongroups.h"
#include "../../definations/tooltiporders.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/imainwindow.h"
#include "../../interfaces/itraymanager.h"
#include "../../interfaces/istatusicons.h"
#include "../../interfaces/isettings.h"
#include "../../utils/errorhandler.h"
#include "discoinfowindow.h"
#include "discoitemswindow.h"

struct QueuedRequest {
  Jid streamJid;
  Jid contactJid;
  QString node;
};

struct EntityCapabilities {
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
  public IIqStanzaOwner,
  public IDiscoHandler,
  public IRosterIndexDataHolder,
  public IRostersClickHooker
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IServiceDiscovery IStanzaHandler IIqStanzaOwner IDiscoHandler IRosterIndexDataHolder IRostersClickHooker);
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
  virtual bool startPlugin();
  //IStanzaHandler
  virtual bool editStanza(int AHandlerId, const Jid &AStreamJid, Stanza * AStanza, bool &AAccept);
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IDiscoHandler
  virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
  virtual void fillDiscoItems(IDiscoItems &/*ADiscoItems*/) {}
  //IRosterIndexDataHolder
  virtual int order() const { return RDHO_DEFAULT; }
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/) { return false; }
  //IRostersClickHooker
  virtual bool rosterIndexClicked(IRosterIndex *AIndex, int AOrder);
  //IServiceDiscovery
  virtual IPluginManager *pluginManager() const { return FPluginManager; }
  virtual IDiscoInfo selfDiscoInfo(const Jid &AStreamJid, const QString &ANode = "") const;
  virtual void showDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL);
  virtual void showDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode, QWidget *AParent = NULL);
  virtual bool checkDiscoFeature(const Jid &AContactJid, const QString &ANode, const QString &AFeature, bool ADefault = true);
  virtual QList<IDiscoInfo> findDiscoInfo(const IDiscoIdentity &AIdentity, const QStringList &AFeatures, const IDiscoItem &AParent) const;
  virtual QIcon discoInfoIcon(const IDiscoInfo &ADiscoInfo) const;
  virtual QIcon discoItemIcon(const IDiscoItem &ADiscoItem) const;
    //DiscoHandler
  virtual void insertDiscoHandler(IDiscoHandler *AHandler);
  virtual void removeDiscoHandler(IDiscoHandler *AHandler);
    //FeatureHandler
  virtual bool hasFeatureHandler(const QString &AFeature) const;
  virtual void insertFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler, int AOrder);
  virtual bool execFeatureHandler(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  virtual void removeFeatureHandler(const QString &AFeature, IDiscoFeatureHandler *AHandler);
    //DiscoFeatures
  virtual void insertDiscoFeature(const IDiscoFeature &AFeature);
  virtual QList<QString> discoFeatures() const;
  virtual IDiscoFeature discoFeature(const QString &AFeatureVar) const;
  virtual void removeDiscoFeature(const QString &AFeatureVar);
    //DiscoInfo
  virtual bool hasDiscoInfo(const Jid &AContactJid, const QString &ANode = "") const;
  virtual QList<Jid> discoInfoContacts() const;
  virtual QList<QString> dicoInfoContactNodes(const Jid &AContactJid) const;
  virtual IDiscoInfo discoInfo(const Jid &AContactJid, const QString &ANode = "") const;
  virtual bool requestDiscoInfo(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = "");
  virtual void removeDiscoInfo(const Jid &AContactJid, const QString &ANode = "");
    //DiscoItems
  virtual bool hasDiscoItems(const Jid &AContactJid, const QString &ANode = "") const;
  virtual QList<Jid> discoItemsContacts() const;
  virtual QList<QString> dicoItemsContactNodes(const Jid &AContactJid) const;
  virtual IDiscoItems discoItems(const Jid &AContactJid, const QString &ANode = "") const;
  virtual bool requestDiscoItems(const Jid &AStreamJid, const Jid &AContactJid, const QString &ANode = "");
  virtual void removeDiscoItems(const Jid &AContactJid, const QString &ANode ="");
signals:
  virtual void discoItemsWindowCreated(IDiscoItemsWindow *AWindow);
  virtual void discoItemsWindowDestroyed(IDiscoItemsWindow *AWindow);
  virtual void discoHandlerInserted(IDiscoHandler *AHandler);
  virtual void discoHandlerRemoved(IDiscoHandler *AHandler);
  virtual void featureHandlerInserted(const QString &AFeature, IDiscoFeatureHandler *AHandler);
  virtual void featureHandlerRemoved(const QString &AFeature, IDiscoFeatureHandler *AHandler);
  virtual void discoFeatureInserted(const IDiscoFeature &AFeature);
  virtual void discoFeatureRemoved(const IDiscoFeature &AFeature);
  virtual void discoInfoReceived(const IDiscoInfo &ADiscoInfo);
  virtual void discoInfoRemoved(const IDiscoInfo &ADiscoInfo);
  virtual void discoItemsReceived(const IDiscoItems &ADiscoItems);
  virtual void discoItemsRemoved(const IDiscoItems &ADiscoItems);
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAftert);
  //IRosterIndexDataHolder
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
  void discoInfoToElem(const IDiscoInfo &AInfo, QDomElement &AElem) const;
  void discoInfoFromElem(const QDomElement &AElem, IDiscoInfo &AInfo) const;
  IDiscoInfo parseDiscoInfo(const Stanza &AStanza, const QPair<Jid,QString> &AJidNode) const;
  IDiscoItems parseDiscoItems(const Stanza &AStanza, const QPair<Jid,QString> &AJidNode) const;
  void registerFeatures();
  void appendQueuedRequest(const QDateTime &ATimeStart, const QueuedRequest &ARequest);
  void removeQueuedRequest(const QueuedRequest &ARequest);
  bool hasEntityCaps(const EntityCapabilities &ACaps) const;
  QString capsFileName(const EntityCapabilities &ACaps, bool AForJid) const;
  IDiscoInfo loadEntityCaps(const EntityCapabilities &ACaps) const;
  bool saveEntityCaps(IDiscoInfo &AInfo) const;
  QString calcCapsHash(const IDiscoInfo &AInfo, const QString &AHash) const;
  bool compareIdentities(const QList<IDiscoIdentity> &AIdentities, const IDiscoIdentity &AWith) const;
  bool compareFeatures(const QStringList &AFeatures, const QStringList &AWith) const;
protected slots:
  void onStreamStateChanged(const Jid &AStreamJid, bool AStateOnline);
  void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  void onRosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem);
  void onStreamOpened(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  void onShowDiscoInfoByAction(bool);
  void onShowDiscoItemsByAction(bool);
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onDiscoInfoChanged(const IDiscoInfo &ADiscoInfo);
  void onDiscoInfoWindowDestroyed(QObject *AObject);
  void onDiscoItemsWindowDestroyed(IDiscoItemsWindow *AWindow);
  void onQueueTimerTimeout();
  void onCapsTimerTimeout();
private:
  IPluginManager *FPluginManager;
  IXmppStreams *FXmppStreams;
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IStanzaProcessor *FStanzaProcessor;
  IRostersView *FRostersView;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersModel *FRostersModel;
  ITrayManager *FTrayManager;
  IMainWindowPlugin *FMainWindowPlugin;
  IStatusIcons *FStatusIcons;
  ISettingsPlugin *FSettingsPlugin;
  IDataForms *FDataForms;
private:
  Menu *FDiscoMenu;
  QTimer FQueueTimer;
  QMultiMap<QDateTime,QueuedRequest> FQueuedRequests;
  QList<IDiscoHandler *> FDiscoHandlers;
  QHash<QString, QMultiMap<int, IDiscoFeatureHandler *> > FFeatureHandlers;
  QHash<Jid ,int> FSHIInfo;
  QHash<Jid ,int> FSHIItems;
  QHash<QString, QPair<Jid,QString> > FInfoRequestsId;
  QHash<QString, QPair<Jid,QString> > FItemsRequestsId;
  QHash<Jid,QHash<QString,IDiscoInfo> > FDiscoInfo;
  QHash<Jid,QHash<QString,IDiscoItems> > FDiscoItems;
  QHash<QString,IDiscoFeature> FDiscoFeatures;
  QHash<Jid, DiscoInfoWindow *> FDiscoInfoWindows;
  QList<DiscoItemsWindow *> FDiscoItemsWindows;
private:
  QTimer FCapsTimer;
  QHash<Jid ,int> FSHIPresenceIn;
  QHash<Jid ,int> FSHIPresenceOut;
  QHash<Jid, EntityCapabilities> FMyCaps;
  QHash<Jid, EntityCapabilities> FEntityCaps;
};

#endif // SERVICEDISCOVERY_H
