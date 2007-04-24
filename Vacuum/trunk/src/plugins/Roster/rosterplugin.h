#ifndef ROSTERPLUGIN_H
#define ROSTERPLUGIN_H

#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ipresence.h"
#include "roster.h"

class RosterPlugin : 
  public QObject,
  public IPlugin,
  public IRosterPlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IRosterPlugin);

public:
  RosterPlugin();
  ~RosterPlugin();

  virtual QObject* instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return ROSTER_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IRosterPlugin
  virtual IRoster *newRoster(IXmppStream *AStream);
  virtual IRoster *getRoster(const Jid &AStreamJid) const;
  virtual void removeRoster(const Jid &AStreamJid);
signals:
  virtual void rosterAdded(IRoster *);
  virtual void rosterOpened(IRoster *);
  virtual void rosterItemPush(IRoster *, IRosterItem *);
  virtual void rosterItemRemoved(IRoster *, IRosterItem *);
  virtual void rosterClosed(IRoster *);
  virtual void rosterRemoved(IRoster *);
protected slots:
  void onStreamAdded(IXmppStream *AStream);
  void onStreamRemoved(IXmppStream *AStream);
  void onRosterOpened();
  void onRosterClosed();
  void onRosterItemPush(IRosterItem *ARosterItem);
  void onRosterItemRemoved(IRosterItem *ARosterItem);
  void onRosterDestroyed(QObject *ARoster);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  QList<Roster *> FRosters;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERPLUGIN_H
