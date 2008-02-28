#include "gateways.h"

#define GATEWAY_TIMEOUT       30000

#define ADR_STREAM_JID        Action::DR_StreamJid
#define ADR_SERVICE_JID       Action::DR_Parametr1
#define ADR_LOG_IN            Action::DR_Parametr2

#define IN_GATEWAYS           "psi/disco"
#define IN_ADD_USER           "psi/addContact"
#define IN_LOG_IN             "status/online"
#define IN_LOG_OUT            "status/offline"
#define IN_KEEP               "psi/advanced"

#define PSN_GATEWAYS_KEEP     "vacuum:gateways:keep"
#define PST_GATEWAYS_SERVICES "services"

#define KEEP_INTERVAL         30000

Gateways::Gateways()
{
  FDiscovery = NULL;
  FStanzaProcessor = NULL;
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FRosterChanger = NULL;
  FRostersViewPlugin = NULL;
  FVCardPlugin = NULL;
  FPrivateStorage = NULL;

  FKeepTimer.setSingleShot(false);
  connect(&FKeepTimer,SIGNAL(timeout()),SLOT(onKeepTimerTimeout()));
}

Gateways::~Gateways()
{

}

void Gateways::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Plugin for interactions between Jabber clients and client proxy gateways to legacy IM services");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Gateway Interaction"); 
  APluginInfo->uid = GATEWAYS_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Gateways::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceRemoved(IPresence *)),SLOT(onPresenceRemoved(IPresence *)));
    }
  }

  plugin = APluginManager->getPlugins("IRosterChanger").value(0,NULL);
  if (plugin)
    FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVCardPlugin)
    {
      connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
      connect(FVCardPlugin->instance(),SIGNAL(vcardError(const Jid &, const QString &)),SLOT(onVCardError(const Jid &, const QString &)));
    }
  }

  plugin = APluginManager->getPlugins("IPrivateStorage").value(0,NULL);
  if (plugin)
  {
    FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
    if (FPrivateStorage)
    {
      connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
        SLOT(onPrivateStorageLoaded(const QString &, const Jid &, const QDomElement &)));
    }
  }

  return FStanzaProcessor!=NULL;
}

bool Gateways::initObjects()
{
  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_JABBER_GATEWAY,this,DFO_DEFAULT);
  }
  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *, Menu *)),
      SLOT(onRostersViewContextMenu(IRosterIndex *, Menu *)));
  }
  return true;
}

void Gateways::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FPromptRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      QString desc = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("desc").text();
      QString prompt = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("prompt").text();
      emit promptReceived(AStanza.id(),desc,prompt);
    }
    else
    {
      ErrorHandler err(AStanza.element());
      emit errorReceived(AStanza.id(),err.message());
    }
    FPromptRequests.removeAt(FPromptRequests.indexOf(AStanza.id()));
  }
  else if (FUserJidRequests.contains(AStanza.id()))
  {
    if (AStanza.type() == "result")
    {
      Jid userJid = AStanza.firstElement("query",NS_JABBER_GATEWAY).firstChildElement("jid").text();
      emit userJidReceived(AStanza.id(),userJid);
    }
    else
    {
      ErrorHandler err(AStanza.element());
      emit errorReceived(AStanza.id(),err.message());
    }
    FUserJidRequests.removeAt(FUserJidRequests.indexOf(AStanza.id()));
  }
}

void Gateways::iqStanzaTimeOut(const QString &AId)
{
  if (FPromptRequests.contains(AId) || FUserJidRequests.contains(AId))
  {
    ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
    emit errorReceived(AId,err.message());
    FPromptRequests.removeAt(FPromptRequests.indexOf(AId));
    FUserJidRequests.removeAt(FUserJidRequests.indexOf(AId));
  }
}

bool Gateways::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_JABBER_GATEWAY)
    return showAddLegacyContactDialog(AStreamJid,ADiscoInfo.contactJid)!=NULL;
  return false;
}

Action *Gateways::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen() && AFeature == NS_JABBER_GATEWAY)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Add Legacy User"));
    action->setIcon(SYSTEM_ICONSETFILE,IN_ADD_USER);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_SERVICE_JID,ADiscoInfo.contactJid.full());
    connect(action,SIGNAL(triggered(bool)),SLOT(onGatewayActionTriggered(bool)));
    return action;
  }
  return NULL;
}

