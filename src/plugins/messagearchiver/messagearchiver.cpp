#include "messagearchiver.h"

#include <QDir>
#include <QFile>

#define ARCHIVE_TIMEOUT       30000
#define ARCHIVE_DIR_NAME      "archive"
#define SESSIONS_FILE_NAME    "sessions.xml"

#define SHC_MESSAGE_BODY      "/message/body"
#define SHC_PREFS             "/iq[@type='set']/pref[@xmlns="NS_ARCHIVE"]"
#define SHC_PREFS_OLD         "/iq[@type='set']/pref[@xmlns="NS_ARCHIVE_OLD"]"

#define ADR_STREAM_JID        Action::DR_StreamJid
#define ADR_CONTACT_JID       Action::DR_Parametr1
#define ADR_ITEM_SAVE         Action::DR_Parametr2
#define ADR_ITEM_OTR          Action::DR_Parametr3
#define ADR_METHOD_LOCAL      Action::DR_Parametr1
#define ADR_METHOD_AUTO       Action::DR_Parametr2
#define ADR_METHOD_MANUAL     Action::DR_Parametr3
#define ADR_FILTER_START      Action::DR_Parametr2
#define ADR_FILTER_END        Action::DR_Parametr3
#define ADR_GROUP_KIND        Action::DR_Parametr4

#define SFP_LOGGING           "logging"
#define SFV_MAY_LOGGING       "may"
#define SFV_MUSTNOT_LOGGING   "mustnot"

#define PST_ARCHIVE_PREFS     "pref"
#define PSN_ARCHIVE_PREFS     NS_ARCHIVE

#define NS_ARCHIVE_OLD        "http://www.xmpp.org/extensions/xep-0136.html#ns"
#define NS_ARCHIVE_OLD_AUTO   "http://www.xmpp.org/extensions/xep-0136.html#ns-auto"
#define NS_ARCHIVE_OLD_MANAGE "http://www.xmpp.org/extensions/xep-0136.html#ns-manage"
#define NS_ARCHIVE_OLD_MANUAL "http://www.xmpp.org/extensions/xep-0136.html#ns-manual"
#define NS_ARCHIVE_OLD_PREF   "http://www.xmpp.org/extensions/xep-0136.html#ns-pref"

MessageArchiver::MessageArchiver()
{
	FPluginManager = NULL;
	FXmppStreams = NULL;
	FStanzaProcessor = NULL;
	FOptionsManager = NULL;
	FPrivateStorage = NULL;
	FAccountManager = NULL;
	FRostersViewPlugin = NULL;
	FDiscovery = NULL;
	FDataForms = NULL;
	FMessageWidgets = NULL;
	FSessionNegotiation = NULL;
	FRosterPlugin = NULL;
	FMultiUserChatPlugin = NULL;
}

MessageArchiver::~MessageArchiver()
{

}

void MessageArchiver::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("History");
	APluginInfo->description = tr("Allows to save the history of communications");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool MessageArchiver::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
	if (plugin)
	{
		FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
		if (FXmppStreams)
		{
			connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
			connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
		}
	}

	plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
	if (plugin)
		FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IPrivateStorage").value(0,NULL);
	if (plugin)
	{
		FPrivateStorage = qobject_cast<IPrivateStorage *>(plugin->instance());
		if (FPrivateStorage)
		{
			connect(FPrivateStorage->instance(),SIGNAL(dataError(const QString &, const QString &)),
				SLOT(onPrivateDataError(const QString &, const QString &)));
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
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexMultiSelection(const QList<IRosterIndex *> &, bool &)), 
				SLOT(onRosterIndexMultiSelection(const QList<IRosterIndex *> &, bool &)));
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(indexContextMenu(const QList<IRosterIndex *> &, int, Menu *)), 
				SLOT(onRosterIndexContextMenu(const QList<IRosterIndex *> &, int, Menu *)));
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
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMessageWidgets").value(0,NULL);
	if (plugin)
	{
		FMessageWidgets = qobject_cast<IMessageWidgets *>(plugin->instance());
		if (FMessageWidgets)
		{
			connect(FMessageWidgets->instance(),SIGNAL(toolBarWidgetCreated(IToolBarWidget *)),SLOT(onToolBarWidgetCreated(IToolBarWidget *)));
		}
	}

	plugin = APluginManager->pluginInterface("ISessionNegotiation").value(0,NULL);
	if (plugin)
	{
		FSessionNegotiation = qobject_cast<ISessionNegotiation *>(plugin->instance());
		if (FSessionNegotiation)
		{
			connect(FSessionNegotiation->instance(),SIGNAL(sessionActivated(const IStanzaSession &)),
				SLOT(onStanzaSessionActivated(const IStanzaSession &)));
			connect(FSessionNegotiation->instance(),SIGNAL(sessionTerminated(const IStanzaSession &)),
				SLOT(onStanzaSessionTerminated(const IStanzaSession &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());

	plugin = APluginManager->pluginInterface("IMultiUserChatPlugin").value(0,NULL);
	if (plugin)
	{
		FMultiUserChatPlugin = qobject_cast<IMultiUserChatPlugin *>(plugin->instance());
		if (FMultiUserChatPlugin)
		{
			connect(FMultiUserChatPlugin->instance(),SIGNAL(multiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)),
				SLOT(onMultiUserContextMenu(IMultiUserChatWindow *, IMultiUser *, Menu *)));
		}
	}

	connect(this,SIGNAL(requestFailed(const QString &, const QString &)),
		SLOT(onSelfRequestFailed(const QString &, const QString &)));
	connect(this,SIGNAL(headersLoaded(const QString &, const QList<IArchiveHeader> &)),
		SLOT(onSelfHeadersLoaded(const QString &, const QList<IArchiveHeader> &)));
	connect(this,SIGNAL(collectionLoaded(const QString &, const IArchiveCollection &)),
		SLOT(onSelfCollectionLoaded(const QString &, const IArchiveCollection &)));

	connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));
	connect(Shortcuts::instance(),SIGNAL(shortcutActivated(const QString &, QWidget *)),SLOT(onShortcutActivated(const QString &, QWidget *)));

	return FXmppStreams!=NULL && FStanzaProcessor!=NULL;
}

bool MessageArchiver::initObjects()
{
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_SHOWHISTORY, tr("Show history"), tr("Ctrl+H","Show history"));
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_HISTORYENABLE, tr("Enable message archiving"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_HISTORYDISABLE, tr("Disable message archiving"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_HISTORYREQUIREOTR, tr("Start Off-The-Record session"), QKeySequence::UnknownKey);
	Shortcuts::declareShortcut(SCT_MESSAGEWINDOWS_HISTORYTERMINATEOTR, tr("Terminate Off-The-Record session"), QKeySequence::UnknownKey);

	Shortcuts::declareShortcut(SCT_ROSTERVIEW_SHOWHISTORY,tr("Show history"),tr("Ctrl+H","Show history"),Shortcuts::WidgetShortcut);

	if (FDiscovery)
	{
		registerDiscoFeatures();
	}
	if (FSessionNegotiation)
	{
		FSessionNegotiation->insertNegotiator(this,SNO_DEFAULT);
	}
	if (FRostersViewPlugin)
	{
		Shortcuts::insertWidgetShortcut(SCT_ROSTERVIEW_SHOWHISTORY,FRostersViewPlugin->rostersView()->instance());
	}
	return true;
}

bool MessageArchiver::initSettings()
{
	Options::setDefaultValue(OPV_ACCOUNT_HISTORYREPLICATION,false);
	Options::setDefaultValue(OPV_HISTORY_ENGINE_ENABLED,true);
	Options::setDefaultValue(OPV_HISTORY_ARCHIVEVIEW_FONTPOINTSIZE,10);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_HISTORY, OPN_HISTORY, tr("History"), MNI_HISTORY };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

bool MessageArchiver::stanzaReadWrite(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
	if (FSHIMessageBlocks.value(AStreamJid) == AHandlerId)
	{
		Jid contactJid = AStanza.to();
		IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,contactJid,AStanza.firstElement("thread").text());
		if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && !isOTRStanzaSession(AStreamJid,contactJid))
		{
			int initResult = FSessionNegotiation!=NULL ? FSessionNegotiation->initSession(AStreamJid,contactJid) : ISessionNegotiator::Cancel;
			if (initResult == ISessionNegotiator::Skip)
				notifyInChatWindow(AStreamJid,contactJid, tr("Off-The-Record session not ready, please wait..."));
			else if (initResult != ISessionNegotiator::Cancel)
				notifyInChatWindow(AStreamJid,contactJid, tr("Negotiating Off-The-Record session..."));
			return true;
		}
	}
	else if (FSHIMessageIn.value(AStreamJid) == AHandlerId)
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
		QDomElement prefElem = AStanza.firstElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid));
		applyArchivePrefs(AStreamJid,prefElem);

		AAccept = true;
		Stanza result = FStanzaProcessor->makeReplyResult(AStanza);
		FStanzaProcessor->sendStanzaOut(AStreamJid,result);
	}
	return false;
}

void MessageArchiver::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
	if (FPrefsLoadRequests.contains(AStanza.id()))
	{
		if (AStanza.type() == "result")
		{
			QDomElement prefElem = AStanza.firstElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid));
			applyArchivePrefs(AStreamJid,prefElem);
		}
		else
		{
			applyArchivePrefs(AStreamJid,QDomElement());
		}
		FPrefsLoadRequests.remove(AStanza.id());
	}
	else if (FPrefsSaveRequests.contains(AStanza.id()))
	{
		FPrefsSaveRequests.remove(AStanza.id());
		if (AStanza.type() == "result")
			startSuspendedStanzaSession(AStreamJid,AStanza.id());
		else
			cancelSuspendedStanzaSession(AStreamJid,AStanza.id(),ErrorHandler(AStanza.element()).message());
	}
	else if (FPrefsAutoRequests.contains(AStanza.id()))
	{
		if (isReady(AStreamJid) && AStanza.type()=="result")
		{
			bool autoSave = FPrefsAutoRequests.value(AStanza.id());
			FArchivePrefs[AStreamJid].autoSave = autoSave;

			if (!isArchivePrefsEnabled(AStreamJid))
				applyArchivePrefs(AStreamJid,QDomElement());
			else if (!isSupported(AStreamJid,NS_ARCHIVE_PREF))
				loadStoragePrefs(AStreamJid);

			emit archivePrefsChanged(AStreamJid);
		}
		FPrefsAutoRequests.remove(AStanza.id());
	}
	else if (FPrefsRemoveItemRequests.contains(AStanza.id()))
	{
		if (isReady(AStreamJid) && AStanza.type()=="result")
		{
			Jid itemJid = FPrefsRemoveItemRequests.value(AStanza.id());
			FArchivePrefs[AStreamJid].itemPrefs.remove(itemJid);
			applyArchivePrefs(AStreamJid,QDomElement());
		}
		FPrefsRemoveItemRequests.remove(AStanza.id());
	}
	else if (FPrefsRemoveSessionRequests.contains(AStanza.id()))
	{
		if (isReady(AStreamJid) && AStanza.type()=="result")
		{
			QString threadId = FPrefsRemoveSessionRequests.value(AStanza.id());
			FArchivePrefs[AStreamJid].sessionPrefs.remove(threadId);
			applyArchivePrefs(AStreamJid,QDomElement());
		}
		FPrefsRemoveSessionRequests.remove(AStanza.id());
	}

	if (FRestoreRequests.contains(AStanza.id()))
	{
		QString sessionId = FRestoreRequests.take(AStanza.id());
		if (AStanza.type() == "result")
			removeStanzaSessionContext(AStreamJid,sessionId);
	}

	if (AStanza.type() == "result")
		emit requestCompleted(AStanza.id());
	else
		emit requestFailed(AStanza.id(),ErrorHandler(AStanza.element()).message());
}

