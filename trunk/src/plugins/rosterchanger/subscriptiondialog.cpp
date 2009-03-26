#include "subscriptiondialog.h"

SubscriptionDialog::SubscriptionDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, const Jid &AContactJid, 
                                       const QString &ANotify, const QString &AMessage, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Subscription request - %1").arg(AStreamJid.bare()));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_SUBSCRIBTION,0,0,"windowIcon");

  FRoster = NULL;
  FVcardPlugin = NULL;
  FMessageProcessor = NULL;

  FRosterChanger = ARosterChanger;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16,16));
  ui.lytMainLayout->setMenuBar(toolBar);
  FToolBarChanger = new ToolBarChanger(toolBar);

  ui.lblNotify->setText(ANotify);
  if (!AMessage.isEmpty())
    ui.lblMessage->setText(AMessage);
  else
    ui.lblMessage->setVisible(false);

  initialize(APluginManager);

  connect(ui.btbDialogButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
  connect(ui.btbDialogButtons,SIGNAL(rejected()),SLOT(onDialogRejected()));
}

SubscriptionDialog::~SubscriptionDialog()
{
  emit dialogDestroyed();
}

void SubscriptionDialog::initialize(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->getPlugins("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    FRoster = rosterPlugin!=NULL ? rosterPlugin->getRoster(FStreamJid) : NULL;
    if (FRoster && FRoster->rosterItem(FContactJid).isValid)
    {
      ui.rbtAddToRoster->setEnabled(false);
      ui.rbtSendAndRequest->setChecked(true);
    }
  }

  plugin = APluginManager->getPlugins("IMessageProcessor").value(0,NULL);
  if (plugin)
  {
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
    if (FMessageProcessor)
    {
      FShowChat = new Action(FToolBarChanger->toolBar());
      FShowChat->setText(tr("Chat"));
      FShowChat->setToolTip(tr("Open chat window"));
      FShowChat->setIcon(RSR_STORAGE_MENUICONS,MNI_MESSAGEHANDLER_CHAT);
      FToolBarChanger->addAction(FShowChat,AG_SRDT_ROSTERCHANGER_ACTIONS);
      connect(FShowChat,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

      FSendMessage = new Action(FToolBarChanger->toolBar());
      FSendMessage->setText(tr("Message"));
      FSendMessage->setToolTip(tr("Send Message"));
      FSendMessage->setIcon(RSR_STORAGE_MENUICONS,MNI_MESSAGEHANDLER_NORMAL);
      FToolBarChanger->addAction(FSendMessage,AG_SRDT_ROSTERCHANGER_ACTIONS);
      connect(FSendMessage,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
    }
  }

  plugin = APluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVcardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVcardPlugin)
    {
      FShowVCard = new Action(FToolBarChanger->toolBar());
      FShowVCard->setText(tr("VCard"));
      FShowVCard->setToolTip(tr("Show VCard"));
      FShowVCard->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
      FToolBarChanger->addAction(FShowVCard,AG_SRDT_ROSTERCHANGER_ACTIONS);
      connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
    }
  }
}

void SubscriptionDialog::onDialogAccepted()
{
  if (ui.rbtAddToRoster->isChecked())
  {
    IAddContactDialog *dialog = FRosterChanger->showAddContactDialog(FStreamJid);
    if (dialog)
    {
      dialog->setContactJid(FContactJid);
      dialog->setNickName(FContactJid.node());
    }
  }
  else if (ui.rbtSendAndRequest->isChecked())
  {
    FRosterChanger->subscribeContact(FStreamJid,FContactJid);
  }
  else if (ui.rbtRemoveAndRefuse->isChecked())
  {
    FRosterChanger->unsubscribeContact(FStreamJid,FContactJid);
  }
  accept();
}

void SubscriptionDialog::onDialogRejected()
{
  reject();
}

void SubscriptionDialog::onToolBarActionTriggered( bool )
{
  Action *action = qobject_cast<Action *>(sender());
  if (action!=NULL && FContactJid.isValid())
  {
    if (action == FShowChat)
    {
      FMessageProcessor->openWindow(FStreamJid,FContactJid,Message::Chat);
    }
    else if (action == FSendMessage)
    {
      FMessageProcessor->openWindow(FStreamJid,FContactJid,Message::Normal);
    }
    else if (action == FShowVCard)
    {
      FVcardPlugin->showVCardDialog(FStreamJid,FContactJid.bare());
    }
  }
}
