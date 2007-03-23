#ifndef ROSTERSMODELPLUGIN_H
#define ROSTERSMODELPLUGIN_H

#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "rostersmodel.h"

#define ROSTERSMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

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
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IRostersModelPlugin
  virtual IRostersModel *rostersModel();
  virtual bool addStreamRoster(IRoster *ARoster, IPresence *APresence);
  virtual bool removeStreamRoster(const Jid &AStreamJid);
signals:
  virtual void streamRosterAdded(const Jid &);
  virtual void streamRosterRemoved(const Jid &);
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
