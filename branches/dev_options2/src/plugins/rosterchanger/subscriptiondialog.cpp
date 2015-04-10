#include "subscriptiondialog.h"

#include <definitions/toolbargroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/logger.h>

SubscriptionDialog::SubscriptionDialog(IRosterChanger *ARosterChanger, IPluginManager *APluginManager, const Jid &AStreamJid, const Jid &AContactJid,
	const QString &ANotify, const QString &AMessage, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Subscription request - %1").arg(AStreamJid.uBare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_RCHANGER_SUBSCRIBTION,0,0,"windowIcon");

	FRoster = NULL;
	FVCardManager = NULL;
	FNotifications = NULL;
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

Jid SubscriptionDialog::streamJid() const
{
	return FStreamJid;
}

Jid SubscriptionDialog::contactJid() const
{
	return FContactJid;
}

QVBoxLayout *SubscriptionDialog::actionsLayout() const
{
	return ui.lytActionsLayout;
}

ToolBarChanger *SubscriptionDialog::toolBarChanger() const
{
	return FToolBarChanger;
}

void SubscriptionDialog::initialize(IPluginManager *APluginManager)
{
	IPlugin *plugin = APluginManager->pluginInterface("IRosterManager").value(0,NULL);
	if (plugin)
	{
		IRosterManager *rosterManager = qobject_cast<IRosterManager *>(plugin->instance());
		FRoster = rosterManager!=NULL ? rosterManager->findRoster(FStreamJid) : NULL;
		if (FRoster && FRoster->hasItem(FContactJid))
		{
			ui.rbtAddToRoster->setEnabled(false);
			ui.rbtSendAndRequest->setChecked(true);
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
			FShowChat->setIcon(RSR_STORAGE_MENUICONS,MNI_CHATMHANDLER_MESSAGE);
			FToolBarChanger->insertAction(FShowChat,TBG_RCSRD_ROSTERCHANGER);
			connect(FShowChat,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

			FSendMessage = new Action(FToolBarChanger->toolBar());
			FSendMessage->setText(tr("Message"));
			FSendMessage->setToolTip(tr("Send Message"));
			FSendMessage->setIcon(RSR_STORAGE_MENUICONS,MNI_NORMALMHANDLER_MESSAGE);
			FToolBarChanger->insertAction(FSendMessage,TBG_RCSRD_ROSTERCHANGER);
			connect(FSendMessage,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
		}
	}

	plugin = APluginManager->pluginInterface("IVCardManager").value(0,NULL);
	if (plugin)
	{
		FVCardManager = qobject_cast<IVCardManager *>(plugin->instance());
		if (FVCardManager)
		{
			FShowVCard = new Action(FToolBarChanger->toolBar());
			FShowVCard->setText(tr("VCard"));
			FShowVCard->setToolTip(tr("Show VCard"));
			FShowVCard->setIcon(RSR_STORAGE_MENUICONS,MNI_VCARD);
			FToolBarChanger->insertAction(FShowVCard,TBG_RCSRD_ROSTERCHANGER);
			connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
		}
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
}

void SubscriptionDialog::onDialogAccepted()
{
	if (ui.rbtAddToRoster->isChecked())
	{
		IAddContactDialog *dialog = FRosterChanger->showAddContactDialog(FStreamJid);
		if (dialog)
		{
			dialog->setContactJid(FContactJid);
			dialog->setNickName(FNotifications!=NULL ? FNotifications->contactName(FStreamJid,FContactJid) : FContactJid.uNode());
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
			FMessageProcessor->createMessageWindow(FStreamJid,FContactJid,Message::Chat,IMessageHandler::SM_SHOW);
		}
		else if (action == FSendMessage)
		{
			FMessageProcessor->createMessageWindow(FStreamJid,FContactJid,Message::Normal,IMessageHandler::SM_SHOW);
		}
		else if (action == FShowVCard)
		{
			FVCardManager->showVCardDialog(FStreamJid,FContactJid.bare());
		}
	}
}
