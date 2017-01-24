#include "messagearchiver.h"

#include <QDir>
#include <QFile>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/toolbargroups.h>
#include <definitions/internalerrors.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodes.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <definitions/messagedataroles.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/stanzahandlerorders.h>
#include <definitions/shortcutgrouporders.h>
#include <definitions/statisticsparams.h>
#include <utils/widgetmanager.h>
#include <utils/xmpperror.h>
#include <utils/shortcuts.h>
#include <utils/options.h>
#include <utils/logger.h>

#define ARCHIVE_DIR_NAME           "archive"
#define PENDING_FILE_NAME          "pending.xml"
#define ARCHIVE_REQUEST_TIMEOUT    30000

#define SHC_MESSAGE_BODY           "/message/body"
#define SHC_ARCHIVE_PREFS          "/iq[@type='set']/pref[@xmlns=" NS_ARCHIVE_MAM "]"
#define SHC_ARCHIVE_PREFS_0        "/iq[@type='set']/pref[@xmlns=" NS_ARCHIVE_MAM_0 "]"

#define ADR_STREAM_JID             Action::DR_StreamJid
#define ADR_CONTACT_JID            Action::DR_Parametr1
#define ADR_ITEM_SAVE              Action::DR_Parametr2

#define PST_ARCHIVE_PREFS          "prefs"
#define PSN_ARCHIVE_PREFS          NS_ARCHIVE_MAM

MessageArchiver::MessageArchiver()
{
	FPluginManager = NULL;
	FXmppStreamManager = NULL;
	FStanzaProcessor = NULL;
	FOptionsManager = NULL;
	FPrivateStorage = NULL;
	FAccountManager = NULL;
	FRostersViewPlugin = NULL;
	FDiscovery = NULL;
	FDataForms = NULL;
	FMessageWidgets = NULL;
	FRosterManager = NULL;
}

MessageArchiver::~MessageArchiver()
{

}

void MessageArchiver::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("History");
	APluginInfo->description = tr("Allows to save the history of communications");
	APluginInfo->version = "2.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MessageArchiver::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
		if (FXmppStreamManager)
		{
			connect(FXmppStreamManager->instance(),SIGNAL(streamOpened(IXmppStream *)),SLOT(onXmppStreamOpened(IXmppStream *)));
			connect(FXmppStreamManager->instance(),SIGNAL(streamClosed(IXmppStream *)),SLOT(onXmppStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
	{
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(dataSaved(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataLoadedSaved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataLoaded(const QString &, const Jid &, const QDomElement &)),
				SLOT(onPrivateDataLoadedSaved(const QString &, const Jid &, const QDomElement &)));
			connect(FPrivateStorage->instance(),SIGNAL(dataChanged(const Jid &, const QString &, const QString &)),
				SLOT(onPrivateDataChanged(const Jid &, const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAccountManager").value(0,NULL);
	if (plugin)
	{
		FAccountManager = qobject_cast<IAccountManager *>(plugin->instance());
		if (FAccountManager)
		{
			connect(FAccountManager->instance(),SIGNAL(accountInserted(IAccount *)),SLOT(onAccountInserted(IAccount *)));
			connect(FAccountManager->instance(),SIGNAL(accountRemoved(IAccount *)),SLOT(onAccountRemoved(IAccount *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
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
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoveryInfoReceived(const IDiscoInfo &)));
		}
	}

	plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
	{
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(toolBarWidgetCreated(IMessageToolBarWidget *)),SLOT(onToolBarWidgetCreated(IMessageToolBarWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		FRosterManager = qobject_cast<IRosterManager *>(plugin->instance());
	}

	connect(this,SIGNAL(requestFailed(const QString &, const XmppError &)),
		SLOT(onSelfRequestFailed(const QString &, const XmppError &)));
	connect(this,SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
		SLOT(onSelfHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
	connect(this,SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
		SLOT(onSelfCollectionLoaded(const QString &, const IArchiveCollection &)));

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FXmppStreamManager!=NULL && FStanzaProcessor!=NULL;
}

bool MessageArchiver::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SHOWHISTORY, tr("Show history"), tr("Ctrl+H","Show history"));
	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWHISTORY,tr("Show history"),tr("Ctrl+H","Show history"),Shortcuts::WidgetShortcut);

	XmppError::registerError(NS_INTERNAL_ERROR,IERR_HISTORY_HEADERS_LOAD_ERROR,tr("Failed to load conversation headers"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_HISTORY_CONVERSATION_SAVE_ERROR,tr("Failed to save conversation"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_HISTORY_CONVERSATION_LOAD_ERROR,tr("Failed to load conversation"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_HISTORY_CONVERSATION_REMOVE_ERROR,tr("Failed to remove conversation"));
	XmppError::registerError(NS_INTERNAL_ERROR,IERR_HISTORY_MODIFICATIONS_LOAD_ERROR,tr("Failed to load archive modifications"));

	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FRostersViewPlugin)
	{
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWHISTORY,FRostersViewPlugin->rostersView()->instance());
	}
	if (FOptionsManager)
	{
		IOptionsDialogNode historyNode = { ONO_HISTORY, OPN_HISTORY, MNI_HISTORY, tr("History") };
		FOptionsManager->insertOptionsDialogNode(historyNode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

bool MessageArchiver::initSettings()
{
	Options::setDefaultValue(OPV_HISTORY_ENGINE_ENABLED,true);
	Options::setDefaultValue(OPV_HISTORY_ARCHIVEVIEW_FONTPOINTSIZE,10);

	Options::setDefaultValue(OPV_ACCOUNT_HISTORYDUPLICATE,false);

	return true;
}

bool MessageArchiver::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessageIn.value(AStreamJid) == AHandlerId)
	{
		Message message(AStanza);
		processMessage(AStreamJid,message,true);
	}
	else if (FSHIMessageOut.value(AStreamJid) == AHandlerId)
	{
		Message message(AStanza);
		processMessage(AStreamJid,message,false);
	}
	else if (FSHIPrefs.value(AStreamJid)==AHandlerId && AStanza.isFromServer())
	{
		applyArchivePrefs(AStreamJid,AStanza.firstElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid)));

		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	return false;
}

void MessageArchiver::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	XmppStanzaError err = !AStanza.isResult() ? XmppStanzaError(AStanza) : XmppStanzaError::null;

	if (FPrefsLoadRequests.contains(AStanza.id()))
	{
		FPrefsLoadRequests.remove(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Server archive prefs loaded, id=%1").arg(AStanza.id()));
			applyArchivePrefs(AStreamJid,AStanza.firstElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid)));
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to load server archive prefs, id=%1: %2").arg(AStanza.id(),err.condition()));
			loadStoragePrefs(AStreamJid);
		}
	}
	else if (FPrefsSaveRequests.contains(AStanza.id()))
	{
		FPrefsSaveRequests.remove(AStanza.id());
		if (AStanza.isResult())
		{
			LOG_STRM_INFO(AStreamJid,QString("Server archive prefs saved, id=%1").arg(AStanza.id()));
			applyArchivePrefs(AStreamJid,AStanza.firstElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid)));
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to save server archive prefs, id=%1: %2").arg(AStanza.id(),err.condition()));
		}
	}

	if (AStanza.isResult())
		emit requestCompleted(AStanza.id());
	else
		emit requestFailed(AStanza.id(),err);
}

QMultiMap<int, IOptionsDialogWidget *> MessageArchiver::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *>  widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (nodeTree.count()==3 && nodeTree.at(0)==OPN_ACCOUNTS && nodeTree.at(2)=="History")
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountById(nodeTree.at(1)) : NULL;
		if (account!=NULL && isReady(account->streamJid()))
		{
			OptionsNode options = account->optionsNode();
			
			widgets.insertMulti(OHO_ACCOUNTS_HISTORY_SERVERSETTINGS, FOptionsManager->newOptionsDialogHeader(tr("Archive preferences"),AParent));
			widgets.insertMulti(OWO_ACCOUNTS_HISTORY_SERVERSETTINGS, new ArchiveAccountOptionsWidget(this,account->streamJid(),AParent));

			if (isSupported(account->streamJid()))
			{
				widgets.insertMulti(OHO_ACCOUNTS_HISTORY_REPLICATION,FOptionsManager->newOptionsDialogHeader(tr("Archive synchronization"),AParent));
				widgets.insertMulti(OWO_ACCOUNTS_HISTORY_DUPLICATION,FOptionsManager->newOptionsDialogWidget(options.node("history-duplicate"),tr("Duplicate messages in local archive (not recommended)"),AParent));
			}
		}
	}
	else if (ANodeId == OPN_HISTORY)
	{
		int index = 0;
		widgets.insertMulti(OHO_HISORY_ENGINES, FOptionsManager->newOptionsDialogHeader(tr("Used history archives"),AParent));
		foreach(IArchiveEngine *engine, archiveEngines())
		{
			OptionsNode node = Options::node(OPV_HISTORY_ENGINE_ITEM,engine->engineId().toString()).node("enabled");
			widgets.insertMulti(OWO_HISORY_ENGINE,FOptionsManager->newOptionsDialogWidget(node,engine->engineName(),AParent));

			IOptionsDialogWidget *engineSettings = engine->engineSettingsWidget(AParent);
			if (engineSettings)
			{
				widgets.insertMulti(OHO_HISTORY_ENGINNAME + index,FOptionsManager->newOptionsDialogHeader(engine->engineName(),AParent));
				widgets.insertMulti(OWO_HISTORY_ENGINESETTINGS + index,engineSettings);
				index += 10;
			}
		}
	}
	return widgets;
}

bool MessageArchiver::isReady(const Jid &AStreamJid) const
{
	return FArchivePrefs.contains(AStreamJid);
}

bool MessageArchiver::isSupported(const Jid &AStreamJid) const
{
	return FNamespaces.contains(AStreamJid);
}

QString MessageArchiver::supportedNamespace(const Jid &AStreamJid) const
{
	return FNamespaces.value(AStreamJid);
}

QString MessageArchiver::archiveDirPath(const Jid &AStreamJid) const
{
	if (FArchiveDirPath.isEmpty())
	{
		QDir dir(FPluginManager->homePath());
		dir.mkdir(ARCHIVE_DIR_NAME);
		FArchiveDirPath = dir.cd(ARCHIVE_DIR_NAME) ? dir.absolutePath() : QString::null;
	}
	if (AStreamJid.isValid() && !FArchiveDirPath.isEmpty())
	{
		QString streamDir = Jid::encode(AStreamJid.pBare());

		QDir dir(FArchiveDirPath);
		dir.mkdir(streamDir);
		return dir.cd(streamDir) ? dir.absolutePath() : QString::null;
	}
	return FArchiveDirPath;
}

QWidget *MessageArchiver::showArchiveWindow(const QMultiMap<Jid,Jid> &AAddresses)
{
	ArchiveViewWindow *window = new ArchiveViewWindow(this,AAddresses);
	WidgetManager::showActivateRaiseWindow(window);
	return window;
}

bool MessageArchiver::isPrefsSupported(const Jid &AStreamJid) const
{
	if (isReady(AStreamJid) && isSupported(AStreamJid) && !FInStoragePrefs.contains(AStreamJid))
		return true;
	else if (isReady(AStreamJid) && !isSupported(AStreamJid) && FInStoragePrefs.contains(AStreamJid))
		return true;
	return false;
}

IArchiveStreamPrefs MessageArchiver::archivePrefs(const Jid &AStreamJid) const
{
	return FArchivePrefs.value(AStreamJid);
}

QString MessageArchiver::setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs)
{
	if (isReady(AStreamJid))
	{
		bool inStorage =  FInStoragePrefs.contains(AStreamJid);

		Stanza save(STANZA_KIND_IQ);
		save.setType(STANZA_TYPE_SET).setUniqueId();

		QDomElement prefsElem = save.addElement(PST_ARCHIVE_PREFS, !inStorage ? FNamespaces.value(AStreamJid) : NS_ARCHIVE_MAM);
		prefsElem.setAttribute("default", APrefs.defaults);

		if (!APrefs.always.isEmpty())
		{
			QDomElement alwaysElem = save.createElement("always");
			foreach(const Jid &itemJid, APrefs.always)
			{
				if (itemJid.isValid())
				{
					QDomElement jidElem = save.createElement("jid");
					jidElem.appendChild(save.createTextNode(itemJid.pFull()));
					alwaysElem.appendChild(jidElem);
				}
			}
			prefsElem.appendChild(alwaysElem);
		}

		if (!APrefs.never.isEmpty())
		{
			QDomElement neverElem = save.createElement("never");
			foreach(const Jid &itemJid, APrefs.never)
			{
				if (itemJid.isValid())
				{
					QDomElement jidElem = save.createElement("jid");
					jidElem.appendChild(save.createTextNode(itemJid.pFull()));
					neverElem.appendChild(jidElem);
				}
			}
			prefsElem.appendChild(neverElem);
		}

		QString requestId;
		if (inStorage)
			requestId = FPrivateStorage!=NULL ? FPrivateStorage->saveData(AStreamJid,prefsElem) : QString::null;
		else if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,save,ARCHIVE_REQUEST_TIMEOUT))
			requestId = save.id();
		
		if (!requestId.isEmpty())
		{
			LOG_STRM_INFO(AStreamJid,QString("Update archive prefs request sent, id=%1").arg(requestId));
			FPrefsSaveRequests.insert(requestId,AStreamJid);
			return requestId;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send update archive prefs request"));
		}
	}
	return QString::null;
}

