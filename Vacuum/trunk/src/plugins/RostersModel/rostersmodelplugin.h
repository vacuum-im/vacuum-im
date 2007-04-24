#ifndef ROSTERSMODELPLUGIN_H
#define ROSTERSMODELPLUGIN_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "rostersmodel.h"

class RostersModelPlugin : 
  public QObject,
  public IPlugin,
  public IRostersModelPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRostersModelPlugin);

public:
  RostersModelPlugin();
  ~RostersModelPlugin();

  virtual QObject *instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTERSMODEL_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IRostersModelPlugin
  virtual IRostersModel *rostersModel();
  virtual IRosterIndex *addStream(IRoster *ARoster, IPresence *APresence);
  virtual void removeStream(const Jid &AStreamJid);
signals:
  virtual void modelCreated(IRostersModel *);
  virtual void modelDestroyed(IRostersModel *);
protected:
  void createRostersModel();
protected slots:
  void onRosterAdded(IRoster *ARoster);
  void onRosterRemoved(IRoster *ARoster);
  void onPresenceAdded(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
private:
  RostersModel *FRostersModel;
};

#endif // ROSTERSMODELPLUGIN_H
