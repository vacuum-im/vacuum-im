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

	initialize(APluginManager);
	createActions();
	onEditWidgetContactJidChanged(Jid());
}

ChatWindowMenu::~ChatWindowMenu()
{

}

void ChatWindowMenu::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
	if (plugin)
		FDataForms = qobject_cast<IDataForms *>(plugin->instance());

	plugin = APluginManager->pluginInterface("ISessionNegotiation").value(0,NULL);
	if (plugin && FDataForms)
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
	if (plugin && FSessionNegotiation)
	{
		FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
		if (FDiscovery)
		{
			connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
		}
	}

	connect(FArchiver->instance(),SIGNAL(archivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)),
		SLOT(onArchivePrefsChanged(const Jid &, const IArchiveStreamPrefs &)));
	connect(FArchiver->instance(),SIGNAL(requestCompleted(const QString &)),
    SLOT(onRequestCompleted(const QString &)));
	connect(FArchiver->instance(),SIGNAL(requestFailed(const QString &, const QString &)),
		SLOT(onRequestFailed(const QString &,const QString &)));
	connect(FEditWidget->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onEditWidgetContactJidChanged(const Jid &)));
}

void ChatWindowMenu::createActions()
{
	FSaveTrue = new Action(this);
	FSaveTrue->setText(tr("Enable Message Logging"));
	FSaveTrue->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_ENABLE_LOGGING);
	connect(FSaveTrue,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FSaveTrue,AG_DEFAULT,false);

	FSaveFalse = new Action(this);
	FSaveFalse->setText(tr("Disable Message Logging"));
	FSaveFalse->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_DISABLE_LOGGING);
	connect(FSaveFalse,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FSaveFalse,AG_DEFAULT,false);

	FSessionRequire = new Action(this);
	FSessionRequire->setCheckable(true);
	FSessionRequire->setVisible(false);
	FSessionRequire->setText(tr("Require OTR Session"));
	FSessionRequire->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_REQUIRE_OTR);
	connect(FSessionRequire,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FSessionRequire,AG_DEFAULT,false);

	FSessionTerminate = new Action(this);
	FSessionTerminate->setVisible(false);
	FSessionTerminate->setText(tr("Terminate OTR Session"));
	FSessionTerminate->setIcon(RSR_STORAGE_MENUICONS,MNI_HISTORY_TERMINATE_OTR);
	connect(FSessionTerminate,SIGNAL(triggered(bool)),SLOT(onActionTriggered(bool)));
	addAction(FSessionTerminate,AG_DEFAULT,false);
}

void ChatWindowMenu::onActionTriggered(bool)
{
	Action *action = qobject_cast<Action *>(sender());
	if (FSaveRequest.isEmpty() && FSessionRequest.isEmpty())
	{
		if (action == FSaveTrue)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
			if (iprefs.save == ARCHIVE_SAVE_FALSE)
			{
				IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FEditWidget->streamJid());
				iprefs.save = sprefs.defaultPrefs.save!=ARCHIVE_SAVE_FALSE ? sprefs.defaultPrefs.save : QString(ARCHIVE_SAVE_BODY);
				if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
					iprefs.otr = sprefs.defaultPrefs.otr!=ARCHIVE_OTR_REQUIRE ? sprefs.defaultPrefs.otr : QString(ARCHIVE_OTR_CONCEDE);
				if (iprefs != sprefs.defaultPrefs)
				{
					sprefs.itemPrefs.insert(FEditWidget->contactJid().bare(),iprefs);
					FSaveRequest = FArchiver->setArchivePrefs(FEditWidget->streamJid(),sprefs);
				}
				else
				{
					FSaveRequest = FArchiver->removeArchiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
				}
			}
		}
		else if (action == FSaveFalse)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
			if (iprefs.save != ARCHIVE_SAVE_FALSE)
			{
				IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FEditWidget->streamJid());
				iprefs.save = ARCHIVE_SAVE_FALSE;
				if (iprefs != sprefs.defaultPrefs)
				{
					sprefs.itemPrefs.insert(FEditWidget->contactJid().bare(),iprefs);
					FSaveRequest = FArchiver->setArchivePrefs(FEditWidget->streamJid(),sprefs);
				}
				else
				{
					FSaveRequest = FArchiver->removeArchiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
				}
			}
		}
		else if (action == FSessionRequire)
		{
			IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FEditWidget->streamJid());
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
			FSessionRequire->setChecked(iprefs.otr == ARCHIVE_OTR_REQUIRE);
			if (iprefs.otr==ARCHIVE_OTR_REQUIRE)
				iprefs.otr =  sprefs.defaultPrefs.otr!=ARCHIVE_OTR_REQUIRE ? sprefs.defaultPrefs.otr : QString(ARCHIVE_OTR_CONCEDE);
			else
				iprefs.otr = ARCHIVE_OTR_REQUIRE;
			if (iprefs != sprefs.defaultPrefs)
			{
				sprefs.itemPrefs.insert(FEditWidget->contactJid().bare(),iprefs);
				FSessionRequest = FArchiver->setArchivePrefs(FEditWidget->streamJid(),sprefs);
			}
			else
			{
				FSessionRequest = FArchiver->removeArchiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid().bare());
			}
		}
		else if (action == FSessionTerminate)
		{
			if (FSessionNegotiation)
				FSessionNegotiation->terminateSession(FEditWidget->streamJid(),FEditWidget->contactJid());
		}
	}
	else if (action)
	{
		action->setChecked(!action->isChecked());
	}
}

