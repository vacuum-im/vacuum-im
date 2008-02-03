#include <QDebug>
#include "clientinfo.h"

#define SHC_SOFTWARE                    "/iq[@type='get']/query[@xmlns='" NS_JABBER_VERSION "']"
#define SVN_AUTO_LOAD_SOFTWARE          "autoLoadSoftwareInfo"
#define SOFTWARE_INFO_TIMEOUT           10000
#define LAST_ACTIVITY_TIMEOUT           10000

#define ADR_STREAM_JID                  Action::DR_StreamJid
#define ADR_CONTACT_JID                 Action::DR_Parametr1

#define IN_CLIENTINFO                   "psi/help"

ClientInfo::ClientInfo()
{
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FStanzaProcessor = NULL;
  FRostersViewPlugin = NULL;
  FRostersModelPlugin = NULL;
  FSettingsPlugin = NULL;
  FMultiUserChatPlugin = NULL;
  FDiscovery = NULL;

  FOptions = 0;
  FSoftwareHandler = 0;
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

  plugin = APluginManager->getPlugins("IRostersModelPlugin").value(0,NULL);
  if (plugin)
  {
    FRostersModelPlugin = qobject_cast<IRostersModelPlugin *>(plugin->instance());
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
  FSoftwareHandler = FStanzaProcessor->insertHandler(this,SHC_SOFTWARE,IStanzaProcessor::DirectionIn);

  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(IRosterIndex *,Menu*)),
      SLOT(onRostersViewContextMenu(IRosterIndex *,Menu *)));
    connect(FRostersViewPlugin->rostersView(),SIGNAL(labelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
  }

  if (FRostersModelPlugin)
  {
    FRostersModelPlugin->rostersModel()->insertDefaultDataHolder(this);
    connect(this,SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onSoftwareInfoChanged(const Jid &)));
    connect(this,SIGNAL(lastActivityChanged(const Jid &)),SLOT(onLastActivityChanged(const Jid &)));
  }

  if (FSettingsPlugin)
    FSettingsPlugin->appendOptionsHolder(this);

  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertFeatureHandler(NS_JABBER_VERSION,this,DFO_DEFAULT);
    FDiscovery->insertFeatureHandler(NS_JABBER_LAST,this,DFO_DEFAULT);
  }

  return true;
}

bool ClientInfo::readStanza(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FSoftwareHandler)
  {
    AAccept = true;
    Stanza iq("iq");
    iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
    QDomElement elem = iq.addElement("query",NS_JABBER_VERSION);
    elem.appendChild(iq.createElement("name")).appendChild(iq.createTextNode("Vacuum"));
    elem.appendChild(iq.createElement("version")).appendChild(iq.createTextNode("0.0.0"));
    FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
  }
  return false;
}

void ClientInfo::iqStanza(const Jid &/*AStreamJid*/, const Stanza &AStanza)
{
  QDomElement elem = AStanza.firstElement("query");
  if (elem.namespaceURI() == NS_JABBER_VERSION && FSoftwareId.contains(AStanza.id()))
  {
    Jid contactJid = FSoftwareId.take(AStanza.id());
    SoftwareItem &software = FSoftwareItems[contactJid];
    if (AStanza.type() == "result")
    {
      software.name = elem.firstChildElement("name").text();
      software.version = elem.firstChildElement("version").text();
      software.os = elem.firstChildElement("os").text();
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
  else if (elem.namespaceURI() == NS_JABBER_LAST && FActivityId.contains(AStanza.id()))
  {
    Jid contactJid = FActivityId.take(AStanza.id());
    ActivityItem &activity = FActivityItems[contactJid];
    if (AStanza.type() == "result")
    {
      activity.requestTime = QDateTime::currentDateTime();
      activity.datetime = activity.requestTime.addSecs(0-elem.attribute("seconds","0").toInt());
      activity.text = elem.text();
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      activity.requestTime = QDateTime::currentDateTime();
      activity.datetime = QDateTime();
      activity.text = err.message();
    }
    emit lastActivityChanged(contactJid);
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
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    emit lastActivityChanged(contactJid);
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
  else
    return QVariant();
}

bool ClientInfo::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature == NS_JABBER_VERSION || AFeature == NS_JABBER_LAST)
  {
    showClientInfo(ADiscoInfo.contactJid,AStreamJid);
    return true;
  }
  return false;
}

Action *ClientInfo::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (AFeature == NS_JABBER_VERSION)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Client version"));
    action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_CONTACT_JID,ADiscoInfo.contactJid.full());
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
      connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
      return action;
    }
  }
  return NULL;
}

void ClientInfo::showClientInfo(const Jid &AContactJid, const Jid &AStreamJid)
{
  if (AContactJid.isValid() && AStreamJid.isValid())
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
          IRosterItem *item = roster->item(AContactJid);
          if (item)
            contactName = item->name();
        }
      }
      dialog = new ClientInfoDialog(contactName, AContactJid,AStreamJid,this);
      FClientInfoDialogs.insert(AContactJid,dialog);
      connect(dialog,SIGNAL(clientInfoDialogClosed(const Jid &)),SLOT(onClientInfoDialogClosed(const Jid &)));
      dialog->show();
    }
    else
      dialog->activateWindow();
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

