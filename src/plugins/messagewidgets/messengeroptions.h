#ifndef MESSENGEROPTIONS_H
#define MESSENGEROPTIONS_H

#include <QShortcut>
#include "../../interfaces/imessagewidgets.h"
#include "ui_messengeroptions.h"

class MessengerOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  MessengerOptions(IMessageWidgets *AMessageWidgets, QWidget *AParent = NULL);
  ~MessengerOptions();
public slots:
  void apply();
signals:
  void optionsApplied();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
  Ui::MessengerOptionsClass ui;
private:
  IMessageWidgets *FMessageWidgets;
private:
  QKeySequence FSendKey;
};

#endif // MESSENGEROPTIONS_H
