#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <QSet>
#include "../../definations/version.h"
#include "../../definations/namespaces.h"
#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterdataholderorders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/multiuserdataroles.h"
#include "../../definations/tooltiporders.h"
#include "../../definations/optionnodes.h"
#include "../../definations/optionorders.h"
#include "../../definations/discofeatureorder.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/iclientinfo.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/iroster.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/imultiuserchat.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../utils/errorhandler.h"
#include "../../utils/stanza.h"
#include "../../utils/menu.h"
#include "../../utils/skin.h"
#include "clientinfodialog.h"
#include "optionswidget.h"

class ClientInfo : 
  public QObject,
  public IPlugin,
  public IClientInfo,
  public IStanzaHandler,
  public IIqStanzaOwner,
  public IOptionsHolder,
  public IRosterIndexDataHolder,
  public IDiscoFeatureHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IClientInfo IStanzaHandler IIqStanzaOwner IOptionsHolder IRosterIndexDataHolder IDiscoFeatureHandler);
public:
  ClientInfo();
  ~ClientInfo();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return CLIENTINFO_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool editStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza * /*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IIqStanzaOwner
  virtual void iqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void iqStanzaTimeOut(const QString &AId);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //IRosterIndexDataHolder
  virtual int order() const { return RDHO_DEFAULT; }
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/) { return false; }
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IClientInfo
  virtual void showClientInfo(const Jid &AStreamJid, const Jid &AContactJid, int AInfoTypes);
  virtual bool checkOption(IClientInfo::Option AOption) const;
  virtual void setOption(IClientInfo::Option AOption, bool AValue);
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
  //IRosterIndexDataHolder
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
  //IClientInfo
  virtual void optionChanged(IClientInfo::Option AOption, bool AValue);
  virtual void softwareInfoChanged(const Jid &AContactJid); 
  virtual void lastActivityChanged(const Jid &AContactJid);
  virtual void entityTimeChanged(const Jid &AContactJid);
protected:
  void deleteSoftwareDialogs(const Jid &AStreamJid);
  void registerDiscoFeatures();
protected slots:
  void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  void onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
  void onClientInfoActionTriggered(bool);
  void onClientInfoDialogClosed(const Jid &AContactJid);
  void onSettingsOpened();
  void onSettingsClosed();
  void onRosterRemoved(IRoster *ARoster);
  void onSoftwareInfoChanged(const Jid &AContactJid);
  void onLastActivityChanged(const Jid &AContactJid);
  void onEntityTimeChanged(const Jid &AContactJid);
  void onOptionsDialogAccepted();
  void onOptionsDialogRejected();
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IStanzaProcessor *FStanzaProcessor;
  IRostersViewPlugin *FRostersViewPlugin;
  IRostersModel *FRostersModel;
  ISettingsPlugin *FSettingsPlugin;
  IMultiUserChatPlugin *FMultiUserChatPlugin;
  IServiceDiscovery *FDiscovery;
private:
  struct SoftwareItem {
    SoftwareItem() { status = IClientInfo::SoftwareNotLoaded; }
    QString name;
    QString version;
    QString os;
    int status;
  };
  struct ActivityItem {
    QDateTime requestTime;
    QDateTime datetime;
    QString text;
  };
  struct TimeItem {
    TimeItem() { ping = -1; }
    int tzoSecs;
    int utcSecs;
    int ping;
  };
private:
  OptionsWidget *FOptionsWidget;
private:
  int FOptions;
  int FVersionHandler;
  int FTimeHandler;
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