QMultiMap<int, IOptionsWidget *> MessageArchiver::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *>  widgets;
	QStringList nodeTree = ANodeId.split(".",QString::SkipEmptyParts);
	if (nodeTree.count()==2 && nodeTree.at(0)==OPN_HISTORY)
	{
		IAccount *account = FAccountManager!=NULL ? FAccountManager->accountById(nodeTree.at(1)) : NULL;
		if (account && account->isActive() && isReady(account->xmppStream()->streamJid()))
		{
			widgets.insertMulti(OWO_HISTORY_STREAM, new ArchiveStreamOptions(this,account->xmppStream()->streamJid(),AParent));
		}
	}
	else if(ANodeId == OPN_HISTORY)
	{
		widgets.insertMulti(OWO_HISTORY_ENGINES, new ArchiveEnginesOptions(this,AParent));
	}
	return widgets;
}

int MessageArchiver::sessionInit(const IStanzaSession &ASession, IDataForm &ARequest)
{
	IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
	int result = itemPrefs.otr!=ARCHIVE_OTR_REQUIRE ? ISessionNegotiator::Skip : ISessionNegotiator::Cancel;

	if (FDataForms && isArchivePrefsEnabled(ASession.streamJid))
	{
		IDataField logging;
		logging.var = SFP_LOGGING;
		logging.type = DATAFIELD_TYPE_LISTSINGLE;
		logging.required = false;

		if (itemPrefs.otr != ARCHIVE_OTR_FORBID)
		{
			IDataOption option;
			option.value = SFV_MUSTNOT_LOGGING;
			logging.options.append(option);
		}
		if (itemPrefs.otr != ARCHIVE_OTR_REQUIRE)
		{
			IDataOption option;
			option.value = SFV_MAY_LOGGING;
			logging.options.append(option);
			logging.value = SFV_MAY_LOGGING;
		}
		else
		{
			logging.value = SFV_MUSTNOT_LOGGING;
			logging.required = true;
		}

		if (ASession.status == IStanzaSession::Init)
		{
			ARequest.fields.append(logging);
			result = ISessionNegotiator::Auto;
		}
		else if (ASession.status == IStanzaSession::Renegotiate)
		{
			int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
			if (index<0 || ASession.form.fields.at(index).value!=logging.value)
			{
				ARequest.fields.append(logging);
				result = ISessionNegotiator::Auto;
			}
			else
				result = ISessionNegotiator::Skip;
		}
	}
	return result;
}

int MessageArchiver::sessionAccept(const IStanzaSession &ASession, const IDataForm &ARequest, IDataForm &ASubmit)
{
	IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
	int result = ISessionNegotiator::Skip;

	int rindex = FDataForms!=NULL ? FDataForms->fieldIndex(SFP_LOGGING,ARequest.fields) : -1;
	if (rindex>=0)
	{
		result = ISessionNegotiator::Auto;
		if (ARequest.type == DATAFORM_TYPE_FORM)
		{
			IDataField logging;
			logging.var = SFP_LOGGING;
			logging.type = DATAFIELD_TYPE_LISTSINGLE;
			logging.value = ARequest.fields.at(rindex).value;
			logging.required = false;

			QStringList options;
			for (int i=0; i<ARequest.fields.at(rindex).options.count(); i++)
				options.append(ARequest.fields.at(rindex).options.at(i).value);

			if (itemPrefs.otr == ARCHIVE_OTR_APPROVE)
			{
				if (logging.value == SFV_MUSTNOT_LOGGING)
				{
					result = ISessionNegotiator::Manual;
					ASubmit.pages[0].fieldrefs.append(SFP_LOGGING);
					ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
				}
			}
			else if (itemPrefs.otr == ARCHIVE_OTR_FORBID)
			{
				if (options.contains(SFV_MAY_LOGGING))
					logging.value = SFV_MAY_LOGGING;
				else
					result = ISessionNegotiator::Cancel;
			}
			else if (itemPrefs.otr == ARCHIVE_OTR_OPPOSE)
			{
				if (options.contains(SFV_MAY_LOGGING))
					logging.value = SFV_MAY_LOGGING;
			}
			else if (itemPrefs.otr == ARCHIVE_OTR_PREFER)
			{
				if (options.contains(SFV_MUSTNOT_LOGGING))
					logging.value = SFV_MUSTNOT_LOGGING;
			}
			else if (itemPrefs.otr == ARCHIVE_OTR_REQUIRE)
			{
				if (options.contains(SFV_MUSTNOT_LOGGING))
					logging.value = SFV_MUSTNOT_LOGGING;
				else
					result = ISessionNegotiator::Cancel;
				logging.required = true;
			}
			ASubmit.fields.append(logging);
		}
		else if (ARequest.type == DATAFORM_TYPE_SUBMIT)
		{
			QString sessionValue = ARequest.fields.at(rindex).value.toString();
			if (itemPrefs.otr==ARCHIVE_OTR_FORBID && sessionValue==SFV_MUSTNOT_LOGGING)
				result = ISessionNegotiator::Cancel;
			else if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && sessionValue==SFV_MAY_LOGGING)
				result = ISessionNegotiator::Cancel;
			else if (itemPrefs.otr==ARCHIVE_OTR_APPROVE && sessionValue==SFV_MUSTNOT_LOGGING)
			{
				result = ISessionNegotiator::Manual;
				ASubmit.pages[0].fieldrefs.append(SFP_LOGGING);
				ASubmit.pages[0].childOrder.append(DATALAYOUT_CHILD_FIELDREF);
			}
		}
	}
	else if (ASession.status!=IStanzaSession::Active && itemPrefs.otr==ARCHIVE_OTR_REQUIRE)
	{
		result = ISessionNegotiator::Cancel;
	}
	return result;
}

int MessageArchiver::sessionApply(const IStanzaSession &ASession)
{
	int result = ISessionNegotiator::Skip;
	IArchiveItemPrefs itemPrefs = archiveItemPrefs(ASession.streamJid,ASession.contactJid);
	if (FDataForms && isReady(ASession.streamJid))
	{
		int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
		QString sessionValue = index>=0 ? ASession.form.fields.at(index).value.toString() : QString::null;
		if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE && sessionValue==SFV_MAY_LOGGING)
		{
			result = ISessionNegotiator::Cancel;
		}
		else if (itemPrefs.otr==ARCHIVE_OTR_FORBID && sessionValue==SFV_MUSTNOT_LOGGING)
		{
			result = ISessionNegotiator::Cancel;
		}
		else if (sessionValue==SFV_MUSTNOT_LOGGING && itemPrefs.save!=ARCHIVE_SAVE_FALSE)
		{
			StanzaSession &session = FSessions[ASession.streamJid][ASession.contactJid];
			if (FPrefsSaveRequests.contains(session.requestId))
			{
				result = ISessionNegotiator::Wait;
			}
			else if (!session.error.isEmpty())
			{
				result = ISessionNegotiator::Cancel;
			}
			else
			{
				IArchiveStreamPrefs prefs = archivePrefs(ASession.streamJid);
				if (session.sessionId.isEmpty())
				{
					session.sessionId = ASession.sessionId;
					session.saveMode = itemPrefs.save;
					session.defaultPrefs = !prefs.itemPrefs.contains(ASession.contactJid);
				}
				itemPrefs.save = ARCHIVE_SAVE_FALSE;
				prefs.itemPrefs[ASession.contactJid] = itemPrefs;
				session.requestId = setArchivePrefs(ASession.streamJid,prefs);
				result = !session.requestId.isEmpty() ? ISessionNegotiator::Wait : ISessionNegotiator::Cancel;
			}
		}
		else
		{
			result = ISessionNegotiator::Auto;
		}
	}
	else if (itemPrefs.otr==ARCHIVE_OTR_REQUIRE)
	{
		result = ISessionNegotiator::Cancel;
	}
	return result;
}

void MessageArchiver::sessionLocalize(const IStanzaSession &ASession, IDataForm &AForm)
{
	Q_UNUSED(ASession);
	if (FDataForms)
	{
		int index = FDataForms->fieldIndex(SFP_LOGGING,AForm.fields);
		if (index>=0)
		{
			AForm.fields[index].label = tr("Message logging");
			QList<IDataOption> &options = AForm.fields[index].options;
			for (int i=0;i<options.count();i++)
			{
				if (options[i].value == SFV_MAY_LOGGING)
					options[i].label = tr("Allow message logging");
				else if (options[i].value == SFV_MUSTNOT_LOGGING)
					options[i].label = tr("Disallow all message logging");
			}
		}
	}
}

bool MessageArchiver::isReady(const Jid &AStreamJid) const
{
	return FArchivePrefs.contains(AStreamJid);
}

bool MessageArchiver::isArchivePrefsEnabled(const Jid &AStreamJid) const
{
	return isReady(AStreamJid) && (isSupported(AStreamJid,NS_ARCHIVE_PREF) || !isArchiveAutoSave(AStreamJid));
}

bool MessageArchiver::isSupported(const Jid &AStreamJid, const QString &AFeatureNS) const
{
	return isReady(AStreamJid) && FFeatures.value(AStreamJid).contains(AFeatureNS);
}

