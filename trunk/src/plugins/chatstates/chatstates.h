#ifndef CHATSTATES_H
#define CHATSTATES_H

#include <QMap>
#include <QTimer>
#include <definations/namespaces.h>
#include <definations/stanzahandlerpriority.h>
#include <definations/archivehandlerorders.h>
#include <definations/statusbargroups.h>
#include <definations/optionnodes.h>
#include <definations/optionwidgetorders.h>
#include <definations/sessionnegotiatororders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ichatstates.h>
#include <interfaces/ipresence.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/isettings.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/idataforms.h>
#include <interfaces/isessionnegotiation.h>
#include "statewidget.h"
#include "stateoptionswidget.h"

struct ChatParams 
{
  ChatParams() { 
    userState = IChatStates::StateUnknown;
    selfState = IChatStates::StateUnknown;
    selfLastActive = 0;
    canSendStates = false;
  }
  int userState;
  int selfState;
  uint selfLastActive;
  bool canSendStates;
};

class ChatStates : 
  public QObject,
  public IPlugin,
  public IChatStates,
  public IStanzaHandler,
  public IArchiveHandler,
  public IOptionsHolder,
  public ISessionNegotiator
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IChatStates IStanzaHandler IArchiveHandler IOptionsHolder ISessionNegotiator);
public:
  ChatStates();
  ~ChatStates();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return CHATSTATES_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
  //IArchiveHandler
  virtual bool archiveMessage(int AOrder, const Jid &AStreamJid, Message &AMessage, bool ADirectionIn);
  //IOptionsHolder
  virtual QWidget *optionsWidget(const QString &ANode, int &AOrder);
  //ISessionNegotiator
  virtual int sessionInit(const IStanzaSession &ASession, IDataForm &ARequest);
  virtual int sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit);
  virtual int sessionApply(const IStanzaSession &ASession);
  virtual void sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm);
  //IStanzaHandler
  virtual bool stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IChatStates
  virtual bool isEnabled() const;
  virtual void setEnabled(bool AEnabled);
  virtual int permitStatus(const Jid &AContactJid) const;
  virtual void setPermitStatus(const Jid AContactJid, int AStatus);
  virtual bool isEnabled(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual int userChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual int selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
signals:
  virtual void chatStatesEnabled(bool AEnabled) const;
  virtual void permitStatusChanged(const Jid &AContactJid, int AStatus) const;
  virtual void supportStatusChanged(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported) const;
  virtual void userChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
  virtual void selfChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
  //IOptionsHolder
  virtual void optionsAccepted();
  virtual void optionsRejected();
protected:
  bool isSendingPossible(const Jid &AStreamJid, const Jid &AContactJid) const;
  void sendStateMessage(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
  void resetSupported(const Jid &AContactJid = Jid());
  void setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported);
  void setUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState);
  void setSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend = true);
  void registerDiscoFeatures();
protected slots:
  void onPresenceOpened(IPresence *APresence);
  void onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem);
  void onPresenceClosed(IPresence *APresence);
  void onChatWindowCreated(IChatWindow *AWindow);
  void onChatWindowActivated();
  void onChatWindowTextChanged();
  void onChatWindowClosed();
  void onChatWindowDestroyed(IChatWindow *AWindow);
  void onUpdateSelfStates();
  void onSettingsOpened();
  void onSettingsClosed();
  void onStanzaSessionTerminated(const IStanzaSession &ASession);
private:
  IPresencePlugin *FPresencePlugin;
  IMessageWidgets *FMessageWidgets;
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
  IServiceDiscovery *FDiscovery;
  IMessageArchiver *FMessageArchiver;
  IDataForms *FDataForms;
  ISessionNegotiation *FSessionNegotiation;
private:
  QMap<Jid,int> FSHIMessagesIn;
  QMap<Jid,int> FSHIMessagesOut;
private:
  bool FEnabled;
  QTimer FUpdateTimer;
  QMap<Jid, int> FPermitStatus;
  QMap<Jid, QList<Jid> > FNotSupported;
  QMap<Jid, QMap<Jid, ChatParams> > FChatParams;
  QMap<Jid, QMap<Jid, QString> > FStanzaSessions;
  QMap<QTextEdit *, IChatWindow *> FChatByEditor;
};

#endif // CHATSTATES_H
