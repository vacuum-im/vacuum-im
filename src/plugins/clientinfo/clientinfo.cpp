#include "clientinfo.h"

#include <QApplication>

#ifdef SVNINFO
# include "svninfo.h"
#else
# define SVN_DATE         ""
# define SVN_REVISION     0
#endif

#if defined(Q_OS_UNIX)
# include <sys/utsname.h>
#endif

#define SHC_SOFTWARE_VERSION            "/iq[@type='get']/query[@xmlns='" NS_JABBER_VERSION "']"
#define SHC_ENTITY_TIME                 "/iq[@type='get']/time[@xmlns='" NS_XMPP_TIME "']"

#define SOFTWARE_INFO_TIMEOUT           10000
#define LAST_ACTIVITY_TIMEOUT           10000
#define ENTITY_TIME_TIMEOUT             10000

#define ADR_STREAM_JID                  Action::DR_StreamJid
#define ADR_CONTACT_JID                 Action::DR_Parametr1
#define ADR_INFO_TYPES                  Action::DR_Parametr2

#define FORM_FIELD_SOFTWARE             "software"
#define FORM_FIELD_SOFTWARE_VERSION     "software_version"
#define FORM_FIELD_OS                   "os"
#define FORM_FIELD_OS_VERSION           "os_version"

ClientInfo::ClientInfo()
{
  FRosterPlugin = NULL;
  FPresencePlugin = NULL;
  FStanzaProcessor = NULL;
  FRostersViewPlugin = NULL;
  FRostersModel = NULL;
  FMultiUserChatPlugin = NULL;
  FDiscovery = NULL;
  FDataForms = NULL;
  FMainWindowPlugin = NULL;

  FVersionHandle = 0;
  FTimeHandle = 0;
}

ClientInfo::~ClientInfo()
{

}

void ClientInfo::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Client Info");
  APluginInfo->description = tr("Request contact client information");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
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
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
    }
  }

  plugin = APluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
  {
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
  {
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
  }

  return FStanzaProcessor != NULL;
}

bool ClientInfo::initObjects()
{
  if (FStanzaProcessor)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.priority = SHP_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionIn;
    
    shandle.conditions.append(SHC_SOFTWARE_VERSION);
    FVersionHandle = FStanzaProcessor->insertStanzaHandle(shandle);

    shandle.conditions.clear();
    shandle.conditions.append(SHC_ENTITY_TIME);
    FTimeHandle = FStanzaProcessor->insertStanzaHandle(shandle);
  }

  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(contextMenu(IRosterIndex *,Menu*)),
      SLOT(onRostersViewContextMenu(IRosterIndex *,Menu *)));
    connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)),
      SLOT(onRosterLabelToolTips(IRosterIndex *, int , QMultiMap<int,QString> &)));
  }

  if (FRostersModel)
  {
    FRostersModel->insertDefaultDataHolder(this);
  }

  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertDiscoHandler(this);
    FDiscovery->insertFeatureHandler(NS_JABBER_VERSION,this,DFO_DEFAULT);
    FDiscovery->insertFeatureHandler(NS_JABBER_LAST,this,DFO_DEFAULT);
    FDiscovery->insertFeatureHandler(NS_XMPP_TIME,this,DFO_DEFAULT);
  }

  if (FDataForms)
  {
    FDataForms->insertLocalizer(this,DATA_FORM_SOFTWAREINFO);
  }

  if (FMainWindowPlugin)
  {
    Action *aboutQt = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
    aboutQt->setText(tr("About Qt"));
    aboutQt->setIcon(RSR_STORAGE_MENUICONS,MNI_CLIENTINFO_ABOUT_QT);
    connect(aboutQt,SIGNAL(triggered()),QApplication::instance(),SLOT(aboutQt()));
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(aboutQt,AG_MMENU_CLIENTINFO);

    Action *about = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
    about->setText(tr("About the program"));
    about->setIcon(RSR_STORAGE_MENUICONS,MNI_CLIENTINFO_ABOUT);
    connect(about,SIGNAL(triggered()),SLOT(onShowAboutBox()));
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(about,AG_MMENU_CLIENTINFO);
  }

  return true;
}