bool MessageArchiver::isArchiveAutoSave(const Jid &AStreamJid) const
{
	return isSupported(AStreamJid,NS_ARCHIVE_AUTO) && archivePrefs(AStreamJid).autoSave;
}

bool MessageArchiver::isArchivingAllowed(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId) const
{
	if (isReady(AStreamJid) && AItemJid.isValid())
	{
		IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid, AItemJid, AThreadId);
		return itemPrefs.save != ARCHIVE_SAVE_FALSE;
	}
	return false;
}

QWidget *MessageArchiver::showArchiveWindow(const Jid &AStreamJid, const Jid &AContactJid)
{
	IRoster *roster = FRosterPlugin!=NULL ? FRosterPlugin->findRoster(AStreamJid) : NULL;
	if (roster)
	{
		ArchiveViewWindow *window = new ArchiveViewWindow(FPluginManager,this,roster);
		window->setContactJid(AContactJid);
		WidgetManager::showActivateRaiseWindow(window);
		return window;
	}
	return NULL;
}

QString MessageArchiver::prefsNamespace(const Jid &AStreamJid) const
{
	return FNamespaces.value(AStreamJid);
}

IArchiveStreamPrefs MessageArchiver::archivePrefs(const Jid &AStreamJid) const
{
	return FArchivePrefs.value(AStreamJid);
}

IArchiveItemPrefs MessageArchiver::archiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid, const QString &AThreadId) const
{
	IArchiveStreamPrefs streamPrefs = archivePrefs(AStreamJid);

	IArchiveItemPrefs sessionPrefs, domainPrefs, barePrefs, fullPrefs;
	if (!AThreadId.isEmpty() && streamPrefs.sessionPrefs.contains(AThreadId))
	{
		IArchiveSessionPrefs sprefs = streamPrefs.sessionPrefs.value(AThreadId);
		sessionPrefs = archiveItemPrefs(AStreamJid,AItemJid);
		sessionPrefs.otr = sprefs.otr;
		sessionPrefs.save = sprefs.save;
	}
	else for (QMap<Jid, IArchiveItemPrefs>::const_iterator it = streamPrefs.itemPrefs.constBegin(); it != streamPrefs.itemPrefs.constEnd(); ++it)
	{
		QString node = it.key().pNode();
		QString domain = it.key().pDomain();
		QString resource = it.key().pResource();
		if (it->exactmatch)
		{
			if (it.key() == AItemJid)
			{
				fullPrefs = it.value();
				break;
			}
		}
		else if (domain == AItemJid.pDomain())
		{
			if (node == AItemJid.pNode())
			{
				if (resource == AItemJid.pResource())
				{
					fullPrefs = it.value();
					break;
				}
				else if (resource.isEmpty())
				{
					barePrefs = it.value();
				}
			}
			else if (resource.isEmpty() && node.isEmpty())
			{
				domainPrefs = it.value();
			}
		}
	}

	IArchiveItemPrefs itemPrefs;
	if (!sessionPrefs.save.isEmpty())
		itemPrefs = sessionPrefs;
	else if (!fullPrefs.save.isEmpty())
		itemPrefs = fullPrefs;
	else if (!barePrefs.save.isEmpty())
		itemPrefs = barePrefs;
	else if (!domainPrefs.save.isEmpty())
		itemPrefs = domainPrefs;
	else
		itemPrefs = streamPrefs.defaultPrefs;

	return itemPrefs;
}

QString MessageArchiver::setArchiveAutoSave(const Jid &AStreamJid, bool AAuto)
{
	if (isSupported(AStreamJid,NS_ARCHIVE_AUTO))
	{
		Stanza autoSave("iq");
		autoSave.setType("set").setId(FStanzaProcessor->newId());
		QDomElement autoElem = autoSave.addElement("auto",FNamespaces.value(AStreamJid));
		autoElem.setAttribute("save",QVariant(AAuto).toString());
		if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,autoSave,ARCHIVE_TIMEOUT))
		{
			FPrefsAutoRequests.insert(autoSave.id(),AAuto);
			return autoSave.id();
		}
	}
	return QString::null;
}

QString MessageArchiver::setArchivePrefs(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs)
{
	if (isArchivePrefsEnabled(AStreamJid))
	{
		bool storage = FInStoragePrefs.contains(AStreamJid);

		IArchiveStreamPrefs oldPrefs = archivePrefs(AStreamJid);
		IArchiveStreamPrefs newPrefs = oldPrefs;

		if (!APrefs.defaultPrefs.save.isEmpty() && !APrefs.defaultPrefs.otr.isEmpty())
		{
			newPrefs.defaultPrefs.otr = APrefs.defaultPrefs.otr;
			if (newPrefs.defaultPrefs.otr == ARCHIVE_OTR_REQUIRE)
				newPrefs.defaultPrefs.save = ARCHIVE_SAVE_FALSE;
			else
				newPrefs.defaultPrefs.save = APrefs.defaultPrefs.save;
			newPrefs.defaultPrefs.expire = APrefs.defaultPrefs.expire;
		}

		if (!APrefs.methodLocal.isEmpty())
			newPrefs.methodLocal = APrefs.methodLocal;
		if (!APrefs.methodAuto.isEmpty())
			newPrefs.methodAuto = APrefs.methodAuto;
		if (!APrefs.methodManual.isEmpty())
			newPrefs.methodManual = APrefs.methodManual;

		bool itemsChanged = false;
		foreach(Jid itemJid, APrefs.itemPrefs.keys())
		{
			IArchiveItemPrefs newItemPrefs = APrefs.itemPrefs.value(itemJid);
			if (!newItemPrefs.save.isEmpty() && !newItemPrefs.otr.isEmpty())
			{
				newPrefs.itemPrefs.insert(itemJid,newItemPrefs);
				if (newPrefs.itemPrefs.value(itemJid).otr == ARCHIVE_OTR_REQUIRE)
					newPrefs.itemPrefs[itemJid].save = ARCHIVE_SAVE_FALSE;
			}
			else
			{
				itemsChanged = true;
				newPrefs.itemPrefs.remove(itemJid);
			}
		}

		bool sessionsChanged = false;
		foreach(QString threadId, APrefs.sessionPrefs.keys())
		{
			IArchiveSessionPrefs newSessionPrefs = APrefs.sessionPrefs.value(threadId);
			if (!newSessionPrefs.save.isEmpty() && !newSessionPrefs.otr.isEmpty())
			{
				newPrefs.sessionPrefs[threadId] = newSessionPrefs;
			}
			else
			{
				sessionsChanged = true;
				newPrefs.sessionPrefs.remove(threadId);
			}
		}

		Stanza save("iq");
		save.setType("set").setId(FStanzaProcessor->newId());

		QDomElement prefElem = save.addElement(PST_ARCHIVE_PREFS,!storage ? FNamespaces.value(AStreamJid) : NS_ARCHIVE);

		bool defChanged = oldPrefs.defaultPrefs!=newPrefs.defaultPrefs;
		if (storage || defChanged)
		{
			QDomElement defElem = prefElem.appendChild(save.createElement("default")).toElement();
			if (newPrefs.defaultPrefs.expire > 0)
				defElem.setAttribute("expire",newPrefs.defaultPrefs.expire);
			defElem.setAttribute("save",newPrefs.defaultPrefs.save);
			defElem.setAttribute("otr",newPrefs.defaultPrefs.otr);
		}

		bool methodChanged = oldPrefs.methodAuto!=newPrefs.methodAuto  || oldPrefs.methodLocal!=newPrefs.methodLocal || oldPrefs.methodManual!=newPrefs.methodManual;
		if (!storage && methodChanged)
		{
			QDomElement methodAuto = prefElem.appendChild(save.createElement("method")).toElement();
			methodAuto.setAttribute("type","auto");
			methodAuto.setAttribute("use",newPrefs.methodAuto);

			QDomElement methodLocal = prefElem.appendChild(save.createElement("method")).toElement();
			methodLocal.setAttribute("type","local");
			methodLocal.setAttribute("use",newPrefs.methodLocal);

			QDomElement methodManual = prefElem.appendChild(save.createElement("method")).toElement();
			methodManual.setAttribute("type","manual");
			methodManual.setAttribute("use",newPrefs.methodManual);
		}

		foreach(Jid itemJid, newPrefs.itemPrefs.keys())
		{
			IArchiveItemPrefs newItemPrefs = newPrefs.itemPrefs.value(itemJid);
			IArchiveItemPrefs oldItemPrefs = oldPrefs.itemPrefs.value(itemJid);
			bool itemChanged = oldItemPrefs!=newItemPrefs;
			if (storage || itemChanged)
			{
				QDomElement itemElem = prefElem.appendChild(save.createElement("item")).toElement();
				if (newItemPrefs.expire > 0)
					itemElem.setAttribute("expire",newItemPrefs.expire);
				if (newItemPrefs.exactmatch)
					itemElem.setAttribute("exactmatch",QVariant(newItemPrefs.exactmatch).toString());
				itemElem.setAttribute("jid",itemJid.eFull());
				itemElem.setAttribute("otr",newItemPrefs.otr);
				itemElem.setAttribute("save",newItemPrefs.save);
			}
			itemsChanged |= itemChanged;
		}

		foreach(QString threadId, newPrefs.sessionPrefs.keys())
		{
			IArchiveSessionPrefs newSessionPrefs = newPrefs.sessionPrefs.value(threadId);
			IArchiveSessionPrefs oldSessionPrefs = oldPrefs.sessionPrefs.value(threadId);
			bool sessionChanged = oldSessionPrefs!=newSessionPrefs;
			if (storage || sessionChanged)
			{
				QDomElement sessionElem = prefElem.appendChild(save.createElement("session")).toElement();
				sessionElem.setAttribute("save",newSessionPrefs.save);
				sessionElem.setAttribute("otr",newSessionPrefs.otr);
			}
			sessionsChanged |= sessionChanged;
		}

		if (defChanged || methodChanged || itemsChanged || sessionsChanged)
		{
			QString requestId;
			if (storage)
				requestId = FPrivateStorage!=NULL ? FPrivateStorage->saveData(AStreamJid,prefElem) : QString::null;
			else if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,save,ARCHIVE_TIMEOUT))
				requestId = save.id();
			if (!requestId.isEmpty())
			{
				FPrefsSaveRequests.insert(requestId,AStreamJid);
				return requestId;
			}
		}
	}
	return QString::null;
}

