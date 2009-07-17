#include "chatstates.h"

#include <QSet>
#include <QDateTime>

#define SVN_ENABLED               "enabled"
#define SVN_PERMIT_STATUS         "permitStatus[]"

#define SHC_CONTENT_MESSAGES      "/message[@type='chat']/body"
#define SHC_STATE_MESSAGES        "/message[@type='chat']/[@xmlns='" NS_CHATSTATES "']"

#define STATE_ACTIVE              "active"
#define STATE_COMPOSING           "composing"
#define STATE_PAUSED              "paused"
#define STATE_INACTIVE            "inactive"
#define STATE_GONE                "gone"

#define PAUSED_TIMEOUT            30
#define INACTIVE_TIMEOUT          2*60
#define GONE_TIMEOUT              10*60

ChatStates::ChatStates()
{
  FPresencePlugin = NULL;
  FMessageWidgets = NULL;
  FStanzaProcessor = NULL;
  FSettingsPlugin = NULL;
  FDiscovery = NULL;
  FMessageArchiver = NULL;

  FEnabled = true;
  FUpdateTimer.setSingleShot(false);
  FUpdateTimer.setInterval(5000);
  connect(&FUpdateTimer,SIGNAL(timeout()),SLOT(onUpdateSelfStates()));
}

ChatStates::~ChatStates()
{

}

