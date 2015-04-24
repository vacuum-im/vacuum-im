#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QPointer>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/itraymanager.h>
#include <interfaces/inotifications.h>
#include "shortcutoptionswidget.h"

#define SHORTCUTMANAGER_UUID "{3F6D20F1-401D-4832-92C3-DB6687891EFD}"

class ShortcutManager : 
	public QObject,
	public IPlugin,
	public IOptionsDialogHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IOptionsDialogHolder);
	Q_PLUGIN_METADATA(IID "org.vacuum-im.plugins.ShortcutManager");
public:
	ShortcutManager();
	~ShortcutManager();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return SHORTCUTMANAGER_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings();
	virtual bool startPlugin();
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsDialogWidget *> optionsDialogWidgets(const QString &ANodeId, QWidget *AParent);
protected:
	void hideAllWidgets();
	void showHiddenWidgets(bool ACheckPassword = true);
protected slots:
	void onOptionsOpened();
	void onOptionsClosed();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	ITrayManager *FTrayManager;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
private:
	bool FAllHidden;
	bool FTrayHidden;
	ushort FNotifyHidden;
	QList< QPointer<QWidget> > FHiddenWidgets;
};

#endif // SHORTCUTMANAGER_H