bool ClientInfo::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (AHandlerId == FVersionHandle)
  {
    AAccept = true;
    Stanza iq("iq");
    iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
    QDomElement elem = iq.addElement("query",NS_JABBER_VERSION);
    elem.appendChild(iq.createElement("name")).appendChild(iq.createTextNode(CLIENT_NAME));
    elem.appendChild(iq.createElement("version")).appendChild(iq.createTextNode(CLIENT_VERSION));
    elem.appendChild(iq.createElement("os")).appendChild(iq.createTextNode(osVersion()));
    FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
  }
  else if (AHandlerId == FTimeHandle)
  {
    AAccept = true;
    Stanza iq("iq");
    iq.setTo(AStanza.from()).setId(AStanza.id()).setType("result");
    QDomElement elem = iq.addElement("time",NS_XMPP_TIME);
    DateTime dateTime(QDateTime::currentDateTime());
    elem.appendChild(iq.createElement("tzo")).appendChild(iq.createTextNode(dateTime.toX85TZD()));
    elem.appendChild(iq.createElement("utc")).appendChild(iq.createTextNode(dateTime.toX85UTC()));
    FStanzaProcessor->sendStanzaOut(AStreamJid,iq);
  }
  return false;
}

void ClientInfo::stanzaRequestResult(const Jid &/*AStreamJid*/, const Stanza &AStanza)
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
      activity.dateTime = QDateTime::currentDateTime().addSecs(0-query.attribute("seconds","0").toInt());
      activity.text = query.text();
    }
    else if (AStanza.type() == "error")
    {
      ErrorHandler err(AStanza.element());
      activity.dateTime = QDateTime();
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
    if (utc.endsWith('Z'))
      utc.chop(1);
    if (AStanza.type() == "result" && !tzo.isEmpty() && !utc.isEmpty())
    {
      TimeItem &tItem = FTimeItems[contactJid];
      DateTime dateTime(utc+tzo);
      tItem.zone = dateTime.timeZone();
      tItem.delta = QDateTime::currentDateTime().secsTo(dateTime.toLocal());
      tItem.ping = tItem.ping - QTime::currentTime().msecsTo(QTime(0,0,0,0));
    }
    else
      FTimeItems.remove(contactJid);
    emit entityTimeChanged(contactJid);
  }
}

void ClientInfo::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
  Q_UNUSED(AStreamJid);
  if (FSoftwareId.contains(AStanzaId))
  {
    Jid contactJid = FSoftwareId.take(AStanzaId);
    SoftwareItem &software = FSoftwareItems[contactJid];
    ErrorHandler err(ErrorHandler::REMOTE_SERVER_TIMEOUT);
    software.name = err.message();
    software.version.clear();
    software.os.clear();
    software.status = SoftwareError;
    emit softwareInfoChanged(contactJid);
  }
  else if (FActivityId.contains(AStanzaId))
  {
    Jid contactJid = FActivityId.take(AStanzaId);
    emit lastActivityChanged(contactJid);
  }
  else if (FTimeId.contains(AStanzaId))
  {
    Jid contactJid = FTimeId.take(AStanzaId);
    FTimeItems.remove(contactJid);
    emit entityTimeChanged(contactJid);
  }
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
    << RIT_CONTACT << RIT_AGENT << RIT_MY_RESOURCE;
  return indexTypes;
}

QVariant ClientInfo::data(const IRosterIndex *AIndex, int ARole) const
{
  Jid contactJid = AIndex->data(RDR_JID).toString();
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

IDataFormLocale ClientInfo::dataFormLocale(const QString &AFormType)
{
  IDataFormLocale locale;
  if (AFormType == DATA_FORM_SOFTWAREINFO)
  {
    locale.title = tr("Software information");
    locale.fields[FORM_FIELD_SOFTWARE].label = tr("Software");
    locale.fields[FORM_FIELD_SOFTWARE_VERSION].label = tr("Software Version");
    locale.fields[FORM_FIELD_OS].label = tr("OS");
    locale.fields[FORM_FIELD_OS_VERSION].label = tr("OS Version");
  }
  return locale;
}

void ClientInfo::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node.isEmpty())
  {
    IDataForm form;
    form.type = DATAFORM_TYPE_RESULT;

    IDataField ftype;
    ftype.required = false;
    ftype.var  = "FORM_TYPE";
    ftype.type = DATAFIELD_TYPE_HIDDEN;
    ftype.value = DATA_FORM_SOFTWAREINFO;
    form.fields.append(ftype);

    IDataField soft;
    soft.required = false;
    soft.var   = FORM_FIELD_SOFTWARE;
    soft.value = CLIENT_NAME;
    form.fields.append(soft);

    IDataField soft_ver;
    soft_ver.required = false;
    soft_ver.var   = FORM_FIELD_SOFTWARE_VERSION;
    soft_ver.value = CLIENT_VERSION;
    form.fields.append(soft_ver);

    IDataField os;
    os.required = false;
    os.var   = FORM_FIELD_OS;
    os.value = osVersion();
    form.fields.append(os);

    ADiscoInfo.extensions.append(form);
  }
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
      Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
      return action;
    }
    else if (AFeature == NS_JABBER_LAST)
    {
      if (FPresencePlugin && !FPresencePlugin->isContactOnline(ADiscoInfo.contactJid))
      {
        Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
        return action;
      }
    }
    else if (AFeature == NS_XMPP_TIME)
    {
      Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
      return action;
    }
  }
  return NULL;
}