void ChatStates::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->uid = CHATSTATES_UUID;
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->name = tr("Chat State Notifications"); 
  APluginInfo->description = tr("Communicating the status of a user in a chat session");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(PRESENCE_UUID);
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ChatStates::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IMessageWidgets").value(0);
  if (plugin)
  {
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
    if (FMessageWidgets)
    {
      connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
      connect(FMessageWidgets->instance(),SIGNAL(chatWindowDestroyed(IChatWindow *)),SLOT(onChatWindowDestroyed(IChatWindow *)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0);
  if (plugin)
  {
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceClosed(IPresence *)),SLOT(onPresenceClosed(IPresence *)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMessageArchiver").value(0);
  if (plugin)
  {
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
  }

  return FMessageWidgets!=NULL && FStanzaProcessor!=NULL;
}

bool ChatStates::initObjects()
{
  if (FDiscovery)
    registerDiscoFeatures();

  //if (FMessageArchiver)
  //  FMessageArchiver->insertArchiveHandler(this,AHO_DEFAULT);

  if (FSettingsPlugin)
    FSettingsPlugin->insertOptionsHolder(this);

  if (FStanzaProcessor)
  {
    FSHIMessagesIn = FStanzaProcessor->insertHandler(this,SHC_STATE_MESSAGES,IStanzaProcessor::DirectionIn,SHP_CHATSTATES);
    FStanzaProcessor->appendCondition(FSHIMessagesIn,SHC_CONTENT_MESSAGES);

    FSHIMessagesOut = FStanzaProcessor->insertHandler(this,SHC_CONTENT_MESSAGES,IStanzaProcessor::DirectionOut,SHP_CHATSTATES);
  }

  return true;
}

bool ChatStates::startPlugin()
{
  FUpdateTimer.start();
  return true;
}

bool ChatStates::archiveMessage(int /*AOrder*/, const Jid &/*AStreamJid*/, Message &AMessage, bool /*ADirectionIn*/)
{
  QDomElement elem = AMessage.stanza().firstElement(QString::null,NS_CHATSTATES);
  if (!elem.isNull())
    elem.parentNode().removeChild(elem);
  return true;
}

QWidget *ChatStates::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MESSAGES)
  {
    AOrder = OWO_MESSAGES_CHATSTATES;
    StateOptionsWidget *widget = new StateOptionsWidget(this);
    connect(widget,SIGNAL(optionsApplied()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

bool ChatStates::editStanza(int AHandlerId, const Jid &AStreamJid, Stanza *AStanza, bool &/*AAccept*/)
{
  if (AHandlerId==FSHIMessagesOut && FChatParams.contains(AStreamJid))
  {
    bool stateSent = false;
    Jid contactJid = AStanza->to();
    if (isEnabled(contactJid) && isSupported(AStreamJid,contactJid))
    {
      IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,contactJid) : NULL;
      if (window)
      {
        stateSent = true;
        AStanza->addElement(STATE_ACTIVE,NS_CHATSTATES);
      }
    }
    FChatParams[AStreamJid][contactJid].canSendStates = stateSent;
    setSelfState(AStreamJid,contactJid,IChatStates::StateActive,false);
  }
  return false;
}

bool ChatStates::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId==FSHIMessagesIn && FChatParams.contains(AStreamJid))
  {
    Jid contactJid = AStanza.from();
    bool hasBody = !AStanza.firstElement("body").isNull();
    QDomElement elem = AStanza.firstElement(QString::null,NS_CHATSTATES);
    if (!elem.isNull())
    {
      if (hasBody || FChatParams.value(AStreamJid).value(contactJid).canSendStates==true)
      {
        AAccept = true;
        setSupported(AStreamJid,contactJid,true);
        FChatParams[AStreamJid][contactJid].canSendStates = true;
        
        int state = IChatStates::StateUnknown;
        if (elem.tagName() == STATE_ACTIVE)
          state = IChatStates::StateActive;
        else if (elem.tagName() == STATE_COMPOSING)
          state = IChatStates::StateComposing;
        else if (elem.tagName() == STATE_PAUSED)
          state = IChatStates::StatePaused;
        else if (elem.tagName() == STATE_INACTIVE)
          state = IChatStates::StateInactive;
        else if (elem.tagName() == STATE_GONE)
          state = IChatStates::StateGone;
        setUserState(AStreamJid,contactJid,state);
      }
    }
    else if (hasBody)
    {
      if (userChatState(AStreamJid,contactJid) != IChatStates::StateUnknown)
        setUserState(AStreamJid,contactJid,IChatStates::StateUnknown);
      setSupported(AStreamJid,contactJid,false);
    }
    return !hasBody;
  }
  return false;
}

bool ChatStates::isEnabled() const
{
  return FEnabled;
}

void ChatStates::setEnabled(bool AEnabled)
{
  if (FEnabled != AEnabled)
  {
    if (AEnabled)
      resetSupported();
    FEnabled = AEnabled;
    emit chatStatesEnabled(AEnabled);
  }
}

bool ChatStates::isEnabled(const Jid &AContactJid) const
{
  int status = permitStatus(AContactJid);
  return (isEnabled() || status==IChatStates::StatusEnable) && status!=IChatStates::StatusDisable;
}

int ChatStates::permitStatus(const Jid &AContactJid) const
{
  return FPermitStatus.value(AContactJid.bare(),IChatStates::StatusDefault);
}

void ChatStates::setPermitStatus(const Jid AContactJid, int AStatus)
{
  Jid bareJid = AContactJid.bare();
  if (FPermitStatus.value(bareJid,IChatStates::StatusDefault) != AStatus)
  {
    bool enabled = isEnabled(AContactJid);

    if (AStatus==IChatStates::StatusDisable)
    {
      FPermitStatus.insert(bareJid,AStatus);
    }
    else if (AStatus==IChatStates::StatusEnable)
    {
      FPermitStatus.insert(bareJid,AStatus);
    }
    else
    {
      FPermitStatus.remove(bareJid);
    }

    if (!enabled && isEnabled(AContactJid))
      resetSupported(AContactJid);

    emit permitStatusChanged(bareJid,AStatus);
  }
}

bool ChatStates::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
  bool supported = !FNotSupported.value(AStreamJid).contains(AContactJid);
  if (FDiscovery && supported && userChatState(AStreamJid,AContactJid)==IChatStates::StateUnknown)
    supported = !FDiscovery->hasDiscoInfo(AContactJid) || FDiscovery->discoInfo(AContactJid).features.contains(NS_CHATSTATES);
  return supported;
}

int ChatStates::userChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return FChatParams.value(AStreamJid).value(AContactJid).userState;
}

int ChatStates::selfChatState(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return FChatParams.value(AStreamJid).value(AContactJid).selfState;
}

bool ChatStates::isSendingPossible(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return isEnabled(AContactJid) && isSupported(AStreamJid,AContactJid) && FChatParams.value(AStreamJid).value(AContactJid).canSendStates;
}

