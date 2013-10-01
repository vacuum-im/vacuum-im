#include "clientinfo.h"

#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>

#if defined(Q_OS_UNIX)
# include <sys/utsname.h>
#endif

#if defined(Q_WS_HAIKU)
#include <Path.h>
#include <AppFileInfo.h>
#include <FindDirectory.h>
#endif

#define SHC_SOFTWARE_VERSION            "/iq[@type='get']/query[@xmlns='" NS_JABBER_VERSION "']"
#define SHC_LAST_ACTIVITY               "/iq[@type='get']/query[@xmlns='" NS_JABBER_LAST "']"
#define SHC_ENTITY_TIME                 "/iq[@type='get']/time[@xmlns='" NS_XMPP_TIME "']"
#define SHC_XMPP_PING                   "/iq[@type='get']/ping[@xmlns='" NS_XMPP_PING "']"

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
	FPluginManager = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FStanzaProcessor = NULL;
	FRostersViewPlugin = NULL;
	FDiscovery = NULL;
	FDataForms = NULL;
	FOptionsManager = NULL;
	FStatusIcons = NULL;

	FPingHandle = 0;
	FTimeHandle = 0;
	FVersionHandle = 0;
	FActivityHandler = 0;
}

ClientInfo::~ClientInfo()
{

}

void ClientInfo::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Client Information");
	APluginInfo->description = tr("Allows to send and receive information about the version of the application, the local time and the last activity of contact");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ClientInfo::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterRemoved(IRoster *)),SLOT(onRosterRemoved(IRoster *)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
		if (FPresencePlugin)
		{
			connect(FPresencePlugin->instance(),SIGNAL(contactStateChanged(const Jid &, const Jid &, bool)), SLOT(onContactStateChanged(const Jid &, const Jid &, bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)), 
				SLOT(onRostersViewIndexContextMenu(const QList<IRosterIndex *> &, quint32, Menu *)));
		}
	}

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IStatusIcons").value(0,NULL);
	if (plugin)
	{
		FStatusIcons = qobject_cast<IStatusIcons *>(plugin->instance());
	}

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

	return FStanzaProcessor != NULL;
}

bool ClientInfo::initObjects()
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;

		shandle.conditions.append(SHC_SOFTWARE_VERSION);
		FVersionHandle = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_LAST_ACTIVITY);
		FActivityHandler = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_ENTITY_TIME);
		FTimeHandle = FStanzaProcessor->insertStanzaHandle(shandle);

		shandle.conditions.clear();
		shandle.conditions.append(SHC_XMPP_PING);
		FPingHandle = FStanzaProcessor->insertStanzaHandle(shandle);
	}

	if (FDiscovery)
	{
		registerDiscoFeatures();
		FDiscovery->insertFeatureHandler(NS_JABBER_VERSION,this,DFO_DEFAULT);
		FDiscovery->insertFeatureHandler(NS_JABBER_LAST,this,DFO_DEFAULT);
		FDiscovery->insertFeatureHandler(NS_XMPP_TIME,this,DFO_DEFAULT);
	}

	if (FDataForms)
	{
		FDataForms->insertLocalizer(this,DATA_FORM_SOFTWAREINFO);
	}

	return true;
}

