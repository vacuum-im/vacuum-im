#ifndef SIMPLEMESSAGESTYLEPLUGIN_H
#define SIMPLEMESSAGESTYLEPLUGIN_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagestyles.h"

#define SIMPLEMESSAGESTYLE_UUID   "{cfad7d10-58d0-4638-9940-dda64c1dd509}"

class SimpleMessageStylePlugin : 
  public QObject,
  public IPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin);
public:
  SimpleMessageStylePlugin();
  ~SimpleMessageStylePlugin();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return SIMPLEMESSAGESTYLE_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
};

#endif // SIMPLEMESSAGESTYLEPLUGIN_H
