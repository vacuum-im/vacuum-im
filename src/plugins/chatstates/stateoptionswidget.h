#ifndef STATEOPTIONSWIDGET_H
#define STATEOPTIONSWIDGET_H

#include "../../interfaces/ichatstates.h"
#include "ui_stateoptionswidget.h"

class StateOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  StateOptionsWidget(IChatStates *AChatStates, QWidget *AParent = NULL);
  ~StateOptionsWidget();
public slots:
  void apply();
signals:
  void optionsApplied();
private:
  Ui::StateOptionsWidgetClass ui;
private:
  IChatStates *FChatStates;
};

#endif // STATEOPTIONSWIDGET_H