void ChatWindowMenu::onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs)
{
	Q_UNUSED(APrefs);
	if (FEditWidget->streamJid() == AStreamJid)
	{
		bool logEnabled = FArchiver->isAutoArchiving(AStreamJid);
		if (FArchiver->isArchivePrefsEnabled(AStreamJid))
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(AStreamJid,FEditWidget->contactJid());
			logEnabled = iprefs.save!=ARCHIVE_SAVE_FALSE;
			FSaveTrue->setVisible(!logEnabled);
			FSaveFalse->setVisible(logEnabled);
			if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
			{
				FSessionRequire->setChecked(true);
				FSessionRequire->setVisible(true);
			}
			else
			{
				FSessionRequire->setChecked(false);
			}
			menuAction()->setEnabled(true);
		}
		else
		{
			menuAction()->setEnabled(false);
		}
		menuAction()->setToolTip(logEnabled ? tr("Message logging enabled") : tr("Message logging disabled"));
		menuAction()->setIcon(RSR_STORAGE_MENUICONS,logEnabled ? MNI_HISTORY_ENABLE_LOGGING : MNI_HISTORY_DISABLE_LOGGING);
	}
}

void ChatWindowMenu::onRequestCompleted(const QString &AId)
{
	if (FSessionRequest == AId)
	{
		if (FDataForms && FSessionNegotiation)
		{
			IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FEditWidget->streamJid(),FEditWidget->contactJid());
			IStanzaSession session = FSessionNegotiation->getSession(FEditWidget->streamJid(),FEditWidget->contactJid());
			if (session.status == IStanzaSession::Active)
			{
				int index = FDataForms->fieldIndex(SFP_LOGGING,session.form.fields);
				if (index>=0)
				{
					if (iprefs.otr==ARCHIVE_OTR_REQUIRE && session.form.fields.at(index).value.toString()!=SFV_MUSTNOT_LOGGING)
						FSessionNegotiation->initSession(FEditWidget->streamJid(),FEditWidget->contactJid());
					else if (iprefs.otr!=ARCHIVE_OTR_REQUIRE && session.form.fields.at(index).value.toString()==SFV_MUSTNOT_LOGGING)
						FSessionNegotiation->initSession(FEditWidget->streamJid(),FEditWidget->contactJid());
				}
			}
			else if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
			{
				FSessionNegotiation->initSession(FEditWidget->streamJid(),FEditWidget->contactJid());
			}
		}
		FSessionRequest.clear();
	}
	else if (FSaveRequest == AId)
	{
		FSaveRequest.clear();
	}
}

void ChatWindowMenu::onRequestFailed(const QString &AId, const QString &AError)
{
	if (FSaveRequest==AId || FSessionRequest==AId)
	{
		if (FToolBarWidget->viewWidget() != NULL)
		{
			IMessageContentOptions options;
			options.kind = IMessageContentOptions::Status;
			options.type |= IMessageContentOptions::Event;
			options.direction = IMessageContentOptions::DirectionIn;
			options.time = QDateTime::currentDateTime();
			FToolBarWidget->viewWidget()->appendText(tr("Changing archive preferences failed: %1").arg(AError),options);
		}
		if (FSessionRequest == AId)
			FSessionRequest.clear();
		else
			FSaveRequest.clear();
	}
}

void ChatWindowMenu::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
	if (ADiscoInfo.contactJid == FEditWidget->contactJid())
	{
		FSessionRequire->setVisible(FSessionRequire->isChecked() || ADiscoInfo.features.contains(NS_STANZA_SESSION));
	}
}

void ChatWindowMenu::onStanzaSessionActivated(const IStanzaSession &ASession)
{
	if (FDataForms && ASession.streamJid==FEditWidget->streamJid() && ASession.contactJid==FEditWidget->contactJid())
	{
		int index = FDataForms->fieldIndex(SFP_LOGGING,ASession.form.fields);
		if (index>=0)
		{
			if (ASession.form.fields.at(index).value.toString() == SFV_MUSTNOT_LOGGING)
			{
				FSaveTrue->setEnabled(false);
				FSaveFalse->setEnabled(false);
				FSessionTerminate->setVisible(true);
			}
			else
			{
				FSaveTrue->setEnabled(true);
				FSaveFalse->setEnabled(true);
				FSessionTerminate->setVisible(false);
			}
		}
	}
}

void ChatWindowMenu::onStanzaSessionTerminated(const IStanzaSession &ASession)
{
	if (ASession.streamJid==FEditWidget->streamJid() && ASession.contactJid==FEditWidget->contactJid())
	{
		FSaveTrue->setEnabled(true);
		FSaveFalse->setEnabled(true);
		FSessionTerminate->setVisible(false);
	}
}

void ChatWindowMenu::onEditWidgetContactJidChanged(const Jid &ABefore)
{
	if (FDiscovery)
	{
		if (FDiscovery->hasDiscoInfo(FEditWidget->streamJid(), FEditWidget->contactJid()))
			onDiscoInfoReceived(FDiscovery->discoInfo(FEditWidget->streamJid(), FEditWidget->contactJid()));
		else
			FDiscovery->requestDiscoInfo(FEditWidget->streamJid(),FEditWidget->contactJid());
	}

	if (FSessionNegotiation)
	{
		onStanzaSessionTerminated(FSessionNegotiation->getSession(FEditWidget->streamJid(),ABefore));

		IStanzaSession session = FSessionNegotiation->getSession(FEditWidget->streamJid(),FEditWidget->contactJid());
		if (session.status == IStanzaSession::Active)
			onStanzaSessionActivated(session);
	}

	onArchivePrefsChanged(FEditWidget->streamJid(),FArchiver->archivePrefs(FEditWidget->streamJid()));
}