bool ClientInfo::initSettings()
{
	Options::setDefaultValue(OPV_MISC_SHAREOSVERSION,true);
	if (FOptionsManager)
	{
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

bool ClientInfo::startPlugin()
{
	SystemManager::startSystemIdle();
	return true;
}

QMultiMap<int, IOptionsWidget *> ClientInfo::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (FOptionsManager && ANodeId == OPN_MISC)
	{
		widgets.insertMulti(OWO_MISC_CLIENTINFO, FOptionsManager->optionsNodeWidget(Options::node(OPV_MISC_SHAREOSVERSION),tr("Share information about OS version"),AParent));
	}
	return widgets;
}

bool ClientInfo::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (AHandlerId == FVersionHandle)
	{
		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		QDomElement elem = result.addElement("query",NS_JABBER_VERSION);
		elem.appendChild(result.createElement("name")).appendChild(result.createTextNode(CLIENT_NAME));
		elem.appendChild(result.createElement("version")).appendChild(result.createTextNode(QString("%1.%2 %3").arg(FPluginManager->version()).arg(FPluginManager->revision()).arg(CLIENT_VERSION_SUFIX).trimmed()));
		if (Options::node(OPV_MISC_SHAREOSVERSION).value().toBool())
			elem.appendChild(result.createElement("os")).appendChild(result.createTextNode(osVersion()));
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	else if (AHandlerId == FActivityHandler)
	{
		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		QDomElement elem = result.addElement("query",NS_JABBER_LAST);
		elem.setAttribute("seconds", SystemManager::systemIdle());
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	else if (AHandlerId == FTimeHandle)
	{
		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		QDomElement elem = result.addElement("time",NS_XMPP_TIME);
		DateTime dateTime(QDateTime::currentDateTime());
		elem.appendChild(result.createElement("tzo")).appendChild(result.createTextNode(dateTime.toX85TZD()));
		elem.appendChild(result.createElement("utc")).appendChild(result.createTextNode(dateTime.toX85UTC()));
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	else if (AHandlerId == FPingHandle)
	{
		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	return false;
}

void ClientInfo::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	Q_UNUSED(AStreamJid);
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
			software.name = XmppStanzaError(AStanza).errorMessage();
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
			activity.dateTime = QDateTime();
			activity.text = XmppStanzaError(AStanza).errorMessage();
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
			tItem.zone = DateTime::tzdFromX85(tzo);
			tItem.delta = QDateTime::currentDateTime().secsTo(DateTime(utc).toLocal());
			tItem.ping = tItem.ping - QTime::currentTime().msecsTo(QTime(0,0,0,0));
		}
		else
		{
			FTimeItems.remove(contactJid);
		}
		emit entityTimeChanged(contactJid);
	}
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
	IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(AStreamJid) : NULL;
	if (presence && presence->isOpen())
	{
		if (AFeature == NS_JABBER_VERSION)
		{
			Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
			return action;
		}
		else if (AFeature == NS_JABBER_LAST)
		{
			Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
			return action;
		}
		else if (AFeature == NS_XMPP_TIME)
		{
			Action *action = createInfoAction(AStreamJid,ADiscoInfo.contactJid,AFeature,AParent);
			return action;
		}
	}
	return NULL;
}

QString ClientInfo::osVersion() const
{
	static QString osver;
	if (osver.isEmpty())
	{
#if defined(Q_WS_MAC)
		switch (QSysInfo::MacintoshVersion)
		{
		# if QT_VERSION >= 0x040803
			case QSysInfo::MV_MOUNTAINLION:
				osver = "OS X 10.8 Mountain Lion";
				break;
		# endif
		case QSysInfo::MV_LION:
			osver = "OS X 10.7 Lion";
			break;
		case QSysInfo::MV_SNOWLEOPARD:
			osver = "Mac OS X 10.6 Snow Leopard)";
			break;
		case QSysInfo::MV_LEOPARD:
			osver = "Mac OS X 10.5 Leopard";
			break;
		case QSysInfo::MV_TIGER:
			osver = "Mac OS X 10.4 Tiger";
			break;
		case QSysInfo::MV_PANTHER:
			osver = "Mac OS X 10.3 Panther";
			break;
		case QSysInfo::MV_JAGUAR:
			osver = "Mac OS X 10.2 Jaguar";
			break;
		case QSysInfo::MV_PUMA:
			osver = "Mac OS X 10.1 Puma";
			break;
		case QSysInfo::MV_CHEETAH:
			osver = "Mac OS X 10.0 Cheetah";
			break;
		case QSysInfo::MV_9:
			osver = "Mac OS 9";
			break;
		case QSysInfo::MV_Unknown:
		default:
			osver = "MacOS (unknown)";
			break;
		}
#elif defined(Q_WS_X11)
		QStringList path;
		foreach(const QString &env, QProcess::systemEnvironment())
			if (env.startsWith("PATH="))
				path = env.split('=').value(1).split(':');

		QString found;
		foreach(const QString &dirname, path)
		{
			QDir dir(dirname);
			QFileInfo cand(dir.filePath("lsb_release"));
			if (cand.isExecutable())
			{
				found = cand.absoluteFilePath();
				break;
			}
		}

		if (!found.isEmpty())
		{
			QProcess process;
			process.start(found, QStringList()<<"--description"<<"--short", QIODevice::ReadOnly);
			if (process.waitForStarted())
			{
				QTextStream stream(&process);
				while (process.waitForReadyRead())
					osver += stream.readAll();
				process.close();
				osver = osver.trimmed();
			}
		}

		if (osver.isEmpty())
		{
			utsname buf;
			if (uname(&buf) != -1)
			{
				osver.append(buf.release).append(QLatin1Char(' '));
				osver.append(buf.sysname).append(QLatin1Char(' '));
				osver.append(buf.machine).append(QLatin1Char(' '));
				osver.append(QLatin1String(" (")).append(buf.machine).append(QLatin1Char(')'));
			}
			else
			{
				osver = QLatin1String("Linux/Unix (unknown)");
			}
		}
#elif defined(Q_WS_WIN) || defined(Q_OS_CYGWIN)
		switch (QSysInfo::WindowsVersion)
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
		# if QT_VERSION >= 0x040803
			case QSysInfo::WV_WINDOWS8:
				osver = "Windows 8";
				break;
		# endif
		case QSysInfo::WV_WINDOWS7:
			osver = "Windows 7";
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
			osver = "Windows (unknown)";
			break;
		}
#elif defined(Q_WS_HAIKU)
		BPath path;
		QString strVersion("Haiku");
		if (find_directory(B_BEOS_LIB_DIRECTORY, &path) == B_OK) 
		{
			path.Append("libbe.so");

			BAppFileInfo appFileInfo;
			version_info versionInfo;
			BFile file;
			if (file.SetTo(path.Path(), B_READ_ONLY) == B_OK
				&& appFileInfo.SetTo(&file) == B_OK
				&& appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK
				&& versionInfo.short_info[0] != '\0')
			{
				strVersion = versionInfo.short_info;
			}
		}

		utsname uname_info;
		if (uname(&uname_info) == 0) 
		{
			osver = uname_info.sysname;
			long revision = 0;
			if (sscanf(uname_info.version, "r%10ld", &revision) == 1)
			{
				char version[16];
				snprintf(version, sizeof(version), "%ld", revision);
				osver += " ( " + strVersion + " Rev. ";
				osver += version;
				osver += ")";
			}
		}
#else
		osver = "Unknown";
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
			QString contactName =  AContactJid.uNode();
			if (FDiscovery!=NULL && FDiscovery->discoInfo(AStreamJid,AContactJid.bare()).identity.value(0).category == "conference")
				contactName = AContactJid.resource();
			if (contactName.isEmpty())
				contactName = FDiscovery!=NULL ? FDiscovery->discoInfo(AStreamJid,AContactJid).identity.value(0).name : AContactJid.domain();
			if (FRosterPlugin)
			{
				IRoster *roster = FRosterPlugin->findRoster(AStreamJid);
				if (roster)
				{
					IRosterItem ritem = roster->rosterItem(AContactJid);
					if (!ritem.name.isEmpty())
						contactName = ritem.name;
				}
			}
			dialog = new ClientInfoDialog(this,AStreamJid,AContactJid,!contactName.isEmpty() ? contactName : AContactJid.uFull(),AInfoTypes);
			connect(dialog,SIGNAL(clientInfoDialogClosed(const Jid &)),SLOT(onClientInfoDialogClosed(const Jid &)));
			FClientInfoDialogs.insert(AContactJid,dialog);
			dialog->show();
		}
		else
		{
			dialog->setInfoTypes(dialog->infoTypes() | AInfoTypes);
			WidgetManager::showActivateRaiseWindow(dialog);
		}
	}
}

