#ifndef CHATSTATES_H
#define CHATSTATES_H

#include <QMap>
#include <QTimer>
#include "../../definations/namespaces.h"
#include "../../definations/stanzahandlerpriority.h"
#include "../../definations/archivehandlerorders.h"
#include "../../definations/statusbargroups.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ichatstates.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/imessagewidgets.h"
#include "../../interfaces/isettings.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/imessagearchiver.h"
#include "statewidget.h"

struct ChatParams 
{
  ChatParams() { 
    userState = IChatStates::StateUnknown;
    selfState = IChatStates::StateUnknown;
    selfLastActive = 0;
  }
  int userState;
  int selfState;
  uint selfLastActive;
};

class ChatStates : 
  public QObject,
  public IPlugin,
  public IChatStates,
  public IStanzaHandler,
  public IArchiveHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IChatStates IStanzaHandler IArchiveHandler);
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
  //IStanzaHandler
  virtual bool editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &AAccept);
  virtual bool readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IChatStates
  virtual bool isEnabled() const;
  virtual void setEnabled(bool AEnabled);
  virtual bool isEnabled(const Jid &AContactJid) const;
  virtual int permitStatus(const Jid &AContactJid) const;
  virtual void setPermitStatus(const Jid AContactJid, int AStatus);
  virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual int userChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual int selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const;
signals:
  virtual void chatStatesEnabled(bool AEnabled) const;
  virtual void permitStatusChanged(const Jid &AContactJid, int AStatus) const;
  virtual void supportStatusChanged(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported) const;
  virtual void userChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
  virtual void selfChatStateChanged(const Jid &AStreamJid, const Jid &AContactJid, int AState) const;
protected:
  void setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported);
  void setUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState);
  void setSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend);
  void sendStateMessage(const Jid &AStreamJid, const Jid &AContactJid, int AState);
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
private:
  IPresencePlugin *FPresencePlugin;
  IMessageWidgets *FMessageWidgets;
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
  IServiceDiscovery *FDiscovery;
  IMessageArchiver *FMessageArchiver;
private:
  int FSHIMessagesIn;
  int FSHIMessagesOut;
private:
  bool FEnabled;
  QTimer FUpdateTimer;
  QMap<Jid, int> FPermitStatus;
  QMap<Jid, QList<Jid> > FNotSupported;
  QMap<Jid, QMap<Jid, ChatParams> > FChatParams;
  QMap<QTextEdit *, IChatWindow *> FChatByEditor;
};

#endif // CHATSTATES_H