void Gateways::resolveNickName(const Jid &AStreamJid, const Jid &AContactJid)
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  IRosterItem ritem = roster!=NULL ? roster->rosterItem(AContactJid) : IRosterItem();
  if (ritem.isValid)
  {
    if (FVCardPlugin->hasVCard(ritem.itemJid))
    {
      IVCard *vcard = FVCardPlugin->vcard(ritem.itemJid);
      QString nick = vcard->value(VVN_NICKNAME);
      if (!nick.isEmpty())
        roster->renameItem(ritem.itemJid,nick);
      vcard->unlock();
    }
    else 
    {
      if (!FResolveNicks.contains(ritem.itemJid))
        FVCardPlugin->requestVCard(AStreamJid,ritem.itemJid);
      FResolveNicks.insertMulti(ritem.itemJid,AStreamJid);
    }
  }
}

void Gateways::sendLogPresence(const Jid &AStreamJid, const Jid &AServiceJid, bool ALogIn)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    if (ALogIn)
      presence->sendPresence(AServiceJid,presence->show(),presence->status(),presence->priority());
    else
      presence->sendPresence(AServiceJid,IPresence::Offline,tr("Log Out"),0);
  }
}

void Gateways::setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence)
  {
    if (AEnabled)
      FKeepConnections.insertMulti(presence,AServiceJid);
    else
      FKeepConnections.remove(presence,AServiceJid);
  }
}

QString Gateways::sendPromptRequest(const Jid &AStreamJid, const Jid &AServiceJid)
{
  Stanza request("iq");
  request.setType("get").setTo(AServiceJid.eFull()).setId(FStanzaProcessor->newId());
  request.addElement("query",NS_JABBER_GATEWAY);
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,GATEWAY_TIMEOUT))
  {
    FPromptRequests.append(request.id());
    return request.id();
  }
  return QString();
}

QString Gateways::sendUserJidRequest(const Jid &AStreamJid, const Jid &AServiceJid, const QString &AContactID)
{
  Stanza request("iq");
  request.setType("set").setTo(AServiceJid.eFull()).setId(FStanzaProcessor->newId());
  QDomElement elem = request.addElement("query",NS_JABBER_GATEWAY);
  elem.appendChild(request.createElement("prompt")).appendChild(request.createTextNode(AContactID));
  if (FStanzaProcessor->sendIqStanza(this,AStreamJid,request,GATEWAY_TIMEOUT))
  {
    FUserJidRequests.append(request.id());
    return request.id();
  }
  return QString();
}

QDialog *Gateways::showAddLegacyContactDialog(const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    AddLegacyContactDialog *dialog = new AddLegacyContactDialog(this,FRosterChanger,AStreamJid,AServiceJid,AParent);
    connect(presence->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    dialog->show();
    return dialog;
  }
  return NULL;
}

void Gateways::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = false;
  dfeature.var = NS_JABBER_GATEWAY;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_GATEWAYS);
  dfeature.name = tr("Gateway Interaction");
  dfeature.description = tr("Enables a client to send a legacy username to the gateway and receive a properly-formatted JID");
  FDiscovery->insertDiscoFeature(dfeature);
}

void Gateways::savePrivateStorageKeep(const Jid &AStreamJid)
{
  if (FPrivateStorage && FPrivateStorageKeep.contains(AStreamJid))
  {
    QDomDocument doc;
    doc.appendChild(doc.createElement("services"));
    QDomElement elem = doc.documentElement().appendChild(doc.createElementNS(PSN_GATEWAYS_KEEP,PST_GATEWAYS_SERVICES)).toElement();
    QSet<Jid> services = FPrivateStorageKeep.value(AStreamJid);
    foreach(Jid service, services)
      elem.appendChild(doc.createElement("service")).appendChild(doc.createTextNode(service.bare()));
    FPrivateStorage->saveData(AStreamJid,elem);
  }
}

void Gateways::onGatewayActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid serviceJid = action->data(ADR_SERVICE_JID).toString();
    showAddLegacyContactDialog(streamJid,serviceJid);
  }
}

void Gateways::onLogActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid serviceJid = action->data(ADR_SERVICE_JID).toString();
    bool logIn = action->data(ADR_LOG_IN).toBool();
    sendLogPresence(streamJid,serviceJid,logIn);
  }
}

void Gateways::onResolveActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_SERVICE_JID).toString();
    resolveNickName(streamJid,contactJid);
  }
}

void Gateways::onKeepActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid serviceJid = action->data(ADR_SERVICE_JID).toString();
    if (FPrivateStorageKeep.contains(streamJid))
    {
      if (action->isChecked())
        FPrivateStorageKeep[streamJid] += serviceJid;
      else
        FPrivateStorageKeep[streamJid] -= serviceJid;
      savePrivateStorageKeep(streamJid);
    }
    setKeepConnection(streamJid,serviceJid,action->isChecked());
  }
}

