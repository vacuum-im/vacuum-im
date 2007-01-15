#ifndef ROSTERMODELPLUGIN_H
#define ROSTERMODELPLUGIN_H

#include <QObjectCleanupHandler>
#include "interfaces/ipluginmanager.h"
#include "interfaces/irostermodel.h"
#include "rostermodel.h"

#define ROSTERMODEL_UUID "{C1A1BBAB-06AF-41c8-BFBE-959F1065D80D}"

class RosterModelPlugin : 
  public QObject,
  public IPlugin,
  public IRosterModelPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRosterModelPlugin);

public:
  RosterModelPlugin();
  ~RosterModelPlugin();

  virtual QObject* instance() { return this; }

  //IPlugin
  virtual QUuid getPluginUuid() const { return ROSTERMODEL_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IRosterModelPlugin
  virtual IRosterModel *newRosterModel(IRoster *ARoster, IPresence *APresence);
  virtual IRosterModel *getRosterModel(const Jid &AStreamJid) const;
  virtual void removeRosterModel(const Jid &AStreamJid);
signals:
  virtual void rosterModelAdded(IRosterModel *);
  virtual void rosterModelRemoved(IRosterModel *);
protected slots:
  void onRosterAdded(IRoster *ARoster);
  void onRosterRemoved(IRoster *ARoster);
  void onPresenceAdded(IPresence *APresence);
  void onPresenceRemoved(IPresence *APresence);
  void onRosterModelDestroyed(QObject *ARosterModel);
private:
  IRosterPlugin *FRosterPlugin;
  IPresencePlugin *FPresencePlugin;
private:
  QList<RosterModel *> FRosterModels;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERMODELPLUGIN_H
