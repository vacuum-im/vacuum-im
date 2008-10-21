#include "sessionnegotiation.h"

#include <QDebug>
#include <QUuid>
#include <QCryptographicHash>

#define SHC_STANZA_SESSION            "/message/feature[@xmlns='"NS_FEATURENEG"']"

#define SFP_DISCLOSURE                "disclosure"
#define SFP_MULTISESSION              "multisession"

#define SFV_DISCLOSURE_NEVER          "never"
#define SFV_DISCLOSURE_DISABLED       "disabled"
#define SFV_DISCLOSURE_ENABLED        "enabled"

#define ADR_STREAM_JID                Action::DR_Parametr1
#define ADR_CONTACT_JID               Action::DR_Parametr2
#define ADR_SESSION_FIELD             Action::DR_Parametr3

#define IN_SESSION                    "psi/smile"

#define NOTIFICATOR_ID                "SessionNegotiation"

SessionNegotiation::SessionNegotiation()
{
  FDataForms = NULL;
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FPresencePlugin = NULL;
  FNotifications = NULL;
}

SessionNegotiation::~SessionNegotiation()
{

}

void SessionNegotiation::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Negotiating the exchange of XML stanzas between two XMPP entities.");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Session Negotiation"); 
  APluginInfo->uid = SESSIONNEGOTIATION_UUID;
  APluginInfo ->version = "0.1";
  APluginInfo->dependences.append(DATAFORMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool SessionNegotiation::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin) 
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    connect(plugin->instance(), SIGNAL(opened(IXmppStream *)), SLOT(onStreamOpened(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(aboutToClose(IXmppStream *)), SLOT(onStreamAboutToClose(IXmppStream *))); 
    connect(plugin->instance(), SIGNAL(closed(IXmppStream *)), SLOT(onStreamClosed(IXmppStream *))); 
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin) 
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin) 
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),
        SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
    }
  }

  plugin = APluginManager->getPlugins("INotifications").value(0,NULL);
  if (plugin) 
  {
    FNotifications = qobject_cast<INotifications *>(plugin->instance());
    if (FNotifications)
    {
      connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
    }
  }

  return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool SessionNegotiation::initObjects()
{
  if (FDiscovery)
  {
    registerDiscoFeatures();
    //FDiscovery->insertFeatureHandler(NS_STANZA_SESSION,this,DFO_DEFAULT);
  }
  if (FNotifications)
  {
    uchar kindMask = INotification::TrayIcon|INotification::TrayAction|INotification::PopupWindow|INotification::PlaySound;
    FNotifications->insertNotificator(NOTIFICATOR_ID,tr("Negotiate session requests"),kindMask,kindMask);
  }
  if (FDataForms)
  {
    FDataForms->insertLocalizer(this,DATA_FORM_SESSION_NEGOTIATION);
  }
  insertNegotiator(this,SNO_DEFAULT);
  return true;
}

bool SessionNegotiation::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHISession.value(AStreamJid) == AHandlerId)
  {
    QString sessionId = AStanza.firstElement("thread").text();
    QDomElement featureElem = AStanza.firstElement("feature",NS_FEATURENEG);
    QDomElement formElem = featureElem.firstChildElement("x");
    while (!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
      formElem = formElem.nextSiblingElement("x");
    if (!sessionId.isEmpty() && !formElem.isNull())
    {
      Jid contactJid = AStanza.from();
      IStanzaSession session = currentSession(AStreamJid,contactJid);
      QString stanzaType = AStanza.type();
      if (stanzaType.isEmpty() || stanzaType=="normal")
      {
        IDataForm form = FDataForms->dataForm(formElem);
        bool isAccept = FDataForms->fieldIndex(SESSION_FIELD_ACCEPT, form.fields) >= 0;
        
        //Переводим сессию на ресурс с которого получен ответ
        if (isAccept && form.type==DATAFORM_TYPE_SUBMIT && session.sessionId != sessionId)
        {
          session = currentSession(AStreamJid,contactJid.bare());
          if (session.sessionId == sessionId)
          {
            removeSession(session);
            session.contactJid = contactJid;
            updateSession(session);
          }
        }

        if (session.sessionId == sessionId)
        {
          bool isRenegotiate = FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE, form.fields) >= 0;
          bool isContinue = FDataForms->fieldIndex(SESSION_FIELD_CONTINUE, form.fields) >= 0;
          bool isTerminate = FDataForms->fieldIndex(SESSION_FIELD_TERMINATE, form.fields) >= 0;
          if (isAccept && session.status != IStanzaSession::Active)
            processAccept(session,form);
          else if (isRenegotiate && session.status == IStanzaSession::Active)
            processRenegotiate(session,form);
          else if (isContinue && session.status == IStanzaSession::Active)
            processContinue(session,form);
          else if (isTerminate)
            processTerminate(session,form);
        }
        else if (isAccept && form.type==DATAFORM_TYPE_FORM)
        {
          terminateSession(AStreamJid,contactJid);
          session.sessionId = sessionId;
          session.streamJid = AStreamJid;
          session.contactJid = contactJid;
          processAccept(session,form);
        }
      }
      else if (stanzaType == "error")
      {
        QDomElement errorElem = AStanza.firstElement("error");
        if (!errorElem.isNull() && session.sessionId == sessionId && session.status!=IStanzaSession::Active)
        {
          ErrorHandler err(AStanza.element());
          session.status = IStanzaSession::Error;
          session.errorCondition = err.condition();

          session.errorFields.clear();
          QDomElement featureElem = errorElem.firstChildElement("feature");
          while (!featureElem.isNull() && featureElem.namespaceURI()!=NS_FEATURENEG)
            featureElem = featureElem.nextSiblingElement("feature");
          QDomElement fieldElem = featureElem.firstChildElement("field");
          while (!fieldElem.isNull())
          {
            if (fieldElem.hasAttribute("var"))
              session.errorFields.append(fieldElem.attribute("var"));
            fieldElem = fieldElem.nextSiblingElement("field");
          }
          updateSession(session);
          emit sessionFailed(session);
        }
      }
      AAccept = true;
    }
  }
  return false;
}

