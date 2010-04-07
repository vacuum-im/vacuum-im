#include "skinmanager.h"

SkinManager::SkinManager()
{
  FSettingsPlugin = NULL;
}

SkinManager::~SkinManager()
{

}

void SkinManager::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Skin Manager"); 
  APluginInfo->description = tr("Allows to change the look and feel of the application");
  APluginInfo ->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool SkinManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      connect(FSettingsPlugin->instance(),SIGNAL(settingsOpened()),SLOT(onSettingsOpened()));
      connect(FSettingsPlugin->instance(),SIGNAL(settingsClosed()),SLOT(onSettingsClosed()));
    }
  }

  return true;
}

bool SkinManager::initObjects()
{
  return true;
}

bool SkinManager::startPlugin()
{
  return true;
}


void SkinManager::onSettingsOpened()
{

}

void SkinManager::onSettingsClosed()
{

}

Q_EXPORT_PLUGIN2(plg_skinmanager, SkinManager)
