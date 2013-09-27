#ifndef NOTIFYWIDGET_H
#define NOTIFYWIDGET_H

#include <QMouseEvent>
#include <QDesktopWidget>
#include <QNetworkAccessManager>
#include <definitions/optionvalues.h>
#include <definitions/notificationdataroles.h>
#include <interfaces/inotifications.h>
#include <interfaces/imainwindow.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/textmanager.h>
#include "ui_notifywidget.h"

class NotifyWidget :
	public QFrame
{
	Q_OBJECT;
public:
	NotifyWidget(const INotification &ANotification);
	~NotifyWidget();
	void appear();
	static void setMainWindow(IMainWindow *AMainWindow);
	static void setNetworkManager(QNetworkAccessManager *ANetworkManager);
signals:
	void notifyActivated();
	void notifyRemoved();
	void windowDestroyed();
protected:
	void animateTo(int AYPos);
	void enterEvent(QEvent *AEvent);
	void leaveEvent(QEvent *AEvent);
	void resizeEvent(QResizeEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
	void adjustHeight();
	void updateElidedText();
protected slots:
	void onAnimateStep();
	void onCloseTimerTimeout();
private:
	Ui::NotifyWidgetClass ui;
private:
	int FYPos;
	int FTimeOut;
	int FAnimateStep;
	QString FTitle;
	QString FNotice;
	QString FCaption;
	QTimer FCloseTimer;
private:
	static QRect FDisplay;
	static QDesktopWidget *FDesktop;
	static QList<NotifyWidget *> FWidgets;
	static void layoutWidgets();
private:
	static IMainWindow *FMainWindow;
	static QNetworkAccessManager *FNetworkManager;
};

#endif // NOTIFYWIDGET_H