bool SessionNegotiation::execDiscoFeature(const Jid &/*AStreamJid*/, const QString &/*AFeature*/, const IDiscoInfo &/*ADiscoInfo*/)
{
  return false;
}

Action *SessionNegotiation::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (AFeature == NS_STANZA_SESSION)
  {
    Action *action = new Action(AParent);
    action->setIcon(SYSTEM_ICONSETFILE,IN_SESSION);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
    connect(action,SIGNAL(triggered(bool)),SLOT(onSessionActionTriggered(bool)));

    IStanzaSession session = currentSession(AStreamJid,ADiscoInfo.contactJid);
    if (session.status!=IStanzaSession::Accept && session.status!=IStanzaSession::Pending && 
        session.status!=IStanzaSession::Active && session.status!=IStanzaSession::Continue)
    {
      action->setData(ADR_SESSION_FIELD,SESSION_FIELD_ACCEPT);
      action->setText(tr("Negotiate Session"));
    }
    else
    {
      action->setData(ADR_SESSION_FIELD,SESSION_FIELD_TERMINATE);
      action->setText(tr("Terminate Session"));
    }
    return action;
  }
  return NULL;
}

IDataFormLocale SessionNegotiation::dataFormLocale(const QString &AFormType)
{
  IDataFormLocale locale;
  if (AFormType == DATA_FORM_SESSION_NEGOTIATION)
  {
    locale.title = tr("Session Negotiation");
    locale.fields["accept"].label = tr("Accept the Invitation?");
    locale.fields["continue"].label = tr("Another Resource");
    locale.fields["disclosure"].label = tr("Disclosure of Content");
    locale.fields["http://jabber.org/protocol/chatstates"].label = tr("Enable Chat State Notifications?");
    locale.fields["http://jabber.org/protocol/xhtml-im"].label = tr("Enable XHTML-IM formatting?");
    locale.fields["language"].label = tr("Primary Written Language of the Chat");
    locale.fields["logging"].label = tr("Enable Message Loggings?");
    locale.fields["renegotiate"].label = tr("Renegotiate the Session?");
    locale.fields["security"].label = tr("Minimum Security Level");
    locale.fields["terminate"].label = tr("Terminate the Session?");
    locale.fields["urn:xmpp:receipts"].label = tr("Enable Message Receipts?");
  }
  return locale;
}

int SessionNegotiation::sessionInit(const IStanzaSession &/*ASession*/, IDataForm &ARequest)
{
  int result = ISessionNegotiator::Skip;

  //Disclosure
  if (FDataForms->fieldIndex(SFP_DISCLOSURE,ARequest.fields)<0)
  {
    IDataField disclosure;
    disclosure.var = SFP_DISCLOSURE;
    disclosure.type = DATAFIELD_TYPE_LISTSINGLE;
    disclosure.value = SFV_DISCLOSURE_DISABLED;
    disclosure.options.append(IDataOption());
    disclosure.options[0].value = SFV_DISCLOSURE_NEVER;
    disclosure.options.append(IDataOption());
    disclosure.options[1].value = SFV_DISCLOSURE_DISABLED;
    disclosure.options.append(IDataOption());
    disclosure.options[2].value = SFV_DISCLOSURE_ENABLED;
    disclosure.required = false;
    ARequest.fields.append(disclosure);
    result |= ISessionNegotiator::Auto;
  }

  //MultiSession
  if (FDataForms->fieldIndex(SFP_MULTISESSION,ARequest.fields)<0)
  {
    IDataField multisession;
    multisession.var = SFP_MULTISESSION;
    multisession.type = DATAFIELD_TYPE_BOOLEAN;
    multisession.value = false;
    multisession.required = false;
    ARequest.fields.append(multisession);
    result |= ISessionNegotiator::Auto;
  }

  return result;
}

