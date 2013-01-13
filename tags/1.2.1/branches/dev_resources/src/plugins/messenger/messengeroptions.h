#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <QWidget>
#include <QShortcut>
#include "../../interfaces/imessenger.h"
#include "ui_messengeroptions.h"

class MessengerOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  MessengerOptions(IMessenger *AMessenger, QWidget *AParent = NULL);
  ~MessengerOptions();
public slots:
  void apply();
signals:
  void optionsApplied();
protected:
  void setChatFont(const QFont &AFont);
  void setMessageFont(const QFont &AFont);
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
  void onChangeChatFont();
  void onChangeMessageFont();
private:
  Ui::MessengerOptionsClass ui;
private:
  IMessenger *FMessenger;
private:
  QKeySequence FSendKey;
};

#endif // MESSENGEROPTIONS_H