bool ClientInfo::hasSoftwareInfo(const Jid &AContactJid) const
{
	return FSoftwareItems.value(AContactJid).status == SoftwareLoaded;
}

bool ClientInfo::requestSoftwareInfo(const Jid &AStreamJid, const Jid &AContactJid)
{
	bool sent = FSoftwareId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("query",NS_JABBER_VERSION);
		iq.setTo(AContactJid.full()).setId(FStanzaProcessor->newId()).setType("get");
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,SOFTWARE_INFO_TIMEOUT);
		if (sent)
		{
			FSoftwareId.insert(iq.id(),AContactJid);
			FSoftwareItems[AContactJid].status = SoftwareLoading;
		}
	}
	return sent;
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
	bool sent = FActivityId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("query",NS_JABBER_LAST);
		iq.setTo(AContactJid.full()).setId(FStanzaProcessor->newId()).setType("get");
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,LAST_ACTIVITY_TIMEOUT);
		if (sent)
			FActivityId.insert(iq.id(),AContactJid);
	}
	return sent;
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
	bool sent = FTimeId.values().contains(AContactJid);
	if (!sent && AStreamJid.isValid() && AContactJid.isValid())
	{
		Stanza iq("iq");
		iq.addElement("time",NS_XMPP_TIME);
		iq.setTo(AContactJid.full()).setType("get").setId(FStanzaProcessor->newId());
		sent = FStanzaProcessor->sendStanzaRequest(this,AStreamJid,iq,ENTITY_TIME_TIMEOUT);
		if (sent)
		{
			TimeItem &tItem = FTimeItems[AContactJid];
			tItem.ping = QTime::currentTime().msecsTo(QTime(0,0,0,0));
			FTimeId.insert(iq.id(),AContactJid);
			emit entityTimeChanged(AContactJid);
		}
	}
	return sent;
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
		action->setText(tr("Software Version"));
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

		if (AContactJid.node().isEmpty())
			action->setText(tr("Service Uptime"));
		else if (AContactJid.resource().isEmpty())
			action->setText(tr("Last Activity"));
		else
			action->setText(tr("Idle Time"));

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
		action->setText(tr("Entity Time"));
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
	dfeature.name = tr("Software Version");
	dfeature.description = tr("Supports the exchanging of the information about the application version");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CLIENTINFO_ACTIVITY);
	dfeature.var = NS_JABBER_LAST;
	dfeature.name = tr("Last Activity");
	dfeature.description = tr("Supports the exchanging of the information about the user last activity");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_CLIENTINFO_TIME);
	dfeature.var = NS_XMPP_TIME;
	dfeature.name = tr("Entity Time");
	dfeature.description = tr("Supports the exchanging of the information about the user local time");
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.active = true;
	dfeature.icon = QIcon();
	dfeature.var = NS_XMPP_PING;
	dfeature.name = tr("XMPP Ping");
	dfeature.description = tr("Supports the exchanging of the application-level pings over XML streams");
	FDiscovery->insertDiscoFeature(dfeature);
}