QString MessageArchiver::removeArchiveItemPrefs(const Jid &AStreamJid, const Jid &AItemJid)
{
	if (isArchivePrefsEnabled(AStreamJid) && archivePrefs(AStreamJid).itemPrefs.contains(AItemJid))
	{
		if (isSupported(AStreamJid,NS_ARCHIVE_PREF))
		{
			Stanza remove("iq");
			remove.setType("set").setId(FStanzaProcessor->newId());
			QDomElement itemElem = remove.addElement("itemremove",FNamespaces.value(AStreamJid)).appendChild(remove.createElement("item")).toElement();
			itemElem.setAttribute("jid",AItemJid.eFull());
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,remove,ARCHIVE_TIMEOUT))
			{
				FPrefsRemoveItemRequests.insert(remove.id(),AItemJid);
				return remove.id();
			}
		}
		else
		{
			IArchiveStreamPrefs prefs;
			prefs.itemPrefs[AItemJid].otr = QString::null;
			prefs.itemPrefs[AItemJid].save = QString::null;
			return setArchivePrefs(AStreamJid,prefs);
		}
	}
	return QString::null;
}

QString MessageArchiver::removeArchiveSessionPrefs(const Jid &AStreamJid, const QString &AThreadId)
{
	if (isArchivePrefsEnabled(AStreamJid) && archivePrefs(AStreamJid).sessionPrefs.contains(AThreadId))
	{
		if (isSupported(AStreamJid,NS_ARCHIVE_PREF))
		{
			Stanza remove("iq");
			remove.setType("set").setId(FStanzaProcessor->newId());
			QDomElement sessionElem = remove.addElement("sessionremove",FNamespaces.value(AStreamJid)).appendChild(remove.createElement("session")).toElement();
			sessionElem.setAttribute("thread",AThreadId);
			if (FStanzaProcessor->sendStanzaRequest(this,AStreamJid,remove,ARCHIVE_TIMEOUT))
			{
				FPrefsRemoveSessionRequests.insert(remove.id(),AThreadId);
				return remove.id();
			}
		}
		else
		{
			IArchiveStreamPrefs prefs;
			prefs.sessionPrefs[AThreadId].save = QString::null;
			prefs.sessionPrefs[AThreadId].otr = QString::null;
			return setArchivePrefs(AStreamJid,prefs);
		}
	}
	return QString::null;
}

bool MessageArchiver::saveMessage(const Jid &AStreamJid, const Jid &AItemJid, const Message &AMessage)
{
	if (!isArchiveAutoSave(AStreamJid) && isArchivingAllowed(AStreamJid,AItemJid,AMessage.threadId()))
	{
		IArchiveEngine *engine = findEngineByCapability(IArchiveEngine::DirectArchiving,AStreamJid);
		if (engine)
		{
			Message message = AMessage;
			bool directionIn = AItemJid==message.from() || AStreamJid==message.to();
			if (prepareMessage(AStreamJid,message,directionIn))
				return engine->saveMessage(AStreamJid,message,directionIn);
		}
	}
	return false;
}

bool MessageArchiver::saveNote(const Jid &AStreamJid, const Jid &AItemJid, const QString &ANote, const QString &AThreadId)
{
	if (!isArchiveAutoSave(AStreamJid) && isArchivingAllowed(AStreamJid,AItemJid,AThreadId))
	{
		IArchiveEngine *engine = findEngineByCapability(IArchiveEngine::DirectArchiving,AStreamJid);
		if (engine)
		{
			Message message;
			message.setTo(AStreamJid.eFull()).setFrom(AItemJid.eFull()).setBody(ANote).setThreadId(AThreadId);
			return engine->saveNote(AStreamJid,message,true);
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
		return localId;
	}
	return QString::null;
}

QString MessageArchiver::loadHeaders(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	HeadersRequest request;
	QString localId = QUuid::createUuid().toString();
	foreach(IArchiveEngine *engine, engineOrderByCapability(IArchiveEngine::ArchiveManagement,AStreamJid))
	{
		if (ARequest.text.isEmpty() || engine->isCapable(AStreamJid,IArchiveEngine::TextSearch))
		{
			QString id = engine->loadHeaders(AStreamJid,ARequest);
			if (!id.isEmpty())
			{
				request.engines.append(engine);
				FRequestId2LocalId.insert(id,localId);
			}
		}
	}
	if (!request.engines.isEmpty())
	{
		request.request = ARequest;
		FHeadersRequests.insert(localId,request);
		return localId;
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
			return localId;
		}
	}
	return QString::null;
}

QString MessageArchiver::removeCollections(const Jid &AStreamJid, const IArchiveRequest &ARequest)
{
	RemoveRequest request;
	QString localId = QUuid::createUuid().toString();
	foreach(IArchiveEngine *engine, engineOrderByCapability(IArchiveEngine::ArchiveManagement,AStreamJid))
	{
		QString id = engine->removeCollections(AStreamJid,ARequest);
		if (!id.isEmpty())
		{
			FRequestId2LocalId.insert(id,localId);
			request.engines.append(engine);
		}
	}
	if (!request.engines.isEmpty())
	{
		request.request = ARequest;
		FRemoveRequests.insert(localId,request);
		return localId;
	}
	return QString::null;
}

void MessageArchiver::elementToCollection(const QDomElement &AChatElem, IArchiveCollection &ACollection) const
{
	ACollection.header.with = AChatElem.attribute("with");
	ACollection.header.start = DateTime(AChatElem.attribute("start")).toLocal();
	ACollection.header.subject = AChatElem.attribute("subject");
	ACollection.header.threadId = AChatElem.attribute("thread");
	ACollection.header.version = AChatElem.attribute("version").toUInt();

	int secsLast = 0;
	QDomElement nodeElem = AChatElem.firstChildElement();
	bool isSecsFromStart = AChatElem.attribute("secsFromLast")!="true";
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

	secsLast = 0;
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
				message.setTo(contactJid.eFull());
				message.setData(MDR_MESSAGE_DIRECTION,IMessageProcessor::MessageOut);
			}
			else
			{
				message.setFrom(contactJid.eFull());
				message.setData(MDR_MESSAGE_DIRECTION,IMessageProcessor::MessageIn);
			}

			message.setType(nick.isEmpty() ? Message::Chat : Message::GroupChat);

			QString utc = nodeElem.attribute("utc");
			if (utc.isEmpty())
			{
				int secs = nodeElem.attribute("secs").toInt();
				message.setDateTime(ACollection.header.start.addSecs(isSecsFromStart ? secs : secsLast+secs));
			}
			else
			{
				QDateTime messageDT = DateTime(utc).toLocal();
				message.setDateTime(messageDT.isValid() ? messageDT : ACollection.header.start.addSecs(secsLast));
			}
			secsLast = ACollection.header.start.secsTo(message.dateTime());

			message.setThreadId(ACollection.header.threadId);

			QDomElement childElem = nodeElem.firstChildElement();
			while (!childElem.isNull())
			{
				message.stanza().element().appendChild(childElem.cloneNode(true));
				childElem = childElem.nextSiblingElement();
			}

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

