#include "gateways.h"

#define GATEWAY_TIMEOUT       30000

#define ADR_STREAM_JID        Action::DR_StreamJid
#define ADR_SERVICE_JID       Action::DR_Parametr1
#define ADR_NEW_SERVICE_JID   Action::DR_Parametr2
#define ADR_LOG_IN            Action::DR_Parametr3

#define IN_GATEWAYS           "psi/disco"
#define IN_ADD_USER           "psi/addContact"
#define IN_LOG_IN             "status/online"
#define IN_LOG_OUT            "status/offline"
#define IN_KEEP               "psi/advanced"
#define IN_CHANGE_TRANSPORT   "psi/arrowRight"

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
  FStatusIcons = NULL;

  FKeepTimer.setSingleShot(false);
  connect(&FKeepTimer,SIGNAL(timeout()),SLOT(onKeepTimerTimeout()));
}

Gateways::~Gateways()
{

}

void Gateways::pluginInfo(IPluginInfo *APluginInfo)
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
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoItemsWindowCreated(IDiscoItemsWindow *)),
        SLOT(onDiscoItemsWindowCreated(IDiscoItemsWindow *)));
    }
  }

  plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterSubscription(IRoster *, const Jid &, int , const QString &)),
        SLOT(onRosterSubscription(IRoster *, const Jid &, int , const QString &)));
      connect(FRosterPlugin->instance(),SIGNAL(rosterJidAboutToBeChanged(IRoster *, const Jid &)),
        SLOT(onRosterJidAboutToBeChanged(IRoster *, const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceOpened(IPresence *)),SLOT(onPresenceOpened(IPresence *)));
      connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
        SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
      connect(FPresencePlugin->instance(),SIGNAL(presenceClosed(IPresence *)),SLOT(onPresenceClosed(IPresence *)));
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

  plugin = APluginManager->getPlugins("IStatusIcons").value(0,NULL);
  if (plugin)
    FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());

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

QList<Jid> Gateways::keepConnections(const Jid &AStreamJid) const
{
  return FKeepConnections.values(AStreamJid);
}

void Gateways::setKeepConnection(const Jid &AStreamJid, const Jid &AServiceJid, bool AEnabled)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence)
  {
    if (AEnabled)
      FKeepConnections.insertMulti(presence->streamJid(),AServiceJid);
    else
      FKeepConnections.remove(presence->streamJid(),AServiceJid);
  }
}

QList<Jid> Gateways::streamServices(const Jid &AStreamJid, const IDiscoIdentity &AIdentity) const
{
  QList<Jid> services;
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
  foreach(IRosterItem ritem, ritems)
    if (ritem.itemJid.node().isEmpty())
    {
      if (FDiscovery && (!AIdentity.category.isEmpty() || !AIdentity.type.isEmpty()))
      {
        IDiscoInfo dinfo = FDiscovery->discoInfo(ritem.itemJid);
        foreach(IDiscoIdentity identity, dinfo.identity)
        {
          if ((AIdentity.category.isEmpty() || AIdentity.category == identity.category) &&
              (AIdentity.type.isEmpty() || AIdentity.type == identity.type))
          {
            services.append(ritem.itemJid);
            break;
          }
        }
      }
      else
        services.append(ritem.itemJid);
    }
  return services;
}

QList<Jid> Gateways::serviceContacts(const Jid &AStreamJid, const Jid &AServiceJid) const
{
  QList<Jid> contacts;
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
  foreach(IRosterItem ritem, ritems)
    if (!ritem.itemJid.node().isEmpty() && ritem.itemJid.pDomain()==AServiceJid.pDomain())
      contacts.append(ritem.itemJid);
  return contacts;
}

