#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QSet>
#include <QPointer>
#include <definations/version.h>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/dataformtypes.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterdataholderorders.h>
#include <definations/rosterlabelorders.h>
#include <definations/rostertooltiporders.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iclientinfo.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/imainwindow.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/menu.h>
#include <utils/datetime.h>
#include "clientinfodialog.h"

struct SoftwareItem {
  SoftwareItem() { status = IClientInfo::SoftwareNotLoaded; }
  QString name;
  QString version;
  QString os;
  int status;
};
struct ActivityItem {
  QDateTime requestTime;
  QDateTime dateTime;
  QString text;
};
struct TimeItem {
  TimeItem() { ping = -1; delta = 0; zone = 0; }
  int ping;
  int delta;
  int zone;
};

class ClientInfo : 
  public QObject,
  public IPlugin,
  public IClientInfo,
  public IStanzaHandler,
  public IStanzaRequestOwner,
  public IRosterIndexDataHolder,
  public IDataLocalizer,
  public IDiscoHandler,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IClientInfo IStanzaHandler IStanzaRequestOwner IRosterIndexDataHolder IDataLocalizer IDiscoHandler IDiscoFeatureHandler);
public:
  ClientInfo();
  ~ClientInfo();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return CLIENTINFO_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool stanzaEdit(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza &/*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IRosterIndexDataHolder
  virtual int order() const { return RDHO_DEFAULT; }
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/) { return false; }
  //IDataLocalizer
  virtual IDataFormLocale dataFormLocale(const QString &AFormType);
  //IDiscoHandler
  virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
  virtual void fillDiscoItems(IDiscoItems &/*ADiscoItems*/) {};
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IClientInfo
  virtual QString osVersion() const;
  virtual void showClientInfo(const Jid &AStreamJid, const Jid &AContactJid, int AInfoTypes);
  //Software Version
  virtual bool hasSoftwareInfo(const Jid &AContactJid) const;
  virtual bool requestSoftwareInfo( const Jid &AStreamJid, const Jid &AContactJid);
  virtual int softwareStatus(const Jid &AContactJid) const;
  virtual QString softwareName(const Jid &AContactJid) const;
  virtual QString softwareVersion(const Jid &AContactJid) const;
  virtual QString softwareOs(const Jid &AContactJid) const;
  //Last Activity
  virtual bool hasLastActivity(const Jid &AContactJid) const;
  virtual bool requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QDateTime lastActivityTime(const Jid &AContactJid) const;
  virtual QString lastActivityText(const Jid &AContactJid) const;
  //Entity Time
  virtual bool hasEntityTime(const Jid &AContactJid) const;
  virtual bool requestEntityTime(const Jid &AStreamJid, const Jid &AContactJid);
  virtual QDateTime entityTime(const Jid &AContactJid) const;
  virtual int entityTimeDelta(const Jid &AContactJid) const;
  virtual int entityTimePing(const Jid &AContactJid) const;
signals:
  //IClientInfo
  virtual void softwareInfoChanged(const Jid &AContactJid); 
  virtual void lastActivityChanged(const Jid &AContactJid);
  virtual void entityTimeChanged(const Jid &AContactJid);
  //IRosterIndexDataHolder
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
protected:
  Action *createInfoAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFeature, QObject *AParent) const;
  void deleteSoftwareDialogs(const Jid &AStreamJid);
  void registerDiscoFeatures();
protected slots:
  void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  void onClientInfoActionTriggered(bool);
  void onClientInfoDialogClosed(const Jid &AContactJid);
  void onRosterRemoved(IRoster *ARoster);
  void onSoftwareInfoChanged(const Jid &AContactJid);
  void onLastActivityChanged(const Jid &AContactJid);
  void onEntityTimeChanged(const Jid &AContactJid);
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IStanzaProcessor *FStanzaProcessor;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersModel *FRostersModel;
  IServiceDiscovery *FDiscovery;
  IDataForms *FDataForms;
private:
  int FTimeHandle;
  int FVersionHandle;
  QHash<Jid, QSet<IPresence *> > FContactPresences;
  QHash<QString, Jid> FSoftwareId;
  QHash<Jid, SoftwareItem> FSoftwareItems;
  QHash<QString, Jid> FActivityId;
  QHash<Jid, ActivityItem> FActivityItems;
  QHash<QString, Jid> FTimeId;
  QHash<Jid, TimeItem> FTimeItems;
  QHash<Jid, ClientInfoDialog *> FClientInfoDialogs;
};

#endif // CLIENTINFO_H