void MessageArchiver::collectionToElement(const IArchiveCollection &ACollection, QDomElement &AChatElem, const QString &ASaveMode) const
{
	QDomDocument ownerDoc = AChatElem.ownerDocument();
	AChatElem.setAttribute("with",ACollection.header.with.eFull());
	AChatElem.setAttribute("start",DateTime(ACollection.header.start).toX85UTC());
	AChatElem.setAttribute("version",ACollection.header.version);
	if (!ACollection.header.subject.isEmpty())
		AChatElem.setAttribute("subject",ACollection.header.subject);
	if (!ACollection.header.threadId.isEmpty())
		AChatElem.setAttribute("thread",ACollection.header.threadId);
	AChatElem.setAttribute("secsFromLast","true");

	if (ACollection.previous.with.isValid() && ACollection.previous.start.isValid())
	{
		QDomElement prevElem = AChatElem.appendChild(ownerDoc.createElement("previous")).toElement();
		prevElem.setAttribute("with",ACollection.previous.with.eFull());
		prevElem.setAttribute("start",DateTime(ACollection.previous.start).toX85UTC());
	}

	if (ACollection.next.with.isValid() && ACollection.next.start.isValid())
	{
		QDomElement nextElem = AChatElem.appendChild(ownerDoc.createElement("next")).toElement();
		nextElem.setAttribute("with",ACollection.next.with.eFull());
		nextElem.setAttribute("start",DateTime(ACollection.next.start).toX85UTC());
	}

	if (FDataForms && FDataForms->isFormValid(ACollection.attributes))
	{
		FDataForms->xmlForm(ACollection.attributes,AChatElem);
	}

	int secLast = 0;
	bool groupChat = false;
	foreach(Message message, ACollection.body.messages)
	{
		Jid fromJid = message.from();
		groupChat |= message.type()==Message::GroupChat;
		if (!groupChat || !fromJid.resource().isEmpty())
		{
			bool directionIn = ACollection.header.with && message.from();
			QDomElement messageElem = AChatElem.appendChild(ownerDoc.createElement(directionIn ? "from" : "to")).toElement();

			int secs = ACollection.header.start.secsTo(message.dateTime());
			if (secs >= secLast)
			{
				messageElem.setAttribute("secs",secs-secLast);
				secLast = secs;
			}
			else
				messageElem.setAttribute("utc",DateTime(message.dateTime()).toX85UTC());

			if (groupChat)
				messageElem.setAttribute("name",fromJid.resource());

			if (ASaveMode==ARCHIVE_SAVE_MESSAGE || ASaveMode==ARCHIVE_SAVE_STREAM)
			{
				QDomElement childElem = message.stanza().element().firstChildElement();
				while (!childElem.isNull())
				{
					messageElem.appendChild(childElem.cloneNode(true));
					childElem = childElem.nextSiblingElement();
				}
			}
			else if (ASaveMode == ARCHIVE_SAVE_BODY)
			{
				messageElem.appendChild(ownerDoc.createElement("body")).appendChild(ownerDoc.createTextNode(message.body()));
			}
		}
	}

	QMultiMap<QDateTime,QString>::const_iterator it = ACollection.body.notes.constBegin();
	while (it != ACollection.body.notes.constEnd())
	{
		QDomElement noteElem = AChatElem.appendChild(ownerDoc.createElement("note")).toElement();
		noteElem.setAttribute("utc",DateTime(it.key()).toX85UTC());
		noteElem.appendChild(ownerDoc.createTextNode(it.value()));
		it++;
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
		connect(AEngine->instance(),SIGNAL(capabilitiesChanged(const Jid &)),
			SLOT(onEngineCapabilitiesChanged(const Jid &)));
		connect(AEngine->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
			SLOT(onEngineRequestFailed(const QString &, const QString &)));
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

void MessageArchiver::registerDiscoFeatures()
{
	IDiscoFeature dfeature;

	dfeature.active = false;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.var = NS_ARCHIVE;
	dfeature.name = tr("Messages Archiving");
	dfeature.description = tr("Supports the archiving of the messages");
	FDiscovery->insertDiscoFeature(dfeature);
	dfeature.var = NS_ARCHIVE_OLD;
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = NS_ARCHIVE_AUTO;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.name = tr("Automatic Messages Archiving");
	dfeature.description = tr("Supports the automatic archiving of the messages");
	FDiscovery->insertDiscoFeature(dfeature);
	dfeature.var = NS_ARCHIVE_OLD_AUTO;
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = NS_ARCHIVE_MANAGE;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.name = tr("Managing Archived Messages");
	dfeature.description = tr("Supports the managing of the archived messages");
	FDiscovery->insertDiscoFeature(dfeature);
	dfeature.var = NS_ARCHIVE_OLD_MANAGE;
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = NS_ARCHIVE_MANUAL;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.name = tr("Manual Messages Archiving");
	dfeature.description = tr("Supports the manual archiving of the messages");
	FDiscovery->insertDiscoFeature(dfeature);
	dfeature.var = NS_ARCHIVE_OLD_MANUAL;
	FDiscovery->insertDiscoFeature(dfeature);

	dfeature.var = NS_ARCHIVE_PREF;
	dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_HISTORY);
	dfeature.name = tr("Messages Archive Preferences");
	dfeature.description = tr("Supports the storing of the archive preferences");
	FDiscovery->insertDiscoFeature(dfeature);
	dfeature.var = NS_ARCHIVE_OLD_PREF;
	FDiscovery->insertDiscoFeature(dfeature);
}

QString MessageArchiver::loadServerPrefs(const Jid &AStreamJid)
{
	Stanza load("iq");
	load.setType("get").setId(FStanzaProcessor!=NULL ? FStanzaProcessor->newId() : QString::null);
	load.addElement(PST_ARCHIVE_PREFS,FNamespaces.value(AStreamJid));
	if (FStanzaProcessor && FStanzaProcessor->sendStanzaRequest(this,AStreamJid,load,ARCHIVE_TIMEOUT))
	{
		FPrefsLoadRequests.insert(load.id(),AStreamJid);
		return load.id();
	}
	else
	{
		applyArchivePrefs(AStreamJid,QDomElement());
	}
	return QString::null;
}

QString MessageArchiver::loadStoragePrefs(const Jid &AStreamJid)
{
	QString requestId = FPrivateStorage!=NULL ? FPrivateStorage->loadData(AStreamJid,PST_ARCHIVE_PREFS,PSN_ARCHIVE_PREFS) : QString::null;
	if (!requestId.isEmpty())
		FPrefsLoadRequests.insert(requestId,AStreamJid);
	else
		applyArchivePrefs(AStreamJid,QDomElement());
	return requestId;
}

void MessageArchiver::applyArchivePrefs(const Jid &AStreamJid, const QDomElement &AElem)
{
	if (isReady(AStreamJid) || AElem.hasChildNodes() || FInStoragePrefs.contains(AStreamJid))
	{
		//Hack for Jabberd 1.4.3
		if (!FInStoragePrefs.contains(AStreamJid) && AElem.hasAttribute("j_private_flag"))
			FInStoragePrefs.append(AStreamJid);

		bool initPrefs = !isReady(AStreamJid);
		IArchiveStreamPrefs &prefs = FArchivePrefs[AStreamJid];

		QDomElement autoElem = isSupported(AStreamJid,NS_ARCHIVE_PREF) ? AElem.firstChildElement("auto") : QDomElement();
		if (!autoElem.isNull())
		{
			prefs.autoSave = QVariant(autoElem.attribute("save","false")).toBool();
			prefs.autoScope = autoElem.attribute("scope",ARCHIVE_SCOPE_GLOBAL);
		}
		else if (initPrefs)
		{
			prefs.autoSave = isSupported(AStreamJid,NS_ARCHIVE_AUTO);
			prefs.autoScope = ARCHIVE_SCOPE_GLOBAL;
		}

		bool prefsDisabled = !isArchivePrefsEnabled(AStreamJid);

		QDomElement defElem = AElem.firstChildElement("default");
		if (!defElem.isNull())
		{
			prefs.defaultPrefs.save = defElem.attribute("save",prefs.defaultPrefs.save);
			prefs.defaultPrefs.otr = defElem.attribute("otr",prefs.defaultPrefs.otr);
			prefs.defaultPrefs.expire = defElem.attribute("expire","0").toUInt();
		}
		else if (initPrefs || prefsDisabled)
		{
			prefs.defaultPrefs.save = ARCHIVE_SAVE_MESSAGE;
			prefs.defaultPrefs.otr = ARCHIVE_OTR_CONCEDE;
			prefs.defaultPrefs.expire = 0;
		}

		QDomElement methodElem = AElem.firstChildElement("method");
		if (methodElem.isNull() && (initPrefs || prefsDisabled))
		{
			prefs.methodAuto = ARCHIVE_METHOD_PREFER;
			prefs.methodLocal = ARCHIVE_METHOD_PREFER;
			prefs.methodManual = ARCHIVE_METHOD_PREFER;
		}
		else while (!methodElem.isNull())
		{
			if (methodElem.attribute("type") == "auto")
				prefs.methodAuto = methodElem.attribute("use",prefs.methodAuto);
			else if (methodElem.attribute("type") == "local")
				prefs.methodLocal = methodElem.attribute("use",prefs.methodLocal);
			else if (methodElem.attribute("type") == "manual")
				prefs.methodManual = methodElem.attribute("use",prefs.methodManual);
			methodElem = methodElem.nextSiblingElement("method");
		}

		QSet<Jid> oldItemJids = prefs.itemPrefs.keys().toSet();
		QDomElement itemElem = AElem.firstChildElement("item");
		while (!itemElem.isNull())
		{
			Jid itemJid = itemElem.attribute("jid");
			oldItemJids -= itemJid;

			IArchiveItemPrefs itemPrefs;
			itemPrefs.save = itemElem.attribute("save",prefs.defaultPrefs.save);
			itemPrefs.otr = itemElem.attribute("otr",prefs.defaultPrefs.otr);
			itemPrefs.expire = itemElem.attribute("expire","0").toUInt();
			itemPrefs.exactmatch = QVariant(itemElem.attribute("exactmatch","false")).toBool();

			if (!itemPrefs.save.isEmpty() && !itemPrefs.otr.isEmpty())
				prefs.itemPrefs.insert(itemJid,itemPrefs);
			else
				prefs.itemPrefs.remove(itemJid);

			itemElem = itemElem.nextSiblingElement("item");
		}

		QSet<QString> oldSessionIds = prefs.sessionPrefs.keys().toSet();
		QDomElement sessionElem = AElem.firstChildElement("session");
		while (!sessionElem.isNull())
		{
			QString threadId = sessionElem.attribute("thread");
			oldSessionIds -= threadId;

			IArchiveSessionPrefs sessionPrefs;
			sessionPrefs.save = sessionElem.attribute("save");
			sessionPrefs.otr = sessionElem.attribute("otr",prefs.defaultPrefs.otr);
			sessionPrefs.timeout = sessionElem.attribute("timeout","0").toInt();

			if (!sessionPrefs.save.isEmpty())
				prefs.sessionPrefs.insert(threadId,sessionPrefs);
			else
				prefs.sessionPrefs.remove(threadId);

			sessionElem = sessionElem.nextSiblingElement("session");
		}

		if (FInStoragePrefs.contains(AStreamJid))
		{
			foreach(Jid itemJid, oldItemJids)
				prefs.itemPrefs.remove(itemJid);
			foreach(QString threadId, oldSessionIds)
				prefs.sessionPrefs.remove(threadId);
		}

		if (initPrefs)
		{
			restoreStanzaSessionContext(AStreamJid);

			QList< QPair<Message,bool> > messages = FPendingMessages.take(AStreamJid);
			for (int i = 0; i<messages.count(); i++)
			{
				QPair<Message, bool> message = messages.at(i);
				processMessage(AStreamJid, message.first, message.second);
			}

			if (prefsDisabled)
				setArchiveAutoSave(AStreamJid,prefs.autoSave);

			openHistoryOptionsNode(AStreamJid);
			emit archivePrefsOpened(AStreamJid);
		}
		else
		{
			renegotiateStanzaSessions(AStreamJid);
		}

		emit archivePrefsChanged(AStreamJid);
	}
	else
	{
		FInStoragePrefs.append(AStreamJid);
		loadStoragePrefs(AStreamJid);
	}
}

bool MessageArchiver::prepareMessage(const Jid &AStreamJid, Message &AMessage, bool ADirectionIn)
{
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

	if (AMessage.body().isEmpty())
		return false;

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

IArchiveEngine *MessageArchiver::findEngineByCapability(quint32 ACapability, const Jid &AStreamJid) const
{
	IArchiveEngine *engine = findArchiveEngine(Options::node(OPV_HISTORY_CAPABILITY_ITEM,QString::number(ACapability)).value("default").toString());
	if (engine==NULL || !isArchiveEngineEnabled(engine->engineId()) || engine->capabilityOrder(ACapability,AStreamJid)<=0)
	{
		QMultiMap<int, IArchiveEngine *> order = engineOrderByCapability(ACapability,AStreamJid);
		engine = !order.isEmpty() ? order.constBegin().value() : NULL;
	}
	return engine;
}

QMultiMap<int, IArchiveEngine *> MessageArchiver::engineOrderByCapability(quint32 ACapability, const Jid &AStreamJid) const
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

void MessageArchiver::openHistoryOptionsNode(const Jid &AStreamJid)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
	if (FOptionsManager && account)
	{
		IOptionsDialogNode node = { ONO_HISTORY, OPN_HISTORY"." + account->accountId().toString(), account->name(), MNI_HISTORY };
		FOptionsManager->insertOptionsDialogNode(node);
	}
}

void MessageArchiver::closeHistoryOptionsNode(const Jid &AStreamJid)
{
	IAccount *account = FAccountManager!=NULL ? FAccountManager->accountByStream(AStreamJid) : NULL;
	if (FOptionsManager && account)
	{
		FOptionsManager->removeOptionsDialogNode(OPN_HISTORY"." + account->accountId().toString());
	}
}

Menu *MessageArchiver::createContextMenu(const Jid &AStreamJid, const QStringList &AContacts, QWidget *AParent) const
{
	bool isStreamMenu = AStreamJid==AContacts.value(0);

	Menu *menu = new Menu(AParent);
	menu->setTitle(tr("History"));
	menu->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);

	if (AContacts.count()==1 && !engineOrderByCapability(IArchiveEngine::ArchiveManagement,AStreamJid).isEmpty())
	{
		Action *viewAction = new Action(menu);
		viewAction->setText(tr("View History"));
		viewAction->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);
		viewAction->setData(ADR_STREAM_JID,AStreamJid.full());
		if (!isStreamMenu)
			viewAction->setData(ADR_CONTACT_JID,AContacts.first());
		viewAction->setShortcutId(SCT_ROSTERVIEW_SHOWHISTORY);
		connect(viewAction,SIGNAL(triggered(bool)),SLOT(onShowArchiveWindowByAction(bool)));
		menu->addAction(viewAction,AG_DEFAULT,false);
	}

	if (isStreamMenu && isSupported(AStreamJid,NS_ARCHIVE_AUTO))
	{
		Action *autoAction = new Action(menu);
		autoAction->setCheckable(true);
		autoAction->setText(tr("Automatic Archiving"));
		autoAction->setData(ADR_STREAM_JID,AStreamJid.full());
		autoAction->setChecked(isArchiveAutoSave(AStreamJid));
		connect(autoAction,SIGNAL(triggered(bool)),SLOT(onSetAutoArchivingByAction(bool)));
		menu->addAction(autoAction,AG_DEFAULT+100,false);
	}

	if (isArchivePrefsEnabled(AStreamJid))
	{
		IArchiveStreamPrefs prefs = archivePrefs(AStreamJid);
		bool isSingleItemPrefs = AContacts.count()==1;
		bool isSeparateItemPrefs = !isStreamMenu && AContacts.count()==1 && prefs.itemPrefs.contains(AContacts.first());
		IArchiveItemPrefs itemPrefs = isStreamMenu ? archivePrefs(AStreamJid).defaultPrefs : archiveItemPrefs(AStreamJid,AContacts.value(0));

		Action *fullSaveAction = new Action(menu);
		fullSaveAction->setCheckable(true);
		fullSaveAction->setText(tr("Save Messages with Formatting"));
		fullSaveAction->setData(ADR_STREAM_JID,AStreamJid.full());
		fullSaveAction->setData(ADR_CONTACT_JID,AContacts);
		fullSaveAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_MESSAGE);
		fullSaveAction->setChecked(isSingleItemPrefs && (itemPrefs.save==ARCHIVE_SAVE_MESSAGE || itemPrefs.save==ARCHIVE_SAVE_STREAM));
		connect(fullSaveAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(fullSaveAction,AG_DEFAULT+200);

		Action *bodySaveAction = new Action(menu);
		bodySaveAction->setCheckable(true);
		bodySaveAction->setText(tr("Save Only Messages Text"));
		bodySaveAction->setData(ADR_STREAM_JID,AStreamJid.full());
		bodySaveAction->setData(ADR_CONTACT_JID,AContacts);
		bodySaveAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_BODY);
		bodySaveAction->setChecked(isSingleItemPrefs && itemPrefs.save==ARCHIVE_SAVE_BODY);
		connect(bodySaveAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(bodySaveAction,AG_DEFAULT+200);

		Action *disableSaveAction = new Action(menu);
		disableSaveAction->setCheckable(true);
		disableSaveAction->setText(tr("Do not Save Messages"));
		disableSaveAction->setData(ADR_STREAM_JID,AStreamJid.full());
		disableSaveAction->setData(ADR_CONTACT_JID,AContacts);
		disableSaveAction->setData(ADR_ITEM_SAVE,ARCHIVE_SAVE_FALSE);
		disableSaveAction->setChecked(isSingleItemPrefs && itemPrefs.save==ARCHIVE_SAVE_FALSE);
		connect(disableSaveAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(disableSaveAction,AG_DEFAULT+200);

		Action *allowOTRAction = new Action(menu);
		allowOTRAction->setCheckable(true);
		allowOTRAction->setText(tr("Allow Off-The-Record Sessions"));
		allowOTRAction->setData(ADR_STREAM_JID,AStreamJid.full());
		allowOTRAction->setData(ADR_CONTACT_JID,AContacts);
		allowOTRAction->setData(ADR_ITEM_OTR,ARCHIVE_OTR_CONCEDE);
		allowOTRAction->setChecked(isSingleItemPrefs && itemPrefs.otr!=ARCHIVE_OTR_APPROVE &&  itemPrefs.otr!=ARCHIVE_OTR_FORBID);
		connect(allowOTRAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(allowOTRAction,AG_DEFAULT+300);

		Action *forbidOTRAction = new Action(menu);
		forbidOTRAction->setCheckable(true);
		forbidOTRAction->setText(tr("Forbid Off-The-Record Sessions"));
		forbidOTRAction->setData(ADR_STREAM_JID,AStreamJid.full());
		forbidOTRAction->setData(ADR_CONTACT_JID,AContacts);
		forbidOTRAction->setData(ADR_ITEM_OTR,ARCHIVE_OTR_FORBID);
		forbidOTRAction->setChecked(isSingleItemPrefs && itemPrefs.otr==ARCHIVE_OTR_FORBID);
		connect(forbidOTRAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(forbidOTRAction,AG_DEFAULT+300);

		Action *approveOTRAction = new Action(menu);
		approveOTRAction->setCheckable(true);
		approveOTRAction->setText(tr("Manually Approve Off-The-Record Sessions"));
		approveOTRAction->setData(ADR_STREAM_JID,AStreamJid.full());
		approveOTRAction->setData(ADR_CONTACT_JID,AContacts);
		approveOTRAction->setData(ADR_ITEM_OTR,ARCHIVE_OTR_APPROVE);
		approveOTRAction->setChecked(isSingleItemPrefs && itemPrefs.otr==ARCHIVE_OTR_APPROVE);
		connect(approveOTRAction,SIGNAL(triggered(bool)),SLOT(onSetItemPrefsByAction(bool)));
		menu->addAction(approveOTRAction,AG_DEFAULT+300);

		if (!isStreamMenu)
		{
			Action *defAction = new Action(menu);
			defAction->setCheckable(true);
			defAction->setText(tr("Use Default Options"));
			defAction->setData(ADR_STREAM_JID,AStreamJid.full());
			defAction->setData(ADR_CONTACT_JID,AContacts);
			defAction->setChecked(isSingleItemPrefs && !isSeparateItemPrefs);
			connect(defAction,SIGNAL(triggered(bool)),SLOT(onRemoveItemPrefsByAction(bool)));
			menu->addAction(defAction,AG_DEFAULT+500,false);
		}
	}

	if (isStreamMenu && isReady(AStreamJid))
	{
		Action *optionsAction = new Action(menu);
		optionsAction->setText(tr("Options..."));
		optionsAction->setData(ADR_STREAM_JID,AStreamJid.full());
		connect(optionsAction,SIGNAL(triggered(bool)),SLOT(onShowHistoryOptionsDialogByAction(bool)));
		menu->addAction(optionsAction,AG_DEFAULT+500,false);
	}

	return menu;
}

void MessageArchiver::notifyInChatWindow(const Jid &AStreamJid, const Jid &AContactJid, const QString &AMessage) const
{
	IChatWindow *window = FMessageWidgets!=NULL ? FMessageWidgets->findChatWindow(AStreamJid,AContactJid) : NULL;
	if (window)
	{
		IMessageContentOptions options;
		options.kind = IMessageContentOptions::KindStatus;
		options.type |= IMessageContentOptions::TypeEvent;
		options.direction = IMessageContentOptions::DirectionIn;
		options.time = QDateTime::currentDateTime();
		window->viewWidget()->appendText(AMessage,options);
	}
}

bool MessageArchiver::hasStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
	return FSessionNegotiation!=NULL ? FSessionNegotiation->getSession(AStreamJid,AContactJid).status==IStanzaSession::Active : false;
}

bool MessageArchiver::isOTRStanzaSession(const IStanzaSession &ASession) const
{
	if (FDataForms)
	{
		int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
		if (index>=0)
			return ASession.form.fields.at(index).value.toString() == SFV_MUSTNOT_LOGGING;
	}
	return false;
}

bool MessageArchiver::isOTRStanzaSession(const Jid &AStreamJid, const Jid &AContactJid) const
{
	if (FSessionNegotiation && FDataForms)
	{
		IStanzaSession session = FSessionNegotiation->getSession(AStreamJid,AContactJid);
		if (session.status == IStanzaSession::Active)
			return isOTRStanzaSession(session);
	}
	return false;
}

QString MessageArchiver::stanzaSessionDirPath(const Jid &AStreamJid) const
{
	bool noError = true;

	QDir dir(FPluginManager->homePath());
	if (!dir.exists(ARCHIVE_DIR_NAME))
		noError &= dir.mkdir(ARCHIVE_DIR_NAME);
	noError &= dir.cd(ARCHIVE_DIR_NAME);

	QString streamDir = Jid::encode(AStreamJid.pBare());
	if (!dir.exists(streamDir))
		noError &= dir.mkdir(streamDir);
	noError &= dir.cd(streamDir);

	return noError ? dir.path() : QString::null;
}

void MessageArchiver::saveStanzaSessionContext(const Jid &AStreamJid, const Jid &AContactJid) const
{
	QString dirPath = stanzaSessionDirPath(AStreamJid);
	if (AStreamJid.isValid() && !dirPath.isEmpty())
	{
		QDomDocument sessions;
		QFile file(dirPath+"/"SESSIONS_FILE_NAME);
		if (file.open(QFile::ReadOnly))
		{
			if (!sessions.setContent(&file))
				sessions.clear();
			file.close();
		}
		if (file.open(QFile::WriteOnly|QFile::Truncate))
		{
			if (sessions.isNull())
				sessions.appendChild(sessions.createElement("stanzaSessions"));
			const StanzaSession &session = FSessions.value(AStreamJid).value(AContactJid);
			QDomElement elem = sessions.documentElement().appendChild(sessions.createElement("session")).toElement();
			elem.setAttribute("id",session.sessionId);
			elem.appendChild(sessions.createElement("jid")).appendChild(sessions.createTextNode(AContactJid.pFull()));
			if (!session.defaultPrefs)
				elem.appendChild(sessions.createElement("saveMode")).appendChild(sessions.createTextNode(session.saveMode));
			file.write(sessions.toByteArray());
			file.close();
		}
	}
}

void MessageArchiver::restoreStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId)
{
	QString dirPath = stanzaSessionDirPath(AStreamJid);
	if (AStreamJid.isValid() && !dirPath.isEmpty())
	{
		QFile file(dirPath+"/"SESSIONS_FILE_NAME);
		if (file.open(QFile::ReadOnly))
		{
			QDomDocument sessions;
			sessions.setContent(&file);
			file.close();

			QDomElement elem = sessions.documentElement().firstChildElement("session");
			while (!elem.isNull())
			{
				if (ASessionId.isEmpty() || elem.attribute("id")==ASessionId)
				{
					QString requestId;
					Jid contactJid = elem.firstChildElement("jid").text();
					QString saveMode= elem.firstChildElement("saveMode").text();

					if (saveMode.isEmpty() && archivePrefs(AStreamJid).itemPrefs.contains(contactJid))
					{
						requestId = removeArchiveItemPrefs(AStreamJid,contactJid);
					}
					else if (!saveMode.isEmpty() && archiveItemPrefs(AStreamJid,contactJid).save!=saveMode)
					{
						IArchiveStreamPrefs prefs = archivePrefs(AStreamJid);
						prefs.itemPrefs[contactJid].save = saveMode;
						requestId = setArchivePrefs(AStreamJid,prefs);
					}
					else
					{
						removeStanzaSessionContext(AStreamJid,elem.attribute("id"));
					}

					if (!requestId.isEmpty())
						FRestoreRequests.insert(requestId,elem.attribute("id"));
				}
				elem = elem.nextSiblingElement("session");
			}
		}
	}
}

