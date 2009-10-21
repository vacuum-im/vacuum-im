#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <interfaces/inotifications.h>
#include "ui_optionswidget.h"

class OptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  OptionsWidget(INotifications *ANotifications, QWidget *AParent = NULL);
  ~OptionsWidget();
  void applyOptions();
  void updateWidget();
private:
  Ui::OptionsWidgetClass ui;
private:
  INotifications *FNotifications;
};

#endif // OPTIONSWIDGET_H
