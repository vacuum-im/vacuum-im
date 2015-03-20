#ifndef AUTOSTATUSOPTIONSWIDGET_H
#define AUTOSTATUSOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/ipresence.h>
#include <interfaces/iautostatus.h>
#include <interfaces/istatuschanger.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_autostatusoptionswidget.h"

class AutoStatusOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	AutoStatusOptionsWidget(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent);
	virtual QWidget* instance() { return this; }
public slots:
	virtual void apply();
	virtual void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onHelpButtonClicked();
	void onShowRulesLinkActivayed();
private:
	Ui::AutoStatusOptionsWidgetClass ui;
private:
	IAutoStatus *FAutoStatus;
	IStatusChanger *FStatusChanger;
};

#endif // AUTOSTATUSOPTIONSWIDGET_H
