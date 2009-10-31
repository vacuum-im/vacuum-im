#include "chatwindowmenu.h"

#define SFP_LOGGING           "logging"
#define SFV_MUSTNOT_LOGGING   "mustnot"

ChatWindowMenu::ChatWindowMenu(IMessageArchiver *AArchiver, IChatWindow *AWindow) : Menu(AWindow->toolBarWidget()->toolBarChanger()->toolBar())
{
  FWindow = AWindow;
  FArchiver = AArchiver;
  FDataForms = NULL;
  FDiscovery = NULL;
  FSessionNegotiation = NULL;

  initialize();
  createActions();
  onWindowContactJidChanged(Jid());
}

ChatWindowMenu::~ChatWindowMenu()
{

}

void ChatWindowMenu::initialize()
{
  IPlugin *plugin = FArchiver->pluginManager()->pluginInterface("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = FArchiver->pluginManager()->pluginInterface("ISessionNegotiation").value(0,NULL);
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

  plugin = FArchiver->pluginManager()->pluginInterface("IServiceDiscovery").value(0,NULL);
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
  connect(FWindow->instance(),SIGNAL(contactJidChanged(const Jid &)),SLOT(onWindowContactJidChanged(const Jid &)));
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
      IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
      if (iprefs.save == ARCHIVE_SAVE_FALSE)
      {
        IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FWindow->streamJid());
        iprefs.save = sprefs.defaultPrefs.save!=ARCHIVE_SAVE_FALSE ? sprefs.defaultPrefs.save : QString(ARCHIVE_SAVE_BODY);
        if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
          iprefs.otr = sprefs.defaultPrefs.otr!=ARCHIVE_OTR_REQUIRE ? sprefs.defaultPrefs.otr : QString(ARCHIVE_OTR_CONCEDE);
        if (iprefs != sprefs.defaultPrefs)
        {
          sprefs.itemPrefs.insert(FWindow->contactJid().bare(),iprefs);
          FSaveRequest = FArchiver->setArchivePrefs(FWindow->streamJid(),sprefs);
        }
        else
        {
          FSaveRequest = FArchiver->removeArchiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
        }
      }
    }
    else if (action == FSaveFalse)
    {
      IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
      if (iprefs.save != ARCHIVE_SAVE_FALSE)
      {
        IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FWindow->streamJid());
        iprefs.save = ARCHIVE_SAVE_FALSE;
        if (iprefs != sprefs.defaultPrefs)
        {
          sprefs.itemPrefs.insert(FWindow->contactJid().bare(),iprefs);
          FSaveRequest = FArchiver->setArchivePrefs(FWindow->streamJid(),sprefs);
        }
        else
        {
          FSaveRequest = FArchiver->removeArchiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
        }
      }
    }
    else if (action == FSessionRequire)
    {
      IArchiveStreamPrefs sprefs = FArchiver->archivePrefs(FWindow->streamJid());
      IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
      FSessionRequire->setChecked(iprefs.otr == ARCHIVE_OTR_REQUIRE);
      if (iprefs.otr==ARCHIVE_OTR_REQUIRE)
        iprefs.otr =  sprefs.defaultPrefs.otr!=ARCHIVE_OTR_REQUIRE ? sprefs.defaultPrefs.otr : QString(ARCHIVE_OTR_CONCEDE);
      else
        iprefs.otr = ARCHIVE_OTR_REQUIRE;
      if (iprefs != sprefs.defaultPrefs)
      {
        sprefs.itemPrefs.insert(FWindow->contactJid().bare(),iprefs);
        FSessionRequest = FArchiver->setArchivePrefs(FWindow->streamJid(),sprefs);
      }
      else
      {
        FSessionRequest = FArchiver->removeArchiveItemPrefs(FWindow->streamJid(),FWindow->contactJid().bare());
      }
    }
    else if (action == FSessionTerminate)
    {
      if (FSessionNegotiation)
        FSessionNegotiation->terminateSession(FWindow->streamJid(),FWindow->contactJid());
    }
  }
  else if (action)
  {
    action->setChecked(!action->isChecked());
  }
}

void ChatWindowMenu::onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &/*APrefs*/)
{
  if (FWindow->streamJid() == AStreamJid)
  {
    bool logEnabled = false;
    if (FArchiver->isReady(AStreamJid))
    {
      IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(AStreamJid,FWindow->contactJid());
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
      IArchiveItemPrefs iprefs = FArchiver->archiveItemPrefs(FWindow->streamJid(),FWindow->contactJid());
      IStanzaSession session = FSessionNegotiation->getSession(FWindow->streamJid(),FWindow->contactJid());
      if (session.status == IStanzaSession::Active)
      {
        int index = FDataForms->fieldIndex(SFP_LOGGING,session.form.fields);
        if (index>=0)
        {
          if (iprefs.otr==ARCHIVE_OTR_REQUIRE && session.form.fields.at(index).value.toString()!=SFV_MUSTNOT_LOGGING)
            FSessionNegotiation->initSession(FWindow->streamJid(),FWindow->contactJid());
          else if (iprefs.otr!=ARCHIVE_OTR_REQUIRE && session.form.fields.at(index).value.toString()==SFV_MUSTNOT_LOGGING)
            FSessionNegotiation->initSession(FWindow->streamJid(),FWindow->contactJid());
        }
      }
      else if (iprefs.otr == ARCHIVE_OTR_REQUIRE)
      {
        FSessionNegotiation->initSession(FWindow->streamJid(),FWindow->contactJid());
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
    IMessageContentOptions options;
    options.kind = IMessageContentOptions::Status;
    options.type |= IMessageContentOptions::Event;
    options.direction = IMessageContentOptions::DirectionIn;
    options.time = QDateTime::currentDateTime();
    FWindow->viewWidget()->appendText(tr("Changing archive preferences failed: %1").arg(AError),options);
    if (FSessionRequest == AId)
      FSessionRequest.clear();
    else
      FSaveRequest.clear();
  }
}

void ChatWindowMenu::onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.contactJid == FWindow->contactJid())
  {
    FSessionRequire->setVisible(FSessionRequire->isChecked() || ADiscoInfo.features.contains(NS_STANZA_SESSION));
  }
}

void ChatWindowMenu::onStanzaSessionActivated(const IStanzaSession &ASession)
{
  if (FDataForms && ASession.streamJid==FWindow->streamJid() && ASession.contactJid==FWindow->contactJid())
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
  if (ASession.streamJid==FWindow->streamJid() && ASession.contactJid==FWindow->contactJid())
  {
    FSaveTrue->setEnabled(true);
    FSaveFalse->setEnabled(true);
    FSessionTerminate->setVisible(false);
  }
}

void ChatWindowMenu::onWindowContactJidChanged(const Jid &ABefour)
{

  if (FDiscovery)
  {
    if (FDiscovery->hasDiscoInfo(FWindow->contactJid()))
      onDiscoInfoReceived(FDiscovery->discoInfo(FWindow->contactJid()));
    else
      FDiscovery->requestDiscoInfo(FWindow->streamJid(),FWindow->contactJid());
  }
  
  if (FSessionNegotiation)
  {
    onStanzaSessionTerminated(FSessionNegotiation->getSession(FWindow->streamJid(),ABefour));

    IStanzaSession session = FSessionNegotiation->getSession(FWindow->streamJid(),FWindow->contactJid());
    if (session.status == IStanzaSession::Active)
      onStanzaSessionActivated(session);
  }

  onArchivePrefsChanged(FWindow->streamJid(),FArchiver->archivePrefs(FWindow->streamJid()));
}