bool MessageArchiver::isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid) const
{
	if (isReady(AStreamJid) && AItemJid.isValid())
	{
		const IArchiveStreamPrefs &prefs = FArchivePrefs[AStreamJid];

		foreach(const Jid &itemJid, prefs.always)
		{
			if (itemJid.hasResource() && itemJid==AItemJid)
				return true;
			else if (!itemJid.hasResource() && itemJid.isBareEqual(AItemJid))
				return true;
		}

		foreach(const Jid &itemJid, prefs.never)
		{
			if (itemJid.hasResource() && itemJid==AItemJid)
				return false;
			else if (!itemJid.hasResource() && itemJid.isBareEqual(AItemJid))
				return false;
		}

		if (prefs.defaults == ARCHIVE_SAVE_ROSTER)
		{
			IRoster *roster = FRosterManager!=NULL ? FRosterManager->findRoster(AStreamJid) : NULL;
			return roster!=NULL ? !roster->findItem(AItemJid).isNull() : false;
		}
		else if (prefs.defaults == ARCHIVE_SAVE_NEVER)
		{
			return false;
		}

		return true;
	}
	return false;
}

bool MessageArchiver::saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage)
{
	if (!isSupported(AStreamJid) || isArchiveDuplicationEnabled(AStreamJid))
	{
		if (isArchivingAllowed(AStreamJid,AItemJid))
		{
			IArchiveEngine *engine = findEngineByCapability(AStreamJid,IArchiveEngine::DirectArchiving);
			if (engine)
			{
				Message message = AMessage;
				bool directionIn = AItemJid==message.from() || AStreamJid==message.to();
				if (prepareMessage(AStreamJid,message,directionIn))
					return engine->saveMessage(AStreamJid,message,directionIn);
			}
		}
	}
	return false;
}

bool MessageArchiver::saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId)
{
	if (!isSupported(AStreamJid) || isArchiveDuplicationEnabled(AStreamJid))
	{
		if (isArchivingAllowed(AStreamJid,AItemJid))
		{
			IArchiveEngine *engine = findEngineByCapability(AStreamJid,IArchiveEngine::DirectArchiving);
			if (engine)
			{
				Message message;
				message.setTo(AStreamJid.full()).setFrom(AItemJid.full()).setBody(ANote).setThreadId(AThreadId);
				return engine->saveNote(AStreamJid,message,true);
			}
		}
	}
	return false;
}

