#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include <interfaces/idefaultconnection.h>
#include <interfaces/ioptionsmanager.h>
#include "ui_connectionoptionswidget.h"

class ConnectionOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent = NULL);
	~ConnectionOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	void apply(OptionsNode ANode);
	void apply();
	void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected slots:
	void onUseLegacySSLStateChanged(int AState);
private:
	Ui::ConnectionOptionsWidgetClass ui;
private:
	IConnectionManager *FManager;
private:
	OptionsNode FOptions;
	IOptionsDialogWidget *FProxySettings;
};

#endif // CONNECTIONOPTIONSWIDGET_H
