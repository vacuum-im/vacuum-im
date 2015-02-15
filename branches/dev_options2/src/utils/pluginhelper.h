#ifndef PLUGINHELPER_H
#define PLUGINHELPER_H

#include <interfaces/ipluginmanager.h>
#include "utilsexport.h"

class UTILS_EXPORT PluginHelper
{
public:
	template <class I> static inline I *pluginInstance()
	{
		IPlugin *plugin = FPluginManager!=NULL ? FPluginManager->pluginInterface(qobject_interface_iid<I *>()).value(0) : NULL;
		return plugin!=NULL ? qobject_cast<I *>(plugin->instance()) : NULL;
	}
	template <class I> static inline QList<I *> pluginInstances()
	{
		QList<I *> instances;
		QList<IPlugin *> plugins = FPluginManager!=NULL ? FPluginManager->pluginInterface(qobject_interface_iid<I *>()) : QList<IPlugin *>();
		foreach(IPlugin *plugin, plugins)
		{
			I *instance = qobject_cast<I *>(plugin->instance());
			if (instance)
				instances.append(instance);
		}
		return instances;
	}
public:
	static IPluginManager *pluginManager();
	static void setPluginManager(IPluginManager *AManager);
private:
	PluginHelper();
	static IPluginManager *FPluginManager;
};

#endif // PLUGINHELPER_H
