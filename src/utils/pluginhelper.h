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
	static inline IPluginManager *pluginManager() {
		return FPluginManager;
	}
	static inline void setPluginManager(IPluginManager *AManager) {
		FPluginManager = AManager;
	}
private:
	PluginHelper() {}
	static IPluginManager *FPluginManager;
};

template <class I>
class PluginPointer
{
public:
	inline PluginPointer() {
		FInstance = NULL;
		FHasInstance = false;
	}
	inline I* operator->() const {
		return getInstance();
	}
	inline operator I*() const {
		return getInstance();
	}
	inline bool isNull() const {
		return getInstance() == NULL;
	}
	inline operator bool() const {
		return getInstance() != NULL;
	}
	inline bool operator!() const {
		return getInstance() == NULL;
	}
	inline PluginPointer<I> &operator=(I *APointer) {
		setInstance(APointer);
		return *this;
	}
protected:
	inline I *getInstance() const {
		if (!FHasInstance && PluginHelper::pluginManager()!=NULL)
			setInstance(PluginHelper::pluginInstance<I>());
		return FInstance;
	}
	inline void setInstance(I *APointer) const {
		FHasInstance = true;
		FInstance = APointer;
	}
private:
	mutable I *FInstance;
	mutable bool FHasInstance;
};

template <class I> inline bool operator==(const PluginPointer<I> &p1, const I *p2) { return p1.operator->() == p2; }
template <class I> inline bool operator!=(const PluginPointer<I> &p1, const I *p2) { return p1.operator->() != p2; }
template <class I> inline bool operator==(const I *p1, const PluginPointer<I> &p2) { return p1 == p2.operator->(); }
template <class I> inline bool operator!=(const I *p1, const PluginPointer<I> &p2) { return p1 != p2.operator->(); }
template <class I> inline bool operator==(const PluginPointer<I> &p1, const PluginPointer<I> &p2) { return p1.operator->() == p2->operator->(); }
template <class I> inline bool operator!=(const PluginPointer<I> &p1, const PluginPointer<I> &p2) { return p1.operator->() != p2->operator->(); }

#endif // PLUGINHELPER_H
