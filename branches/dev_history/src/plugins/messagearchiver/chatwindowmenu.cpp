#include "chatwindowmenu.h"

#define SFP_LOGGING           "logging"
#define SFV_MUSTNOT_LOGGING   "mustnot"

ChatWindowMenu::ChatWindowMenu(IMessageArchiver *AArchiver, IPluginManager *APluginManager, IToolBarWidget *AToolBarWidget, QWidget *AParent) : Menu(AParent)
{
	FToolBarWidget = AToolBarWidget;
	FEditWidget = AToolBarWidget->editWidget();
	
	FArchiver = AArchiver;
	FDataForms = NULL;
	FDiscovery = NULL;
	FSessionNegotiation = NULL;

	FRestorePrefs = false;

	initialize(APluginManager);
	createActions();

	updateMenu();
}

ChatWindowMenu::~ChatWindowMenu()
{

}

Jid ChatWindowMenu::streamJid() const
{
	return FEditWidget->streamJid();
}

Jid ChatWindowMenu::contactJid() const
{
	return FEditWidget->contactJid();
}

void ChatWindowMenu::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

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

	plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
	if (plugin)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
			connect(FDiscovery->instance(),SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoChanged(const IDiscoInfo &)));
		}
	}

	connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &)),SLOT(onArchivePrefsChanged(const Jid &)));
	connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),SLOT(onArchiveRequestCompleted(const QString &)));
	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),SLOT(onArchiveRequestFailed(const QString &,const QString &)));
	connect(FEditWidget->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onEditWidgetContactJidChanged(const Jid &)));
}

void ChatWindowMenu::createActions()
{
	FEnableArchiving = new Action(this);
	FEnableArchiving->setCheckable(true);
	FEnableArchiving->setText(tr("Enable Message Archiving"));
	FEnableArchiving->setShortcutId(SCT_MESSAGEWINDOWS_HISTORYENABLE);
	connect(FEnableArchiving,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FEnableArchiving,AG_DEFAULT,false);

	FDisableArchiving = new Action(this);
	FDisableArchiving->setCheckable(true);
	FDisableArchiving->setText(tr("Disable Message Archiving"));
	FDisableArchiving->setShortcutId(SCT_MESSAGEWINDOWS_HISTORYDISABLE);
	connect(FDisableArchiving,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FDisableArchiving,AG_DEFAULT,false);

	FStartOTRSession = new Action(this);
	FStartOTRSession->setVisible(false);
	FStartOTRSession->setText(tr("Start Off-The-Record Session"));
	FStartOTRSession->setShortcutId(SCT_MESSAGEWINDOWS_HISTORYREQUIREOTR);
	connect(FStartOTRSession,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FStartOTRSession,AG_DEFAULT+100,false);

	FStopOTRSession = new Action(this);
	FStopOTRSession->setVisible(false);
	FStopOTRSession->setText(tr("Terminate Off-The-Record Session"));
	FStopOTRSession->setShortcutId(SCT_MESSAGEWINDOWS_HISTORYTERMINATEOTR);
	connect(FStopOTRSession,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FStopOTRSession,AG_DEFAULT+100,false);
}

bool ChatWindowMenu::isOTRStanzaSession(const IStanzaSession &ASession) const
{
	if (FDataForms && ASession.status==IStanzaSession::Active)
	{
		int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
		if (index>=0)
			return ASession.form.fields.at(index).value.toString() == SFV_MUSTNOT_LOGGING;
	}
	return false;
}

void ChatWindowMenu::restoreSessionPrefs(const Jid &AContactJid)
{
	if (FRestorePrefs)
	{
		if (!FSessionPrefs.save.isEmpty() && !FSessionPrefs.otr.isEmpty())
		{
			IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(streamJid());
			sprefs.itemPrefs[AContactJid] = FSessionPrefs;
			FSaveRequest = FArchiver->setArchivePrefs(streamJid(),sprefs);
		}
		else
		{
			FSaveRequest = FArchiver->removeArchiveItemPrefs(streamJid(),AContactJid);
		}
		FRestorePrefs = false;
	}
}

void ChatWindowMenu::updateMenu()
{
	if (FArchiver->isArchivePrefsEnabled(streamJid()))
	{
		IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(streamJid(),contactJid());
		bool isOTRSession = FSessionNegotiation!=NULL ? isOTRStanzaSession(FSessionNegotiation->getSession(streamJid(),contactJid())) : false;

		FEnableArchiving->setVisible(true);
		FEnableArchiving->setChecked(iprefs.save != ARCHIVE_SAVE_FALSE);
		FEnableArchiving->setEnabled(FSaveRequest.isEmpty() && FSessionRequest.isEmpty() && !isOTRSession);

		FDisableArchiving->setVisible(true);
		FDisableArchiving->setChecked(iprefs.save == ARCHIVE_SAVE_FALSE);
		FDisableArchiving->setEnabled(FSaveRequest.isEmpty() && FSessionRequest.isEmpty() && !isOTRSession);

		if (FSessionNegotiation && FDataForms && FDiscovery)
		{
			FStartOTRSession->setEnabled(FSaveRequest.isEmpty() && FSessionRequest.isEmpty() && iprefs.otr!=ARCHIVE_OTR_FORBID);
			FStartOTRSession->setVisible(!isOTRSession && FDiscovery->discoInfo(streamJid(),contactJid()).features.contains(NS_STANZA_SESSION));

			FStopOTRSession->setEnabled(FSaveRequest.isEmpty() && FSessionRequest.isEmpty());
			FStopOTRSession->setVisible(isOTRSession);
		}
		else
		{
			FStartOTRSession->setVisible(false);
			FStopOTRSession->setVisible(false);
		}
	}
	else
	{
		FEnableArchiving->setVisible(false);
		FDisableArchiving->setVisible(false);
		FStartOTRSession->setVisible(false);
		FStopOTRSession->setVisible(false);
	}
}