void ChatStates::sendStateMessage(const Jid &AStreamJid, const Jid &AContactJid, int AState) const
{
  if (FStanzaProcessor && isSendingPossible(AStreamJid,AContactJid))
  {
    QString state;
    if (AState == IChatStates::StateActive)
      state = STATE_ACTIVE;
    else if (AState == IChatStates::StateComposing)
      state = STATE_COMPOSING;
    else if (AState == IChatStates::StatePaused)
      state = STATE_PAUSED;
    else if (AState == IChatStates::StateInactive)
      state = STATE_INACTIVE;
    else if (AState == IChatStates::StateGone)
      state = STATE_GONE;

    if (!state.isEmpty())
    {
      Stanza stanza("message");
      stanza.setType("chat").setTo(AContactJid.eFull());
      stanza.addElement(state,NS_CHATSTATES);
      FStanzaProcessor->sendStanzaOut(AStreamJid,stanza);
    }
  }
}

void ChatStates::resetSupported(const Jid &AContactJid)
{
  foreach (Jid streamJid, FNotSupported.keys())
    foreach (Jid contactJid, FNotSupported.value(streamJid))
      if (AContactJid.isEmpty() || (AContactJid && contactJid))
        setSupported(streamJid,contactJid,true);
}

void ChatStates::setSupported(const Jid &AStreamJid, const Jid &AContactJid, bool ASupported)
{
  if (FNotSupported.contains(AStreamJid))
  {
    QList<Jid> &notSuppotred = FNotSupported[AStreamJid];
    int index = notSuppotred.indexOf(AContactJid);
    if (ASupported != (index<0))
    {
      if (ASupported)
      {
        notSuppotred.removeAt(index);
      }
      else
      {
        notSuppotred.append(AContactJid);
      }
      emit supportStatusChanged(AStreamJid,AContactJid,ASupported);
    }
  }
}

void ChatStates::setUserState(const Jid &AStreamJid, const Jid &AContactJid, int AState)
{
  if (FChatParams.contains(AStreamJid))
  {
    ChatParams &params = FChatParams[AStreamJid][AContactJid];
    if (params.userState != AState)
    {
      params.userState = AState;
      emit userChatStateChanged(AStreamJid,AContactJid,AState);
    }
  }
}

void ChatStates::setSelfState(const Jid &AStreamJid, const Jid &AContactJid, int AState, bool ASend)
{
  if (FChatParams.contains(AStreamJid))
  {
    ChatParams &params = FChatParams[AStreamJid][AContactJid];
    params.selfLastActive = QDateTime::currentDateTime().toTime_t();
    if (params.selfState != AState)
    {
      params.selfState = AState;
      if (ASend)
        sendStateMessage(AStreamJid,AContactJid,AState);
      emit selfChatStateChanged(AStreamJid,AContactJid,AState);
    }
  }
}

void ChatStates::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.var = NS_CHATSTATES;
  dfeature.active = true;
  dfeature.name = tr("Chat State Notifications");
  dfeature.description = tr("Communicating the status of a user in a chat session");
  FDiscovery->insertDiscoFeature(dfeature);
}

void ChatStates::onPresenceOpened(IPresence *APresence)
{
  FNotSupported[APresence->streamJid()].clear();
  FChatParams[APresence->streamJid()].clear();
}

void ChatStates::onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem)
{
  if (AItem.show==IPresence::Offline || AItem.show==IPresence::Error)
  {
    if (userChatState(APresence->streamJid(),AItem.itemJid) != IChatStates::StateUnknown)
      setUserState(APresence->streamJid(),AItem.itemJid,IChatStates::StateGone);
  }
}

void ChatStates::onPresenceClosed(IPresence *APresence)
{
  foreach(Jid contactJid, FChatParams.value(APresence->streamJid()).keys())
  {
    setUserState(APresence->streamJid(),contactJid,IChatStates::StateUnknown);
    setSelfState(APresence->streamJid(),contactJid,IChatStates::StateUnknown,false);
  }
  FNotSupported.remove(APresence->streamJid());
  FChatParams.remove(APresence->streamJid());
}

