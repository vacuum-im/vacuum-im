#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <QWidget>
#include "../../interfaces/imessenger.h"
#include "ui_messengeroptions.h"

class MessengerOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  MessengerOptions(QWidget *AParent = NULL);
  ~MessengerOptions();
  bool checkOption(IMessenger::Option AOption) const;
  void setOption(IMessenger::Option AOption, bool AValue);
private:
  Ui::MessengerOptionsClass ui;
private:
  int FOptions;
};

#endif // MESSENGEROPTIONS_H
