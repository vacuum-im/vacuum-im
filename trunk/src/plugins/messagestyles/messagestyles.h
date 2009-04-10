#ifndef MESSAGESTYLES_H
#define MESSAGESTYLES_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/imessagestyles.h"
#include "messagestyle.h"

class MessageStyles : 
  public QObject,
  public IPlugin,
  public IMessageStyles
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IMessageStyles);
public:
  MessageStyles();
  ~MessageStyles();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return MESSAGESTYLES_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin();
private:

};

#endif // MESSAGESTYLES_H