int SessionNegotiation::sessionAccept(const IStanzaSession &/*ASession*/, const IDataForm &ARequest, IDataForm &ASubmit)
{
  int result = ISessionNegotiator::Skip;

  //Disclosure
  int index = FDataForms->fieldIndex(SFP_DISCLOSURE,ARequest.fields);
  if (index>=0)
  {
    if (ARequest.type == DATAFORM_TYPE_FORM)
    {
      IDataField disclosure;
      disclosure.var = SFP_DISCLOSURE;
      disclosure.type = DATAFIELD_TYPE_LISTSINGLE;
      disclosure.value = ARequest.fields.at(index).value;
      ASubmit.fields.append(disclosure);
      result |= ISessionNegotiator::Auto;
    }
    else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
    {
      result |= ISessionNegotiator::Auto;
    }
  }

  //Multisession
  index = FDataForms->fieldIndex(SFP_MULTISESSION,ARequest.fields);
  if (index>=0)
  {
    if (ARequest.type == DATAFORM_TYPE_FORM)
    {
      IDataField multisession;
      multisession.var = SFP_MULTISESSION;
      multisession.type = DATAFIELD_TYPE_BOOLEAN;
      multisession.value = false;
      ASubmit.fields.append(multisession);
      result |= ISessionNegotiator::Auto;
    }
    else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
    {
      result |= ARequest.fields[index].value.toBool() ? ISessionNegotiator::Cancel : ISessionNegotiator::Auto;
    }
  }

  return result;
}

int SessionNegotiation::sessionContinue(const IStanzaSession &/*ASession*/, const QString &/*AResource*/)
{
  return ISessionNegotiator::Auto;
}

void SessionNegotiation::sessionLocalize(const IStanzaSession &/*ASession*/, IDataForm &AForm)
{
  //Disclosure
  int index = FDataForms->fieldIndex(SFP_DISCLOSURE,AForm.fields);
  if (index>=0)
  {
    AForm.fields[index].label = tr("Disclosure");
    QList<IDataOption> &options = AForm.fields[index].options;
    for (int i=0;i<options.count();i++)
    {
      if (options[i].value == SFV_DISCLOSURE_NEVER)
        options[i].label = tr("Guarantee disclosure not implemented");
      else if (options[i].value == SFV_DISCLOSURE_DISABLED)
        options[i].label = tr("Disable all disclosures");
      else if (options[i].value == SFV_DISCLOSURE_ENABLED)
        options[i].label = tr("Allow disclosures");
    }
  }
  
  //Multi-session
  index = FDataForms->fieldIndex(SFP_MULTISESSION,AForm.fields);
  if (index>=0)
  {
    AForm.fields[index].label = tr("Allow multiple sessions?");
  }
}

IStanzaSession SessionNegotiation::currentSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return FSessions.value(AStreamJid).value(AContactJid);
}

int SessionNegotiation::initSession(const Jid &AStreamJid, const Jid &AContactJid)
{
  IStanzaSession session = currentSession(AStreamJid,AContactJid);
  if (session.status!=IStanzaSession::Accept && 
      session.status!=IStanzaSession::Pending && 
      session.status!=IStanzaSession::Continue)
  {
    bool isRenegotiate = session.status==IStanzaSession::Active;
    IDataForm request = defaultForm(isRenegotiate ? SESSION_FIELD_RENEGOTIATE : SESSION_FIELD_ACCEPT);
    request.type = DATAFORM_TYPE_FORM;

    if (!isRenegotiate)
    {
      session.sessionId = QUuid::createUuid().toString();
      session.status = IStanzaSession::Init;
      session.streamJid = AStreamJid;
      session.contactJid = AContactJid;
      session.form = IDataForm();
      session.errorCondition.clear();
      session.errorFields.clear();
    }

    int result = 0;
    foreach(ISessionNegotiator *negotiator, FNegotiators)
      result = result | negotiator->sessionInit(session,request);
    
    if ((result & ISessionNegotiator::Cancel) > 0)
    {
      return ISessionNegotiator::Cancel;
    }
    else if ((result & ISessionNegotiator::Wait) > 0)
    {
      return ISessionNegotiator::Wait;
    }
    else if ((result & ISessionNegotiator::Manual) > 0)
    {
      updateSession(session);
      localizeSession(session,request);
      showSessionDialog(session,request);
      return ISessionNegotiator::Manual;
    }
    else if ((result & ISessionNegotiator::Auto) > 0)
    {
      updateSession(session);
      localizeSession(session,request);
      if (isRenegotiate ? sendSessionRenegotiate(session,request) : sendSessionInit(session,request))
        return ISessionNegotiator::Auto;
    }
  }
  return ISessionNegotiator::Cancel;
}

