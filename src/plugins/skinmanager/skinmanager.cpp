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
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing skins");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Skin Manager"); 
  APluginInfo->uid = SKINMANAGER_UUID;
  APluginInfo ->version = "0.1";
}

bool SkinManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
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

Q_EXPORT_PLUGIN2(SkinManagerPlugin, SkinManager)