QString MessageArchiver::loadMessages(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	QString id = loadHeaders(AStreamJid,ARequest);
	if (!id.isEmpty())
	{
		MessagesRequest request;
		request.request = ARequest;
		request.streamJid = AStreamJid;
		QString localId = QUuid::createUuid().toString();
		FRequestId2LocalId.insert(id,localId);
		FMesssagesRequests.insert(localId,request);
		LOG_STRM_DEBUG(AStreamJid,QString("Load messages request sent, id=%1").arg(localId));
		Logger::startTiming(STMP_HISTORY_MESSAGES_LOAD,localId);
		return localId;
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send load messages request: Headers not requested");
	}
	return QString::null;
}

QString MessageArchiver::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	HeadersRequest request;
	QString localId = QUuid::createUuid().toString();
	foreach(IArchiveEngine *engine, engineOrderByCapability(AStreamJid,IArchiveEngine::ArchiveManagement))
	{
		if (ARequest.text.isEmpty() || engine->isCapable(AStreamJid,IArchiveEngine::FullTextSearch))
		{
			QString id = engine->loadHeaders(AStreamJid,ARequest);
			if (!id.isEmpty())
			{
				request.engines.append(engine);
				FRequestId2LocalId.insert(id,localId);
			}
			else
			{
				LOG_STRM_WARNING(AStreamJid,QString("Failed to send load headers request to engine=%1").arg(engine->engineName()));
			}
		}
	}

	if (!request.engines.isEmpty())
	{
		request.request = ARequest;
		FHeadersRequests.insert(localId,request);
		LOG_STRM_DEBUG(AStreamJid,QString("Load headers request sent to %1 engines, id=%2").arg(request.engines.count()).arg(localId));
		Logger::startTiming(STMP_HISTORY_HEADERS_LOAD,localId);
		return localId;
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send load headers request to any engine");
	}

	return QString::null;
}

QString MessageArchiver::loadCollection(const Jid &AStreamJid, const IArchiveHeader &AHeader)
{
	IArchiveEngine *engine = findArchiveEngine(AHeader.engineId);
	if (engine)
	{
		QString id = engine->loadCollection(AStreamJid,AHeader);
		if (!id.isEmpty())
		{
			CollectionRequest request;
			QString localId = QUuid::createUuid().toString();
			FRequestId2LocalId.insert(id,localId);
			FCollectionRequests.insert(localId,request);
			LOG_STRM_DEBUG(AStreamJid,QString("Load collection request sent to engine=%1, id=%2").arg(engine->engineName(),localId));
			return localId;
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send load collection request to engine=%1").arg(engine->engineName()));
		}
	}
	else
	{
		REPORT_ERROR("Failed to send load collection request: Engine not found");
	}
	return QString::null;
}

QString MessageArchiver::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	RemoveRequest request;
	QString localId = QUuid::createUuid().toString();
	foreach(IArchiveEngine *engine, engineOrderByCapability(AStreamJid,IArchiveEngine::ArchiveManagement))
	{
		QString id = engine->removeCollections(AStreamJid,ARequest);
		if (!id.isEmpty())
		{
			FRequestId2LocalId.insert(id,localId);
			request.engines.append(engine);
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,QString("Failed to send remove collections request to engine=%1").arg(engine->engineName()));
		}
	}

	if (!request.engines.isEmpty())
	{
		request.request = ARequest;
		FRemoveRequests.insert(localId,request);
		LOG_STRM_DEBUG(AStreamJid,QString("Remove collections request sent to %1 engines, id=%2").arg(request.engines.count()).arg(localId));
		return localId;
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send remove collections request to any engine");
	}

	return QString::null;
}

void MessageArchiver::elementToCollection(const Jid &AStreamJid, const QDomElement &AChatElem, IArchiveCollection &ACollection) const
{
	ACollection.header.with = AChatElem.attribute("with");
	ACollection.header.start = DateTime(AChatElem.attribute("start")).toLocal();
	ACollection.header.subject = AChatElem.attribute("subject");
	ACollection.header.threadId = AChatElem.attribute("thread");
	ACollection.header.version = AChatElem.attribute("version").toUInt();

	QDomElement nodeElem = AChatElem.firstChildElement();

	bool isSecsFromStart;
	if (!AChatElem.hasAttribute("secsFromLast"))
	{
		int secsLast = 0;
		isSecsFromStart = true;
		while (!nodeElem.isNull() && isSecsFromStart)
		{
			if (nodeElem.hasAttribute("secs"))
			{
				int secs = nodeElem.attribute("secs").toInt();
				if (secs < secsLast)
					isSecsFromStart = false;
				secsLast = secs;
			}
			nodeElem = nodeElem.nextSiblingElement();
		}
	}
	else
	{
		isSecsFromStart = AChatElem.attribute("secsFromLast")!="true";
	}

	int secsLast = 0;
	QDateTime lastMessageDT;
	nodeElem = AChatElem.firstChildElement();
	while (!nodeElem.isNull())
	{
		if (nodeElem.tagName()=="to" || nodeElem.tagName()=="from")
		{
			Message message;

			Jid with = ACollection.header.with;
			QString nick = nodeElem.attribute("name");
			Jid contactJid(with.node(),with.domain(),nick.isEmpty() ? with.resource() : nick);

			if (nodeElem.tagName()=="to")
			{
				message.setTo(contactJid.full());
				message.setFrom(AStreamJid.full());
				message.setData(MDR_MESSAGE_DIRECTION,IMessageProcessor::DirectionOut);
			}
			else
			{
				message.setTo(AStreamJid.full());
				message.setFrom(contactJid.full());
				message.setData(MDR_MESSAGE_DIRECTION,IMessageProcessor::DirectionIn);
			}

			message.setType(nick.isEmpty() ? Message::Chat : Message::GroupChat);

			QString utc = nodeElem.attribute("utc");
			if (utc.isEmpty())
			{
				int secs = nodeElem.attribute("secs").toInt();
				QDateTime messageDT = ACollection.header.start.addSecs(isSecsFromStart ? secs : secsLast+secs);

				if (lastMessageDT.isValid() && lastMessageDT>=messageDT)
					messageDT = lastMessageDT.addMSecs(1);

				message.setDateTime(messageDT);
				lastMessageDT = messageDT;
			}
			else
			{
				QDateTime messageDT = DateTime(utc).toLocal();
				message.setDateTime(messageDT.isValid() ? messageDT : ACollection.header.start.addSecs(secsLast));
			}
			secsLast = ACollection.header.start.secsTo(message.dateTime());

			QDomElement childElem = nodeElem.firstChildElement();
			while (!childElem.isNull())
			{
				message.stanza().element().appendChild(childElem.cloneNode(true));
				childElem = childElem.nextSiblingElement();
			}
			message.setThreadId(ACollection.header.threadId);

			ACollection.body.messages.append(message);
		}
		else if (nodeElem.tagName() == "note")
		{
			QString utc = nodeElem.attribute("utc");
			ACollection.body.notes.insertMulti(DateTime(utc).toLocal(),nodeElem.text());
		}
		else if(nodeElem.tagName() == "next")
		{
			ACollection.next.with = nodeElem.attribute("with");
			ACollection.next.start = DateTime(nodeElem.attribute("start")).toLocal();
		}
		else if(nodeElem.tagName() == "previous")
		{
			ACollection.previous.with = nodeElem.attribute("with");
			ACollection.previous.start = DateTime(nodeElem.attribute("start")).toLocal();
		}
		else if (FDataForms && nodeElem.tagName()=="x" && nodeElem.namespaceURI()==NS_JABBER_DATA)
		{
			ACollection.attributes = FDataForms->dataForm(nodeElem);
		}
		nodeElem = nodeElem.nextSiblingElement();
	}
}

