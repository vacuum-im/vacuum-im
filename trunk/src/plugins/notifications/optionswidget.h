#ifndef OPTIONSWIDGET_H
#define OPTIONSWIDGET_H

#include <definations/optionvalues.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include "notifykindswidget.h"
#include "ui_optionswidget.h"

class OptionsWidget :
			public QWidget,
			public IOptionsWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsWidget);
public:
	OptionsWidget(INotifications *ANotifications, QWidget *AParent);
	~OptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
private:
	Ui::OptionsWidgetClass ui;
private:
	INotifications *FNotifications;
};

#endif // OPTIONSWIDGET_H
