#ifndef NOTIFYKINDSWIDGET_H
#define NOTIFYKINDSWIDGET_H

#include <QWidget>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_notifykindswidget.h"

class NotifyKindsWidget :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	NotifyKindsWidget(INotifications *ANotifications, const QString &AId, const QString &ATitle, uchar AKindMask, QWidget *AParent);
	~NotifyKindsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::NotifyKindsWidgetClass ui;
private:
	INotifications *FNotifications;
private:
	QString FNotificatorId;
};

#endif // NOTIFYKINDSWIDGET_H
