#ifndef ROSTERPLUGIN_H
#define ROSTERPLUGIN_H

#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ipresence.h"
#include "../../interfaces/isettings.h"
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
  virtual IRoster *addRoster(IXmppStream *AXmppStream);
  virtual IRoster *getRoster(const Jid &AStreamJid) const;
  virtual void loadRosterItems(const Jid &AStreamJid);
  virtual void removeRoster(IXmppStream *AXmppStream);
signals:
  virtual void rosterAdded(IRoster *ARoster);
  virtual void rosterOpened(IRoster *ARoster);
  virtual void rosterItemPush(IRoster *ARoster, IRosterItem *ARosterItem);
  virtual void rosterItemRemoved(IRoster *ARoster, IRosterItem *ARosterItem);
  virtual void rosterSubscription(IRoster *ARoster, const Jid &AJid, IRoster::SubsType, const QString &AText);
  virtual void rosterRemoved(IRoster *ARoster);
  virtual void rosterClosed(IRoster *ARoster);
protected:
  QString rosterFile(const Jid &AStreamJid) const;
protected slots:
  void onRosterOpened();
  void onRosterItemPush(IRosterItem *ARosterItem);
  void onRosterItemRemoved(IRosterItem *ARosterItem);
  void onRosterSubscription(const Jid &AJid, IRoster::SubsType ASType, const QString &AStatus);
  void onRosterClosed();
  void onStreamAdded(IXmppStream *AStream);
  void onStreamRemoved(IXmppStream *AStream);
  void onRosterDestroyed(QObject *AObject);
private:
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
private:
  QList<Roster *> FRosters;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERPLUGIN_H
