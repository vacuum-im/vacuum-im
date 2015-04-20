#ifndef NOTIFYKINDOPTIONSWIDGET_H
#define NOTIFYKINDOPTIONSWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>

class NotifyKindOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	NotifyKindOptionsWidget(INotifications *ANotifications, QWidget *AParent);
	virtual QWidget *instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	virtual void showEvent(QShowEvent *AEvent);
private:
	INotifications *FNotifications;
private:
	QTableWidget *tbwNotifies;
};

#endif // NOTIFYKINDOPTIONSWIDGET_H
