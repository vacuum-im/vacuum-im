#ifndef PRESENCEPLUGIN_H
#define PRESENCEPLUGIN_H

#include <QSet>
#include <QObjectCleanupHandler>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ipresence.h"
#include "presence.h"

class PresencePlugin : 
  public QObject,
  public IPlugin,
  public IPresencePlugin
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IPresencePlugin);
public:
  PresencePlugin();
  ~PresencePlugin();
  virtual QObject* instance() { return this; }
  //IPlugin
  virtual QUuid pluginUuid() const { return PRESENCE_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }
  //IPresencePlugin
  virtual IPresence *addPresence(IXmppStream *AXmppStream);
  virtual IPresence *getPresence(const Jid &AStreamJid) const;
  virtual bool isContactOnline(const Jid &AContactJid) const { return FContactPresences.contains(AContactJid); }
  virtual QList<Jid> contactsOnline() const { return FContactPresences.keys(); }
  virtual QList<IPresence *> contactPresences(const Jid &AContactJid) const { return FContactPresences.value(AContactJid).toList(); }
  virtual void removePresence(IXmppStream *AXmppStream);
signals:
  virtual void streamStateChanged(const Jid &AStreamJid, bool AStateOnline);
  virtual void contactStateChanged(const Jid &AStreamJid, const Jid &AContactJid, bool AStateOnline);
  virtual void presenceAdded(IPresence *APresence);
  virtual void presenceOpened(IPresence *APresence);
  virtual void presenceChanged(IPresence *APresence, int AShow, const QString &AStatus, int APriotity);
  virtual void presenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem);
  virtual void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
  virtual void presenceClosed(IPresence *APresence);
  virtual void presenceRemoved(IPresence *APresence);
protected slots:
  void onPresenceOpened();
  void onPresenceChanged(int AShow, const QString &AStatus, int APriority);
  void onPresenceReceived(const IPresenceItem &APresenceItem);
  void onPresenceAboutToClose(int AShow, const QString &AStatus);
  void onPresenceClosed();
  void onPresenceDestroyed(QObject *AObject);
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  QList<Presence *> FPresences;
  QObjectCleanupHandler FCleanupHandler;
  QHash<Jid, QSet<IPresence *> > FContactPresences;
};

#endif // PRESENCEPLUGIN_H
