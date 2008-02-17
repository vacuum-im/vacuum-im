#ifndef ROSTERSMODELPLUGIN_H
#define ROSTERSMODELPLUGIN_H

#include "../../definations/accountvaluenames.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/iaccountmanager.h"
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
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IRostersModelPlugin
  virtual IRostersModel *rostersModel();
  virtual IRosterIndex *addStream(IRoster *ARoster, IPresence *APresence);
  virtual void removeStream(const Jid &AStreamJid);
signals:
  virtual void streamJidChanged(const Jid &ABefour, const Jid &AAfter);
protected slots:
  void onRosterAdded(IRoster *ARoster);
  void onRosterRemoved(IRoster *ARoster);
  void onPresenceAdded(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
  void onAccountChanged(const QString &AName, const QVariant &AValue);
  void onStreamJidChanged(const Jid &ABefour, const Jid &AAfter);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
  IAccountManager *FAccountManager;
private:
  RostersModel *FRostersModel;
};

#endif // ROSTERSMODELPLUGIN_H
