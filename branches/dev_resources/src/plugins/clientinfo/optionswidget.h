#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include "../../interfaces/iclientinfo.h"
#include "ui_optionswidget.h"

class OptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  OptionsWidget(IClientInfo *AClientInfo);
  ~OptionsWidget();
public:
  void apply();
private:
  Ui::optionswidgetClass ui;
private:
  IClientInfo *FClientInfo;
};

#endif // OPTIONSWIDGET_H
