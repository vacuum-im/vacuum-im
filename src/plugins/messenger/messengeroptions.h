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
  QFont chatFont() const { return FChatFont; }
  void setChatFont(const QFont &AFont);
  QFont messageFont() const { return FMessageFont; }
  void setMessageFont(const QFont &AFont);
  bool checkOption(IMessenger::Option AOption) const;
  void setOption(IMessenger::Option AOption, bool AValue);
protected slots:
  void onChangeChatFont();
  void onChangeMessageFont();
private:
  Ui::MessengerOptionsClass ui;
private:
  int FOptions;
  QFont FChatFont;
  QFont FMessageFont;
};

#endif // MESSENGEROPTIONS_H