void ChatWindowMenu::onActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (action && FSaveRequest.isEmpty() && FSessionRequest.isEmpty())
	{
		if (action == FEnableArchiving)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(streamJid(),contactJid().bare());
			if (iprefs.save == ARCHIVE_SAVE_FALSE)
			{
				IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(streamJid());
				iprefs.save = sprefs.defaultPrefs.save!=ARCHIVE_SAVE_FALSE ? sprefs.defaultPrefs.save : QString(ARCHIVE_SAVE_MESSAGE);
				if (iprefs != sprefs.defaultPrefs)
				{
					sprefs.itemPrefs.insert(contactJid().bare(),iprefs);
					FSaveRequest = FArchiver->setArchivePrefs(streamJid(),sprefs);
				}
				else
				{
					FSaveRequest = FArchiver->removeArchiveItemPrefs(streamJid(),contactJid().bare());
				}
			}
		}
		else if (action == FDisableArchiving)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(streamJid(),contactJid().bare());
			if (iprefs.save != ARCHIVE_SAVE_FALSE)
			{
				IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(streamJid());
				iprefs.save = ARCHIVE_SAVE_FALSE;
				if (iprefs != sprefs.defaultPrefs)
				{
					sprefs.itemPrefs.insert(contactJid().bare(),iprefs);
					FSaveRequest = FArchiver->setArchivePrefs(streamJid(),sprefs);
				}
				else
				{
					FSaveRequest = FArchiver->removeArchiveItemPrefs(streamJid(),contactJid().bare());
				}
			}
		}
		else if (action == FStartOTRSession)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(streamJid(),contactJid());
			if (iprefs.otr != ARCHIVE_OTR_REQUIRE)
			{
				IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(streamJid());
				
				FRestorePrefs = true;
				FSessionPrefs = sprefs.itemPrefs.value(contactJid());

				iprefs.otr = ARCHIVE_OTR_REQUIRE;
				sprefs.itemPrefs.insert(contactJid(),iprefs);
				FSessionRequest = FArchiver->setArchivePrefs(streamJid(),sprefs);
			}
			else if (FSessionNegotiation)
			{
				FSessionNegotiation->initSession(streamJid(),contactJid());
			}
		}
		else if (action == FStopOTRSession)
		{
			if (FSessionNegotiation)
				FSessionNegotiation->terminateSession(streamJid(),contactJid());
		}
		updateMenu();
	}
}

void ChatWindowMenu::onArchivePrefsChanged(const Jid &AStreamJid)
{
	if (streamJid() == AStreamJid)
	{
		updateMenu();
	}
}

void ChatWindowMenu::onArchiveRequestCompleted(const QString &AId)
{
	if (FSessionRequest == AId)
	{
		if (FSessionNegotiation)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(streamJid(),contactJid());
			IStanzaSession session = FSessionNegotiation->getSession(streamJid(),contactJid());
			if (session.status == IStanzaSession::Active)
			{
				bool isOTRSession = isOTRStanzaSession(session);
				if (!isOTRSession && iprefs.otr==ARCHIVE_OTR_REQUIRE)
					FSessionNegotiation->initSession(streamJid(),contactJid());
				else if (!isOTRSession && iprefs.otr!=ARCHIVE_OTR_REQUIRE)
					FSessionNegotiation->initSession(streamJid(),contactJid());
			}
			else if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
			{
				FSessionNegotiation->initSession(streamJid(),contactJid());
			}
		}
		FSessionRequest.clear();
		updateMenu();
	}
	else if (FSaveRequest == AId)
	{
		FSaveRequest.clear();
		updateMenu();
	}
}

void ChatWindowMenu::onArchiveRequestFailed(const QString &AId, const QString &AError)
{
	if (FSaveRequest==AId || FSessionRequest==AId)
	{
		if (FToolBarWidget->viewWidget() != NULL)
		{
			IMessageContentOptions options;
			options.kind = IMessageContentOptions::KindStatus;
			options.type |= IMessageContentOptions::TypeEvent;
			options.direction = IMessageContentOptions::DirectionIn;
			options.time = QDateTime::currentDateTime();
			FToolBarWidget->viewWidget()->appendText(tr("Failed to change archive preferences: %1").arg(AError),options);
		}
		if (FSessionRequest == AId)
			FSessionRequest.clear();
		else
			FSaveRequest.clear();
		updateMenu();
	}
}

void ChatWindowMenu::onDiscoInfoChanged(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.streamJid==streamJid() && ADiscoInfo.contactJid==contactJid())
	{
		updateMenu();
	}
}

void ChatWindowMenu::onStanzaSessionActivated(const IStanzaSession &ASession)
{
	if (ASession.streamJid==streamJid() && ASession.contactJid==contactJid())
	{
		updateMenu();
	}
}

void ChatWindowMenu::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
	if (ASession.streamJid==streamJid() && ASession.contactJid==contactJid())
	{
		restoreSessionPrefs(contactJid());
		updateMenu();
	}
}

void ChatWindowMenu::onEditWidgetContactJidChanged(const Jid &ABefore)
{
	restoreSessionPrefs(ABefore);
	updateMenu();
}
