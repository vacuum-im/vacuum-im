#include <QDebug>
#include "clientinfo.h"

#include <QDateTime>

#define SHC_SOFTWARE_VERSION            "/iq[@type='get']/query[@xmlns='" NS_JABBER_VERSION "']"
#define SHC_ENTITY_TIME                 "/iq[@type='get']/time[@xmlns='" NS_XMPP_TIME "']"

#define SVN_AUTO_LOAD_SOFTWARE          "autoLoadSoftwareInfo"

#define SOFTWARE_INFO_TIMEOUT           10000
#define LAST_ACTIVITY_TIMEOUT           10000
#define ENTITY_TIME_TIMEOUT             10000

#define ADR_STREAM_JID                  Action::DR_StreamJid
#define ADR_CONTACT_JID                 Action::DR_Parametr1
#define ADR_INFO_TYPES                  Action::DR_Parametr2

#define IN_CLIENTINFO                   "psi/help"

ClientInfo::ClientInfo()
{
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FStanzaProcessor = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FSettingsPlugin = NULL;
  FMultiUserChatPlugin = NULL;
  FDiscovery = NULL;

  FOptions = 0;
  FVersionHandler = 0;
  FTimeHandler = 0;
}

ClientInfo::~ClientInfo()
{

}

void ClientInfo::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Request contacts client information");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Client Info"); 
  APluginInfo->uid = CLIENTINFO_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ClientInfo::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("IStanzaProcessor").value(0,NULL);
  if (plugin) 
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance()); 
    if (FRosterPlugin)
    {
      connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
    }
  }

  plugin = APluginManager->getPlugins("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance()); 
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)),
        SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
    }
  }

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IRostersModel").value(0,NULL);
  if (plugin)
  {
    FRostersModel = qobject_cast<IRostersModel*>(plugin->instance());
    if (FRostersModel)
    {
      connect(this,SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onSoftwareInfoChanged(const Jid &)));
      connect(this,SIGNAL(lastActivityChanged(const Jid &)),SLOT(onLastActivityChanged(const Jid &)));
      connect(this,SIGNAL(entityTimeChanged(const Jid &)),SLOT(onEntityTimeChanged(const Jid &)));
    }
  }

  plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin) 
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
    }
  }

  plugin = APluginManager->getPlugins("IMultiUserChatPlugin").value(0,NULL);
  if (plugin)
  {
    FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
    if (FMultiUserChatPlugin)
    {
      connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)),
        SLOT(onMultiUserContextMenu(IMultiUserChatWindow *,IMultiUser *, Menu *)));
    }
  }

  plugin = APluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
  }

  return FStanzaProcessor != NULL;
}

bool ClientInfo::initObjects()
{
  FVersionHandler = FStanzaProcessor->insertHandler(this,SHC_SOFTWARE_VERSION,IStanzaProcessor::DirectionIn);
  FTimeHandler = FStanzaProcessor->insertHandler(this,SHC_ENTITY_TIME,IStanzaProcessor::DirectionIn);

  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *,Menu*)),
      SLOT(onRostersViewContextMenu(IRosterIndex *,Menu *)));
    connect(FRostersViewPlugin->rostersView(),SIGNAL(labelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
  }

  if (FRostersModel)
  {
    FRostersModel->insertDefaultDataHolder(this);
  }

  if (FSettingsPlugin)
    FSettingsPlugin->appendOptionsHolder(this);

  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_JABBER_VERSION,this,DFO_DEFAULT);
    FDiscovery->insertFeatureHandler(NS_JABBER_LAST,this,DFO_DEFAULT);
    FDiscovery->insertFeatureHandler(NS_XMPP_TIME,this,DFO_DEFAULT);
  }

  return true;
}