bool SessionNegotiation::isSuspenedSession(const QString &ASessionId) const
{
  return FSuspended.contains(ASessionId);
}

void SessionNegotiation::resumeSession(const Jid &AStreamJid, const Jid &AContactJid)
{
  IStanzaSession session = currentSession(AStreamJid,AContactJid);
  if (FSuspended.contains(session.sessionId))
  {
    IDataForm request = FSuspended.take(session.sessionId);
    if (session.status == IStanzaSession::Accept)
      processAccept(session,request);
    else if (session.status == IStanzaSession::Active)
      processRenegotiate(session,request);
    else if (session.status == IStanzaSession::Continue)
      processContinue(session,request);
  }
}

void SessionNegotiation::terminateSession(const Jid &AStreamJid, const Jid &AContactJid)
{
  IStanzaSession session = currentSession(AStreamJid,AContactJid);
  if (session.status != IStanzaSession::Empty && 
      session.status != IStanzaSession::Init &&
      session.status != IStanzaSession::Terminate && 
      session.status != IStanzaSession::Error)
  {
    IDataForm request = defaultForm(SESSION_FIELD_TERMINATE);
    request.type = DATAFORM_TYPE_SUBMIT;
    if (sendSessionData(session,request))
    {
      session.status = IStanzaSession::Terminate;
      updateSession(session);
      emit sessionTerminated(session);
    }
  }
}

void SessionNegotiation::insertNegotiator(ISessionNegotiator *ANegotiator, int AOrder)
{
  if (!FNegotiators.contains(AOrder,ANegotiator))
    FNegotiators.insert(AOrder,ANegotiator);
}

void SessionNegotiation::removeNegotiator(ISessionNegotiator *ANegotiator, int AOrder)
{
  if (FNegotiators.contains(AOrder,ANegotiator))
    FNegotiators.remove(AOrder,ANegotiator);
}

bool SessionNegotiation::sendSessionData(const IStanzaSession &ASession, const IDataForm &AForm)
{
  if (FStanzaProcessor && FDataForms)
  {
    Stanza data("message");
    data.setType("normal").setTo(ASession.contactJid.eFull());
    data.addElement("thread").appendChild(data.createTextNode(ASession.sessionId));
    QDomElement featureElem = data.addElement("feature",NS_FEATURENEG);
    FDataForms->xmlForm(AForm,featureElem);
    return FStanzaProcessor->sendStanzaOut(ASession.streamJid,data);
  }
  return false;
}

bool SessionNegotiation::sendSessionError(const IStanzaSession &ASession)
{
  if (FStanzaProcessor && FDataForms && !ASession.errorCondition.isEmpty())
  {
    Stanza data("message");
    data.setType("error").setTo(ASession.contactJid.eFull());
    data.addElement("thread").appendChild(data.createTextNode(ASession.sessionId));
    QDomElement featureElem = data.addElement("feature",NS_FEATURENEG);
    FDataForms->xmlForm(ASession.form,featureElem);
    QDomElement errorElem = data.addElement("error");
    errorElem.setAttribute("code",ErrorHandler::codeByCondition(ASession.errorCondition));
    errorElem.setAttribute("type",ErrorHandler::typeToString(ErrorHandler::typeByCondition(ASession.errorCondition)));
    errorElem.appendChild(data.createElement(ASession.errorCondition,EHN_DEFAULT));
    if (!ASession.errorFields.isEmpty())
    {
      QDomElement featureElem = errorElem.appendChild(data.createElement("feature",NS_FEATURENEG)).toElement();
      foreach(QString var, ASession.errorFields) {
        featureElem.appendChild(data.createElement("field")).toElement().setAttribute("var",var); }
    }
    return FStanzaProcessor->sendStanzaOut(ASession.streamJid,data);
  }
  return false;
}

bool SessionNegotiation::sendSessionInit(const IStanzaSession &ASession, const IDataForm &AForm)
{
  QVariant actionField = FDataForms!=NULL ? FDataForms->fieldValue(SESSION_FIELD_ACCEPT,AForm.fields) : QVariant();
  if (!actionField.isNull() && actionField.toBool() && sendSessionData(ASession,AForm))
  {
    IStanzaSession session = ASession;
    session.status = IStanzaSession::Pending;
    updateFields(AForm,session.form,true,false);
    updateSession(session);
    emit sessionInited(session);
    return true;
  }
  else if (ASession.status == IStanzaSession::Init)
  {
    removeSession(ASession);
  }
  return false;
}

