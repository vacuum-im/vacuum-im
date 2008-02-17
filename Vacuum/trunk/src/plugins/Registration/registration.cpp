#include "registration.h"

#define REGISTRATION_TIMEOUT    30000

#define IN_REGISTER             "psi/register"

#define ADR_StreamJid           Action::DR_StreamJid
#define ADR_ServiveJid          Action::DR_Parametr1
#define ADR_Operation           Action::DR_Parametr2

Registration::Registration()
{
  FDataForms = NULL;
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FPresencePlugin = NULL;
  FSettingsPlugin = NULL;
  FAccountManager = NULL;
}

Registration::~Registration()
{

}

void Registration::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("In-band registration with instant messaging servers and associated services");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("In-Band Registration"); 
  APluginInfo->uid = REGISTRATION_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(DATAFORMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Registration::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogClosed()),SLOT(onOptionsDialogClosed()));
    }
  }

  plugin = APluginManager->getPlugins("IAccountManager").value(0,NULL);
  if (plugin)
  {
    FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
    if (FAccountManager)
    {
      connect(FAccountManager->instance(),SIGNAL(optionsAccepted()),SLOT(onOptionsAccepted()));
      connect(FAccountManager->instance(),SIGNAL(optionsRejected()),SLOT(onOptionsRejected()));
    }
  }
  
  plugin = APluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    IXmppStreams *xmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (xmppStreams)
    {
      connect(xmppStreams->instance(),SIGNAL(destroyed(IXmppStream *)),SLOT(onStreamDestroyed(IXmppStream *)));
    }
  }

  return FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool Registration::initObjects()
{
  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_JABBER_REGISTER,this,DFO_DEFAULT);
  }
  if (FSettingsPlugin)
  {
    FSettingsPlugin->appendOptionsHolder(this);
  }
  return true;
}

void Registration::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FSendRequests.contains(AStanza.id()) || FSubmitRequests.contains(AStanza.id()))
  {
    QDomElement dataForm; 
    QDomElement query = AStanza.firstElement("query",NS_JABBER_REGISTER);
    dataForm = query.firstChildElement("x");
    while (!dataForm.isNull() && (dataForm.namespaceURI()!=NS_JABBER_DATA || dataForm.attribute("type",FORM_FORM)!=FORM_FORM))
      dataForm = dataForm.nextSiblingElement("x");
    
    if (FSubmitRequests.contains(AStanza.id()) && AStanza.type() == "result")
    {
      emit registerSuccessful(AStanza.id());
    }
    else if (AStanza.type() == "result" || !dataForm.isNull())
    {                                                           
      IRegisterFields fields;
      fields.fieldMask = 0;
      fields.registered = !query.firstChildElement("registered").isNull();
      fields.serviceJid = AStanza.from();
      fields.instructions = query.firstChildElement("instructions").text();
      if (!query.firstChildElement("username").isNull())
      {
        fields.fieldMask += IRegisterFields::Username;
        fields.username = query.firstChildElement("username").text();
      }
      else if (!query.firstChildElement("name").isNull())
      {
        fields.fieldMask += IRegisterFields::Username;
        fields.username = query.firstChildElement("name").text();
      }
      if (!query.firstChildElement("password").isNull())
      {
        fields.fieldMask += IRegisterFields::Password;
        fields.password = query.firstChildElement("password").text();
      }
      if (!query.firstChildElement("email").isNull())
      {
        fields.fieldMask += IRegisterFields::Email;
        fields.email = query.firstChildElement("email").text();
      }
      fields.key = query.firstChildElement("key").text();
      fields.dataForm = dataForm;
      QDomElement oob = query.firstChildElement("x");
      while (!oob.isNull() && oob.namespaceURI()!=NS_JABBER_OOB)
        oob = oob.nextSiblingElement("x");
      if (!oob.isNull())
      {
        fields.fieldMask += IRegisterFields::URL;
        fields.url = oob.firstChildElement("url").text();
      }
      emit registerFields(AStanza.id(),fields);
    }
    else
    {
      ErrorHandler err(AStanza.element());
      emit registerError(AStanza.id(),err.message());
    }
    FSendRequests.removeAt(FSendRequests.indexOf(AStanza.id()));
    FSubmitRequests.removeAt(FSubmitRequests.indexOf(AStanza.id()));
  }
}

