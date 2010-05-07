#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QMouseEvent>
#include <QDesktopWidget>
#include <definations/notificationdataroles.h>
#include <interfaces/inotifications.h>
#include <utils/widgetmanager.h>
#include "ui_notifywidget.h"

class NotifyWidget :
			public QWidget
{
	Q_OBJECT;
public:
	NotifyWidget(const INotification &ANotification);
	~NotifyWidget();
	void appear();
	void animateTo(int AYPos);
signals:
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
protected:
	virtual void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
	void onAnimateStep();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
private:
	static void layoutWidgets();
	static QDesktopWidget *FDesktop;
	static QList<NotifyWidget *> FWidgets;
};

#endif // NOTIFYWIDGET_H
