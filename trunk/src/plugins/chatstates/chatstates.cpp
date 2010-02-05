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

#define SFV_MAY_SEND              "may"
#define SFV_MUSTNOT_SEND          "mustnot"

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
  FDataForms = NULL;
  FSessionNegotiation = NULL;

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
  APluginInfo->name = tr("Chat State Notifications");
  APluginInfo->description = tr("Allows to share information about the user's activity in the chat");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
  APluginInfo->dependences.append(PRESENCE_UUID);
  APluginInfo->dependences.append(MESSAGEWIDGETS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ChatStates::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IMessageWidgets").value(0);
  if (plugin)
  {
    FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
    if (FMessageWidgets)
    {
      connect(FMessageWidgets->instance(),SIGNAL(chatWindowCreated(IChatWindow *)),SLOT(onChatWindowCreated(IChatWindow *)));
      connect(FMessageWidgets->instance(),SIGNAL(chatWindowDestroyed(IChatWindow *)),SLOT(onChatWindowDestroyed(IChatWindow *)));
    }
  }

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0);
  if (plugin)
  {
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IPresencePlugin").value(0);
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

  plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IMessageArchiver").value(0);
  if (plugin)
  {
    FMessageArchiver = qobject_cast<IMessageArchiver *>(plugin->instance());
  }

  plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->pluginInterface("ISessionNegotiation").value(0,NULL);
  if (plugin)
  {
    FSessionNegotiation = qobject_cast<ISessionNegotiation *>(plugin->instance());
    if (FSessionNegotiation && FDataForms)
    {
      connect(FSessionNegotiation->instance(),SIGNAL(sessionTerminated(const IStanzaSession &)),
        SLOT(onStanzaSessionTerminated(const IStanzaSession &)));
    }
  }

  return FPresencePlugin!=NULL && FMessageWidgets!=NULL && FStanzaProcessor!=NULL;
}

bool ChatStates::initObjects()
{
  if (FDiscovery)
    registerDiscoFeatures();

  if (FMessageArchiver)
    FMessageArchiver->insertArchiveHandler(this,AHO_DEFAULT);

  if (FSettingsPlugin)
    FSettingsPlugin->insertOptionsHolder(this);

  if (FSessionNegotiation && FDataForms)
    FSessionNegotiation->insertNegotiator(this,SNO_DEFAULT);

  return true;
}

bool ChatStates::startPlugin()
{
  FUpdateTimer.start();
  return true;
}

bool ChatStates::archiveMessage(int /*AOrder*/, const Jid &/*AStreamJid*/, Message &AMessage, bool /*ADirectionIn*/)
{
  if (!AMessage.stanza().firstElement(QString::null,NS_CHATSTATES).isNull())
  {
    AMessage.detach();
    QDomElement elem = AMessage.stanza().firstElement(QString::null,NS_CHATSTATES);
    elem.parentNode().removeChild(elem);
  }
  return true;
}

QWidget *ChatStates::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_MESSAGES)
  {
    AOrder = OWO_MESSAGES_CHATSTATES;
    StateOptionsWidget *widget = new StateOptionsWidget(this);
    connect(widget,SIGNAL(optionsAccepted()),SIGNAL(optionsAccepted()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),widget,SLOT(apply()));
    connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SIGNAL(optionsRejected()));
    return widget;
  }
  return NULL;
}

int ChatStates::sessionInit(const IStanzaSession &ASession, IDataForm &ARequest)
{
  IDataField chatstates;
  chatstates.var = NS_CHATSTATES;
  chatstates.type = DATAFIELD_TYPE_LISTSINGLE;
  chatstates.required = false;

  bool enabled = isEnabled(Jid(),ASession.contactJid);
  if (enabled)
  {
    IDataOption maysend;
    maysend.value = SFV_MAY_SEND;
    chatstates.options.append(maysend);
  }
  if (permitStatus(ASession.contactJid) != IChatStates::StatusEnable)
  {
    IDataOption mustnotsend;
    mustnotsend.value = SFV_MUSTNOT_SEND;
    chatstates.options.append(mustnotsend);
  }
  chatstates.value = enabled ? SFV_MAY_SEND : SFV_MUSTNOT_SEND;
  
  if (ASession.status == IStanzaSession::Init)
  {
    ARequest.fields.append(chatstates);
    return ISessionNegotiator::Auto;
  }
  else if (ASession.status == IStanzaSession::Renegotiate)
  {
    int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ASession.form.fields) : -1;
    if (index<0 || ASession.form.fields.at(index).value!=chatstates.value)
    {
      ARequest.fields.append(chatstates);
      return ISessionNegotiator::Auto;
    }
  }
  return ISessionNegotiator::Skip;
}

