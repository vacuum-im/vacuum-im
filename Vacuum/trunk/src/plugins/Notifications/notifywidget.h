#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QTimer>
#include <QMouseEvent>
#include <QDesktopWidget>
#include "../../definations/notificationdataroles.h"
#include "../../interfaces/inotifications.h"
#include "ui_notifywidget.h"

class NotifyWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  NotifyWidget(const INotification &ANotification);
  ~NotifyWidget();
  void appear();
  void animateTo(int AXPos, int AYPos);
public slots:
  void animateStep();
  void disappear();
signals:
  void notifyActivated();
  void notifyRemoved();
protected:
  virtual void mouseReleaseEvent(QMouseEvent *AEvent);
private:
  Ui::NotifyWidgetClass ui;
private:
  int FTimeOut;
  int FXPos;
  int FYPos;
  int FAnimateStep;
  QTimer FAnimator;
private:
  static void layoutWidgets();
  static QDesktopWidget FDesktop;
  static QList<NotifyWidget *> FWidgets;
};

#endif // NOTIFYWIDGET_H