bool ClientInfo::requestSoftwareInfo(const Jid &AContactJid, const Jid &AStreamJid)
{
  bool sended = false;
  if (AStreamJid.isValid() && AContactJid.isValid() && !FSoftwareId.values().contains(AContactJid))
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

bool ClientInfo::requestLastActivity(const Jid &AContactJid, const Jid &AStreamJid)
{
  bool sended = false;
  if (AStreamJid.isValid() && AContactJid.isValid() && !FActivityId.values().contains(AContactJid))
  {
    Stanza iq("iq");
    iq.addElement("query",NS_JABBER_LAST);
    iq.setTo(AContactJid.eBare()).setId(FStanzaProcessor->newId()).setType("get");
    sended = FStanzaProcessor->sendIqStanza(this,AStreamJid,iq,LAST_ACTIVITY_TIMEOUT);
    if (sended)
    {
      FActivityId.insert(iq.id(),AContactJid);
    }
  }
  return sended;
}

QDateTime ClientInfo::lastActivityRequest(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).requestTime;
}

QDateTime ClientInfo::lastActivityTime(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).datetime;
}

QString ClientInfo::lastActivityText(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).text;
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
  dfeature.name = tr("Client version");
  dfeature.description = tr("Request contacts client version");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = false;
  dfeature.icon = Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_CLIENTINFO);
  dfeature.var = NS_JABBER_LAST;
  dfeature.name = tr("Last activity");
  dfeature.description = tr("Request contacts last activity");
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
    if (checkOption(AutoLoadSoftwareInfo) && software.status == SoftwareNotLoaded)
      requestSoftwareInfo(AContactJid,AStreamJid);
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
  }
}

void ClientInfo::onRostersViewContextMenu(IRosterIndex *AIndex, Menu *AMenu)
{
  if (AIndex->type() == RIT_Contact || AIndex->type() == RIT_MyResource)
  {
    Jid streamJid = AIndex->data(RDR_StreamJid).toString();
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (presence && presence->xmppStream()->isOpen() && AIndex->data(RDR_Show).toInt() == IPresence::Offline)
    {
      Action *action = new Action(AMenu);
      action->setText(tr("Last activity"));
      action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
      action->setData(ADR_STREAM_JID,AIndex->data(RDR_StreamJid));
      action->setData(ADR_CONTACT_JID,AIndex->data(RDR_Jid));
      connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
      AMenu->addAction(action,AG_CLIENTINFO_ROSTER,true);
    }
  }
}

void ClientInfo::onMultiUserContextMenu(IMultiUserChatWindow * /*AWindow*/, IMultiUser *AUser, Menu *AMenu)
{
  Action *action = new Action(AMenu);
  action->setText(tr("Client version"));
  action->setIcon(SYSTEM_ICONSETFILE,IN_CLIENTINFO);
  action->setData(ADR_STREAM_JID,AUser->data(MUDR_STREAMJID));
  if (!AUser->data(MUDR_REALJID).toString().isEmpty())
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_REALJID));
  else
    action->setData(ADR_CONTACT_JID,AUser->data(MUDR_CONTACTJID));
  connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
  AMenu->addAction(action,AG_CLIENTINFO_MUCM,true);
}

void ClientInfo::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if ((ALabelId == RLID_DISPLAY || ALabelId == RLID_FOOTER_TEXT) && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->data(RDR_Jid).toString();
    
    if (hasSoftwareInfo(contactJid))
      AToolTips.insert(TTO_SOFTWARE_INFO,tr("Client: %1 %2").arg(softwareName(contactJid)).arg(softwareVersion(contactJid)));
    
    if (hasLastActivity(contactJid) && AIndex->data(RDR_Show).toInt() == IPresence::Offline)
      AToolTips.insert(TTO_LAST_ACTIVITY,tr("Offline since: %1").arg(lastActivityTime(contactJid).toString()));
  }
}

void ClientInfo::onClientInfoActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid contactJid = action->data(ADR_CONTACT_JID).toString();
    if (streamJid.isValid() && contactJid.isValid())
      showClientInfo(contactJid,streamJid);
  }
}

void ClientInfo::onClientInfoDialogClosed(const Jid &AContactJid)
{
  FClientInfoDialogs.remove(AContactJid);
}

void ClientInfo::onSettingsOpened()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(CLIENTINFO_UUID);
  setOption(AutoLoadSoftwareInfo, settings->value(SVN_AUTO_LOAD_SOFTWARE,true).toBool());
}

void ClientInfo::onSettingsClosed()
{
  ISettings *settings = FSettingsPlugin->settingsForPlugin(CLIENTINFO_UUID);
  settings->setValue(SVN_AUTO_LOAD_SOFTWARE,checkOption(AutoLoadSoftwareInfo));
}

void ClientInfo::onRosterRemoved(IRoster *ARoster)
{
  deleteSoftwareDialogs(ARoster->streamJid());
}

void ClientInfo::onSoftwareInfoChanged(const Jid &AContactJid)
{
  IRosterIndexList indexList;
  IRostersModel *model = FRostersModelPlugin->rostersModel();
  QStringList streamJids = model->streams();
  foreach(QString streamJid, streamJids)
  {
    IRosterIndexList indexList = model->getContactIndexList(streamJid,AContactJid);
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
  IRostersModel *model = FRostersModelPlugin->rostersModel();
  QStringList streamJids = model->streams();
  foreach(QString streamJid, streamJids)
  {
    IRosterIndexList indexList = model->getContactIndexList(streamJid,AContactJid);
    foreach(IRosterIndex *index, indexList)
    {
      emit dataChanged(index,RDR_LAST_ACTIVITY_TIME);
      emit dataChanged(index,RDR_LAST_ACTIVITY_TEXT);
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
