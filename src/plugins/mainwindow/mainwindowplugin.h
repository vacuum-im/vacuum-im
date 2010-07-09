#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <QTime>
#include <definations/actiongroups.h>
#include <definations/version.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/optionvalues.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/itraymanager.h>
#include <utils/widgetmanager.h>
#include <utils/action.h>
#include <utils/options.h>
#include "mainwindow.h"

class MainWindowPlugin :
		public QObject,
		public IPlugin,
		public IMainWindowPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IMainWindowPlugin);
public:
	MainWindowPlugin();
	~MainWindowPlugin();
	virtual QObject* instance() { return this; }
	//IPlugin
	virtual QUuid pluginUuid() const { return MAINWINDOW_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IMainWindowPlugin
	virtual IMainWindow *mainWindow() const;
protected:
	void updateTitle();
	void showMainWindow();
	void correctWindowPosition();
protected:
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onProfileRenamed(const QString &AProfile, const QString &ANewName);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
	void onShowMainWindowByAction(bool);
private:
	IPluginManager *FPluginManager;
	IOptionsManager *FOptionsManager;
	ITrayManager *FTrayManager;
private:
	MainWindow *FMainWindow;
private:
	QTime FActivationChanged;
};

#endif // MAINWINDOW_H