int ChatStates::sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit)
{
  int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ARequest.fields) : -1;
  if (index>=0)
  {
    int result = ISessionNegotiator::Auto;
    if (ARequest.type == DATAFORM_TYPE_FORM)
    {
      IDataField chatstates;
      chatstates.var = NS_CHATSTATES;
      chatstates.type = DATAFIELD_TYPE_LISTSINGLE;
      chatstates.value = ARequest.fields.at(index).value;
      chatstates.required = false;

      QStringList options;
      for (int i=0; i<ARequest.fields.at(index).options.count(); i++)
        options.append(ARequest.fields.at(index).options.at(i).value);

      int status = permitStatus(ASession.contactJid);
      bool enabled = isEnabled(Jid(),ASession.contactJid);
      if ((!enabled && !options.contains(SFV_MUSTNOT_SEND)) || (status==IChatStates::StatusEnable && !options.contains(SFV_MAY_SEND)))
      {
        ASubmit.pages[0].fieldrefs.append(NS_CHATSTATES);
        ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
        result = ISessionNegotiator::Manual;
      }
      ASubmit.fields.append(chatstates);
    }
    else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
    {
      QString value = ARequest.fields.at(index).value.toString();
      int status = permitStatus(ASession.contactJid);
      bool enabled = isEnabled(Jid(),ASession.contactJid);
      if ((!enabled && value==SFV_MAY_SEND) || (status==IChatStates::StatusEnable && value==SFV_MUSTNOT_SEND))
      {
        ASubmit.pages[0].fieldrefs.append(NS_CHATSTATES);
        ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
        result = ISessionNegotiator::Manual;
      }
    }
    return result;
  }
  return ISessionNegotiator::Skip;
}

int ChatStates::sessionApply(const IStanzaSession &ASession)
{
  int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,ASession.form.fields) : -1;
  if (index >= 0)
  {
    QString value = ASession.form.fields.at(index).value.toString();
    FStanzaSessions[ASession.streamJid].insert(ASession.contactJid,value);
    if (value == SFV_MAY_SEND)
    {
      ChatParams &params = FChatParams[ASession.streamJid][ASession.contactJid];
      params.canSendStates = true;
      setSupported(ASession.streamJid,ASession.contactJid,true);
      sendStateMessage(ASession.streamJid,ASession.contactJid,params.selfState);
    }
    return ISessionNegotiator::Auto;
  }
  return ISessionNegotiator::Skip;
}

void ChatStates::sessionLocalize(const IStanzaSession &/*ASession*/, IDataForm &AForm)
{
  int index = FDataForms!=NULL ? FDataForms->fieldIndex(NS_CHATSTATES,AForm.fields) : -1;
  if (index >= 0)
  {
    AForm.fields[index].label = tr("Chat State Notifications");
    QList<IDataOption> &options = AForm.fields[index].options;
    for (int i=0; i<options.count(); i++)
    {
      if (options[i].value == SFV_MAY_SEND)
        options[i].label = tr("Allow Chat State Notifications");
      else if (options[i].value == SFV_MUSTNOT_SEND)
        options[i].label = tr("Disallow Chat State Notifications");
    }
  }
}

bool ChatStates::stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &/*AAccept*/)
{
  if (FSHIMessagesOut.value(AStreamJid)==AHandlerId && FChatParams.contains(AStreamJid))
  {
    bool stateSent = false;
    Jid contactJid = AStanza.to();
    if (isSupported(AStreamJid,contactJid))
    {
      IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,contactJid) : NULL;
      if (window)
      {
        stateSent = true;
        AStanza.addElement(STATE_ACTIVE,NS_CHATSTATES);
      }
    }
    FChatParams[AStreamJid][contactJid].canSendStates = stateSent;
    setSelfState(AStreamJid,contactJid,IChatStates::StateActive,false);
  }
  return false;
}

