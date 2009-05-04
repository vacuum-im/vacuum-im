#ifndef ADIUMMESSAGESTYLEPLUGIN_H
#define ADIUMMESSAGESTYLEPLUGIN_H

#include "../../interfaces/ipluginmanager.h"

#define ADIUMMESSAGESTYLE_UUID    "{703bae73-1905-4840-a186-c70b359d4f21}"

class AdiumMessageStylePlugin : 
  public QObject,
  public IPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin);
public:
  AdiumMessageStylePlugin();
  ~AdiumMessageStylePlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return ADIUMMESSAGESTYLE_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
};

#endif // ADIUMMESSAGESTYLEPLUGIN_H