void Registration::iqStanzaTimeOut(const QString &AId)
{
  if (FSendRequests.contains(AId))
  {
    FSendRequests.removeAt(FSendRequests.indexOf(AId));
    ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
    emit registerError(AId,err.message());
  }
  else if (FSubmitRequests.contains(AId))
  {
    FSubmitRequests.removeAt(FSubmitRequests.indexOf(AId));
    ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
    emit registerError(AId,err.message());
  }
}

bool Registration::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_JABBER_REGISTER)
  {
    showRegisterDialog(AStreamJid,ADiscoInfo.contactJid,NULL);
    return true;
  }
  return false;
}

Action *Registration::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if ( presence && presence->isOpen() && AFeature == NS_JABBER_REGISTER)
  {   
    Menu *regMenu = new Menu(AParent);
    regMenu->setTitle(tr("Registration"));
    regMenu->setIcon(SYSTEM_ICONSETFILE,IN_REGISTER);

    Action *action = new Action(regMenu);
    action->setText(tr("Register"));
    action->setData(ADR_StreamJid,AStreamJid.full());
    action->setData(ADR_ServiveJid,ADiscoInfo.contactJid.full());
    action->setData(ADR_Operation,IRegistration::Register);
    connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
    regMenu->addAction(action,AG_DEFAULT,false);

    action = new Action(regMenu);
    action->setText(tr("Unregister"));
    action->setData(ADR_StreamJid,AStreamJid.full());
    action->setData(ADR_ServiveJid,ADiscoInfo.contactJid.full());
    action->setData(ADR_Operation,IRegistration::Unregister);
    connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
    regMenu->addAction(action,AG_DEFAULT,false);

    action = new Action(regMenu);
    action->setText(tr("Change password"));
    action->setData(ADR_StreamJid,AStreamJid.full());
    action->setData(ADR_ServiveJid,ADiscoInfo.contactJid.full());
    action->setData(ADR_Operation,IRegistration::ChangePassword);
    connect(action,SIGNAL(triggered(bool)),SLOT(onRegisterActionTriggered(bool)));
    regMenu->addAction(action,AG_DEFAULT,false);

    return regMenu->menuAction();
  }
  return NULL;
}

IStreamFeature *Registration::getStreamFeature(const QString &AFeatureNS, IXmppStream *AXmppStream)
{
  if (AFeatureNS == NS_FEATURE_REGISTER)
  {
    IStreamFeature *feature = FStreamFeatures.value(AXmppStream);
    if (!feature)
    {
      feature = new RegisterStream(this,AXmppStream);
      FStreamFeatures.insert(AXmppStream,feature);
      emit featureCreated(feature);
    }
    return feature;
  }
  return NULL;
}

void Registration::destroyStreamFeature(IStreamFeature *AFeature)
{
  if (AFeature && FStreamFeatures.value(AFeature->xmppStream()) == AFeature)
  {
    FStreamFeatures.remove(AFeature->xmppStream());
    AFeature->xmppStream()->removeFeature(AFeature);
    emit featureDestroyed(AFeature);
    AFeature->instance()->deleteLater();
  }
}

QWidget *Registration::optionsWidget(const QString &ANode, int &AOrder)
{
  QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
  if (nodeTree.count()==2 && nodeTree.at(0)==ON_ACCOUNTS)
  {
    AOrder = OO_ACCOUNT_REGISTER;
    QCheckBox *checkBox = new QCheckBox;
    checkBox->setText(tr("Register new account on server"));

    QString accountId = nodeTree.at(1);
    IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(accountId) : NULL;
    if (account)
      checkBox->setCheckState(FStreamFeatures.contains(account->xmppStream()) ? Qt::Checked : Qt::Unchecked);
    FOptionWidgets.insert(accountId,checkBox);
    return checkBox;
  }
  return NULL;
}

QString Registration::sendRegiterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
  Stanza reg("iq");
  reg.setTo(AServiceJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
  reg.addElement("query",NS_JABBER_REGISTER);
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,reg,REGISTRATION_TIMEOUT))
  {
    FSendRequests.append(reg.id());
    return reg.id();
  }
  return QString();
}