void MessageArchiver::removeStanzaSessionContext(const Jid &AStreamJid, const QString &ASessionId) const
{
	QString dirPath = stanzaSessionDirPath(AStreamJid);
	if (AStreamJid.isValid() && !dirPath.isEmpty())
	{
		QDomDocument sessions;
		QFile file(dirPath+"/"SESSIONS_FILE_NAME);
		if (file.open(QFile::ReadOnly))
		{
			if (!sessions.setContent(&file))
				sessions.clear();
			file.close();
		}
		if (!sessions.isNull())
		{
			QDomElement elem = sessions.documentElement().firstChildElement("session");
			while (!elem.isNull())
			{
				if (elem.attribute("id") == ASessionId)
				{
					elem.parentNode().removeChild(elem);
					break;
				}
				elem = elem.nextSiblingElement("session");
			}
		}
		if (sessions.documentElement().hasChildNodes() && file.open(QFile::WriteOnly|QFile::Truncate))
		{
			file.write(sessions.toByteArray());
			file.close();
		}
		else
		{
			file.remove();
		}
	}
}

void MessageArchiver::startSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId)
{
	if (FSessionNegotiation)
	{
		foreach(Jid contactJid, FSessions.value(AStreamJid).keys())
		{
			const StanzaSession &session = FSessions.value(AStreamJid).value(contactJid);
			if (session.requestId == ARequestId)
			{
				saveStanzaSessionContext(AStreamJid,contactJid);
				FSessionNegotiation->resumeSession(AStreamJid,contactJid);
				break;
			}
		}
	}
}

