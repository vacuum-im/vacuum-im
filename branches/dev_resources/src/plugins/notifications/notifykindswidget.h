#ifndef NOTIFYKINDSWIDGET_H
#define NOTIFYKINDSWIDGET_H

#include <QWidget>
#include "../../interfaces/inotifications.h"
#include "ui_notifykindswidget.h"

class NotifyKindsWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  NotifyKindsWidget(INotifications *ANotifications, const QString &AId, const QString &ATitle, uchar AKindMask, QWidget *AParent = NULL);
  ~NotifyKindsWidget();
  void applyOptions();
protected:
  void updateWidget();
private:
  Ui::NotifyKindsWidgetClass ui;
private:
  INotifications *FNotifications;
private:
  QString FNotificatorId;
};

#endif // NOTIFYKINDSWIDGET_H