QString Registration::sendUnregiterRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
  Stanza unreg("iq");
  unreg.setTo(AServiceJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
  unreg.addElement("query",NS_JABBER_REGISTER).appendChild(unreg.createElement("remove"));
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,unreg,REGISTRATION_TIMEOUT))
  {
    FSubmitRequests.append(unreg.id());
    return unreg.id();
  }
  return QString();
}

QString Registration::sendChangePasswordRequest(const Jid &AStreamJid, const Jid &AServiceJid, 
                                                const QString &AUserName, const QString &APassword)
{
  Stanza change("iq");
  change.setTo(AServiceJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
  QDomElement elem = change.addElement("query",NS_JABBER_REGISTER);
  elem.appendChild(change.createElement("username")).appendChild(change.createTextNode(AUserName));
  elem.appendChild(change.createElement("password")).appendChild(change.createTextNode(APassword));
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,change,REGISTRATION_TIMEOUT))
  {
    FSubmitRequests.append(change.id());
    return change.id();
  }
  return QString();
}

QString Registration::sendSubmit(const Jid &AStreamJid, const IRegisterSubmit &ASubmit)
{
  Stanza submit("iq");
  submit.setTo(ASubmit.serviceJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
  QDomElement query = submit.addElement("query",NS_JABBER_REGISTER);
  if (ASubmit.dataForm == NULL)
  {
    if (!ASubmit.username.isEmpty())
      query.appendChild(submit.createElement("username")).appendChild(submit.createTextNode(ASubmit.username));
    if (!ASubmit.password.isEmpty())
      query.appendChild(submit.createElement("password")).appendChild(submit.createTextNode(ASubmit.password));
    if (!ASubmit.email.isEmpty())
      query.appendChild(submit.createElement("email")).appendChild(submit.createTextNode(ASubmit.email));
    if (!ASubmit.key.isEmpty())
      query.appendChild(submit.createElement("key")).appendChild(submit.createTextNode(ASubmit.key));
  }
  else
  {
    QDomElement form = query.appendChild(submit.createElement("x",NS_JABBER_DATA)).toElement();
    ASubmit.dataForm->createSubmit(form);
  }
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,submit,REGISTRATION_TIMEOUT))
  {
    FSubmitRequests.append(submit.id());
    return submit.id();
  }
  return QString();
}

void Registration::showRegisterDialog(const Jid &AStreamJid, const Jid &AServiceJid, int AOperation, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    RegisterDialog *dialog = new RegisterDialog(this,FDataForms,AStreamJid,AServiceJid,AOperation,AParent);
    connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    dialog->show();
  }
}

void Registration::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_REGISTER);
  dfeature.var = NS_JABBER_REGISTER;
  dfeature.name = tr("In-Band Registration");
  dfeature.description = tr("In-band registration with instant messaging servers and associated services");
  FDiscovery->insertDiscoFeature(dfeature);
}

void Registration::onRegisterActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_StreamJid).toString();
    Jid serviceJid = action->data(ADR_ServiveJid).toString();
    int operation = action->data(ADR_Operation).toInt();
    showRegisterDialog(streamJid,serviceJid,operation,NULL);
  }
}

void Registration::onStreamDestroyed(IXmppStream *AXmppStream)
{
  destroyStreamFeature(FStreamFeatures.value(AXmppStream));
}

void Registration::onOptionsAccepted()
{
  QList<QString> accounts = FOptionWidgets.keys();
  foreach(QString accountId, accounts)
  {
    IAccount *account = FAccountManager->accountById(accountId);
    if (account && account->isActive())
    {
      QCheckBox *checkBox = FOptionWidgets.value(accountId);
      if (checkBox->isChecked())
        account->xmppStream()->addFeature(getStreamFeature(NS_FEATURE_REGISTER,account->xmppStream()));
      else
        destroyStreamFeature(FStreamFeatures.value(account->xmppStream()));
    }
  }
  emit optionsAccepted();
}

void Registration::onOptionsRejected()
{
  emit optionsRejected();
}

void Registration::onOptionsDialogClosed()
{
  FOptionWidgets.clear();
}

Q_EXPORT_PLUGIN2(RegistrationPlugin, Registration)