bool ClientInfo::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FVersionHandler)
  {
    AAccept = true;
    Stanza iq("iq");
    iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
    QDomElement elem = iq.addElement("query",NS_JABBER_VERSION);
    elem.appendChild(iq.createElement("name")).appendChild(iq.createTextNode(CLIENT_NAME));
    elem.appendChild(iq.createElement("version")).appendChild(iq.createTextNode(CLIENT_VERSION));
    FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
  }
  else if (AHandlerId == FTimeHandler)
  {
    QDateTime lcl = QDateTime::currentDateTime();
    QDateTime utc = lcl.toUTC();
    lcl.setTimeSpec(Qt::UTC);
    int minToUtc = lcl<utc ? lcl.secsTo(utc)/60 : utc.secsTo(lcl)/60;
    int hourToUtc = minToUtc/60;
    minToUtc = minToUtc-(hourToUtc*60);
    QString tzo = QString("%1%2:%3").arg(lcl<utc ? "-" : "+").arg(hourToUtc,2,10,QLatin1Char('0')).arg(minToUtc,2,10,QLatin1Char('0'));

    AAccept = true;
    Stanza iq("iq");
    iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
    QDomElement elem = iq.addElement("time",NS_XMPP_TIME);
    elem.appendChild(iq.createElement("tzo")).appendChild(iq.createTextNode(tzo));
    elem.appendChild(iq.createElement("utc")).appendChild(iq.createTextNode(utc.toString(Qt::ISODate)+"Z"));
    FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
  }
  return false;
}

void ClientInfo::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  if (FSoftwareId.contains(AStanza.id()))
  {
    Jid contactJid = FSoftwareId.take(AStanza.id());
    SoftwareItem &software = FSoftwareItems[contactJid];
    if (AStanza.type() == "result")
    {
      QDomElement query = AStanza.firstElement("query");
      software.name = query.firstChildElement("name").text();
      software.version = query.firstChildElement("version").text();
      software.os = query.firstChildElement("os").text();
      software.status = SoftwareLoaded;
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      software.name = err.message();
      software.version.clear();
      software.os.clear();
      software.status = SoftwareError;
    }
    emit softwareInfoChanged(contactJid);
  }
  else if (FActivityId.contains(AStanza.id()))
  {
    Jid contactJid = FActivityId.take(AStanza.id());
    ActivityItem &activity = FActivityItems[contactJid];
    if (AStanza.type() == "result")
    {
      QDomElement query = AStanza.firstElement("query");
      activity.datetime = QDateTime::currentDateTime().addSecs(0-query.attribute("seconds","0").toInt());
      activity.text = query.text();
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      activity.datetime = QDateTime();
      activity.text = err.message();
    }
    emit lastActivityChanged(contactJid);
  }
  else if (FTimeId.contains(AStanza.id()))
  {
    Jid contactJid = FTimeId.take(AStanza.id());
    QDomElement time = AStanza.firstElement("time");
    QString tzo = time.firstChildElement("tzo").text();
    QString utc = time.firstChildElement("utc").text();
    if (AStanza.type() == "result" && !tzo.isEmpty() && !utc.isEmpty())
    {
      TimeItem &tItem = FTimeItems[contactJid];
      tItem.tzoSecs = tzo.startsWith("-") ? 1 : -1;
      tzo.remove(0,1);
      QTime tzoT = QTime::fromString(tzo,"hh:mm");
      QDateTime utcDT = QDateTime::fromString(utc,Qt::ISODate);
      tItem.ping = tItem.ping - QTime::currentTime().msecsTo(QTime(0,0,0,0));
      tItem.tzoSecs =  tItem.tzoSecs * tzoT.secsTo(QTime(0,0,0));
      tItem.utcSecs = utcDT.secsTo(QDateTime::currentDateTime().toUTC()) - tItem.ping/2000;
    }
    else
      FTimeItems.remove(contactJid);
    emit entityTimeChanged(contactJid);
  }
}

void ClientInfo::iqStanzaTimeOut(const QString &AId)
{
  if (FSoftwareId.contains(AId))
  {
    Jid contactJid = FSoftwareId.take(AId);
    SoftwareItem &software = FSoftwareItems[contactJid];
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    software.name = err.message();
    software.version.clear();
    software.os.clear();
    software.status = SoftwareError;
    emit softwareInfoChanged(contactJid);
  }
  else if (FActivityId.contains(AId))
  {
    Jid contactJid = FActivityId.take(AId);
    emit lastActivityChanged(contactJid);
  }
  else if (FTimeId.contains(AId))
  {
    Jid contactJid = FTimeId.take(AId);
    FTimeItems.remove(contactJid);
    emit entityTimeChanged(contactJid);
  }
}