bool SessionNegotiation::sendSessionAccept(const IStanzaSession &ASession, const IDataForm &AForm)
{
  QVariant actionField = FDataForms!=NULL ? FDataForms->fieldValue(SESSION_FIELD_ACCEPT,AForm.fields) : QVariant();
  if (!actionField.isNull() && sendSessionData(ASession,AForm))
  {
    if (actionField.toBool())
    {
      IStanzaSession session = ASession;
      session.status = IStanzaSession::Pending;
      updateFields(AForm,session.form,false,false);
      updateSession(session);
      emit sessionAccepted(session);
    }
    return true;
  }
  return false;
}

bool SessionNegotiation::sendSessionRenegotiate(const IStanzaSession &ASession, const IDataForm &AForm)
{
  QVariant actionField = FDataForms->fieldValue(SESSION_FIELD_RENEGOTIATE,AForm.fields);
  if (!actionField.isNull() && (actionField.toBool() || AForm.type==DATAFORM_TYPE_SUBMIT) && sendSessionData(ASession,AForm))
  {
    if (AForm.type == DATAFORM_TYPE_SUBMIT && actionField.toBool())
    {
      IStanzaSession session = ASession;
      updateFields(AForm,session.form,false,false);
      updateSession(session);
      emit sessionActivated(session);
    }
    else if (AForm.type == DATAFORM_TYPE_FORM)
    {
      FRenegotiate.insert(ASession.sessionId,AForm);
    }
    return true;
  }
  return false;
}

void SessionNegotiation::processAccept(const IStanzaSession &ASession, const IDataForm &ARequest)
{
  if (ARequest.type == DATAFORM_TYPE_FORM)
  {
    IDataForm submit = defaultForm(SESSION_FIELD_ACCEPT);
    submit.type = DATAFORM_TYPE_SUBMIT;

    int result = 0;
    foreach(ISessionNegotiator *negotiator, FNegotiators)
      result = result | negotiator->sessionAccept(ASession,ARequest,submit);

    IStanzaSession session = ASession;
    session.form = ARequest;
    if (!FDataForms->isSubmitValid(ARequest,submit))
    {
      session.status = IStanzaSession::Error;
      session.errorCondition = "feature-not-implemented";
      session.errorFields = unsubmitedFields(ARequest,submit,true);
      sendSessionError(session);
      updateSession(session);
      emit sessionFailed(session);
    }
    else if ((result & ISessionNegotiator::Cancel) > 0)
    {
      session.status = IStanzaSession::Terminate;
      submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
      sendSessionData(session,submit);
      updateSession(session);
      emit sessionTerminated(session);
    }
    else if ((result & ISessionNegotiator::Wait) > 0)
    {
      session.status = IStanzaSession::Accept;
      updateSession(session);
      FSuspended.insert(session.sessionId,ARequest);
    }
    else if ((result & ISessionNegotiator::Manual) > 0)
    {
      session.status = IStanzaSession::Accept;
      updateFields(submit,session.form,false,true);
      localizeSession(session,session.form);
      updateSession(session);
      showSessionDialog(session,session.form);
    }
    else if ((result & ISessionNegotiator::Auto) > 0)
    {
      session.status = IStanzaSession::Accept;
      updateFields(submit,session.form,false,true);
      localizeSession(session,session.form);
      updateSession(session);
      sendSessionAccept(session,submit);
    }
  }
  else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
  {
    if (FDataForms->fieldValue(SESSION_FIELD_ACCEPT,ARequest.fields).toBool())
    {
      IDataForm submit = defaultForm(SESSION_FIELD_ACCEPT);
      submit.type = DATAFORM_TYPE_RESULT;

      int result = 0;
      foreach(ISessionNegotiator *negotiator, FNegotiators)
        result = result | negotiator->sessionAccept(ASession,ARequest,submit);

      IStanzaSession session = ASession;
      if (!FDataForms->isSubmitValid(ASession.form,ARequest))
      {
        session.status = IStanzaSession::Error;
        session.errorCondition = "not-acceptable";
        session.errorFields = unsubmitedFields(ARequest,submit,true);
        sendSessionError(session);
        updateSession(session);
        emit sessionFailed(session);
      }
      else if ((result & ISessionNegotiator::Cancel) > 0)
      {
        session.status = IStanzaSession::Terminate;
        submit.fields[FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,submit.fields)].value = false;
        sendSessionData(session,submit);
        updateFields(ARequest,session.form,false,false);
        updateSession(session);
        emit sessionTerminated(session);
      }
      else if ((result & ISessionNegotiator::Wait) > 0)
      {
        session.status = IStanzaSession::Accept;
        updateSession(session);
        FSuspended.insert(session.sessionId,ARequest);
      }
      else if ((result & ISessionNegotiator::Auto|ISessionNegotiator::Manual) > 0)
      {
        session.status = IStanzaSession::Active;
        sendSessionData(session,submit);
        updateFields(ARequest,session.form,false,false);
        updateSession(session);
        emit sessionActivated(session);
      }
    }
    else
    {
      IStanzaSession session = ASession;
      session.status = IStanzaSession::Terminate;
      updateSession(session);
      emit sessionTerminated(session);
    }
  }
  else if (ARequest.type == DATAFORM_TYPE_RESULT)
  {
    if (FDataForms->fieldValue(SESSION_FIELD_ACCEPT,ARequest.fields).toBool())
    {
      IStanzaSession session = ASession;
      session.status = IStanzaSession::Active;
      updateSession(session);
      emit sessionActivated(session);
    }
    else
    {
      IStanzaSession session = ASession;
      session.status = IStanzaSession::Terminate;
      updateSession(session);
      emit sessionTerminated(session);
    }
  }
}