void MessageArchiver::collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem) const
{
	QDomDocument ownerDoc = AChatElem.ownerDocument();
	AChatElem.setAttribute("with",ACollection.header.with.full());
	AChatElem.setAttribute("start",DateTime(ACollection.header.start).toX85UTC());
	AChatElem.setAttribute("version",ACollection.header.version);
	if (!ACollection.header.subject.isEmpty())
		AChatElem.setAttribute("subject",ACollection.header.subject);
	if (!ACollection.header.threadId.isEmpty())
		AChatElem.setAttribute("thread",ACollection.header.threadId);
	AChatElem.setAttribute("secsFromLast","false");

	bool groupChat = false;
	QList<Message>::const_iterator messageIt = ACollection.body.messages.constBegin();
	QMultiMap<QDateTime,QString>::const_iterator noteIt = ACollection.body.notes.constBegin();
	while(messageIt!=ACollection.body.messages.constEnd() || noteIt!=ACollection.body.notes.constEnd())
	{
		bool writeNote = false;
		bool writeMessage = false;
		if (noteIt == ACollection.body.notes.constEnd())
			writeMessage = true;
		else if (messageIt == ACollection.body.messages.constEnd())
			writeNote = true;
		else if (messageIt->dateTime() <= noteIt.key())
			writeMessage = true;
		else
			writeNote = true;

		if (writeMessage)
		{
			groupChat |= messageIt->type()==Message::GroupChat;
			if (!groupChat || messageIt->fromJid().hasResource())
			{
				bool directionIn = ACollection.header.with.pBare() == messageIt->fromJid().pBare();
				QDomElement messageElem = AChatElem.appendChild(ownerDoc.createElement(directionIn ? "from" : "to")).toElement();

				int secs = ACollection.header.start.secsTo(messageIt->dateTime());
				if (secs >= 0)
					messageElem.setAttribute("secs",secs);
				else
					messageElem.setAttribute("utc",DateTime(messageIt->dateTime()).toX85UTC());

				if (groupChat)
					messageElem.setAttribute("name",messageIt->fromJid().resource());

				QDomElement childElem = messageIt->stanza().element().firstChildElement();
				while (!childElem.isNull())
				{
					if (childElem.tagName() != "thread")
						messageElem.appendChild(childElem.cloneNode(true));
					childElem = childElem.nextSiblingElement();
				}
			}
			++messageIt;
		}

		if (writeNote)
		{
			QDomElement noteElem = AChatElem.appendChild(ownerDoc.createElement("note")).toElement();
			noteElem.setAttribute("utc",DateTime(noteIt.key()).toX85UTC());
			noteElem.appendChild(ownerDoc.createTextNode(noteIt.value()));
			++noteIt;
		}
	}

	if (ACollection.previous.with.isValid() && ACollection.previous.start.isValid())
	{
		QDomElement prevElem = AChatElem.appendChild(ownerDoc.createElement("previous")).toElement();
		prevElem.setAttribute("with",ACollection.previous.with.full());
		prevElem.setAttribute("start",DateTime(ACollection.previous.start).toX85UTC());
	}

	if (ACollection.next.with.isValid() && ACollection.next.start.isValid())
	{
		QDomElement nextElem = AChatElem.appendChild(ownerDoc.createElement("next")).toElement();
		nextElem.setAttribute("with",ACollection.next.with.full());
		nextElem.setAttribute("start",DateTime(ACollection.next.start).toX85UTC());
	}

	if (FDataForms && FDataForms->isFormValid(ACollection.attributes))
	{
		FDataForms->xmlForm(ACollection.attributes,AChatElem);
	}
}

void MessageArchiver::insertArchiveHandler(int AOrder, IArchiveHandler *AHandler)
{
	if (AHandler)
		FArchiveHandlers.insertMulti(AOrder,AHandler);
}

void MessageArchiver::removeArchiveHandler(int AOrder, IArchiveHandler *AHandler)
{
	FArchiveHandlers.remove(AOrder,AHandler);
}

quint32 MessageArchiver::totalCapabilities(const Jid &AStreamJid) const
{
	quint32 caps = 0;
	foreach(IArchiveEngine *engine, FArchiveEngines)
	{
		if (isArchiveEngineEnabled(engine->engineId()))
			caps |= engine->capabilities(AStreamJid);
	}
	return caps;
}

QList<IArchiveEngine *> MessageArchiver::archiveEngines() const
{
	return FArchiveEngines.values();
}

IArchiveEngine *MessageArchiver::findArchiveEngine(const QUuid &AId) const
{
	return FArchiveEngines.value(AId);
}

bool MessageArchiver::isArchiveEngineEnabled(const QUuid &AId) const
{
	return Options::node(OPV_HISTORY_ENGINE_ITEM,AId.toString()).value("enabled").toBool();
}

void MessageArchiver::setArchiveEngineEnabled(const QUuid &AId, bool AEnabled)
{
	if (isArchiveEngineEnabled(AId) != AEnabled)
		Options::node(OPV_HISTORY_ENGINE_ITEM,AId.toString()).setValue(AEnabled,"enabled");
}

void MessageArchiver::registerArchiveEngine(IArchiveEngine *AEngine)
{
	if (AEngine!=NULL && !FArchiveEngines.contains(AEngine->engineId()))
	{
		LOG_DEBUG(QString("Archive engine registered, id=%1, name=%2").arg(AEngine->engineId(),AEngine->engineName()));
		connect(AEngine->instance(),SIGNAL(capabilitiesChanged(const Jid &)),
			SLOT(onEngineCapabilitiesChanged(const Jid &)));
		connect(AEngine->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
			SLOT(onEngineRequestFailed(const QString &, const XmppError &)));
		connect(AEngine->instance(),SIGNAL(collectionsRemoved(const QString &, const IArchiveRequest &)),
			SLOT(onEngineCollectionsRemoved(const QString &, const IArchiveRequest &)));
		connect(AEngine->instance(),SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
			SLOT(onEngineHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
		connect(AEngine->instance(),SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
			SLOT(onEngineCollectionLoaded(const QString &, const IArchiveCollection &)));
		FArchiveEngines.insert(AEngine->engineId(),AEngine);
		emit archiveEngineRegistered(AEngine);
		emit totalCapabilitiesChanged(Jid::null);
	}
}

QString MessageArchiver::archiveFilePath(const Jid &AStreamJid, const QString &AFileName) const
{
	if (AStreamJid.isValid() && !AFileName.isEmpty())
	{
		QString dirPath = archiveDirPath(AStreamJid);
		if (!dirPath.isEmpty())
			return dirPath+"/"+AFileName;
	}
	return QString::null;
}

QString MessageArchiver::loadServerPrefs(const Jid &AStreamJid)
{
	if (FStanzaProcessor)
	{
		Stanza load(STANZA_KIND_IQ);
		load.setType(STANZA_TYPE_GET).setUniqueId();
		load.addElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid));
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,load,ARCHIVE_REQUEST_TIMEOUT))
		{
			LOG_STRM_INFO(AStreamJid,QString("Load server archive prefs request sent, id=%1").arg(load.id()));
			FPrefsLoadRequests.insert(load.id(),AStreamJid);
			return load.id();
		}
		else
		{
			LOG_STRM_WARNING(AStreamJid,"Failed to send load server archive prefs request");
			loadStoragePrefs(AStreamJid);
		}
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send load server archive prefs request: StanzaProcessor is NULL");
		loadStoragePrefs(AStreamJid);
	}
	return QString::null;
}

QString MessageArchiver::loadStoragePrefs(const Jid &AStreamJid)
{
	FInStoragePrefs += AStreamJid;

	QString requestId = FPrivateStorage!=NULL ? FPrivateStorage->loadData(AStreamJid,PST_ARCHIVE_PREFS,PSN_ARCHIVE_PREFS) : QString::null;
	if (!requestId.isEmpty())
	{
		LOG_STRM_INFO(AStreamJid,QString("Load storage archive prefs request sent, id=%1").arg(requestId));
		FPrefsLoadRequests.insert(requestId,AStreamJid);
	}
	else
	{
		LOG_STRM_WARNING(AStreamJid,"Failed to send load storage archive prefs request");
		applyArchivePrefs(AStreamJid,QDomElement());
	}
	return requestId;
}