QWidget *ClientInfo::optionsWidget(const QString &ANode, int &AOrder)
{
  if (ANode == ON_ROSTER)
  {
    AOrder = OO_CLIENTINFO;
    FOptionsWidget = new OptionsWidget(this);
    return FOptionsWidget;
  }
  return NULL;
}

QList<int> ClientInfo::roles() const
{
  static QList<int> indexRoles = QList<int>()
    << RDR_CLIENT_NAME << RDR_CLIENT_VERSION << RDR_CLIENT_OS 
    << RDR_LAST_ACTIVITY_TIME << RDR_LAST_ACTIVITY_TEXT
    << RDR_ENTITY_TIME;
  return indexRoles;
}

QList<int> ClientInfo::types() const
{
  static QList<int> indexTypes =  QList<int>() 
    << RIT_Contact << RIT_Agent << RIT_MyResource;
  return indexTypes;
}

QVariant ClientInfo::data(const IRosterIndex *AIndex, int ARole) const
{
  Jid contactJid = AIndex->data(RDR_Jid).toString();
  if (ARole == RDR_CLIENT_NAME)
    return hasSoftwareInfo(contactJid) ? softwareName(contactJid) : QVariant();
  else if (ARole == RDR_CLIENT_VERSION)
    return hasSoftwareInfo(contactJid) ? softwareVersion(contactJid) : QVariant();
  else if (ARole == RDR_CLIENT_OS)
    return hasSoftwareInfo(contactJid) ? softwareOs(contactJid) : QVariant();
  else if (ARole == RDR_LAST_ACTIVITY_TIME)
    return hasLastActivity(contactJid) ? lastActivityTime(contactJid) : QVariant();
  else if (ARole == RDR_LAST_ACTIVITY_TEXT)
    return hasLastActivity(contactJid) ? lastActivityText(contactJid) : QVariant();
  else if (ARole == RDR_ENTITY_TIME)
    return hasEntityTime(contactJid) ? entityTime(contactJid) : QVariant();
  else
    return QVariant();
}

bool ClientInfo::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_JABBER_VERSION)
  {
    showClientInfo(AStreamJid,ADiscoInfo.contactJid,IClientInfo::SoftwareVersion);
    return true;
  }
  else if (AFeature == NS_JABBER_LAST)
  {
    showClientInfo(AStreamJid,ADiscoInfo.contactJid,IClientInfo::LastActivity);
    return true;
  }
  else if (AFeature == NS_XMPP_TIME)
  {
    showClientInfo(AStreamJid,ADiscoInfo.contactJid,IClientInfo::EntityTime);
    return true;
  }
  return false;
}

Action *ClientInfo::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(AStreamJid) : NULL;
  if (presence && presence->isOpen())
  {
    if (AFeature == NS_JABBER_VERSION)
    {
      Action *action = new Action(AParent);
      action->setText(tr("Software version"));
      action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
      action->setData(ADR_INFO_TYPES,IClientInfo::SoftwareVersion);
      connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
      return action;
    }
    else if (AFeature == NS_JABBER_LAST)
    {
      if (FPresencePlugin && !FPresencePlugin->isContactOnline(ADiscoInfo.contactJid))
      {
        Action *action = new Action(AParent);
        action->setText(tr("Last activity"));
        action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
        action->setData(ADR_INFO_TYPES,IClientInfo::LastActivity);
        connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
        return action;
      }
    }
    else if (AFeature == NS_XMPP_TIME)
    {
      Action *action = new Action(AParent);
      action->setText(tr("Entity time"));
      action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
      action->setData(ADR_INFO_TYPES,IClientInfo::EntityTime);
      connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
      return action;
    }
  }
  return NULL;
}

