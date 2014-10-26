#ifndef MAINWINDOWPLUGIN_H
#define MAINWINDOWPLUGIN_H

#include <QTime>
#include <interfaces/ipluginmanager.h>
#include <interfaces/imainwindow.h>
#include <interfaces/itraymanager.h>
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
	bool eventFilter(QObject *AWatched, QEvent *AEvent);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onApplicationShutdownStarted();
	void onShowMainWindowOnStart();
	void onShowMainWindowByAction(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason);
private:
	IPluginManager *FPluginManager;
	ITrayManager *FTrayManager;
private:
	int FStartShowLoopCount;
	MainWindow *FMainWindow;
	QTime FActivationChanged;
};

#endif // MAINWINDOW_H