void MessageArchiver::cancelSuspendedStanzaSession(const Jid &AStreamJid, const QString &ARequestId, const QString &AError)
{
	if (FSessionNegotiation)
	{
		foreach(Jid contactJid, FSessions.value(AStreamJid).keys())
		{
			StanzaSession &session = FSessions[AStreamJid][contactJid];
			if (session.requestId == ARequestId)
			{
				session.error = AError;
				FSessionNegotiation->resumeSession(AStreamJid,contactJid);
				break;
			}
		}
	}
}

void MessageArchiver::renegotiateStanzaSessions(const Jid &AStreamJid) const
{
	if (FSessionNegotiation)
	{
		QList<IStanzaSession> sessions = FSessionNegotiation->getSessions(AStreamJid,IStanzaSession::Active);
		foreach(IStanzaSession session, sessions)
		{
			bool isOTRSession = isOTRStanzaSession(session);
			IArchiveItemPrefs itemPrefs = archiveItemPrefs(AStreamJid,session.contactJid);
			if ((isOTRSession && itemPrefs.save!=ARCHIVE_SAVE_FALSE) || (!isOTRSession && itemPrefs.otr==ARCHIVE_OTR_REQUIRE))
			{
				removeStanzaSessionContext(AStreamJid,session.sessionId);
				FSessionNegotiation->initSession(AStreamJid,session.contactJid);
			}
		}
	}
}

bool MessageArchiver::isSelectionAccepted(const QList<IRosterIndex *> &ASelected) const
{
	static const QList<int> acceptTypes = QList<int>() << RIT_CONTACT << RIT_AGENT;
	if (!ASelected.isEmpty())
	{
		Jid singleStream;
		foreach(IRosterIndex *index, ASelected)
		{
			int indexType = index->type();
			Jid streamJid = index->data(RDR_STREAM_JID).toString();
			if (!acceptTypes.contains(indexType))
				return false;
			else if(!singleStream.isEmpty() && singleStream!=streamJid)
				return false;
			singleStream = streamJid;
		}
		return true;
	}
	return false;

}

void MessageArchiver::processRemoveRequest(const QString &ALocalId, RemoveRequest &ARequest)
{
	if (ARequest.engines.isEmpty())
	{
		if (ARequest.lastError.isEmpty())
			emit collectionsRemoved(ALocalId,ARequest.request);
		else
			emit requestFailed(ALocalId,ARequest.lastError);
		FRemoveRequests.remove(ALocalId);
	}
}

void MessageArchiver::processHeadersRequest(const QString &ALocalId, HeadersRequest &ARequest)
{
	if (ARequest.engines.count() == ARequest.headers.count())
	{
		if (!ARequest.engines.isEmpty() || ARequest.lastError.isEmpty())
		{
			QList<IArchiveHeader> headers;
			foreach(IArchiveEngine *engine, ARequest.engines)
			{
				foreach(IArchiveHeader header, ARequest.headers.value(engine))
				{
					if (!headers.contains(header))
						headers.append(header);
				}
			}

			if (ARequest.request.order == Qt::AscendingOrder)
				qSort(headers.begin(),headers.end(),qLess<IArchiveHeader>());
			else
				qSort(headers.begin(),headers.end(),qGreater<IArchiveHeader>());

			if (ARequest.request.maxItems>0 && headers.count()>ARequest.request.maxItems)
				headers = headers.mid(0,ARequest.request.maxItems);

			emit headersLoaded(ALocalId,headers);
		}
		else
		{
			emit requestFailed(ALocalId,ARequest.lastError);
		}
		FHeadersRequests.remove(ALocalId);
	}
}

void MessageArchiver::processCollectionRequest(const QString &ALocalId, CollectionRequest &ARequest)
{
	if (ARequest.lastError.isEmpty())
	{
		emit collectionLoaded(ALocalId,ARequest.collection);
	}
	else
	{
		emit requestFailed(ALocalId,ARequest.lastError);
	}
	FCollectionRequests.remove(ALocalId);
}

void MessageArchiver::processMessagesRequest(const QString &ALocalId, MessagesRequest &ARequest)
{
	if (!ARequest.lastError.isEmpty())
	{
		emit requestFailed(ALocalId,ARequest.lastError);
		FMesssagesRequests.remove(ALocalId);
	}
	else if (ARequest.headers.isEmpty() || (ARequest.request.maxItems>0 && ARequest.body.messages.count()>ARequest.request.maxItems))
	{
		if (ARequest.request.order == Qt::AscendingOrder)
			qSort(ARequest.body.messages.begin(),ARequest.body.messages.end(),qLess<Message>());
		else
			qSort(ARequest.body.messages.begin(),ARequest.body.messages.end(),qGreater<Message>());

		//Do not break collections
		//if (ARequest.request.maxItems>0 && ARequest.body.messages.count()>ARequest.request.maxItems)
		//	ARequest.body.messages = ARequest.body.messages.mid(0,ARequest.request.maxItems);

		emit messagesLoaded(ALocalId,ARequest.body);
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
			ARequest.lastError = tr("Failed to load conversation");
			processMessagesRequest(ALocalId,ARequest);
		}
	}
}

void MessageArchiver::onEngineCapabilitiesChanged(const Jid &AStreamJid)
{
	emit totalCapabilitiesChanged(AStreamJid);
}

void MessageArchiver::onEngineRequestFailed(const QString &AId, const QString &AError)
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

void MessageArchiver::onSelfRequestFailed(const QString &AId, const QString &AError)
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

void MessageArchiver::onStreamOpened(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		IStanzaHandle shandle;
		shandle.handler = this;
		shandle.streamJid = AXmppStream->streamJid();

		shandle.order = SHO_DEFAULT;
		shandle.direction = IStanzaHandle::DirectionIn;
		shandle.conditions.append(SHC_PREFS);
		shandle.conditions.append(SHC_PREFS_OLD);
		FSHIPrefs.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.conditions.clear();
		shandle.conditions.append(SHC_MESSAGE_BODY);
		FSHIMessageIn.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.direction = IStanzaHandle::DirectionOut;
		FSHIMessageOut.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));

		shandle.order = SHO_MO_ARCHIVER;
		FSHIMessageBlocks.insert(shandle.streamJid,FStanzaProcessor->insertStanzaHandle(shandle));
	}

	if (!FDiscovery || !FDiscovery->requestDiscoInfo(AXmppStream->streamJid(),AXmppStream->streamJid().domain()))
		applyArchivePrefs(AXmppStream->streamJid(),QDomElement());
}

void MessageArchiver::onStreamClosed(IXmppStream *AXmppStream)
{
	if (FStanzaProcessor)
	{
		FStanzaProcessor->removeStanzaHandle(FSHIPrefs.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessageIn.take(AXmppStream->streamJid()));
		FStanzaProcessor->removeStanzaHandle(FSHIMessageOut.take(AXmppStream->streamJid()));
	}

	closeHistoryOptionsNode(AXmppStream->streamJid());
	FFeatures.remove(AXmppStream->streamJid());
	FNamespaces.remove(AXmppStream->streamJid());
	FArchivePrefs.remove(AXmppStream->streamJid());
	FInStoragePrefs.removeAll(AXmppStream->streamJid());
	FSessions.remove(AXmppStream->streamJid());
	FPendingMessages.remove(AXmppStream->streamJid());

	emit archivePrefsChanged(AXmppStream->streamJid());
	emit archivePrefsClosed(AXmppStream->streamJid());
}

