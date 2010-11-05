#include "shortcutmanager.h"

ShortcutManager::ShortcutManager()
{
	FOptionsManager = NULL;
}

ShortcutManager::~ShortcutManager()
{

}

void ShortcutManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Shortcut Manager");
	APluginInfo->description = tr("Allows to setup user defined shortcuts");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(OPTIONSMANAGER_UUID);
}

bool ShortcutManager::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FOptionsManager!=NULL;
}

bool ShortcutManager::initSettings()
{
	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_SHORTCUTS, OPN_SHORTCUTS, tr("Shortcuts"), MNI_SHORTCUTS };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

QMultiMap<int, IOptionsWidget *> ShortcutManager::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsWidget *> widgets;
	if (ANodeId == OPN_SHORTCUTS)
	{
		widgets.insertMulti(OWO_SHORTCUTS, new ShortcutOptionsWidget(AParent));
	}
	return widgets;
}

void ShortcutManager::onOptionsOpened()
{
	OptionsNode options = Options::node(OPV_SHORTCUTS);
	foreach(QString shortcutId, Shortcuts::shortcuts())
	{
		if (options.hasNode(shortcutId))
		{
			Shortcuts::updateShortcut(shortcutId,options.value(shortcutId).toString());
		}
	}
}

void ShortcutManager::onOptionsClosed()
{
	OptionsNode options = Options::node(OPV_SHORTCUTS);
	foreach(QString shortcutId, Shortcuts::shortcuts())
	{
		ShortcutDescriptor descriptor = Shortcuts::shortcutDescriptor(shortcutId);
		if (descriptor.activeKey != descriptor.defaultKey)
			options.setValue(descriptor.activeKey.toString(),shortcutId);
		else
			options.removeNode(shortcutId);
	}
}

Q_EXPORT_PLUGIN2(plg_shortcutmanager, ShortcutManager)
