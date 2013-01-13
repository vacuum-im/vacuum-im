#ifndef SUBSCRIPTIONOPTIONS_H
#define SUBSCRIPTIONOPTIONS_H

#include <QWidget>
#include "../../interfaces/irosterchanger.h"
#include "ui_subscriptionoptions.h"

class SubscriptionOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  SubscriptionOptions(IRosterChanger *ARosterChanger, QWidget *AParent = NULL);
  ~SubscriptionOptions();
public slots:
  void apply();
signals:
  void applied();
private:
  Ui::SubscriptionOptionsClass ui;
private:
  IRosterChanger *FRosterChanger;
};

#endif // SUBSCRIPTIONOPTIONS_H