void SessionNegotiation::processRenegotiate(const IStanzaSession &ASession, const IDataForm &ARequest)
{
  if (ARequest.type == DATAFORM_TYPE_FORM)
  {
    IDataForm submit = defaultForm(SESSION_FIELD_RENEGOTIATE);
    submit.type = DATAFORM_TYPE_SUBMIT;

    int result = 0;
    foreach(ISessionNegotiator *negotiator, FNegotiators)
      result = result | negotiator->sessionAccept(ASession,ARequest,submit);

    if (!FDataForms->isSubmitValid(ARequest,submit))
    {
      IStanzaSession session = ASession;
      session.form = ARequest;
      session.errorCondition = "feature-not-implemented";
      session.errorFields = unsubmitedFields(ARequest,submit,true);
      sendSessionError(session);
    }
    else if ((result & ISessionNegotiator::Cancel) > 0)
    {
      submit.fields[FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,submit.fields)].value = false;
      sendSessionData(ASession,submit);
    }
    else if ((result & ISessionNegotiator::Wait) > 0)
    {
      FSuspended.insert(ASession.sessionId,ARequest);
    }
    else if ((result & ISessionNegotiator::Manual) > 0)
    {
      IDataForm request = ARequest;
      updateFields(submit,request,false,false);
      localizeSession(ASession,request);
      showSessionDialog(ASession,request);
    }
    else if ((result & ISessionNegotiator::Auto) > 0)
    {
      sendSessionRenegotiate(ASession,submit);
    }
  }
  else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
  {
    if (FRenegotiate.contains(ASession.sessionId))
    {
      IDataForm form = FRenegotiate.take(ASession.sessionId);
      bool accepted = FDataForms->fieldValue(SESSION_FIELD_RENEGOTIATE,ARequest.fields).toBool();
      if (accepted && FDataForms->isSubmitValid(form,ARequest))
      {
        IStanzaSession session = ASession;
        updateFields(ARequest,form,false,true);
        updateFields(form,session.form,true,false);
        updateSession(session);
        emit sessionActivated(session);
      }
    }
  }
}

void SessionNegotiation::processContinue(const IStanzaSession &ASession, const IDataForm &ARequest)
{
  if (ARequest.type == DATAFORM_TYPE_SUBMIT)
  {
    QString resource = FDataForms->fieldValue(SESSION_FIELD_CONTINUE,ARequest.fields).toString();
    if (ASession.status == IStanzaSession::Active && !resource.isEmpty())
    {
      int result = 0;
      foreach(ISessionNegotiator *negotiator, FNegotiators)
        result = result | negotiator->sessionContinue(ASession,resource);

      if ((result & ISessionNegotiator::Cancel) > 0)
      {
        terminateSession(ASession.streamJid,ASession.contactJid);
      }
      else if ((result & ISessionNegotiator::Wait) > 0)
      {
        IStanzaSession session = ASession;
        session.status = IStanzaSession::Continue;
        session.contactJid.setResource(resource);
        updateSession(session);
        FSuspended.insert(session.sessionId,ARequest);
      }
      else if ((result & (ISessionNegotiator::Manual|ISessionNegotiator::Auto)) > 0)
      {
        IDataForm response = ARequest;
        response.type == DATAFORM_TYPE_RESULT;
        sendSessionData(ASession,response);

        IStanzaSession session = ASession;
        session.status = IStanzaSession::Active;
        session.contactJid.setResource(resource);
        updateSession(session);
        emit sessionActivated(session);
      }
    }
  }
}

void SessionNegotiation::processTerminate(const IStanzaSession &ASession, const IDataForm &ARequest)
{
  if (ARequest.type == DATAFORM_TYPE_SUBMIT)
  {
    IStanzaSession session = ASession;
    session.status = IStanzaSession::Terminate;
    updateSession(session);
    emit sessionTerminated(session);
  }
}

