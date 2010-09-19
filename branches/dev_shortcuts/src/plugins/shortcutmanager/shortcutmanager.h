#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <definitions/menuicons.h>
#include <definitions/optionnodes.h>
#include <definitions/optionvalues.h>
#include <definitions/optionnodeorders.h>
#include <definitions/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include "shortcutoptionswidget.h"

#define SHORTCUTMANAGER_UUID "{3F6D20F1-401D-4832-92C3-DB6687891EFD}"

class ShortcutManager : 
	public QObject,
	public IPlugin,
	public IOptionsHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsHolder);
public:
	ShortcutManager();
	~ShortcutManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SHORTCUTMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects() { return true; }
	virtual bool initSettings();
	virtual bool startPlugin() { return true; }
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
private:
	IOptionsManager *FOptionsManager;
};

#endif // SHORTCUTMANAGER_H
