#include "consoleplugin.h"

#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <utils/widgetmanager.h>
#include <utils/iconstorage.h>
#include <utils/options.h>
#include <utils/action.h>
#include <utils/logger.h>

ConsolePlugin::ConsolePlugin()
{
	FMainWindowPlugin = NULL;
	FXmppStreamManager = NULL;
}

ConsolePlugin::~ConsolePlugin()
{
	FCleanupHandler.clear();
}

void ConsolePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Console");
	APluginInfo->description = tr("Allows to view XML stream between the client and server");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(XMPPSTREAMS_UUID);
	APluginInfo->dependences.append(MAINWINDOW_UUID);
}

bool ConsolePlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	IPlugin *plugin = APluginManager->pluginInterface("IXmppStreamManager").value(0,NULL);
	if (plugin)
	{
		FXmppStreamManager = qobject_cast<IXmppStreamManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	return FXmppStreamManager!=NULL && FMainWindowPlugin!=NULL;
}

bool ConsolePlugin::initObjects()
{
	if (FMainWindowPlugin)
	{
		Action *action = new Action(FMainWindowPlugin->mainWindow()->mainMenu());
		action->setText(tr("XML Console"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_CONSOLE);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowXMLConsole(bool)));
		FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action,AG_MMENU_CONSOLE,true);
	}
	return true;
}

bool ConsolePlugin::initSettings()
{
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_NAME,tr("Default Context"));
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_WORDWRAP,false);
	Options::setDefaultValue(OPV_CONSOLE_CONTEXT_HIGHLIGHTXML,Qt::Checked);
	return true;
}

void ConsolePlugin::onShowXMLConsole(bool)
{
	ConsoleWidget *widget = new ConsoleWidget();
	WidgetManager::setWindowSticky(widget,true);
	FCleanupHandler.add(widget);
	widget->show();
}