void ClientInfo::onContactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline)
{
	Q_UNUSED(AStreamJid);
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

void ClientInfo::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && AIndexes.count()==1)
	{
		IRosterIndex *index = AIndexes.first();
		if (index->kind()==RIK_CONTACT || index->kind()==RIK_AGENT || index->kind()==RIK_MY_RESOURCE)
		{
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->findPresence(streamJid) : NULL;
			if (presence && presence->isOpen())
			{
				int show = index->data(RDR_SHOW).toInt();
				Jid contactJid = index->data(RDR_FULL_JID).toString();
				if (show==IPresence::Offline || show==IPresence::Error)
				{
					Action *action = createInfoAction(streamJid,contactJid,NS_JABBER_LAST,AMenu);
					AMenu->addAction(action,AG_RVCM_CLIENTINFO,true);
				}
			}
		}
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

void ClientInfo::onRosterRemoved(IRoster *ARoster)
{
	deleteSoftwareDialogs(ARoster->streamJid());
}

void ClientInfo::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (FDataForms && AInfo.node.isEmpty() && !hasSoftwareInfo(AInfo.contactJid))
	{
		foreach(const IDataForm &form, AInfo.extensions)
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

void ClientInfo::onOptionsChanged(const OptionsNode &ANode)
{
	if (FDiscovery && ANode.path()==OPV_MISC_SHAREOSVERSION)
	{
		FDiscovery->updateSelfEntityCapabilities();
	}
}

Q_EXPORT_PLUGIN2(plg_clientinfo, ClientInfo)
