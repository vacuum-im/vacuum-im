#ifndef CONSOLEPLUGIN_H
#define CONSOLEPLUGIN_H

#include <QObjectCleanupHandler>
#include <definitions/actiongroups.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/imainwindow.h>
#include <utils/iconstorage.h>
#include <utils/action.h>
#include "consolewidget.h"

#define CONSOLE_UUID  "{2572D474-5F3E-8d24-B10A-BAA57C2BC693}"

class ConsolePlugin :
			public QObject,
			public IPlugin
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin);
public:
	ConsolePlugin();
	~ConsolePlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return CONSOLE_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
protected slots:
	void onShowXMLConsole(bool);
private:
	IPluginManager *FPluginManager;
	IMainWindowPlugin *FMainWindowPlugin;
private:
	QObjectCleanupHandler FCleanupHandler;
};

#endif // CONSOLEPLUGIN_H
