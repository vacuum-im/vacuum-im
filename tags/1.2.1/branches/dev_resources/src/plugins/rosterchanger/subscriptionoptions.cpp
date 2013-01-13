#include "subscriptionoptions.h"

SubscriptionOptions::SubscriptionOptions(IRosterChanger *ARosterChanger, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  FRosterChanger = ARosterChanger;

  ui.chbAutoSubscribe->setChecked(FRosterChanger->checkOption(IRosterChanger::AutoSubscribe));
  ui.chbAutoUnsubscribe->setChecked(FRosterChanger->checkOption(IRosterChanger::AutoUnsubscribe));
}

SubscriptionOptions::~SubscriptionOptions()
{

}

void SubscriptionOptions::apply()
{
  FRosterChanger->setOption(IRosterChanger::AutoSubscribe,ui.chbAutoSubscribe->isChecked());
  FRosterChanger->setOption(IRosterChanger::AutoUnsubscribe,ui.chbAutoUnsubscribe->isChecked());
}