bool Gateways::changeService(const Jid &AStreamJid, const Jid &AServiceFrom, const Jid &AServiceTo, bool ARemove, bool ASubscribe)
{
  IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (roster && presence && presence->isOpen() && AServiceFrom.isValid() && AServiceTo.isValid() && AServiceFrom.pDomain()!=AServiceTo.pDomain())
  {
    IRosterItem ritemOld = roster->rosterItem(AServiceFrom);
    IRosterItem ritemNew = roster->rosterItem(AServiceTo);

    //Удаляем подписку у старого транспорта
    if (ritemOld.isValid)
    {
      roster->sendSubscription(ritemOld.itemJid,IRoster::Unsubscribe);
      roster->sendSubscription(ritemOld.itemJid,IRoster::Unsubscribed);
    }

    //Разлогиниваемся на старом транспорте
    if (!presence->presenceItems(AServiceFrom).isEmpty())
      sendLogPresence(AStreamJid,AServiceFrom,false);

    //Добавляем новый транспорт в ростер и удаляем старый
    if (!ritemNew.isValid)
      roster->setItem(AServiceTo,ritemOld.name,ritemOld.groups);
    if (ARemove)
      roster->removeItem(ritemOld.itemJid);

    //Добавляем контакты нового транспорта и удаляем старые
    QList<Jid> newItems;
    QList<IRosterItem> ritems = roster->rosterItems();
    foreach(IRosterItem ritem, ritems)
    {
      if (ritem.itemJid.pDomain() == AServiceFrom.pDomain())
      {
        Jid newItemJid = ritem.itemJid;
        newItemJid.setDomain(AServiceTo.domain());
        if (!roster->rosterItem(newItemJid).isValid)
        {
          newItems.append(newItemJid);
          roster->setItem(newItemJid,ritem.name,ritem.groups);
        }
        if (ARemove)
          roster->removeItem(ritem.itemJid);
      }
    }

    //Запрашиваем подписку у нового транспорта
    if (ASubscribe)
    {
      FSubscribeServices.insertMulti(AStreamJid,AServiceTo);
      foreach(Jid newItem, newItems)
        FSubscribeContacts.insertMulti(AStreamJid,newItem);
      roster->sendSubscription(AServiceTo,IRoster::Subscribe);
    }

    return true;
  }
  return false;
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
    if (FPrivateStorageKeep.value(streamJid).contains(serviceJid))
      setKeepConnection(streamJid,serviceJid,logIn);
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
    if (contactJid.node().isEmpty())
    {
      QList<Jid> contactJids = serviceContacts(streamJid,contactJid);
      foreach(Jid contact, contactJids)
        resolveNickName(streamJid,contact);
    }
    else
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
    if (FPrivateStorageKeep.contains(streamJid) && FPrivateStorageKeep.value(streamJid).contains(serviceJid)!=action->isChecked())
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

void Gateways::onChangeActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid serviceFrom = action->data(ADR_SERVICE_JID).toString();
    Jid serviceTo = action->data(ADR_NEW_SERVICE_JID).toString();
    changeService(streamJid,serviceFrom,serviceTo,true,true);
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

      if (FPrivateStorageKeep.contains(streamJid))
      {
        Action *action = new Action(AMenu);
        action->setText(tr("Keep connection"));
        action->setIcon(SYSTEM_ICONSETFILE,IN_KEEP);
        action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
        action->setData(ADR_SERVICE_JID,AIndex->data(RDR_BareJid));
        action->setCheckable(true);
        action->setChecked(FKeepConnections.contains(streamJid,AIndex->data(RDR_BareJid).toString()));
        connect(action,SIGNAL(triggered(bool)),SLOT(onKeepActionTriggered(bool)));
        AMenu->addAction(action,AG_GATEWAYS_LOGIN_ROSTER,true);
      }
    }
  }

  if (FRosterPlugin && FVCardPlugin && (AIndex->type() == RIT_Contact || AIndex->type() == RIT_Agent))
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
    if (roster && roster->isOpen() && roster->rosterItem(contactJid).isValid && roster->rosterItem(contactJid.domain()).isValid)
    {
      Action *action = new Action(AMenu);
      action->setText(contactJid.node().isEmpty() ? tr("Resolve nick names") : tr("Resolve nick name"));
      action->setData(ADR_STREAM_JID,streamJid.full());
      action->setData(ADR_SERVICE_JID,contactJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onResolveActionTriggered(bool)));
      AMenu->addAction(action,AG_GATEWAYS_RESOLVE_ROSTER,true);
    }
  }
}

void Gateways::onPresenceOpened(IPresence *APresence)
{
  if (FPrivateStorage)
    FPrivateStorage->loadData(APresence->streamJid(),PST_GATEWAYS_SERVICES,PSN_GATEWAYS_KEEP);
  FKeepTimer.start(KEEP_INTERVAL);
}

void Gateways::onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline)
{
  if (AStateOnline && FSubscribeServices.contains(AStreamJid,AContactJid.bare()))
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(AStreamJid) : NULL;
    QList<IRosterItem> ritems = roster!=NULL ? roster->rosterItems() : QList<IRosterItem>();
    foreach(IRosterItem ritem, ritems)
      if (ritem.itemJid.pDomain()==AContactJid.pDomain() && ritem.subscription!=SUBSCRIPTION_BOTH && ritem.subscription!=SUBSCRIPTION_TO)
        roster->sendSubscription(ritem.itemJid,IRoster::Subscribe);
  }
}