QString ClientInfo::version() const
{
  return CLIENT_VERSION;
}

int ClientInfo::revision() const
{
  return SVN_REVISION;
}

QDateTime ClientInfo::revisionDate() const
{
  return QDateTime::fromString(SVN_DATE,"yyyy/MM/dd hh:mm:ss");
}

QString ClientInfo::osVersion() const
{
  static QString osver;
  if(osver.isEmpty())
  {
#if defined(Q_OS_MAC)
    switch(QSysInfo::MacintoshVersion)
    {
    case QSysInfo::MV_LEOPARD:
      osver = "MacOS 10.5(Leopard)";
      break;
    case QSysInfo::MV_TIGER:
      osver = "MacOS 10.4(Tiger)";
      break;
    case QSysInfo::MV_PANTHER:
      osver = "MacOS 10.3(Panther)";
      break;
    case QSysInfo::MV_JAGUAR:
      osver = "MacOS 10.2(Jaguar)";
      break;
    case QSysInfo::MV_PUMA:
      osver = "MacOS 10.1(Puma)";
      break;
    case QSysInfo::MV_CHEETAH:
      osver = "MacOS 10.0(Cheetah)";
      break;
    case QSysInfo::MV_9:
      osver = "MacOS 9";
      break;
    case QSysInfo::MV_Unknown:
    default:
      osver = tr("MacOS(unknown)");
      break;
    }
#elif defined(Q_OS_UNIX)
    utsname buf;
    if(uname(&buf) != -1)
    {
      osver.append(buf.release).append(QLatin1Char(' '));
      osver.append(buf.sysname).append(QLatin1Char(' '));
      osver.append(buf.machine).append(QLatin1Char(' '));
      osver.append(QLatin1String(" (")).append(buf.machine).append(QLatin1Char(')'));
    }
    else
    {
      osver = QLatin1String("Linux/Unix(unknown)");
    }
#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)
    switch(QSysInfo::WindowsVersion)
    {
    case QSysInfo::WV_CE_6:
      osver = "Windows CE 6.x";
      break;
    case QSysInfo::WV_CE_5:
      osver = "Windows CE 5.x";
      break;
    case QSysInfo::WV_CENET:
      osver = "Windows CE .NET";
      break;
    case QSysInfo::WV_CE:
      osver = "Windows CE";
      break;
    case QSysInfo::WV_VISTA:
      osver = "Windows Vista";
      break;
    case QSysInfo::WV_2003:
      osver = "Windows Server 2003";
      break;
    case QSysInfo::WV_XP:
      osver = "Windows XP";
      break;
    case QSysInfo::WV_2000:
      osver = "Windows 2000";
      break;
    case QSysInfo::WV_NT:
      osver = "Windows NT";
      break;
    case QSysInfo::WV_Me:
      osver = "Windows Me";
      break;
    case QSysInfo::WV_98:
      osver = "Windows 98";
      break;
    case QSysInfo::WV_95:
      osver = "Windows 95";
      break;
    case QSysInfo::WV_32s:
      osver = "Windows 3.1 with Win32s";
      break;
    default:
      osver = "Windows(unknown)";
      break;
    }

    if(QSysInfo::WindowsVersion & QSysInfo::WV_CE_based)
      osver.append(" (CE-based)");
    else if(QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)
      osver.append(" (NT-based)");
    else if(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)
      osver.append(" (MS-DOS-based)");

#else
    return QLatin1String("Unknown");
#endif
  }
  return osver;
}

