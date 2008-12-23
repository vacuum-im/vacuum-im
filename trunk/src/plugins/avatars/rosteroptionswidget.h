#ifndef ROSTEROPTIONSWIDGET_H
#define ROSTEROPTIONSWIDGET_H

#include "../../interfaces/iavatars.h"
#include "ui_rosteroptionswidget.h"

class RosterOptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  RosterOptionsWidget(IAvatars *AAvatars, QWidget *AParent = NULL);
  ~RosterOptionsWidget();
public slots:
  void applyOptions();
private:
  Ui::RosterOptionsWidgetClass ui;
private:
  IAvatars *FAvatars;
};

#endif // ROSTEROPTIONSWIDGET_H