void SessionNegotiation::showSessionDialog(const IStanzaSession &ASession, const IDataForm &ARequest)
{
  if (FDataForms)
  {
    IDataDialogWidget *dialog = FDialogs.value(ASession.streamJid).value(ASession.contactJid);
    if (!dialog)
    {
      dialog = FDataForms->dialogWidget(ARequest,NULL);
      dialog->instance()->setWindowIcon(Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_SESSION));
      dialog->dialogButtons()->setStandardButtons(QDialogButtonBox::Ok);
      connect(dialog->instance(),SIGNAL(accepted()),SLOT(onSessionDialogAccepted()));
      connect(dialog->instance(),SIGNAL(dialogDestroyed(IDataDialogWidget *)),SLOT(onSessionDialogDestroyed(IDataDialogWidget *)));
      FDialogs[ASession.streamJid].insert(ASession.contactJid,dialog);
    }
    else
      dialog->setForm(ARequest);

    if (FNotifications && !dialog->instance()->isVisible())
    {
      INotification notify;
      notify.kinds = FNotifications->notificatorKinds(NOTIFICATOR_ID);
      notify.data.insert(NDR_ICON,Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_SESSION));
      notify.data.insert(NDR_TOOLTIP,tr("Settings for session with %1").arg(ASession.contactJid.full()));
      notify.data.insert(NDR_WINDOW_CAPTION,tr("Session negotiation"));
      notify.data.insert(NDR_WINDOW_TITLE,FNotifications->contactName(ASession.streamJid,ASession.contactJid));
      notify.data.insert(NDR_WINDOW_IMAGE,FNotifications->contactAvatar(ASession.contactJid));
      notify.data.insert(NDR_WINDOW_TEXT, notify.data.value(NDR_TOOLTIP));
      int notifyId = FNotifications->appendNotification(notify);
      FDialogByNotify.insert(notifyId,dialog);
    }
    else
      dialog->instance()->show();
  }
}

void SessionNegotiation::closeSessionDialog(const IStanzaSession &ASession)
{
  if (FDialogs.value(ASession.streamJid).contains(ASession.contactJid))
  {
    IDataDialogWidget *dialog = FDialogs[ASession.streamJid].take(ASession.contactJid);
    dialog->instance()->deleteLater();
    if (FNotifications)
    {
      int notifyId = FDialogByNotify.key(dialog);
      FDialogByNotify.remove(notifyId);
      FNotifications->removeNotification(notifyId);
    }
  }
}

void SessionNegotiation::localizeSession(const IStanzaSession &ASession, IDataForm &AForm) const
{
  AForm.title = tr("Settings for session with %1").arg(ASession.contactJid.full());
  if (FDataForms)
  {
    int index = FDataForms->fieldIndex(SESSION_FIELD_ACCEPT,AForm.fields);
    if (index>=0)
      AForm.fields[index].label = tr("Accept this session?");

    index = FDataForms->fieldIndex(SESSION_FIELD_RENEGOTIATE,AForm.fields);
    if (index>=0)
      AForm.fields[index].label = tr("Renegotiate this settings?");

    foreach(ISessionNegotiator *negotiator, FNegotiators)
      negotiator->sessionLocalize(ASession,AForm);
  }
}

void SessionNegotiation::updateSession(const IStanzaSession &ASession)
{
  FSessions[ASession.streamJid].insert(ASession.contactJid,ASession);
  FSuspended.remove(ASession.sessionId);
  closeSessionDialog(ASession);
}

void SessionNegotiation::removeSession(const IStanzaSession &ASession)
{
  if (FSessions.value(ASession.streamJid).contains(ASession.contactJid))
  {
    IStanzaSession sesssion = FSessions[ASession.streamJid].take(ASession.contactJid);
    FSuspended.remove(sesssion.sessionId);
    FRenegotiate.remove(sesssion.sessionId);
    closeSessionDialog(sesssion);
  }
}

void SessionNegotiation::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = true;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_SESSION);
  dfeature.var = NS_STANZA_SESSION;
  dfeature.name = tr("Session Negotiation");
  dfeature.description = tr("Negotiate the exchange of XML stanzas between two XMPP entities");
  FDiscovery->insertDiscoFeature(dfeature);
}

void SessionNegotiation::updateFields(const IDataForm &ASourse, IDataForm &ADestination, bool AInsert, bool ARemove) const
{
  if (FDataForms)
  {
    const static QStringList reservedFields = QStringList() 
      << SESSION_FIELD_ACCEPT << SESSION_FIELD_CONTINUE << SESSION_FIELD_RENEGOTIATE << SESSION_FIELD_TERMINATE;

    QStringList updated;
    foreach(IDataField field, ASourse.fields)
    {
      int index = FDataForms->fieldIndex(field.var,ADestination.fields);
      if (index >= 0)
        ADestination.fields[index].value = field.value;
      else if (AInsert && !reservedFields.contains(field.var))
        ADestination.fields.append(field);
      updated.append(field.var);
    }

    if (ARemove)
    {
      for (int index=0; index<ADestination.fields.count(); index++)
      {
        QString var = ADestination.fields.at(index).var;
        if (!reservedFields.contains(var) && !updated.contains(var))
          ADestination.fields.removeAt(index--);
      }
    }
  }
}

