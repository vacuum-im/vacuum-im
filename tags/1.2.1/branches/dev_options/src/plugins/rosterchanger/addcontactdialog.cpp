#include "addcontactdialog.h"

#include <QSet>
#include <QMessageBox>

AddContactDialog::AddContactDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Add contact - %1").arg(AStreamJid.bare()));
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_ADD_CONTACT,0,0,"windowIcon");
  
  FRoster = NULL;
  FVcardPlugin = NULL;
  FMessageProcessor = NULL;
  FResolving = false;

  FRosterChanger = ARosterChanger;
  FStreamJid = AStreamJid;

  QToolBar *toolBar = new QToolBar(this);
  toolBar->setIconSize(QSize(16,16));
  ui.lytMainLayout->setMenuBar(toolBar);
  FToolBarChanger = new ToolBarChanger(toolBar);

  setSubscriptionMessage(tr("Please, authorize me to your presence."));

  initialize(APluginManager);

  connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
  connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));
}

AddContactDialog::~AddContactDialog()
{
  emit dialogDestroyed();
}

Jid AddContactDialog::streamJid() const
{
  return FStreamJid;
}

Jid AddContactDialog::contactJid() const
{
  return ui.lneContact->text();
}

void AddContactDialog::setContactJid(const Jid &AContactJid)
{
  ui.lneContact->setText(AContactJid.bare());
}

QString AddContactDialog::nickName() const
{
  return ui.lneNickName->text();
}

void AddContactDialog::setNickName(const QString &ANick)
{
  ui.lneNickName->setText(ANick);
}

QString AddContactDialog::group() const
{
  return ui.cmbGroup->currentText();
}

void AddContactDialog::setGroup(const QString &AGroup)
{
  ui.cmbGroup->setEditText(AGroup);
}

QString AddContactDialog::subscriptionMessage() const
{
  return ui.tedMessage->toPlainText();
}

void AddContactDialog::setSubscriptionMessage(const QString &AText)
{
  ui.tedMessage->setPlainText(AText);
}

bool AddContactDialog::subscribeContact() const
{
  return ui.chbSubscribe->isChecked();
}

void AddContactDialog::setSubscribeContact(bool ASubscribe)
{
  ui.chbSubscribe->setCheckState(ASubscribe ? Qt::Checked : Qt::Unchecked);
}

ToolBarChanger *AddContactDialog::toolBarChanger() const
{
  return FToolBarChanger;
}

void AddContactDialog::initialize(IPluginManager *APluginManager)
{
  IPlugin *plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
  if (plugin)
  {
    IRosterPlugin *rosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
    FRoster = rosterPlugin!=NULL ? rosterPlugin->getRoster(FStreamJid) : NULL;
    if (FRoster)
    {
      ui.cmbGroup->addItems(FRoster->groups().toList());
      ui.cmbGroup->model()->sort(0,Qt::AscendingOrder);
      ui.cmbGroup->setCurrentIndex(-1);
      ui.lblGroupDelim->setText(tr("* nested group delimiter - '%1'").arg(Qt::escape(FRoster->groupDelimiter())));
    }
  }

  plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
  if (plugin)
  {
    FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
    if (FMessageProcessor)
    {
      FShowChat = new Action(FToolBarChanger->toolBar());
      FShowChat->setText(tr("Chat"));
      FShowChat->setToolTip(tr("Open chat window"));
      FShowChat->setIcon(RSR_STORAGE_MENUICONS,MNI_CHAT_MHANDLER_MESSAGE);
      FToolBarChanger->insertAction(FShowChat,TBG_RCACD_ROSTERCHANGER);
      connect(FShowChat,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

      FSendMessage = new Action(FToolBarChanger->toolBar());
      FSendMessage->setText(tr("Message"));
      FSendMessage->setToolTip(tr("Send Message"));
      FSendMessage->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMAL_MHANDLER_MESSAGE);
      FToolBarChanger->insertAction(FSendMessage,TBG_RCACD_ROSTERCHANGER);
      connect(FSendMessage,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
    }
  }
  
  plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
  if (plugin)
  {
    FVcardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
    if (FVcardPlugin)
    {
      FShowVCard = new Action(FToolBarChanger->toolBar());
      FShowVCard->setText(tr("VCard"));
      FShowVCard->setToolTip(tr("Show VCard"));
      FShowVCard->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
      FToolBarChanger->insertAction(FShowVCard,TBG_RCACD_ROSTERCHANGER);
      connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

      FResolve = new Action(FToolBarChanger->toolBar());
      FResolve->setText(tr("Nick"));
      FResolve->setToolTip(tr("Resolve nick name"));
      FResolve->setIcon(RSR_STORAGE_MENUICONS,MNI_GATEWAYS_RESOLVE);
      FToolBarChanger->insertAction(FResolve,TBG_RCACD_ROSTERCHANGER);
      connect(FResolve,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

      connect(FVcardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
    }
  }
}

void AddContactDialog::onDialogAccepted()
{
  if (contactJid().isValid())
  {
    if (!FRoster->rosterItem(contactJid()).isValid)
    {
      QSet<QString> groups;
      if (!group().isEmpty())
        groups += group();
      FRoster->setItem(contactJid().bare(),nickName(),groups);
      if (subscribeContact())
        FRosterChanger->subscribeContact(FStreamJid,contactJid(),subscriptionMessage());
      accept();
    }
    else
    {
      QMessageBox::information(NULL,FStreamJid.full(),tr("Contact <b>%1</b> already exists.").arg(contactJid().hBare()));
    }
  }
  else if (!contactJid().isEmpty())
  {
    QMessageBox::warning(this,FStreamJid.bare(),
      tr("Can`t add contact '<b>%1</b>' because it is not a valid Jaber ID").arg(contactJid().hBare())); 
  }
}

void AddContactDialog::onToolBarActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action!=NULL && contactJid().isValid())
  {
    if (action == FShowChat)
    {
      FMessageProcessor->openWindow(FStreamJid,contactJid(),Message::Chat);
    }
    else if (action == FSendMessage)
    {
      FMessageProcessor->openWindow(FStreamJid,contactJid(),Message::Normal);
    }
    else if (action == FShowVCard)
    {
      FVcardPlugin->showVCardDialog(FStreamJid,contactJid().bare());
    }
    else if (action == FResolve)
    {
      FResolving = true;
      if (FVcardPlugin->hasVCard(contactJid().bare()))
        onVCardReceived(contactJid());
      else
        FVcardPlugin->requestVCard(FStreamJid,contactJid());
    }
  }
}

void AddContactDialog::onVCardReceived(const Jid &AContactJid)
{
  if (FResolving && (AContactJid && contactJid()))
  {
    IVCard *vcard = FVcardPlugin->vcard(AContactJid.bare());
    if (vcard)
    {
      setNickName(vcard->value(VVN_NICKNAME));
      vcard->unlock();
    }
    FResolving = false;
  }
}