#include "consoleplugin.h"

ConsolePlugin::ConsolePlugin()
{
  FPluginManager = NULL;
  FMainWindowPlugin = NULL;
}

ConsolePlugin::~ConsolePlugin()
{

}

void ConsolePlugin::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("XML Console");
  APluginInfo->description = tr("Allow to view XMPP stanzas.");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool ConsolePlugin::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  return true;
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

void ConsolePlugin::onShowXMLConsole(bool)
{
  ConsoleWidget *widget = new ConsoleWidget(FPluginManager,NULL);
  widget->show();
}

Q_EXPORT_PLUGIN2(plg_console, ConsolePlugin)