void ClientInfo::showClientInfo(const Jid &AStreamJid, const Jid &AContactJid, int AInfoTypes)
{
  if (AInfoTypes>0 && AContactJid.isValid() && AStreamJid.isValid())
  {
    ClientInfoDialog *dialog = FClientInfoDialogs.value(AContactJid,NULL);
    if (!dialog)
    {
      QString contactName = !AContactJid.node().isEmpty() ? AContactJid.node() : FDiscovery!=NULL ? FDiscovery->discoInfo(AContactJid).identity.value(0).name : AContactJid.domain();
      if (FRosterPlugin)
      {
        IRoster *roster = FRosterPlugin->getRoster(AStreamJid);
        if (roster)
        {
          IRosterItem ritem = roster->rosterItem(AContactJid);
          if (!ritem.name.isEmpty())
            contactName = ritem.name;
        }
      }
      dialog = new ClientInfoDialog(this,AStreamJid,AContactJid,!contactName.isEmpty() ? contactName : AContactJid.full(),AInfoTypes);
      connect(dialog,SIGNAL(clientInfoDialogClosed(const Jid &)),SLOT(onClientInfoDialogClosed(const Jid &)));
      FClientInfoDialogs.insert(AContactJid,dialog);
      dialog->show();
    }
    else
    {
      dialog->setInfoTypes(dialog->infoTypes() | AInfoTypes);
      dialog->raise();
      dialog->activateWindow();
    }
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
    sended = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,SOFTWARE_INFO_TIMEOUT);
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
  return FActivityItems.value(AContactJid).dateTime.isValid();
}

bool ClientInfo::requestLastActivity(const Jid &AStreamJid, const Jid &AContactJid)
{
  bool sended = FActivityId.values().contains(AContactJid);
  if (!sended && AStreamJid.isValid() && AContactJid.isValid())
  {
    Stanza iq("iq");
    iq.addElement("query",NS_JABBER_LAST);
    iq.setTo(AContactJid.eBare()).setId(FStanzaProcessor->newId()).setType("get");
    sended = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,LAST_ACTIVITY_TIMEOUT);
    if (sended)
      FActivityId.insert(iq.id(),AContactJid);
  }
  return sended;
}

QDateTime ClientInfo::lastActivityTime(const Jid &AContactJid) const
{
  return FActivityItems.value(AContactJid).dateTime;
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
    sended = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,ENTITY_TIME_TIMEOUT);
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
    QDateTime dateTime = QDateTime::currentDateTime().toUTC();
    dateTime.setTimeSpec(Qt::LocalTime);
    return dateTime.addSecs(tItem.zone).addSecs(tItem.delta);
  }
  return QDateTime();
}

int ClientInfo::entityTimeDelta(const Jid &AContactJid) const
{
  if (hasEntityTime(AContactJid))
    return FTimeItems.value(AContactJid).delta;
  return 0;
}

int ClientInfo::entityTimePing(const Jid &AContactJid) const
{
  return FTimeItems.value(AContactJid).ping;
}

Action *ClientInfo::createInfoAction(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFeature, QObject *AParent) const
{
  if (AFeature == NS_JABBER_VERSION)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Software version"));
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_CLIENTINFO_VERSION);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_CONTACT_JID,AContactJid.full());
    action->setData(ADR_INFO_TYPES,IClientInfo::SoftwareVersion);
    connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
    return action;
  }
  else if (AFeature == NS_JABBER_LAST)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Last activity"));
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_CLIENTINFO_ACTIVITY);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_CONTACT_JID,AContactJid.full());
    action->setData(ADR_INFO_TYPES,IClientInfo::LastActivity);
    connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
    return action;
  }
  else if (AFeature == NS_XMPP_TIME)
  {
    Action *action = new Action(AParent);
    action->setText(tr("Entity time"));
    action->setIcon(RSR_STORAGE_MENUICONS,MNI_CLIENTINFO_TIME);
    action->setData(ADR_STREAM_JID,AStreamJid.full());
    action->setData(ADR_CONTACT_JID,AContactJid.full());
    action->setData(ADR_INFO_TYPES,IClientInfo::EntityTime);
    connect(action,SIGNAL(triggered(bool)),SLOT(onClientInfoActionTriggered(bool)));
    return action;
  }
  return NULL;
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
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CLIENTINFO_VERSION);
  dfeature.var = NS_JABBER_VERSION;
  dfeature.name = tr("Software version");
  dfeature.description = tr("Request contact software version");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = false;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CLIENTINFO_ACTIVITY);
  dfeature.var = NS_JABBER_LAST;
  dfeature.name = tr("Last activity");
  dfeature.description = tr("Request contact last activity");
  FDiscovery->insertDiscoFeature(dfeature);

  dfeature.active = true;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CLIENTINFO_TIME);
  dfeature.var = NS_XMPP_TIME;
  dfeature.name = tr("Entity time");
  dfeature.description = tr("Request the local time of an entity");
  FDiscovery->insertDiscoFeature(dfeature);
}