void ClientInfo::showClientInfo(const Jid &AStreamJid, const Jid &AContactJid, int AInfoTypes)
{
  if (AInfoTypes>0 && AContactJid.isValid() && AStreamJid.isValid())
  {
    ClientInfoDialog *dialog = FClientInfoDialogs.value(AContactJid,NULL);
    if (!dialog)
    {
      QString contactName = AContactJid.node();
      if (FRosterPlugin)
      {
        IRoster *roster = FRosterPlugin->getRoster(AStreamJid);
        if (roster)
        {
          IRosterItem ritem = roster->rosterItem(AContactJid);
          if (ritem.isValid)
            contactName = ritem.name;
        }
      }
      dialog = new ClientInfoDialog(this,AStreamJid,AContactJid,contactName,AInfoTypes);
      connect(dialog,SIGNAL(clientInfoDialogClosed(const Jid &)),SLOT(onClientInfoDialogClosed(const Jid &)));
      FClientInfoDialogs.insert(AContactJid,dialog);
      dialog->show();
    }
    else
    {
      dialog->setInfoTypes(dialog->infoTypes() | AInfoTypes);
      dialog->activateWindow();
    }
  }
}

bool ClientInfo::checkOption(IClientInfo::Option AOption) const
{
  return (FOptions & AOption) > 0;
}

void ClientInfo::setOption(IClientInfo::Option AOption, bool AValue)
{
  bool changed = checkOption(AOption) != AValue;
  if (changed)
  {
    AValue ? FOptions |= AOption : FOptions &= ~AOption;
    emit optionChanged(AOption,AValue);
  }
}

bool ClientInfo::hasSoftwareInfo(const Jid &AContactJid) const
{
  return FSoftwareItems.value(AContactJid).status == SoftwareLoaded;
}

bool ClientInfo::requestSoftwareInfo(const Jid &AStreamJid, const Jid &AContactJid)
{
  bool sended = FSoftwareId.values().contains(AContactJid);
  if (!sended && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.addElement("query",NS_JABBER_VERSION);
    iq.setTo(AContactJid.eFull()).setId(FStanzaProcessor->newId()).setType("get");
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,SOFTWARE_INFO_TIMEOUT);
    if (sended)
    {
      FSoftwareId.insert(iq.id(),AContactJid);
      FSoftwareItems[AContactJid].status = SoftwareLoading;
    }
  }
  return sended;
}

int ClientInfo::softwareStatus(const Jid &AContactJid) const
{
  return FSoftwareItems.value(AContactJid).status;
}

QString ClientInfo::softwareName(const Jid &AContactJid) const
{
  return FSoftwareItems.value(AContactJid).name;
}

QString ClientInfo::softwareVersion(const Jid &AContactJid) const
{
  return FSoftwareItems.value(AContactJid).version;
}

QString ClientInfo::softwareOs(const Jid &AContactJid) const
{
  return FSoftwareItems.value(AContactJid).os;
}

bool ClientInfo::hasLastActivity(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).datetime.isValid();
}

bool ClientInfo::requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid)
{
  bool sended = FActivityId.values().contains(AContactJid);
  if (!sended && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.addElement("query",NS_JABBER_LAST);
    iq.setTo(AContactJid.eBare()).setId(FStanzaProcessor->newId()).setType("get");
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,LAST_ACTIVITY_TIMEOUT);
    if (sended)
      FActivityId.insert(iq.id(),AContactJid);
  }
  return sended;
}

QDateTime ClientInfo::lastActivityTime(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).datetime;
}

QString ClientInfo::lastActivityText(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).text;
}

bool ClientInfo::hasEntityTime(const Jid &AContactJid) const
{
  return FTimeItems.value(AContactJid).ping >= 0;
}

bool ClientInfo::requestEntityTime(const Jid &AStreamJid, const Jid &AContactJid)
{
  bool sended = FTimeId.values().contains(AContactJid);
  if (!sended && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.addElement("time",NS_XMPP_TIME);
    iq.setTo(AContactJid.eFull()).setType("get").setId(FStanzaProcessor->newId());
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,ENTITY_TIME_TIMEOUT);
    if (sended)
    {
      TimeItem &tItem = FTimeItems[AContactJid];
      tItem.ping = QTime::currentTime().msecsTo(QTime(0,0,0,0));
      FTimeId.insert(iq.id(),AContactJid);
      emit entityTimeChanged(AContactJid);
    }
  }
  return sended;
}

