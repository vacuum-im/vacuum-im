#include "adiummessagestyleplugin.h"

AdiumMessageStylePlugin::AdiumMessageStylePlugin()
{

}

AdiumMessageStylePlugin::~AdiumMessageStylePlugin()
{

}

void AdiumMessageStylePlugin::pluginInfo( IPluginInfo *APluginInfo )
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Implements support for Adium message styles");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Adiun Message Style"); 
  APluginInfo->uid = ADIUMMESSAGESTYLE_UUID;
  APluginInfo->version = "0.1";
}

bool AdiumMessageStylePlugin::initConnections( IPluginManager *APluginManager, int &AInitOrder )
{
  return true;
}

Q_EXPORT_PLUGIN2(AdiumMessageStylePlugin, AdiumMessageStylePlugin)