IDataForm SessionNegotiation::defaultForm(const QString &AActionVar) const
{
  IDataField form_type;
  form_type.var = "FORM_TYPE";
  form_type.type = DATAFIELD_TYPE_HIDDEN;
  form_type.value = DATA_FORM_SESSION_NEGOTIATION;
  form_type.required = false;
  IDataField actionField;
  actionField.var = AActionVar;
  actionField.type = DATAFIELD_TYPE_BOOLEAN;
  actionField.value = true;
  actionField.required = true;
  IDataForm form;
  form.fields.append(form_type);
  form.fields.append(actionField);
  return form;
}

QStringList SessionNegotiation::unsubmitedFields(const IDataForm &ARequest, const IDataForm &ASubmit, bool ARequired) const
{
  QStringList fields;
  foreach(IDataField rField, ARequest.fields)
  {
    int index = FDataForms->fieldIndex(rField.var,ASubmit.fields);
    IDataField sField = index>=0 ? ASubmit.fields.at(index) : IDataField();
    if ((rField.required || !ARequired) && FDataForms->isFieldEmpty(ASubmit.fields.at(index)))
      fields.append(rField.var);  
  }
  return fields;
}

IStanzaSession SessionNegotiation::dialogSession(IDataDialogWidget *ADialog) const
{
  QList<Jid> streams = FDialogs.keys();
  foreach(Jid streamJid, streams)
    if (FDialogs.value(streamJid).values().contains(ADialog))
      return currentSession(streamJid,FDialogs.value(streamJid).key(ADialog));
  return IStanzaSession();
}

void SessionNegotiation::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FDataForms)
  {
    int handler = FStanzaProcessor->insertHandler(this,SHC_STANZA_SESSION,IStanzaProcessor::DirectionIn,SHP_DEFAULT,AXmppStream->jid());
    FSHISession.insert(AXmppStream->jid(),handler);
  }
}

void SessionNegotiation::onPresenceReceived(IPresence *APresence, const IPresenceItem &AItem)
{
  if (AItem.show == IPresence::Offline || AItem.show == IPresence::Error)
    terminateSession(APresence->streamJid(),AItem.itemJid);
}

void SessionNegotiation::onStreamAboutToClose(IXmppStream *AXmppStream)
{
  QList<IStanzaSession> sessions = FSessions.value(AXmppStream->jid()).values();
  foreach(IStanzaSession session, sessions)
  {
    terminateSession(session.streamJid,session.contactJid);
    removeSession(session);
  }
}

void SessionNegotiation::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor && FDataForms)
  {
    FStanzaProcessor->removeHandler(FSHISession.take(AXmppStream->jid()));
  }
  FDialogs.remove(AXmppStream->jid());
  FSessions.remove(AXmppStream->jid());
}

void SessionNegotiation::onNotificationActivated(int ANotifyId)
{
  if (FDialogByNotify.contains(ANotifyId))
  {
    IDataDialogWidget *dialog = FDialogByNotify.take(ANotifyId);
    if (dialog)
      dialog->instance()->show();
    FNotifications->removeNotification(ANotifyId);
  }
}

void SessionNegotiation::onSessionDialogAccepted()
{
  IDataDialogWidget *dialog = qobject_cast<IDataDialogWidget *>(sender());
  if (dialog)
  {
    IStanzaSession session = dialogSession(dialog);
    if (session.status == IStanzaSession::Init)
      sendSessionInit(session,dialog->formWidget()->userDataForm());
    else if (session.status == IStanzaSession::Accept)
      sendSessionAccept(session,FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
    else if (session.status == IStanzaSession::Active)
      sendSessionRenegotiate(session,FDataForms->dataSubmit(dialog->formWidget()->userDataForm()));
    closeSessionDialog(session);
  }
}

void SessionNegotiation::onSessionDialogDestroyed(IDataDialogWidget *ADialog)
{
  IStanzaSession session = dialogSession(ADialog);
  terminateSession(session.streamJid,session.contactJid);
  closeSessionDialog(session);
}

void SessionNegotiation::onSessionActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    QString sessionField = action->data(ADR_SESSION_FIELD).toString();
    if (sessionField == SESSION_FIELD_ACCEPT)
      initSession(streamJid,contactJid);
    else if (sessionField == SESSION_FIELD_TERMINATE)
      terminateSession(streamJid,contactJid);
  }
}

Q_EXPORT_PLUGIN2(SessionNegotiationPlugin, SessionNegotiation)