QDateTime ClientInfo::entityTime(const Jid &AContactJid) const
{
  if (hasEntityTime(AContactJid))
  {
    TimeItem tItem = FTimeItems.value(AContactJid);
    QDateTime utc = QDateTime::currentDateTime().toUTC().addSecs(tItem.utcSecs).addSecs(tItem.tzoSecs);
    return utc;
  }
  return QDateTime();
}

int ClientInfo::entityTimeDelta(const Jid &AContactJid) const
{
  if (hasEntityTime(AContactJid))
    return FTimeItems.value(AContactJid).utcSecs;
  return 0;
}

int ClientInfo::entityTimePing(const Jid &AContactJid) const
{
  return FTimeItems.value(AContactJid).ping;
}

void ClientInfo::deleteSoftwareDialogs(const Jid &AStreamJid)
{
  foreach(ClientInfoDialog *dialog, FClientInfoDialogs)
    if (dialog->streamJid() == AStreamJid)
      dialog->deleteLater();
}

void ClientInfo::registerDiscoFeatures()
{
  IDiscoFeature dfeature;

  dfeature.active = true;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_CLIENTINFO);
  dfeature.var = NS_JABBER_VERSION;
  dfeature.name = tr("Software version");
  dfeature.description = tr("Request contacts software version");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_CLIENTINFO);
  dfeature.var = NS_JABBER_LAST;
  dfeature.name = tr("Last activity");
  dfeature.description = tr("Request contacts last activity");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = true;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_CLIENTINFO);
  dfeature.var = NS_XMPP_TIME;
  dfeature.name = tr("Entity time");
  dfeature.description = tr("Request the local time of an entity");
  FDiscovery->insertDiscoFeature(dfeature);
}

void ClientInfo::onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline)
{
  if (AStateOnline)
  {
    if (FActivityItems.contains(AContactJid))
    {
      FActivityItems.remove(AContactJid);
      emit lastActivityChanged(AContactJid);
    }
    SoftwareItem &software = FSoftwareItems[AContactJid];
    if (checkOption(AutoLoadSoftwareVersion) && software.status == SoftwareNotLoaded)
      requestSoftwareInfo(AStreamJid,AContactJid);
  }
  else
  {
    if (FSoftwareItems.contains(AContactJid))
    {
      SoftwareItem &software = FSoftwareItems[AContactJid];
      if (software.status == SoftwareLoading)
        FSoftwareId.remove(FSoftwareId.key(AContactJid));
      FSoftwareItems.remove(AContactJid);
      emit softwareInfoChanged(AContactJid);
    }
    if (FActivityItems.contains(AContactJid))
    {
      FActivityItems.remove(AContactJid);
      emit lastActivityChanged(AContactJid);
    }
    if (FTimeItems.contains(AContactJid))
    {
      FTimeItems.remove(AContactJid);
      emit entityTimeChanged(AContactJid);
    }
  }
}

void ClientInfo::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_Contact || AIndex->type() == RIT_Agent || AIndex->type() == RIT_MyResource)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (presence && presence->xmppStream()->isOpen() && AIndex->data(RDR_Show).toInt() == IPresence::Offline
      && !FDiscovery->discoInfo(AIndex->data(RDR_Jid).toString()).features.contains(NS_JABBER_LAST))
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Last activity"));
      action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
      action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
      action->setData(ADR_CONTACT_JID,AIndex->data(RDR_Jid));
      action->setData(ADR_INFO_TYPES,IClientInfo::LastActivity);
      connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
      AMenu->addAction(action,AG_CLIENTINFO_ROSTER,true);
    }
  }
}

void ClientInfo::onMultiUserContextMenu(IMultiUserChatWindow * /*AWindow*/, IMultiUser *AUser, Menu *AMenu)
{
  Action *action = new Action(AMenu);
  action->setText(tr("Software version"));
  action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
  action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAMJID));
  if (!AUser->data(MUDR_REALJID).toString().isEmpty())
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REALJID));
  else
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_CONTACTJID));
  action->setData(ADR_INFO_TYPES,IClientInfo::SoftwareVersion);
  connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
  AMenu->addAction(action,AG_MUCM_CLIENTINFO,true);

  action = new Action(AMenu);
  action->setText(tr("Entity time"));
  action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
  action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAMJID));
  if (!AUser->data(MUDR_REALJID).toString().isEmpty())
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REALJID));
  else
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_CONTACTJID));
  action->setData(ADR_INFO_TYPES,IClientInfo::EntityTime);
  connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
  AMenu->addAction(action,AG_MUCM_CLIENTINFO,true);
}

