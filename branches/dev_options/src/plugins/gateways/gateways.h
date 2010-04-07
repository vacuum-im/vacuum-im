#ifndef GATEWAYS_H
#define GATEWAYS_H

#include <QSet>
#include <QTimer>
#include <definations/namespaces.h>
#include <definations/actiongroups.h>
#include <definations/toolbargroups.h>
#include <definations/rosterindextyperole.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/vcardvaluenames.h>
#include <definations/discoitemdataroles.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/igateways.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/irosterchanger.h>
#include <interfaces/irostersview.h>
#include <interfaces/ivcard.h>
#include <interfaces/iprivatestorage.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iregistraton.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>
#include <utils/action.h>
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
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
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
  virtual bool changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe);
  virtual QString sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid);
  virtual QString sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID);
  virtual QDialog *showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
signals:
  void promptReceived(const QString &AId, const QString &ADesc, const QString &APrompt);
  void userJidReceived(const QString &AId, const Jid &AUserJid);
  void errorReceived(const QString &AId, const QString &AError);
protected:
  void registerDiscoFeatures();
  void savePrivateStorageKeep(const Jid &AStreamJid);
  void savePrivateStorageSubscribe(const Jid &AStreamJid);
protected slots:
  void onAddLegacyUserActionTriggered(bool);
  void onLogActionTriggered(bool);
  void onResolveActionTriggered(bool);
  void onKeepActionTriggered(bool);
  void onChangeActionTriggered(bool);
  void onRosterIndexContextMenu(IRosterIndex *AIndex, Menu *AMenu);
  void onPresenceOpened(IPresence *APresence);
  void onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  void onPresenceClosed(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
  void onRosterOpened(IRoster *ARoster);
  void onRosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
  void onRosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  void onPrivateStorateOpened(const Jid &AStreamJid);
  void onPrivateStorageLoaded(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement);
  void onKeepTimerTimeout();
  void onVCardReceived(const Jid &AContactJid);
  void onVCardError(const Jid &AContactJid, const QString &AError);
  void onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow);
  void onDiscoItemContextMenu(const QModelIndex AIndex, Menu *AMenu);
  void onRegisterFields(const QString &AId, const IRegisterFields &AFields);
  void onRegisterError(const QString &AId, const QString &AError);
private:
  IServiceDiscovery *FDiscovery;
  IStanzaProcessor *FStanzaProcessor;
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IRosterChanger *FRosterChanger;
  IRostersViewPlugin *FRostersViewPlugin;
  IVCardPlugin *FVCardPlugin;
  IPrivateStorage *FPrivateStorage;
  IStatusIcons *FStatusIcons;
  IRegistration *FRegistration;
private:
  QTimer FKeepTimer;
private:
  QString FKeepRequest;
  QList<QString> FPromptRequests;
  QList<QString> FUserJidRequests;
  QMap<QString, Jid> FRegisterRequests;
  QMultiMap<Jid, Jid> FResolveNicks;
  QMultiMap<Jid, Jid> FSubscribeServices;
  QMultiMap<Jid, Jid> FKeepConnections;
  QMap<Jid, QSet<Jid> > FPrivateStorageKeep;
};

#endif // GATEWAYS_H
