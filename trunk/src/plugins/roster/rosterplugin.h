#ifndef ROSTERPLUGIN_H
#define ROSTERPLUGIN_H

#include <QObjectCleanupHandler>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ipresence.h>
#include <interfaces/isettings.h>
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
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IRosterPlugin
  virtual IRoster *addRoster(IXmppStream *AXmppStream);
  virtual IRoster *getRoster(const Jid &AStreamJid) const;
  virtual QString rosterFileName(const Jid &AStreamJid) const;
  virtual void removeRoster(IXmppStream *AXmppStream);
signals:
  virtual void rosterAdded(IRoster *ARoster);
  virtual void rosterOpened(IRoster *ARoster);
  virtual void rosterItemReceived(IRoster *ARoster, const IRosterItem &ARosterItem);
  virtual void rosterItemRemoved(IRoster *ARoster, const IRosterItem &ARosterItem);
  virtual void rosterSubscription(IRoster *ARoster, const Jid &AItemJid, int ASubsType, const QString &AText);
  virtual void rosterClosed(IRoster *ARoster);
  virtual void rosterStreamJidAboutToBeChanged(IRoster *ARoster, const Jid &AAfter);
  virtual void rosterStreamJidChanged(IRoster *ARoster, const Jid &ABefore);
  virtual void rosterRemoved(IRoster *ARoster);
protected slots:
  void onRosterOpened();
  void onRosterItemReceived(const IRosterItem &ARosterItem);
  void onRosterItemRemoved(const IRosterItem &ARosterItem);
  void onRosterSubscription(const Jid &AItemJid, int ASubsType, const QString &AText);
  void onRosterClosed();
  void onRosterStreamJidAboutToBeChanged(const Jid &AAfter);
  void onRosterStreamJidChanged(const Jid &ABefour);
  void onRosterDestroyed(QObject *AObject);
  void onStreamAdded(IXmppStream *AStream);
  void onStreamRemoved(IXmppStream *AStream);
private:
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
private:
  QList<IRoster *> FRosters;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // ROSTERPLUGIN_H