void ChatStates::onChatWindowCreated(IChatWindow *AWindow)
{
  StateWidget *widget = new StateWidget(this,AWindow);
  AWindow->statusBarWidget()->statusBarChanger()->insertWidget(widget,SBG_CW_CHATSTATES,true);

  FChatByEditor.insert(AWindow->editWidget()->textEdit(),AWindow);
  connect(AWindow->instance(),SIGNAL(windowActivated()),SLOT(onChatWindowActivated()));
  connect(AWindow->editWidget()->textEdit(),SIGNAL(textChanged()),SLOT(onChatWindowTextChanged()));
  connect(AWindow->instance(),SIGNAL(windowClosed()),SLOT(onChatWindowClosed()));
}

void ChatStates::onChatWindowActivated()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    int state = selfChatState(window->streamJid(),window->contactJid());
    if (state==IChatStates::StateUnknown || state==IChatStates::StateInactive || state==IChatStates::StateGone)
      setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
  }
}

void ChatStates::onChatWindowTextChanged()
{
  QTextEdit *editor = qobject_cast<QTextEdit *>(sender());
  IChatWindow *window = FChatByEditor.value(editor,NULL);
  if (editor && window)
  {
    if (!editor->document()->isEmpty())
      setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateComposing);
    else
      setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
  }
}

void ChatStates::onChatWindowClosed()
{
  IChatWindow *window = qobject_cast<IChatWindow *>(sender());
  if (window)
  {
    int state = selfChatState(window->streamJid(),window->contactJid());
    if (state != IChatStates::StateGone)
      setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
  }
}

void ChatStates::onChatWindowDestroyed(IChatWindow *AWindow)
{
  setSelfState(AWindow->streamJid(),AWindow->contactJid(),IChatStates::StateGone);
  FChatByEditor.remove(AWindow->editWidget()->textEdit());
}

void ChatStates::onUpdateSelfStates()
{
  QList<IChatWindow *> windows = FMessageWidgets!=NULL ? FMessageWidgets->chatWindows() : QList<IChatWindow *>();
  foreach (IChatWindow *window, windows)
  {
    if (FChatParams.value(window->streamJid()).contains(window->contactJid()))
    {
      ChatParams &params = FChatParams[window->streamJid()][window->contactJid()];
      uint timePassed = QDateTime::currentDateTime().toTime_t() - params.selfLastActive;
      if (params.selfState==IChatStates::StateActive && window->isActive())
      {
        setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateActive);
      }
      else if (params.selfState==IChatStates::StateComposing && timePassed>PAUSED_TIMEOUT)
      {
        setSelfState(window->streamJid(),window->contactJid(),IChatStates::StatePaused);
      }
      else if ((params.selfState==IChatStates::StateActive || params.selfState==IChatStates::StatePaused) && timePassed>INACTIVE_TIMEOUT)
      {
        setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateInactive);
      }
      else if (params.selfState==IChatStates::StateInactive && timePassed>GONE_TIMEOUT)
      {
        setSelfState(window->streamJid(),window->contactJid(),IChatStates::StateGone);
      }
    }
  }
}

void ChatStates::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
  if (settings)
  {
    FEnabled = settings->value(SVN_ENABLED,true).toBool();

    FPermitStatus.clear();
    QHash<QString,QVariant> permits = settings->values(SVN_PERMIT_STATUS);
    QHash<QString,QVariant>::const_iterator it = permits.constBegin();
    while (it != permits.constEnd())
    {
      int status = it.value().toInt();
      if (status==IChatStates::StatusEnable || status==IChatStates::StatusDisable)
        FPermitStatus.insert(it.key(),status);
      it++;
    }
  }
}

void ChatStates::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(pluginUuid());
  if (settings)
  {
    settings->setValue(SVN_ENABLED,FEnabled);

    QSet<QString> oldPermits = settings->values(SVN_PERMIT_STATUS).keys().toSet();
    QMap<Jid,int>::const_iterator it = FPermitStatus.constBegin();
    while (it != FPermitStatus.constEnd())
    {
      oldPermits -= it.key().pBare();
      settings->setValueNS(SVN_PERMIT_STATUS,it.key().pBare(),it.value());
      it++;
    }

    foreach(QString ns, oldPermits)
      settings->deleteValueNS(SVN_PERMIT_STATUS,ns);
  }
}

Q_EXPORT_PLUGIN2(ChatStatesPlugin, ChatStates)
