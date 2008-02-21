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
  virtual void saveRosterItems(const Jid &AStreamJid);
  virtual void loadRosterItems(const Jid &AStreamJid);
  virtual void removeRoster(IXmppStream *AXmppStream);
signals:
  virtual void rosterAdded(IRoster *ARoster);
  virtual void rosterJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  virtual void rosterOpened(IRoster *ARoster);
  virtual void rosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem);
  virtual void rosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem);
  virtual void rosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
  virtual void rosterClosed(IRoster *ARoster);
  virtual void rosterRemoved(IRoster *ARoster);
protected:
  QString rosterFileName(const Jid &AStreamJid) const;
protected slots:
  void onRosterJidAboutToBeChanged(const Jid &AAfter);
  void onRosterOpened();
  void onRosterItemReceived(const IRosterItem &ARosterItem);
  void onRosterItemRemoved(const IRosterItem &ARosterItem);
  void onRosterSubscription(const Jid &AItemJid, int ASubsType, const QString &AText);
  void onRosterClosed();
  void onRosterDestroyed(QObject *AObject);
  void onStreamAdded(IXmppStream *AStream);
  void onStreamRemoved(IXmppStream *AStream);
private:
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
private:
  QList<Roster *> FRosters;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERPLUGIN_H