void MessageArchiver::onPrivateDataError(const QString &AId, const QString &AError)
{
	if (FPrefsLoadRequests.contains(AId))
	{
		Jid streamJid = FPrefsLoadRequests.take(AId);
		applyArchivePrefs(streamJid,QDomElement());
		emit requestFailed(AId,AError);
	}
	else if (FPrefsSaveRequests.contains(AId))
	{
		Jid streamJid = FPrefsSaveRequests.take(AId);
		if (FRestoreRequests.contains(AId))
			FRestoreRequests.remove(AId);
		else
			cancelSuspendedStanzaSession(streamJid,AId,AError);
		emit requestFailed(AId,AError);
	}
}

void MessageArchiver::onPrivateDataLoadedSaved(const QString &AId, const Jid &AStreamJid, const QDomElement &AElement)
{
	if (FPrefsLoadRequests.contains(AId))
	{
		FPrefsLoadRequests.remove(AId);
		applyArchivePrefs(AStreamJid,AElement);
		emit requestCompleted(AId);
	}
	else if (FPrefsSaveRequests.contains(AId))
	{
		applyArchivePrefs(AStreamJid,AElement);
		FPrefsSaveRequests.remove(AId);

		if (FRestoreRequests.contains(AId))
			removeStanzaSessionContext(AStreamJid,FRestoreRequests.take(AId));
		else
			startSuspendedStanzaSession(AStreamJid,AId);

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
	if (FRostersViewPlugin && AWidget==FRostersViewPlugin->rostersView()->instance() && !FRostersViewPlugin->rostersView()->hasMultiSelection())
	{
		if (AId == SCT_ROSTERVIEW_SHOWHISTORY)
		{
			QModelIndex index = FRostersViewPlugin->rostersView()->instance()->currentIndex();
			int indexType = index.data(RDR_TYPE).toInt();
			if (indexType==RIT_STREAM_ROOT || indexType==RIT_CONTACT || indexType==RIT_AGENT)
			{
				Jid streamJid = index.data(RDR_STREAM_JID).toString();
				Jid contactJid = indexType!=RIT_STREAM_ROOT ? index.data(RDR_FULL_JID).toString() : Jid::null;
				showArchiveWindow(streamJid,contactJid);
			}
		}
	}
}

void MessageArchiver::onRosterIndexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted)
{
	AAccepted = AAccepted || isSelectionAccepted(ASelected);
}

void MessageArchiver::onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, int ALabelId, Menu *AMenu)
{
	if (ALabelId == RLID_DISPLAY)
	{
		Jid streamJid = !AIndexes.isEmpty() ? AIndexes.first()->data(RDR_STREAM_JID).toString() : QString::null;
		if (AIndexes.count()==1 && AIndexes.first()->type()==RIT_STREAM_ROOT)
		{
			Menu *menu = createContextMenu(streamJid,QStringList()<<streamJid.full(),AMenu);
			if (!menu->isEmpty())
				AMenu->addAction(menu->menuAction(),AG_RVCM_ARCHIVER,true);
			else
				delete menu;
		}
		else if (isSelectionAccepted(AIndexes))
		{
			QMap<int, QStringList> rolesMap = FRostersViewPlugin->rostersView()->indexesRolesMap(AIndexes,QList<int>()<<RDR_PREP_BARE_JID,RDR_PREP_BARE_JID);
			Menu *menu = createContextMenu(streamJid,rolesMap.value(RDR_PREP_BARE_JID),AMenu);
			if (!menu->isEmpty())
				AMenu->addAction(menu->menuAction(),AG_RVCM_ARCHIVER,true);
			else
				delete menu;
		}
	}
}

void MessageArchiver::onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu)
{
	Menu *menu = createContextMenu(AWindow->streamJid(),QStringList()<<AUser->contactJid().full(),AMenu);
	if (!menu->isEmpty())
		AMenu->addAction(menu->menuAction(),AG_MUCM_ARCHIVER,true);
	else
		delete menu;
}

void MessageArchiver::onSetItemPrefsByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		IArchiveStreamPrefs prefs = archivePrefs(streamJid);
		foreach(Jid contactJid, action->data(ADR_CONTACT_JID).toStringList())
		{
			QString itemSave = action->data(ADR_ITEM_SAVE).toString();
			if (!itemSave.isEmpty())
			{
				if (streamJid != contactJid)
				{
					prefs.itemPrefs[contactJid] = archiveItemPrefs(streamJid,contactJid);
					prefs.itemPrefs[contactJid].save = itemSave;
				}
				else
				{
					prefs.defaultPrefs.save = itemSave;
				}
			}

			QString itemOTR = action->data(ADR_ITEM_OTR).toString();
			if (!itemOTR.isEmpty())
			{
				if (streamJid != contactJid)
				{
					prefs.itemPrefs[contactJid] = archiveItemPrefs(streamJid,contactJid);
					prefs.itemPrefs[contactJid].otr = itemOTR;
				}
				else
				{
					prefs.defaultPrefs.otr = itemOTR;
				}
			}
		}
		setArchivePrefs(streamJid,prefs);
	}
}

void MessageArchiver::onSetAutoArchivingByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		setArchiveAutoSave(streamJid,!isArchiveAutoSave(streamJid));
	}
}

void MessageArchiver::onRemoveItemPrefsByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		foreach(Jid contactJid, action->data(ADR_CONTACT_JID).toStringList())
			removeArchiveItemPrefs(streamJid,contactJid);
	}
}

void MessageArchiver::onShowArchiveWindowByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		Jid contactJid = action->data(ADR_CONTACT_JID).toString();
		showArchiveWindow(streamJid,contactJid);
	}
}

void MessageArchiver::onShowArchiveWindowByToolBarAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
	{
		IToolBarWidget *toolBarWidget = qobject_cast<IToolBarWidget *>(action->parent());
		if (toolBarWidget && toolBarWidget->editWidget())
			showArchiveWindow(toolBarWidget->editWidget()->streamJid(),toolBarWidget->editWidget()->contactJid());
	}
}

void MessageArchiver::onShowHistoryOptionsDialogByAction(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (FOptionsManager && FAccountManager && action)
	{
		Jid streamJid = action->data(ADR_STREAM_JID).toString();
		IAccount *account = FAccountManager->accountByStream(streamJid);
		if (account)
			FOptionsManager->showOptionsDialog(OPN_HISTORY"." + account->accountId().toString());
	}
}

void MessageArchiver::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
	if (!FNamespaces.contains(AInfo.streamJid) && !FInStoragePrefs.contains(AInfo.streamJid) && AInfo.node.isEmpty() && AInfo.streamJid.pDomain()==AInfo.contactJid.pFull())
	{
		QList<QString> &features = FFeatures[AInfo.streamJid];
		foreach(QString feature, AInfo.features)
		{
			if (feature==NS_ARCHIVE || feature==NS_ARCHIVE_OLD)
				features.append(NS_ARCHIVE);
			else if (feature==NS_ARCHIVE_AUTO || feature==NS_ARCHIVE_OLD_AUTO)
				features.append(NS_ARCHIVE_AUTO);
			else if (feature==NS_ARCHIVE_MANAGE || feature==NS_ARCHIVE_OLD_MANAGE)
				features.append(NS_ARCHIVE_MANAGE);
			else if (feature==NS_ARCHIVE_MANUAL || feature==NS_ARCHIVE_OLD_MANUAL)
				features.append(NS_ARCHIVE_MANUAL);
			else if (feature==NS_ARCHIVE_PREF || feature==NS_ARCHIVE_OLD_PREF)
				features.append(NS_ARCHIVE_PREF);
		}

		if (!features.isEmpty() && !AInfo.features.contains(features.first()))
			FNamespaces.insert(AInfo.streamJid, NS_ARCHIVE_OLD);
		else
			FNamespaces.insert(AInfo.streamJid, NS_ARCHIVE);

		if (features.contains(NS_ARCHIVE_PREF))
		{
			loadServerPrefs(AInfo.streamJid);
		}
		else if (features.contains(NS_ARCHIVE_AUTO))
		{
			FInStoragePrefs.append(AInfo.streamJid);
			applyArchivePrefs(AInfo.streamJid,QDomElement());
		}
		else
		{
			applyArchivePrefs(AInfo.streamJid,QDomElement());
		}
	}
}

void MessageArchiver::onStanzaSessionActivated(const IStanzaSession &ASession)
{
	bool isOTR = isOTRStanzaSession(ASession);
	if (!isOTR && FSessions.value(ASession.streamJid).contains(ASession.contactJid))
		restoreStanzaSessionContext(ASession.streamJid,ASession.sessionId);
	notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("Session negotiated: message logging %1").arg(isOTR ? tr("disallowed") : tr("allowed")));
}

void MessageArchiver::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
	if (FSessions.value(ASession.streamJid).contains(ASession.contactJid))
	{
		restoreStanzaSessionContext(ASession.streamJid,ASession.sessionId);
		FSessions[ASession.streamJid].remove(ASession.contactJid);
	}
	if (ASession.errorCondition.isEmpty())
		notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("Session terminated"));
	else
		notifyInChatWindow(ASession.streamJid,ASession.contactJid,tr("Session failed: %1").arg(ErrorHandler(ASession.errorCondition).message()));
}

void MessageArchiver::onToolBarWidgetCreated(IToolBarWidget *AWidget)
{
	if (AWidget->editWidget() != NULL)
	{
		Action *action = new Action(AWidget->toolBarChanger()->toolBar());
		action->setText(tr("View History"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY);
		action->setShortcutId(SCT_MESSAGEWINDOWS_SHOWHISTORY);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowArchiveWindowByToolBarAction(bool)));
		QToolButton *chatButton = AWidget->toolBarChanger()->insertAction(action,TBG_MWTBW_ARCHIVE_VIEW);

		ChatWindowMenu *chatMenu = new ChatWindowMenu(this,FPluginManager,AWidget,AWidget->toolBarChanger()->toolBar());
		chatButton->setMenu(chatMenu);
		chatButton->setPopupMode(QToolButton::MenuButtonPopup);
	}
}

void MessageArchiver::onOptionsChanged(const OptionsNode &ANode)
{
	if (Options::cleanNSpaces(ANode.path()) == OPV_HISTORY_ENGINE_ENABLED)
	{
		QUuid id = ANode.parent().nspace();
		emit archiveEngineEnableChanged(id,ANode.value().toBool());
		emit totalCapabilitiesChanged(Jid::null);
	}
}

Q_EXPORT_PLUGIN2(plg_messagearchiver, MessageArchiver)