void Gateways::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_Agent)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (presence && presence->isOpen())
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Log In"));
      action->setIcon(STATUS_ICONSETFILE,IN_LOG_IN);
      action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
      action->setData(ADR_SERVICE_JID,AIndex->data(RDR_BareJid));
      action->setData(ADR_LOG_IN,true);
      connect(action,SIGNAL(triggered(bool)),SLOT(onLogActionTriggered(bool)));
      AMenu->addAction(action,AG_GATEWAYS_LOGIN_ROSTER,true);

      action = new Action(AMenu);
      action->setText(tr("Log Out"));
      action->setIcon(STATUS_ICONSETFILE,IN_LOG_OUT);
      action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
      action->setData(ADR_SERVICE_JID,AIndex->data(RDR_BareJid));
      action->setData(ADR_LOG_IN,false);
      connect(action,SIGNAL(triggered(bool)),SLOT(onLogActionTriggered(bool)));
      AMenu->addAction(action,AG_GATEWAYS_LOGIN_ROSTER,true);

      if (FPrivateStorageKeep.contains(presence->streamJid()))
      {
        Action *action = new Action(AMenu);
        action->setText(tr("Keep connection"));
        action->setIcon(SYSTEM_ICONSETFILE,IN_KEEP);
        action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
        action->setData(ADR_SERVICE_JID,AIndex->data(RDR_BareJid));
        action->setCheckable(true);
        action->setChecked(FKeepConnections.contains(presence,AIndex->data(RDR_BareJid).toString()));
        connect(action,SIGNAL(triggered(bool)),SLOT(onKeepActionTriggered(bool)));
        AMenu->addAction(action,AG_GATEWAYS_LOGIN_ROSTER,true);
      }
    }
  }
  else if (FRosterPlugin && FVCardPlugin && AIndex->type() == RIT_Contact)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
    if (roster && roster->isOpen() && roster->rosterItem(contactJid).isValid && FDiscovery->discoInfo(contactJid.domane()).features.contains(NS_JABBER_GATEWAY))
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Resolve nick"));
      action->setData(ADR_STREAM_JID,streamJid.full());
      action->setData(ADR_SERVICE_JID,contactJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onResolveActionTriggered(bool)));
      AMenu->addAction(action,AG_GATEWAYS_RESOLVE_ROSTER,true);
    }
  }
}

void Gateways::onPresenceOpened(IPresence *APresence)
{
  FPrivateStorage->loadData(APresence->streamJid(),PST_GATEWAYS_SERVICES,PSN_GATEWAYS_KEEP);
  FKeepTimer.start(KEEP_INTERVAL);
}

void Gateways::onPresenceRemoved(IPresence *APresence)
{
  FKeepConnections.remove(APresence);
  FPrivateStorageKeep.remove(APresence->streamJid());
}

void Gateways::onKeepTimerTimeout()
{
  QList<IPresence *> presences = FKeepConnections.keys();
  foreach(IPresence *presence, presences)
  {
    if (presence->isOpen())
    {
      QList<Jid> services = FKeepConnections.values(presence);
      foreach(Jid service, services)
      {
        if (presence->presenceItems(service).isEmpty())
        {
          presence->sendPresence(service,IPresence::Offline,"",0);
          presence->sendPresence(service,presence->show(),presence->status(),presence->priority());
        }
      }
    }
  }
}

void Gateways::onVCardReceived(const Jid &AContactJid)
{
  if (FResolveNicks.contains(AContactJid))
  {
    QList<Jid> streamJids = FResolveNicks.values(AContactJid);
    foreach(Jid streamJid, streamJids)
      resolveNickName(streamJid,AContactJid);
    FResolveNicks.remove(AContactJid);
  }
}

void Gateways::onVCardError(const Jid &AContactJid, const QString &/*AError*/)
{
  FResolveNicks.remove(AContactJid);
}

void Gateways::onPrivateStorageLoaded(const QString &/*AId*/, const Jid &AStreamJid, const QDomElement &AElement)
{
  if (AElement.tagName() == PST_GATEWAYS_SERVICES && AElement.namespaceURI() == PSN_GATEWAYS_KEEP)
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
    if (roster)
    {
      QSet<Jid> services;
      bool changed = false;
      QDomElement elem = AElement.firstChildElement("service");
      while (!elem.isNull())
      {
        Jid service = elem.text();
        if (roster->rosterItem(service).isValid)
        {
          services += service;
          setKeepConnection(AStreamJid,service,true);
        }
        else
          changed = true;
        elem = elem.nextSiblingElement("service");
      }

      QSet<Jid> oldServices = FPrivateStorageKeep.value(AStreamJid) - services;
      foreach(Jid service, oldServices)
        setKeepConnection(AStreamJid,service,false);
      FPrivateStorageKeep[AStreamJid] = services;
      
      if (changed)
        savePrivateStorageKeep(AStreamJid);
    }
  }
}

Q_EXPORT_PLUGIN2(GatewaysPlugin, Gateways)