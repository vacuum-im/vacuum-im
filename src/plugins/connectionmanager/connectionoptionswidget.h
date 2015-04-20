#ifndef CONNECTIONOPTIONSWIDGET_H
#define CONNECTIONOPTIONSWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <interfaces/iconnectionmanager.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/ioptionsmanager.h>
#include "connectionmanager.h"
#include "ui_connectionoptionswidget.h"

class ConnectionManager;

class ConnectionOptionsWidget :
	public QWidget,
	public IOptionsDialogWidget
{
	Q_OBJECT;
	Q_INTERFACES(IOptionsDialogWidget);
public:
	ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent);
	~ConnectionOptionsWidget();
	virtual QWidget* instance() { return this; }
public slots:
	void apply();
	void reset();
signals:
	void modified();
	void childApply();
	void childReset();
protected:
	void setEngineById(const QString &AEngineId);
protected slots:
	void onComboConnectionsChanged(int AIndex);
private:
	IConnectionManager *FManager;
private:
	Ui::ConnectionOptionsWidgetClass ui;
private:
	QString FEngineId;
	OptionsNode FOptions;
	QVBoxLayout *FEngineLayout;
	IOptionsDialogWidget *FEngineSettings;
};

#endif // CONNECTIONOPTIONSWIDGET_H