void MessageArchiver::applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem)
{
	LOG_STRM_INFO(AStreamJid,QString("Applying new archive prefs, ready=%1, in_storage=%2").arg(isReady(AStreamJid)).arg(FInStoragePrefs.contains(AStreamJid)));

	IArchiveStreamPrefs prefs;
	prefs.defaults = AElem.attribute("default", ARCHIVE_SAVE_ALWAYS);

	QDomElement alwaysJidElem = AElem.firstChildElement("always").firstChildElement("jid");
	while (!alwaysJidElem.isNull())
	{
		Jid itemJid = alwaysJidElem.text();
		if (itemJid.isValid())
			prefs.always += itemJid;
		alwaysJidElem = alwaysJidElem.nextSiblingElement("jid");
	}

	QDomElement neverJidElem = AElem.firstChildElement("never").firstChildElement("jid");
	while (!neverJidElem.isNull())
	{
		Jid itemJid = neverJidElem.text();
		if (itemJid.isValid())
			prefs.never += itemJid;
		neverJidElem = neverJidElem.nextSiblingElement("jid");
	}

	FArchivePrefs[AStreamJid] = prefs;
	emit archivePrefsChanged(AStreamJid);
}

void MessageArchiver::loadPendingMessages(const Jid &AStreamJid)
{
	QFile file(archiveFilePath(AStreamJid,PENDING_FILE_NAME));
	if (file.open(QFile::ReadOnly))
	{
		QString xmlError;
		QDomDocument doc;
		if (doc.setContent(&file,true,&xmlError))
		{
			if (AStreamJid.pBare() == doc.documentElement().attribute("jid"))
			{
				QList< QPair<Message,bool> > &messages = FPendingMessages[AStreamJid];
				QDomElement messageElem = doc.documentElement().firstChildElement("message");
				while (!messageElem.isNull())
				{
					bool directionIn = QVariant(messageElem.attribute("x-archive-direction-in")).toBool();
					messageElem.removeAttribute("x-archive-direction-in");

					Stanza stanza(messageElem);
					Message message(stanza);

					if (directionIn)
						message.setTo(AStreamJid.full());
					else
						message.setFrom(AStreamJid.full());
					messages.append(qMakePair<Message,bool>(message,directionIn));

					messageElem = messageElem.nextSiblingElement("message");
				}
				LOG_STRM_INFO(AStreamJid,QString("Pending messages loaded, count=%1").arg(messages.count()));
			}
			else
			{
				REPORT_ERROR("Failed to load pending messages from file content: Invalid stream JID");
				file.remove();
			}
		}
		else
		{
			REPORT_ERROR(QString("Failed to load pending messages from file content: %1").arg(xmlError));
			file.remove();
		}
	}
	else if (file.exists())
	{
		REPORT_ERROR(QString("Failed to load pending messages from file: %1").arg(file.errorString()));
	}
}

void MessageArchiver::savePendingMessages(const Jid &AStreamJid)
{
	QList< QPair<Message,bool> > messages = FPendingMessages.take(AStreamJid);
	if (!messages.isEmpty())
	{
		QDomDocument doc;
		doc.appendChild(doc.createElement("pending-messages"));
		doc.documentElement().setAttribute("version","1.0");
		doc.documentElement().setAttribute("jid",AStreamJid.pBare());

		for (int i=0; i<messages.count(); i++)
		{
			QPair<Message,bool> &message = messages[i];
			message.first.setDelayed(message.first.dateTime(),message.first.from());
			if (prepareMessage(AStreamJid,message.first,message.second))
			{
				QDomElement messageElem = doc.documentElement().appendChild(doc.importNode(message.first.stanza().element(),true)).toElement();
				messageElem.setAttribute("x-archive-direction-in",QVariant(message.second).toString());
			}
		}

		QFile file(archiveFilePath(AStreamJid,PENDING_FILE_NAME));
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			LOG_STRM_INFO(AStreamJid,QString("Pending messages saved, count=%1").arg(messages.count()));
			file.write(doc.toByteArray());
			file.close();
		}
		else
		{
			REPORT_ERROR(QString("Failed to save pending messages to file: %1").arg(file.errorString()));
		}
	}
}

void MessageArchiver::processPendingMessages(const Jid &AStreamJid)
{
	QList< QPair<Message,bool> > messages = FPendingMessages.take(AStreamJid);
	if (!messages.isEmpty())
	{
		LOG_STRM_INFO(AStreamJid,QString("Processing pending messages, count=%1").arg(messages.count()));
		for (int i = 0; i<messages.count(); i++)
		{
			QPair<Message, bool> message = messages.at(i);
			processMessage(AStreamJid, message.first, message.second);
		}
	}
	QFile::remove(archiveFilePath(AStreamJid,PENDING_FILE_NAME));
}

bool MessageArchiver::prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn)
{
	if (AMessage.body().isEmpty())
		return false;
	if (AMessage.type()==Message::Error)
		return false;
	if (AMessage.type()==Message::GroupChat && !ADirectionIn)
		return false;
	if (AMessage.type()==Message::GroupChat && AMessage.isDelayed())
		return false;

	if (ADirectionIn && AMessage.from().isEmpty())
		AMessage.setFrom(AStreamJid.domain());
	else if (!ADirectionIn && AMessage.to().isEmpty())
		AMessage.setTo(AStreamJid.domain());

	for (QMultiMap<int,IArchiveHandler *>::const_iterator it = FArchiveHandlers.constBegin(); it!=FArchiveHandlers.constEnd(); ++it)
	{
		IArchiveHandler *handler = it.value();
		if (handler->archiveMessageEdit(it.key(),AStreamJid,AMessage,ADirectionIn))
			return false;
	}

	if (AMessage.type()==Message::Chat || AMessage.type()==Message::GroupChat)
	{
		if (!AMessage.threadId().isEmpty())
			AMessage.setThreadId(QString::null);
	}

	return true;
}

bool MessageArchiver::processMessage(const Jid &AStreamJid, const Message &AMessage, bool ADirectionIn)
{
	Jid itemJid = ADirectionIn ? (!AMessage.from().isEmpty() ? AMessage.from() : AStreamJid.domain()) : AMessage.to();
	if (!isReady(AStreamJid))
	{
		FPendingMessages[AStreamJid].append(qMakePair<Message,bool>(AMessage,ADirectionIn));
		return true;
	}
	return saveMessage(AStreamJid,itemJid,AMessage);
}

IArchiveEngine *MessageArchiver::findEngineByCapability(const Jid &AStreamJid, quint32 ACapability) const
{
	QMultiMap<int, IArchiveEngine *> order = engineOrderByCapability(AStreamJid,ACapability);
	return !order.isEmpty() ? order.constBegin().value() : NULL;
}

QMultiMap<int, IArchiveEngine *> MessageArchiver::engineOrderByCapability(const Jid &AStreamJid, quint32 ACapability) const
{
	QMultiMap<int, IArchiveEngine *> order;
	for (QMap<QUuid,IArchiveEngine *>::const_iterator it=FArchiveEngines.constBegin(); it!=FArchiveEngines.constEnd(); ++it)
	{
		if (isArchiveEngineEnabled(it.key()))
		{
			int engineOrder = (*it)->capabilityOrder(ACapability,AStreamJid);
			if (engineOrder > 0)
				order.insertMulti(engineOrder,it.value());
		}
	}
	return order;
}

void MessageArchiver::processMessagesRequest(const QString &ALocalId, MessagesRequest &ARequest)
{
	if (!ARequest.lastError.isNull())
	{
		Logger::finishTiming(STMP_HISTORY_MESSAGES_LOAD,ALocalId);
		LOG_WARNING(QString("Failed to load messages, id=%1: %2").arg(ALocalId,ARequest.lastError.condition()));

		emit requestFailed(ALocalId,ARequest.lastError);
		FMesssagesRequests.remove(ALocalId);
	}
	else if (ARequest.headers.isEmpty() || (quint32)ARequest.body.messages.count()>ARequest.request.maxItems)
	{
		if (ARequest.request.order == Qt::AscendingOrder)
			qSort(ARequest.body.messages.begin(),ARequest.body.messages.end(),qLess<Message>());
		else
			qSort(ARequest.body.messages.begin(),ARequest.body.messages.end(),qGreater<Message>());

		REPORT_TIMING(STMP_HISTORY_MESSAGES_LOAD,Logger::finishTiming(STMP_HISTORY_MESSAGES_LOAD,ALocalId));
		LOG_DEBUG(QString("Messages successfully loaded, id=%1").arg(ALocalId));

		emit messagesLoaded(ALocalId,ARequest.body);
		FMesssagesRequests.remove(ALocalId);
	}
	else
	{
		QString id = loadCollection(ARequest.streamJid,ARequest.headers.takeFirst());
		if (!id.isEmpty())
		{
			FRequestId2LocalId.insert(id,ALocalId);
		}
		else
		{
			ARequest.lastError = XmppError(IERR_HISTORY_CONVERSATION_LOAD_ERROR);
			processMessagesRequest(ALocalId,ARequest);
		}
	}
}

