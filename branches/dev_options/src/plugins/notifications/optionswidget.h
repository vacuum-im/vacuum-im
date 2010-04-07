#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <interfaces/inotifications.h>
#include "notifykindswidget.h"
#include "ui_optionswidget.h"

class OptionsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  OptionsWidget(INotifications *ANotifications, QWidget *AParent = NULL);
  ~OptionsWidget();
  void appendKindsWidget(NotifyKindsWidget *AWidgets);
public slots:
  void apply();
signals:
  void optionsAccepted();
private:
  Ui::OptionsWidgetClass ui;
private:
  INotifications *FNotifications;
private:
  QList<NotifyKindsWidget *> FKindsWidgets;
};

#endif // OPTIONSWIDGET_H