bool ChatStates::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHIMessagesIn.value(AStreamJid)==AHandlerId && FChatParams.contains(AStreamJid))
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

int ChatStates::permitStatus(const Jid &AContactJid) const
{
  return FPermitStatus.value(AContactJid.bare(),IChatStates::StatusDefault);
}

void ChatStates::setPermitStatus(const Jid AContactJid, int AStatus)
{
  if (permitStatus(AContactJid) != AStatus)
  {
    bool oldEnabled = isEnabled(Jid(),AContactJid);

    Jid bareJid = AContactJid.bare();
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

    if (!oldEnabled && isEnabled(Jid(),AContactJid))
      resetSupported(AContactJid);

    emit permitStatusChanged(bareJid,AStatus);
  }
}

bool ChatStates::isEnabled(const Jid &AStreamJid, const Jid &AContactJid) const
{
  QString svalue = FStanzaSessions.value(AStreamJid).value(AContactJid);
  if (svalue == SFV_MAY_SEND)
    return true;
  else if (svalue == SFV_MUSTNOT_SEND)
    return false;

  int status = permitStatus(AContactJid);
  return (isEnabled() || status==IChatStates::StatusEnable) && status!=IChatStates::StatusDisable;
}

bool ChatStates::isSupported(const Jid &AStreamJid, const Jid &AContactJid) const
{
  if (!FStanzaSessions.value(AStreamJid).contains(AContactJid))
  {
    bool supported = !FNotSupported.value(AStreamJid).contains(AContactJid);
    if (FDiscovery && supported && userChatState(AStreamJid,AContactJid)==IChatStates::StateUnknown)
      supported = !FDiscovery->hasDiscoInfo(AStreamJid, AContactJid) || FDiscovery->discoInfo(AStreamJid,AContactJid).features.contains(NS_CHATSTATES);
    return supported;
  }
  return true;
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
  return isEnabled(AStreamJid,AContactJid) && isSupported(AStreamJid,AContactJid) && FChatParams.value(AStreamJid).value(AContactJid).canSendStates;
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
  dfeature.description = tr("Supports the exchanging of the information about the user's activity in the chat");
  FDiscovery->insertDiscoFeature(dfeature);
}

void ChatStates::onPresenceOpened(IPresence *APresence)
{
  if (FStanzaProcessor)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.streamJid = APresence->streamJid();

    shandle.direction = IStanzaHandle::DirectionOut;
    shandle.order = SHO_MO_CHATSTATES;
    shandle.conditions.append(SHC_CONTENT_MESSAGES);
    FSHIMessagesOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.order = SHO_MI_CHATSTATES;
    shandle.conditions.append(SHC_STATE_MESSAGES);
    FSHIMessagesIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
  }

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
  
  if (FStanzaProcessor)
  {
    FStanzaProcessor->removeStanzaHandle(FSHIMessagesIn.take(APresence->streamJid()));
    FStanzaProcessor->removeStanzaHandle(FSHIMessagesOut.take(APresence->streamJid()));
  }

  FNotSupported.remove(APresence->streamJid());
  FChatParams.remove(APresence->streamJid());
  FStanzaSessions.remove(APresence->streamJid());
}

void ChatStates::onChatWindowCreated(IChatWindow *AWindow)
{
  StateWidget *widget = new StateWidget(this,AWindow,AWindow->toolBarWidget()->toolBarChanger()->toolBar());
  AWindow->toolBarWidget()->toolBarChanger()->insertWidget(widget,TBG_MWTBW_CHATSTATES);
  widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  widget->setPopupMode(QToolButton::InstantPopup);

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

void ChatStates::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
  FStanzaSessions[ASession.streamJid].remove(ASession.contactJid);
}

Q_EXPORT_PLUGIN2(plg_chatstates, ChatStates)