void MessageArchiver::processHeadersRequest(const QString &ALocalId, HeadersRequest &ARequest)
{
	if (ARequest.engines.count() == ARequest.headers.count())
	{
		if (!ARequest.engines.isEmpty() || ARequest.lastError.isNull())
		{
			QList<IArchiveHeader> headers;
			foreach(IArchiveEngine *engine, ARequest.engines)
			{
				foreach(const IArchiveHeader &header, ARequest.headers.value(engine))
				{
					if (!headers.contains(header))
						headers.append(header);
				}
			}

			if (ARequest.request.order == Qt::AscendingOrder)
				qSort(headers.begin(),headers.end(),qLess<IArchiveHeader>());
			else
				qSort(headers.begin(),headers.end(),qGreater<IArchiveHeader>());

			if ((quint32)headers.count() > ARequest.request.maxItems)
				headers = headers.mid(0,ARequest.request.maxItems);

			REPORT_TIMING(STMP_HISTORY_HEADERS_LOAD,Logger::finishTiming(STMP_HISTORY_HEADERS_LOAD,ALocalId));
			LOG_DEBUG(QString("Headers successfully loaded, id=%1").arg(ALocalId));

			emit headersLoaded(ALocalId,headers);
		}
		else
		{
			Logger::finishTiming(STMP_HISTORY_HEADERS_LOAD,ALocalId);
			LOG_WARNING(QString("Failed to load headers, id=%1: %2").arg(ALocalId,ARequest.lastError.condition()));

			emit requestFailed(ALocalId,ARequest.lastError);
		}
		FHeadersRequests.remove(ALocalId);
	}
}

void MessageArchiver::processCollectionRequest(const QString &ALocalId, CollectionRequest &ARequest)
{
	if (ARequest.lastError.isNull())
	{
		LOG_DEBUG(QString("Collection successfully loaded, id=%1").arg(ALocalId));
		emit collectionLoaded(ALocalId,ARequest.collection);
	}
	else
	{
		LOG_WARNING(QString("Failed to load collection, id=%1").arg(ALocalId));
		emit requestFailed(ALocalId,ARequest.lastError);
	}
	FCollectionRequests.remove(ALocalId);
}

void MessageArchiver::processRemoveRequest(const QString &ALocalId, RemoveRequest &ARequest)
{
	if (ARequest.engines.isEmpty())
	{
		if (ARequest.lastError.isNull())
		{
			LOG_DEBUG(QString("Collections successfully removed, id=%1").arg(ALocalId));
			emit collectionsRemoved(ALocalId,ARequest.request);
		}
		else
		{
			LOG_WARNING(QString("Failed to remove collections, id=%1: %2").arg(ALocalId,ARequest.lastError.condition()));
			emit requestFailed(ALocalId,ARequest.lastError);
		}
		FRemoveRequests.remove(ALocalId);
	}
}

void MessageArchiver::registerDiscoFeatures()
{
	IDiscoFeature dfeature;
	dfeature.active = false;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.var = NS_ARCHIVE_MAM;
	dfeature.name = tr("Message Archive Management");
	dfeature.description = tr("Supports query and control an archive of messages stored on a server");
	FDiscovery->insertDiscoFeature(dfeature);
}

void MessageArchiver::openHistoryOptionsNode(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		QString historyNodeId = QString(OPN_ACCOUNTS_HISTORY).replace("[id]",AAccountId.toString());
		IOptionsDialogNode historyNode = { ONO_ACCOUNTS_HISTORY, historyNodeId, MNI_HISTORY, tr("History") };
		FOptionsManager->insertOptionsDialogNode(historyNode);
	}
}

void MessageArchiver::closeHistoryOptionsNode(const QUuid &AAccountId)
{
	if (FOptionsManager)
	{
		QString historyNodeId = QString(OPN_ACCOUNTS_HISTORY).replace("[id]",AAccountId.toString());
		FOptionsManager->removeOptionsDialogNode(historyNodeId);
	}
}

bool MessageArchiver::isArchiveDuplicationEnabled(const Jid &AStreamJid) const
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->findAccountByStream(AStreamJid) : NULL;
	return account!=NULL ? account->optionsNode().value("history-duplicate").toBool() : false;
}

bool MessageArchiver::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	int singleKind = -1;
	foreach(IRosterIndex *index, ASelected)
	{
		Jid contactJid = index->data(RDR_FULL_JID).toString();
		if (!contactJid.isValid())
			return false;
		else if (singleKind!=-1 && singleKind!=index->kind())
			return false;
		singleKind = index->kind();
	}
	return !ASelected.isEmpty();
}

Menu *MessageArchiver::createContextMenu(const QStringList &AStreams, const QStringList &AContacts, QWidget *AParent) const
{
	QString allDefaults;
	bool isAllNever = true;
	bool isAllAlways = true;
	bool isAllDefault = true;
	bool isAllSupported = true;
	bool isAnyMangement = false;
	bool isMultiSelection = AStreams.count()>1;
	bool isStreamMenu = AContacts.first().isEmpty();

	for (int i=0; i<AStreams.count(); i++)
	{
		isAllSupported = isAllSupported && isPrefsSupported(AStreams.at(i));

		if (isAllSupported)
		{
			const IArchiveStreamPrefs &prefs = FArchivePrefs[AStreams.at(i)];

			if (allDefaults.isNull())
				allDefaults = prefs.defaults;
			else if (allDefaults != prefs.defaults)
				allDefaults = "-=different=-";

			if (prefs.always.contains(AContacts.at(i)))
			{
				isAllDefault = false;
				isAllNever = false;
			}
			else if (prefs.never.contains(AContacts.at(i)))
			{
				isAllDefault = false;
				isAllAlways = false;
			}
			else
			{
				isAllAlways = false;
				isAllNever = false;
			}
		}

		isAnyMangement = isAnyMangement || !engineOrderByCapability(AStreams.at(i),IArchiveEngine::ArchiveManagement).isEmpty();
	}

	Menu *menu = new Menu(AParent);
	menu->setTitle(tr("History"));
	menu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);

	if (isAnyMangement)
	{
		Action *viewAction = new Action(menu);
		viewAction->setText(tr("View History"));
		viewAction->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);
		viewAction->setData(ADR_STREAM_JID,AStreams);
		viewAction->setData(ADR_CONTACT_JID,AContacts);
		viewAction->setShortcutId(SCT_ROSTERVIEW_SHOWHISTORY);
		connect(viewAction,SIGNAL(triggered(bool)),SLOT(onShowArchiveWindowByAction(bool)));
		menu->addAction(viewAction,AG_DEFAULT,false);
	}

	if (isAllSupported && !isStreamMenu)
	{
		Action *alwaysAction = new Action(menu);
		alwaysAction->setCheckable(true);
		alwaysAction->setChecked(isAllAlways);
		alwaysAction->setText(tr("Always Save Messages History"));
		alwaysAction->setData(ADR_STREAM_JID,AStreams);
		alwaysAction->setData(ADR_CONTACT_JID,AContacts);
		alwaysAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_ALWAYS);
		connect(alwaysAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(alwaysAction,AG_DEFAULT+200);

		Action *neverAction = new Action(menu);
		neverAction->setCheckable(true);
		neverAction->setChecked(isAllNever);
		neverAction->setText(tr("Never Save Messages History"));
		neverAction->setData(ADR_STREAM_JID,AStreams);
		neverAction->setData(ADR_CONTACT_JID,AContacts);
		neverAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_NEVER);
		connect(neverAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(neverAction,AG_DEFAULT+200);

		Action *defaultAction = new Action(menu);
		defaultAction->setCheckable(true);
		defaultAction->setChecked(isAllDefault);
		defaultAction->setText(tr("Use Default Options"));
		defaultAction->setData(ADR_STREAM_JID,AStreams);
		defaultAction->setData(ADR_CONTACT_JID,AContacts);
		defaultAction->setData(ADR_ITEM_SAVE,QString());
		connect(defaultAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(defaultAction,AG_DEFAULT+200,false);

		QActionGroup *actionsGroup = new QActionGroup(menu);
		actionsGroup->addAction(alwaysAction);
		actionsGroup->addAction(neverAction);
		actionsGroup->addAction(defaultAction);
	}
	else if (isAllSupported && isStreamMenu)
	{
		Action *rosterAction = new Action(menu);
		rosterAction->setCheckable(true);
		rosterAction->setChecked(allDefaults == ARCHIVE_SAVE_ROSTER);
		rosterAction->setText(tr("Archive Messages from Contacts in Roster"));
		rosterAction->setData(ADR_STREAM_JID,AStreams);
		rosterAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_ROSTER);
		connect(rosterAction,SIGNAL(triggered(bool)),SLOT(onSetDefaultPrefsByAction(bool)));
		menu->addAction(rosterAction,AG_DEFAULT+200,false);

		Action *alwaysAction = new Action(menu);
		alwaysAction->setCheckable(true);
		alwaysAction->setChecked(allDefaults == ARCHIVE_SAVE_ALWAYS);
		alwaysAction->setText(tr("Archive all Messages by Default"));
		alwaysAction->setData(ADR_STREAM_JID,AStreams);
		alwaysAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_ALWAYS);
		connect(alwaysAction,SIGNAL(triggered(bool)),SLOT(onSetDefaultPrefsByAction(bool)));
		menu->addAction(alwaysAction,AG_DEFAULT+200,false);

		Action *neverAction = new Action(menu);
		neverAction->setCheckable(true);
		neverAction->setChecked(allDefaults == ARCHIVE_SAVE_NEVER);
		neverAction->setText(tr("Do not Archive Messages by Default"));
		neverAction->setData(ADR_STREAM_JID,AStreams);
		neverAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_NEVER);
		connect(neverAction,SIGNAL(triggered(bool)),SLOT(onSetDefaultPrefsByAction(bool)));
		menu->addAction(neverAction,AG_DEFAULT+200,false);

		QActionGroup *actionsGroup = new QActionGroup(menu);
		actionsGroup->addAction(rosterAction);
		actionsGroup->addAction(alwaysAction);
		actionsGroup->addAction(neverAction);
	}

	if (isAllSupported && !isMultiSelection && isStreamMenu)
	{
		Action *optionsAction = new Action(menu);
		optionsAction->setText(tr("Options..."));
		optionsAction->setData(ADR_STREAM_JID,AStreams.first());
		connect(optionsAction,SIGNAL(triggered(bool)),SLOT(onShowHistoryOptionsDialogByAction(bool)));
		menu->addAction(optionsAction,AG_DEFAULT+500,false);
	}

	return menu;
}

