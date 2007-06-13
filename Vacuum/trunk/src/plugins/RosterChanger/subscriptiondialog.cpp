#include "subscriptiondialog.h"

SubscriptionDialog::SubscriptionDialog(QWidget *AParent)
  : QDialog(AParent)
{
  setupUi(this);
  FToolBar = new QToolBar(this);
  vboxLayout->insertWidget(vboxLayout->indexOf(tedMessage),FToolBar);
  FButtonGroup = new QButtonGroup(this);
  FButtonGroup->addButton(pbtNext,NextButton);
  FButtonGroup->addButton(pbtSubscribe,AskButton);
  FButtonGroup->addButton(pbtSubscribed,AuthButton);
  FButtonGroup->addButton(pbtUnsubscribe,RefuseButton);
  FButtonGroup->addButton(pbtUnsubscribed,RejectButton);
  connect(FButtonGroup,SIGNAL(buttonClicked(int)),SLOT(onButtonClicked(int)));
  FDialogAction = new Action(this);
}

SubscriptionDialog::~SubscriptionDialog()
{

}

void SubscriptionDialog::setupDialog(const Jid &AStreamJid, const Jid &AContactJid, QDateTime ATime, 
                                     IRoster::SubsType ASubsType, const QString &AStatus)
{
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FDateTime = ATime;
  FSubsType = ASubsType;
  FStatus = AStatus;

  FDialogAction->setData(Action::DR_StreamJid,AStreamJid.pFull());
  FDialogAction->setData(Action::DR_Parametr1,AContactJid.pBare());

  lblAccountJid->setText(AStreamJid.full());
  lblFromJid->setText(AContactJid.bare());
  lblDateTime->setText(ATime.toString());

  switch(ASubsType) 
  {
  case IRoster::Subscribe:
    {
      tedMessage->setText(tr("<b>Subscription request</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>%1 wants to subscribe to your presence.</b>").arg(AContactJid.bare()));
      break;
    }
  case IRoster::Subscribed:
    {
      tedMessage->setText(tr("<b>Subscription approved</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>You are now authorized for %1 presence.</b>").arg(AContactJid.bare()));
      break;
    }
  case IRoster::Unsubscribe:
    {
      tedMessage->setText(tr("<b>Subscription refused</b><br>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>%1 unsubscribed from your presence.</b>").arg(AContactJid.bare()));
      break;
    }
  case IRoster::Unsubscribed:
    {
      tedMessage->setText(tr("<b>Subscription rejected</b><br>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>You are now unsubscribed from %1 presence.</b>").arg(AContactJid.bare()));
      break;
    }
  }
  tedMessage->append(tr("<br>Click <b>Ask</b> to ask for contact presence subscription."));
  tedMessage->append(tr("Click <b>Add/Auth</b> to authorize the subscription and add the contact to your contact list."));
  tedMessage->append(tr("Click <b>Refuse</b> to refuse from presence subscription."));
  tedMessage->append(tr("Click <b>Reject</b> to reject the presence subscription."));
  emit dialogReady();
}

void SubscriptionDialog::setNext(bool ANext)
{
  if (ANext)
    pbtNext->setEnabled(true);
  else
    pbtNext->setEnabled(false);
}

void SubscriptionDialog::onButtonClicked(int AId)
{
  bool doNext = pbtNext->isEnabled();
  bool doAccept = true;

  switch(AId)
  {
  case AskButton:
    {
      FDialogAction->setData(Action::DR_Parametr2,IRoster::Subscribe);
      FDialogAction->trigger();
      break;
    }
  case AuthButton:
    {
      FDialogAction->setData(Action::DR_Parametr2,IRoster::Subscribed);
      FDialogAction->trigger();
      break;
    }
  case RefuseButton:
    {                                                         
      FDialogAction->setData(Action::DR_Parametr2,IRoster::Unsubscribe);
      FDialogAction->trigger();
      break;
    }
  case RejectButton:
    {
      FDialogAction->setData(Action::DR_Parametr2,IRoster::Unsubscribed);
      FDialogAction->trigger();
      break;
    }
  case NextButton:
    {
      break;
    }
  default:
    {
      doNext = false;
      doAccept = false;
    }
  }
  
  if (doNext)
    emit setupNext();
  else if (doAccept)
    accept();
}