void ClientInfo::onContactStateChanged(const Jid &/*AStreamJid*/, const Jid &AContactJid, bool AStateOnline)
{
  if (AStateOnline)
  {
    if (FActivityItems.contains(AContactJid))
    {
      FActivityItems.remove(AContactJid);
      emit lastActivityChanged(AContactJid);
    }
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
  if (AIndex->type() == RIT_CONTACT || AIndex->type() == RIT_AGENT || AIndex->type() == RIT_MY_RESOURCE)
  {
    Jid streamJid = AIndex->data(RDR_STREAM_JID).toString();
    IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
    if (presence && presence->xmppStream()->isOpen())
    {
      Jid contactJid = AIndex->data(RDR_JID).toString();
      int show = AIndex->data(RDR_SHOW).toInt();
      QStringList features = FDiscovery!=NULL ? FDiscovery->discoInfo(contactJid).features : QStringList();
      if (show != IPresence::Offline && show != IPresence::Error && !features.contains(NS_JABBER_VERSION))
      {
        Action *action = createInfoAction(streamJid,contactJid,NS_JABBER_VERSION,AMenu);
        AMenu->addAction(action,AG_RVCM_CLIENTINFO,true);
      }
      if (show == IPresence::Offline && !features.contains(NS_JABBER_LAST))
      {
        Action *action = createInfoAction(streamJid,contactJid,NS_JABBER_LAST,AMenu);
        AMenu->addAction(action,AG_RVCM_CLIENTINFO,true);
      }
    }
  }
}

void ClientInfo::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
  if (ALabelId == RLID_DISPLAY && types().contains(AIndex->type()))
  {
    Jid contactJid = AIndex->data(RDR_JID).toString();

    if (hasSoftwareInfo(contactJid))
      AToolTips.insert(RTTO_SOFTWARE_INFO,tr("Software: %1 %2").arg(softwareName(contactJid)).arg(softwareVersion(contactJid)));

    if (hasLastActivity(contactJid) && AIndex->data(RDR_SHOW).toInt() == IPresence::Offline)
      AToolTips.insert(RTTO_LAST_ACTIVITY,tr("Offline since: %1").arg(lastActivityTime(contactJid).toString()));

    if (hasEntityTime(contactJid))
      AToolTips.insert(RTTO_ENTITY_TIME,tr("Entity time: %1").arg(entityTime(contactJid).time().toString()));
  }
}

void ClientInfo::onMultiUserContextMenu(IMultiUserChatWindow * /*AWindow*/, IMultiUser *AUser, Menu *AMenu)
{
  Jid streamJid = AUser->data(MUDR_STREAM_JID).toString();
  Jid contactJid = AUser->data(AUser->data(MUDR_REAL_JID).toString().isEmpty() ? MUDR_CONTACT_JID : MUDR_REAL_JID).toString();

  Action *action = createInfoAction(streamJid,contactJid,NS_JABBER_VERSION,AMenu);
  AMenu->addAction(action,AG_MUCM_CLIENTINFO,true);

  action = createInfoAction(streamJid,contactJid,NS_XMPP_TIME,AMenu);
  AMenu->addAction(action,AG_MUCM_CLIENTINFO,true);
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

void ClientInfo::onEntityTimeChanged(const Jid &AContactJid)
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

void ClientInfo::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
  if (FDataForms && AInfo.node.isEmpty() && !hasSoftwareInfo(AInfo.contactJid))
  {
    foreach(IDataForm form, AInfo.extensions)
    {
      if (FDataForms->fieldValue("FORM_TYPE",form.fields).toString() == DATA_FORM_SOFTWAREINFO)
      {
        SoftwareItem &software = FSoftwareItems[AInfo.contactJid];
        software.name = FDataForms->fieldValue(FORM_FIELD_SOFTWARE,form.fields).toString();
        software.version = FDataForms->fieldValue(FORM_FIELD_SOFTWARE_VERSION,form.fields).toString();
        software.os = FDataForms->fieldValue(FORM_FIELD_OS,form.fields).toString() + " ";
        software.os += FDataForms->fieldValue(FORM_FIELD_OS_VERSION,form.fields).toString();
        software.status = SoftwareLoaded;
        emit softwareInfoChanged(AInfo.contactJid);
        break;
      }
    }
  }
}

void ClientInfo::onShowAboutBox()
{
  if (FAboutBox.isNull())
  {
    FAboutBox = new AboutBox(this);
    FAboutBox->show();
  }
  else
    FAboutBox->raise();
}

Q_EXPORT_PLUGIN2(ClientInfoPlugin, ClientInfo)
