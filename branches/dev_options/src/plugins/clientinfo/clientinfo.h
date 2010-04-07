#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QSet>
#include <QPointer>
#include <definations/version.h>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/dataformtypes.h>
#include <definations/rosterindextyperole.h>
#include <definations/rosterlabelorders.h>
#include <definations/rostertooltiporders.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/iclientinfo.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irostersview.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/imainwindow.h>
#include <interfaces/iautostatus.h>
#include <interfaces/isettings.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/menu.h>
#include <utils/datetime.h>
#include <utils/widgetmanager.h>
#include "clientinfodialog.h"
#include "miscoptionswidget.h"

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
  public IOptionsHolder,
  public IStanzaHandler,
  public IStanzaRequestOwner,
  public IDataLocalizer,
  public IDiscoHandler,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IClientInfo IOptionsHolder IStanzaHandler IStanzaRequestOwner IDataLocalizer IDiscoHandler IDiscoFeatureHandler);
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
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IStanzaHandler
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IDataLocalizer
  virtual IDataFormLocale dataFormLocale(const QString &AFormType);
  //IDiscoHandler
  virtual void fillDiscoInfo(IDiscoInfo &ADiscoInfo);
  virtual void fillDiscoItems(IDiscoItems &ADiscoItems);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IClientInfo
  virtual QString osVersion() const;
  virtual bool shareOSVersion() const;
  virtual void setShareOSVersion(bool AShare);
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
  void softwareInfoChanged(const Jid &AContactJid); 
  void lastActivityChanged(const Jid &AContactJid);
  void entityTimeChanged(const Jid &AContactJid);
  void shareOsVersionChanged(bool AShare);
  //IOptionsHolder
  void optionsAccepted();
  void optionsRejected();
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
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  IPluginManager *FPluginManager;
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IStanzaProcessor *FStanzaProcessor;
  IRostersViewPlugin *FRostersViewPlugin;
  IServiceDiscovery *FDiscovery;
  IDataForms *FDataForms;
  IAutoStatus *FAutoStatus;
  ISettingsPlugin *FSettingsPlugin;
private:
  int FTimeHandle;
  int FVersionHandle;
  int FActivityHandler;
  bool FShareOSVersion;
  QMap<QString, Jid> FSoftwareId;
  QMap<Jid, SoftwareItem> FSoftwareItems;
  QMap<QString, Jid> FActivityId;
  QMap<Jid, ActivityItem> FActivityItems;
  QMap<QString, Jid> FTimeId;
  QMap<Jid, TimeItem> FTimeItems;
  QMap<Jid, ClientInfoDialog *> FClientInfoDialogs;
};

#endif // CLIENTINFO_H