void ClientInfo::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if ((ALabelId == RLID_DISPLAY || ALabelId == RLID_FOOTER_TEXT) && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    
    if (hasSoftwareInfo(contactJid))
      AToolTips.insert(TTO_SOFTWARE_INFO,tr("Software: %1 %2").arg(softwareName(contactJid)).arg(softwareVersion(contactJid)));
    
    if (hasLastActivity(contactJid) && AIndex->data(RDR_Show).toInt() == IPresence::Offline)
      AToolTips.insert(TTO_LAST_ACTIVITY,tr("Offline since: %1").arg(lastActivityTime(contactJid).toString()));
    
    if (hasEntityTime(contactJid))
      AToolTips.insert(TTO_ENTITY_TIME,tr("Entity time: %1").arg(entityTime(contactJid).time().toString()));
  }
}

void ClientInfo::onClientInfoActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    int infoTypes = action->data(ADR_INFO_TYPES).toInt();
    showClientInfo(streamJid,contactJid,infoTypes);
  }
}

void ClientInfo::onClientInfoDialogClosed(const Jid &AContactJid)
{
  FClientInfoDialogs.remove(AContactJid);
}

void ClientInfo::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(CLIENTINFO_UUID);
  setOption(AutoLoadSoftwareVersion, settings->value(SVN_AUTO_LOAD_SOFTWARE,true).toBool());
}

void ClientInfo::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(CLIENTINFO_UUID);
  settings->setValue(SVN_AUTO_LOAD_SOFTWARE,checkOption(AutoLoadSoftwareVersion));
}

void ClientInfo::onRosterRemoved(IRoster *ARoster)
{
  deleteSoftwareDialogs(ARoster->streamJid());
}

void ClientInfo::onSoftwareInfoChanged(const Jid &AContactJid)
{
  IRosterIndexList indexList;
  QList<Jid> streamJids = FRostersModel->streams();
  foreach(Jid streamJid, streamJids)
  {
    IRosterIndexList indexList = FRostersModel->getContactIndexList(streamJid,AContactJid);
    foreach(IRosterIndex *index, indexList)
    {
      emit dataChanged(index,RDR_CLIENT_NAME);
      emit dataChanged(index,RDR_CLIENT_VERSION);
      emit dataChanged(index,RDR_CLIENT_OS);
    }
  }
}

void ClientInfo::onLastActivityChanged(const Jid &AContactJid)
{
  IRosterIndexList indexList;
  QList<Jid> streamJids = FRostersModel->streams();
  foreach(Jid streamJid, streamJids)
  {
    IRosterIndexList indexList = FRostersModel->getContactIndexList(streamJid,AContactJid);
    foreach(IRosterIndex *index, indexList)
    {
      emit dataChanged(index,RDR_LAST_ACTIVITY_TIME);
      emit dataChanged(index,RDR_LAST_ACTIVITY_TEXT);
    }
  }
}

void ClientInfo::onEntityTimeChanged( const Jid &AContactJid )
{
  IRosterIndexList indexList;
  QList<Jid> streamJids = FRostersModel->streams();
  foreach(Jid streamJid, streamJids)
  {
    IRosterIndexList indexList = FRostersModel->getContactIndexList(streamJid,AContactJid);
    foreach(IRosterIndex *index, indexList)
    {
      emit dataChanged(index,RDR_ENTITY_TIME);
    }
  }
}

void ClientInfo::onOptionsDialogAccepted()
{
  FOptionsWidget->apply();
  emit optionsAccepted();
}

void ClientInfo::onOptionsDialogRejected()
{
  emit optionsRejected();
}

Q_EXPORT_PLUGIN2(ClientInfoPlugin, ClientInfo)
