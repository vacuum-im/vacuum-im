#include "pluginhelper.h"

IPluginManager *PluginHelper::FPluginManager = NULL;

PluginHelper::PluginHelper()
{

}

IPluginManager *PluginHelper::pluginManager()
{
	return FPluginManager;
}

void PluginHelper::setPluginManager(IPluginManager *AManager)
{
	FPluginManager = AManager;
}
