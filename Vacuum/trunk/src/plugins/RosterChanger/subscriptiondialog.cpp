#include "subscriptiondialog.h"

SubscriptionDialog::SubscriptionDialog(QWidget *AParent)
  : QDialog(AParent)
{
  setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  FToolBar = new QToolBar(this);
  vboxLayout->insertWidget(vboxLayout->indexOf(tedMessage),FToolBar);
  FButtonGroup = new QButtonGroup(this);
  FButtonGroup->addButton(pbtSubscribe,AskButton);
  FButtonGroup->addButton(pbtSubscribed,AuthButton);
  FButtonGroup->addButton(pbtUnsubscribe,RefuseButton);
  FButtonGroup->addButton(pbtUnsubscribed,RejectButton);
  FButtonGroup->addButton(pbtNext,NextButton);
  FButtonGroup->addButton(pbtClose,CloseButton);
  connect(FButtonGroup,SIGNAL(buttonClicked(int)),SLOT(onButtonClicked(int)));
  FDialogAction = new Action(this);
}

SubscriptionDialog::~SubscriptionDialog()
{

}

void SubscriptionDialog::setupDialog(const Jid &AStreamJid, const Jid &AContactJid, QDateTime ATime, 
                                     IRoster::SubsType ASubsType, const QString &AStatus, const QString &ASubs)
{
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FDateTime = ATime;
  FSubsType = ASubsType;
  FStatus = AStatus;
  FSubscription = ASubs;

  FDialogAction->setData(Action::DR_StreamJid,AStreamJid.pFull());
  FDialogAction->setData(Action::DR_Parametr1,AContactJid.pBare());

  lblAccountJid->setText(AStreamJid.full());
  lblFromJid->setText(AContactJid.bare());
  lblDateTime->setText(ATime.toString(Qt::LocaleDate));

  bool subsTo = (ASubs == "to" || ASubs == "both");
  bool subsFrom = (ASubs == "from" || ASubs == "both");
  QSet<ButtonId> showButtons;

  switch(ASubsType) 
  {
  case IRoster::Subscribe:
    {
      tedMessage->setText(tr("<b>Subscription request</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>%1 wants to subscribe to your presence.</b>").arg(AContactJid.bare()));
      
      showButtons << AuthButton << RejectButton;
      if (!subsTo)
        showButtons << AskButton;
      
      break;
    }
  case IRoster::Subscribed:
    {
      tedMessage->setText(tr("<b>Subscription approved</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>You are now authorized for %1 presence.</b>").arg(AContactJid.bare()));

      showButtons << RefuseButton;
      if (!subsFrom)
        showButtons << AuthButton;

      break;
    }
  case IRoster::Unsubscribe:
    {
      tedMessage->setText(tr("<b>Subscription refused</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>%1 unsubscribed from your presence.</b>").arg(AContactJid.bare()));

      if (subsTo)
        showButtons << RefuseButton;

      break;
    }
  case IRoster::Unsubscribed:
    {
      tedMessage->setText(tr("<b>Subscription rejected</b>"));
      if (!AStatus.isEmpty())
        tedMessage->append(AStatus+"<br>");
      tedMessage->append(tr("<b>You are now unsubscribed from %1 presence.</b>").arg(AContactJid.bare()));

      showButtons << AskButton;
      if (subsFrom)
        showButtons << RejectButton;

      break;
    }
  }

  tedMessage->append("<br>");
  if (showButtons.contains(AskButton))
  {
    pbtSubscribe->setEnabled(true);
    tedMessage->append(tr("Click <b>Ask</b> to ask for contact presence subscription."));
  }
  else
    pbtSubscribe->setEnabled(false);

  if (showButtons.contains(AuthButton))
  {
    pbtSubscribed->setEnabled(true);
    tedMessage->append(tr("Click <b>Auth</b> to authorize the subscription."));
  }
  else
    pbtSubscribed->setEnabled(false);

  if (showButtons.contains(RefuseButton))
  {
    pbtUnsubscribe->setEnabled(true);
    tedMessage->append(tr("Click <b>Refuse</b> to refuse from presence subscription."));
  }
  else
    pbtUnsubscribe->setEnabled(false);

  if (showButtons.contains(RejectButton))
  {
    pbtUnsubscribed->setEnabled(true);
    tedMessage->append(tr("Click <b>Reject</b> to reject the presence subscription."));
  }
  else
    pbtUnsubscribed->setEnabled(false);

  emit dialogReady();
}

void SubscriptionDialog::setNextCount(int ANextCount)
{
  FNextCount = ANextCount;
  if (FNextCount>0)
  {
    pbtNext->setText(tr("Next - %1").arg(FNextCount));
    pbtNext->setEnabled(true);
  }
  else
  {
    pbtNext->setText(tr("Next"));
    pbtNext->setEnabled(false);
  }
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
  case CloseButton:
    {
      reject();
      doNext = false;
      doAccept = false;
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