void Gateways::onPresenceClosed(IPresence *APresence)
{
  FResolveNicks.remove(APresence->streamJid());
  FSubscribeContacts.remove(APresence->streamJid());
  FSubscribeServices.remove(APresence->streamJid());
}

void Gateways::onPresenceRemoved(IPresence *APresence)
{
  FKeepConnections.remove(APresence->streamJid());
  FPrivateStorageKeep.remove(APresence->streamJid());
}

void Gateways::onRosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &/*AText*/)
{
  if (ASubsType == IRoster::Subscribed)
  {
    if (FSubscribeServices.contains(ARoster->streamJid(),AItemJid))
      sendLogPresence(ARoster->streamJid(),AItemJid,true);
  }
  else if(ASubsType == IRoster::Subscribe)
  {
    if (FSubscribeContacts.contains(ARoster->streamJid(),AItemJid))
    {
      ARoster->sendSubscription(AItemJid,IRoster::Subscribed);
      QString subscription = ARoster->rosterItem(AItemJid).subscription;
      if (subscription!=SUBSCRIPTION_BOTH && subscription!=SUBSCRIPTION_TO)
        ARoster->sendSubscription(AItemJid,IRoster::Subscribe);
    }
    else if (FSubscribeServices.contains(ARoster->streamJid(),AItemJid))
      ARoster->sendSubscription(AItemJid,IRoster::Subscribed);
  }
}

void Gateways::onRosterJidAboutToBeChanged(IRoster *ARoster, const Jid &/*AAfter*/)
{
  FKeepConnections.remove(ARoster->streamJid());
  FPrivateStorageKeep.remove(ARoster->streamJid());
}

void Gateways::onKeepTimerTimeout()
{
  QList<Jid> streamJids = FKeepConnections.keys();
  foreach(Jid streamJid, streamJids)
  {
    IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->getRoster(streamJid) : NULL;
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (roster && presence && presence->isOpen())
    {
      QList<Jid> services = FKeepConnections.values(streamJid);
      foreach(Jid service, services)
      {
        if (roster->rosterItem(service).subscription!=SUBSCRIPTION_NONE && presence->presenceItems(service).isEmpty())
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

void Gateways::onDiscoItemsWindowCreated(IDiscoItemsWindow *AWindow)
{
  connect(AWindow->instance(),SIGNAL(treeItemContextMenu(QTreeWidgetItem *, Menu *)),
    SLOT(onDiscoItemContextMenu(QTreeWidgetItem *, Menu *)));
}

void Gateways::onDiscoItemContextMenu(QTreeWidgetItem *ATreeItem, Menu *AMenu)
{
  Jid itemJid = ATreeItem->data(0,DDR_JID).toString();
  QString itemNode = ATreeItem->data(0,DDR_NODE).toString();
  if (itemJid.node().isEmpty() && itemNode.isEmpty())
  {
    IDiscoInfo dinfo = FDiscovery->discoInfo(itemJid,itemNode);
    if (dinfo.error.code<0 && !dinfo.identity.isEmpty())
    {
      Jid streamJid = ATreeItem->data(0,DDR_STREAMJID).toString();
      QList<Jid> services = streamServices(streamJid,dinfo.identity.value(0));
      foreach(Jid service, streamServices(streamJid))
        if (!services.contains(service) && FDiscovery->discoInfo(service).identity.isEmpty())
          services.append(service);
      if (!services.isEmpty() && !services.contains(itemJid))
      {
        Menu *change = new Menu(AMenu);
        change->setTitle(tr("Use instead of"));
        change->setIcon(SYSTEM_ICONSETFILE,IN_CHANGE_TRANSPORT);
        foreach(Jid service, services)
        {
          Action *action = new Action(change);
          action->setText(service.full());
          if (FStatusIcons!=NULL)
            action->setIcon(FStatusIcons->iconByJid(streamJid,service));
          else
            action->setIcon(STATUS_ICONSETFILE,IN_LOG_IN);
          action->setData(ADR_STREAM_JID,streamJid.full());
          action->setData(ADR_SERVICE_JID,service.full());
          action->setData(ADR_NEW_SERVICE_JID,itemJid.full());
          connect(action,SIGNAL(triggered(bool)),SLOT(onChangeActionTriggered(bool)));
          change->addAction(action,AG_DEFAULT,true);
        }
        AMenu->addAction(change->menuAction(),AG_DIWT_DISCOVERY_ACTIONS,true);
      }
    }
  }
}

Q_EXPORT_PLUGIN2(GatewaysPlugin, Gateways)