void MessageArchiver::notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const
{
	IMessageChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AContactJid,true) : NULL;
	if (window)
	{
		IMessageStyleContentOptions options;
		options.kind = IMessageStyleContentOptions::KindStatus;
		options.type |= IMessageStyleContentOptions::TypeEvent;
		options.direction = IMessageStyleContentOptions::DirectionIn;
		options.time = QDateTime::currentDateTime();
		window->viewWidget()->appendText(AMessage,options);
	}
}

void MessageArchiver::onEngineCapabilitiesChanged(const Jid &AStreamJid)
{
	emit totalCapabilitiesChanged(AStreamJid);
}

void MessageArchiver::onEngineRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		IArchiveEngine *engine = qobject_cast<IArchiveEngine *>(sender());
		if (FHeadersRequests.contains(localId))
		{
			HeadersRequest &request = FHeadersRequests[localId];
			request.lastError = AError;
			request.engines.removeAll(engine);
			processHeadersRequest(localId,request);
		}
		else if (FCollectionRequests.contains(localId))
		{
			CollectionRequest &request = FCollectionRequests[localId];
			request.lastError = AError;
			processCollectionRequest(localId,request);
		}
		else if (FRemoveRequests.contains(localId))
		{
			RemoveRequest &request = FRemoveRequests[localId];
			request.lastError = AError;
			request.engines.removeAll(engine);
			processRemoveRequest(localId,request);
		}
	}
}

void MessageArchiver::onEngineHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FHeadersRequests.contains(localId))
		{
			IArchiveEngine *engine = qobject_cast<IArchiveEngine *>(sender());
			HeadersRequest &request = FHeadersRequests[localId];
			request.headers.insert(engine,AHeaders);
			processHeadersRequest(localId,request);
		}
	}
}

void MessageArchiver::onEngineCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FCollectionRequests.contains(localId))
		{
			CollectionRequest &request = FCollectionRequests[localId];
			request.collection = ACollection;
			processCollectionRequest(localId,request);
		}
	}
}

void MessageArchiver::onEngineCollectionsRemoved(const QString &AId, const IArchiveRequest &ARequest)
{
	Q_UNUSED(ARequest);
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FRemoveRequests.contains(localId))
		{
			IArchiveEngine *engine = qobject_cast<IArchiveEngine *>(sender());
			RemoveRequest &request = FRemoveRequests[localId];
			request.engines.removeAll(engine);
			processRemoveRequest(localId,request);
		}
	}
}

void MessageArchiver::onSelfRequestFailed(const QString &AId, const XmppError &AError)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FMesssagesRequests.contains(localId))
		{
			MessagesRequest &request = FMesssagesRequests[localId];
			request.lastError = AError;
			processMessagesRequest(localId,request);
		}
	}
}

void MessageArchiver::onSelfHeadersLoaded(const QString &AId, const QList<IArchiveHeader> &AHeaders)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FMesssagesRequests.contains(localId))
		{
			MessagesRequest &request = FMesssagesRequests[localId];
			request.headers = AHeaders;
			processMessagesRequest(localId,request);
		}
	}
}

void MessageArchiver::onSelfCollectionLoaded(const QString &AId, const IArchiveCollection &ACollection)
{
	if (FRequestId2LocalId.contains(AId))
	{
		QString localId = FRequestId2LocalId.take(AId);
		if (FMesssagesRequests.contains(localId))
		{
			MessagesRequest &request = FMesssagesRequests[localId];
			request.body.messages += ACollection.body.messages;
			request.body.notes += ACollection.body.notes;
			processMessagesRequest(localId,request);
		}
	}
}

void MessageArchiver::onAccountInserted(IAccount *AAccount)
{
	openHistoryOptionsNode(AAccount->accountId());
}

void MessageArchiver::onAccountRemoved(IAccount *AAccount)
{
	closeHistoryOptionsNode(AAccount->accountId());
}

void MessageArchiver::onXmppStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_ARCHIVE_PREFS);
		shandle.conditions.append(SHC_ARCHIVE_PREFS_0);
		FSHIPrefs.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.conditions.clear();
		shandle.conditions.append(SHC_MESSAGE_BODY);
		FSHIMessageIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.direction = IStanzaHandle::DirectionOut;
		FSHIMessageOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	loadPendingMessages(AXmppStream->streamJid());

	if (FDiscovery == NULL)
		loadServerPrefs(AXmppStream->streamJid());
}

void MessageArchiver::onXmppStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPrefs.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessageIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessageOut.take(AXmppStream->streamJid()));
	}

	savePendingMessages(AXmppStream->streamJid());

	FNamespaces.remove(AXmppStream->streamJid());
	FArchivePrefs.remove(AXmppStream->streamJid());
	FInStoragePrefs.remove(AXmppStream->streamJid());

	emit archivePrefsChanged(AXmppStream->streamJid());
	emit archivePrefsClosed(AXmppStream->streamJid());
}

void MessageArchiver::onPrivateDataLoadedSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	if (FPrefsLoadRequests.contains(AId))
	{
		LOG_STRM_INFO(AStreamJid,QString("Storage archive prefs loaded, id=%1").arg(AId));
		FPrefsLoadRequests.remove(AId);
		
		applyArchivePrefs(AStreamJid,AElement);
		emit requestCompleted(AId);
	}
	else if (FPrefsSaveRequests.contains(AId))
	{
		LOG_STRM_INFO(AStreamJid,QString("Storage archive prefs saved, id=%1").arg(AId));
		FPrefsSaveRequests.remove(AId);
		
		applyArchivePrefs(AStreamJid,AElement);
		emit requestCompleted(AId);
	}
}

