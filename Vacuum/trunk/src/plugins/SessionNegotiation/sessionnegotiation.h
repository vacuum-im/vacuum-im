#ifndef SESSIONNEGOTIATION_H
#define SESSIONNEGOTIATION_H

#include "../../definations/namespaces.h"
#include "../../definations/sessionnegotiatororder.h"
#include "../../definations/discofeatureorder.h"
#include "../../definations/rosterlabelorders.h"
#include "../../definations/notificationdataroles.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/isessionnegotiation.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/inotifications.h"

class SessionNegotiation : 
  public QObject,
  public IPlugin,
  public ISessionNegotiation,
  public IStanzaHandler,
  public IDiscoFeatureHandler,
  public ISessionNegotiator
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin ISessionNegotiation IStanzaHandler IDiscoFeatureHandler ISessionNegotiator);
public:
  SessionNegotiation();
  ~SessionNegotiation();
  virtual QObject *instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return SESSIONNEGOTIATION_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool editStanza(int /*AHandlerId*/, const Jid &/*AStreamJid*/, Stanza * /*AStanza*/, bool &/*AAccept*/) { return false; }
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //ISessionNegotiator
  virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest);
  virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit);
  virtual int sessionContinue(const IStanzaSession &ASession, const QString &AResource);
  virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm);
  //ISessionNegotiation
  virtual IStanzaSession currentSession(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual int initSession(const Jid &AStreamJid, const Jid &AContactJid);
  virtual bool isSuspenedSession(const QString &ASessionId) const;
  virtual void resumeSession(const Jid &AStreamJid, const Jid &AContactJid);
  virtual void terminateSession(const Jid &AStreamJid, const Jid &AContactJid);
  virtual void insertNegotiator(ISessionNegotiator *ANegotiator, int AOrder);
  virtual void removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder);
signals:
  virtual void sessionInited(const IStanzaSession &ASession);
  virtual void sessionAccepted(const IStanzaSession &ASession);
  virtual void sessionActivated(const IStanzaSession &ASession);
  virtual void sessionTerminated(const IStanzaSession &ASession);
  virtual void sessionFailed(const IStanzaSession &ASession);
protected:
  bool sendSessionData(const IStanzaSession &ASession, const IDataForm &AForm);
  bool sendSessionError(const IStanzaSession &ASession);
  bool sendSessionInit(const IStanzaSession &ASession, const IDataForm &AForm);
  bool sendSessionAccept(const IStanzaSession &ASession, const IDataForm &AForm);
  bool sendSessionRenegotiate(const IStanzaSession &ASession, const IDataForm &AForm);
  void processAccept(const IStanzaSession &ASession, const IDataForm &ARequest);
  void processRenegotiate(const IStanzaSession &ASession, const IDataForm &ARequest);
  void processContinue(const IStanzaSession &ASession, const IDataForm &ARequest);
  void processTerminate(const IStanzaSession &ASession, const IDataForm &ARequest);
  void showSessionDialog(const IStanzaSession &ASession, const IDataForm &ARequest);
  void closeSessionDialog(const IStanzaSession &ASession);
  void localizeSession(const IStanzaSession &ASession, IDataForm &AForm) const;
  void updateSession(const IStanzaSession &ASession);
  void removeSession(const IStanzaSession &ASession);
protected:
  void registerDiscoFeatures();
  void updateFields(const IDataForm &ASourse, IDataForm &ADestination, bool AInsert, bool ARemove) const;
  IDataForm defaultForm(const QString &AActionVar) const;
  QStringList unsubmitedFields(const IDataForm &ARequest, const IDataForm &ASubmit, bool ARequired) const;
  IStanzaSession dialogSession(IDataDialogWidget *ADialog) const;
protected slots:
  void onStreamOpened(IXmppStream *AXmppStream);
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem);
  void onStreamAboutToClose(IXmppStream *AXmppStream);
  void onStreamClosed(IXmppStream *AXmppStream);
  void onNotificationActivated(int ANotifyId);
  void onSessionDialogAccepted();
  void onSessionDialogDestroyed(IDataDialogWidget *ADialog);
  void onSessionActionTriggered(bool);
private:
  IDataForms *FDataForms;
  IStanzaProcessor *FStanzaProcessor;
  IServiceDiscovery *FDiscovery;
  IPresencePlugin *FPresencePlugin;
  INotifications *FNotifications;
private:
  QHash<Jid,int> FSHISession;
  QMultiMap<int,ISessionNegotiator *> FNegotiators;
  QHash<QString, IDataForm> FSuspended;
  QHash<QString, IDataForm> FRenegotiate;
  QHash<Jid, QHash<Jid, IStanzaSession> > FSessions;
  QHash<Jid, QHash<Jid, IDataDialogWidget *> > FDialogs;
  QHash<int, IDataDialogWidget *> FDialogByNotify;
};

#endif // SESSIONNEGOTIATION_H