void MessageArchiver::onPrivateDataChanged(const Jid &AStreamJid, const QString &ATagName, const QString &ANamespace)
{
	if (FInStoragePrefs.contains(AStreamJid) && ATagName==PST_ARCHIVE_PREFS && ANamespace==PSN_ARCHIVE_PREFS)
	{
		loadStoragePrefs(AStreamJid);
	}
}

void MessageArchiver::onShortcutActivated(const QString &AId, QWidget *AWidget)
{
	if (FRostersViewPlugin && AWidget==FRostersViewPlugin->rostersView()->instance())
	{
		QList<IRosterIndex *> indexes = FRostersViewPlugin->rostersView()->selectedRosterIndexes();
		if (AId==SCT_ROSTERVIEW_SHOWHISTORY && isSelectionAccepted(indexes))
		{
			QMultiMap<Jid,Jid> addresses;
			foreach(IRosterIndex *index, indexes)
			{
				if (index->kind() == RIK_STREAM_ROOT)
				{
					addresses.insertMulti(index->data(RDR_STREAM_JID).toString(),Jid::null);
				}
				else if (index->kind() == RIK_METACONTACT)
				{
					for (int row=0; row<index->childCount(); row++)
					{
						IRosterIndex *metaItemIndex = index->childIndex(row);
						addresses.insertMulti(metaItemIndex->data(RDR_STREAM_JID).toString(),metaItemIndex->data(RDR_PREP_BARE_JID).toString());
					}
				}
				else
				{
					addresses.insertMulti(index->data(RDR_STREAM_JID).toString(),index->data(RDR_PREP_BARE_JID).toString());
				}
			}
			showArchiveWindow(addresses);
		}
	}
}

void MessageArchiver::onRostersViewIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MessageArchiver::onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu)
{
	if (ALabelId==AdvancedDelegateItem::DisplayId && isSelectionAccepted(AIndexes))
	{
		int indexKind = AIndexes.first()->kind();
		QMap<int, QStringList> rolesMap = FRostersViewPlugin->rostersView()->indexesRolesMap(AIndexes,
			QList<int>()<<RDR_STREAM_JID<<RDR_PREP_BARE_JID<<RDR_ANY_ROLE,RDR_PREP_BARE_JID,RDR_STREAM_JID);
		
		Menu *menu;
		if (indexKind == RIK_STREAM_ROOT)
			menu = createContextMenu(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_ANY_ROLE),AMenu);
		else
			menu = createContextMenu(rolesMap.value(RDR_STREAM_JID),rolesMap.value(RDR_PREP_BARE_JID),AMenu);

		if (!menu->isEmpty())
			AMenu->addAction(menu->menuAction(),AG_RVCM_ARCHIVER_OPTIONS,true);
		else
			delete menu;
	}
}

void MessageArchiver::onSetItemPrefsByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList contacts = action->data(ADR_CONTACT_JID).toStringList();
		QString itemSave = action->data(ADR_ITEM_SAVE).toString();

		QMap<Jid, IArchiveStreamPrefs> updatedPrefs;
		for (int i=0; i<streams.count(); i++)
		{
			Jid streamJid = streams.at(i);
			Jid contactJid = contacts.at(i);

			if (!updatedPrefs.contains(streamJid))
				updatedPrefs.insert(streamJid, archivePrefs(streamJid));
			IArchiveStreamPrefs &prefs = updatedPrefs[streamJid];

			if (itemSave == ARCHIVE_SAVE_ALWAYS)
			{
				prefs.never -= contactJid;
				prefs.always += contactJid;
			}
			else if (itemSave == ARCHIVE_SAVE_NEVER)
			{
				prefs.never += contactJid;
				prefs.always -= contactJid;
			}
			else
			{
				prefs.never -= contactJid;
				prefs.always -= contactJid;
			}
		}

		for (QMap<Jid, IArchiveStreamPrefs>::const_iterator it=updatedPrefs.constBegin(); it!=updatedPrefs.constEnd(); ++it)
			setArchivePrefs(it.key(),it.value());
	}
}

void MessageArchiver::onSetDefaultPrefsByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QString defaults = action->data(ADR_ITEM_SAVE).toString();
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		for (int i=0; i<streams.count(); i++)
		{
			IArchiveStreamPrefs prefs = archivePrefs(streams.at(i));
			if (prefs.defaults != defaults)
			{
				prefs.defaults = defaults;
				setArchivePrefs(streams.at(i),prefs);
			}
		}
	}
}

void MessageArchiver::onShowArchiveWindowByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		QMultiMap<Jid,Jid> addresses;
		QStringList streams = action->data(ADR_STREAM_JID).toStringList();
		QStringList contacts = action->data(ADR_CONTACT_JID).toStringList();
		for(int i=0; i<streams.count() && i<contacts.count(); i++)
			addresses.insertMulti(streams.at(i),contacts.at(i));
		showArchiveWindow(addresses);
	}
}

void MessageArchiver::onShowArchiveWindowByToolBarAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IMessageToolBarWidget *toolBarWidget = qobject_cast<IMessageToolBarWidget *>(action->parent());
		if (toolBarWidget)
			showArchiveWindow(toolBarWidget->messageWindow()->address()->availAddresses(true));
	}
}

void MessageArchiver::onShowHistoryOptionsDialogByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (FOptionsManager && FAccountManager && action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		IAccount *account = FAccountManager->findAccountByStream(streamJid);
		if (account)
		{
			QString rootId = OPN_ACCOUNTS"."+account->accountId().toString();
			QString nodeId = QString(OPN_ACCOUNTS_HISTORY).replace("[id]",account->accountId().toString());
			FOptionsManager->showOptionsDialog(nodeId, rootId);
		}
	}
}

void MessageArchiver::onDiscoveryInfoReceived(const IDiscoInfo &AInfo)
{
	if (!FNamespaces.contains(AInfo.streamJid) && !FInStoragePrefs.contains(AInfo.streamJid) && AInfo.node.isEmpty() && AInfo.streamJid.pDomain()==AInfo.contactJid.pFull())
	{
		if (AInfo.features.contains(NS_ARCHIVE_MAM))
		{
			FNamespaces.insert(AInfo.streamJid,NS_ARCHIVE_MAM);
			loadServerPrefs(AInfo.streamJid);
		}
		else if (AInfo.features.contains(NS_ARCHIVE_MAM_0))
		{
			FNamespaces.insert(AInfo.streamJid,NS_ARCHIVE_MAM_0);
			loadServerPrefs(AInfo.streamJid);
		}
		else
		{
			FNamespaces.remove(AInfo.streamJid);
			loadStoragePrefs(AInfo.streamJid);
		}
	}
}

void MessageArchiver::onToolBarWidgetCreated(IMessageToolBarWidget *AWidget)
{
	Action *action = new Action(AWidget->toolBarChanger()->toolBar());
	action->setText(tr("View History"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);
	action->setShortcutId(SCT_MESSAGEWINDOWS_SHOWHISTORY);
	connect(action,SIGNAL(triggered(bool)),SLOT(onShowArchiveWindowByToolBarAction(bool)));
	QToolButton *historyButton = AWidget->toolBarChanger()->insertAction(action,TBG_MWTBW_ARCHIVE_VIEW);

	ChatWindowMenu *historyMenu = new ChatWindowMenu(this,AWidget,AWidget->toolBarChanger()->toolBar());
	historyButton->setMenu(historyMenu);
	historyButton->setPopupMode(QToolButton::MenuButtonPopup);
}

void MessageArchiver::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.cleanPath() == OPV_HISTORY_ENGINE_ENABLED)
	{
		QUuid id = ANode.parent().nspace();
		emit archiveEngineEnableChanged(id,ANode.value().toBool());
		emit totalCapabilitiesChanged(Jid::null);
	}
}

Q_EXPORT_PLUGIN2(plg_messagearchiver, MessageArchiver)
