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
  virtual void selfPresence(IPresence *APresence, int AShow, const QString &AStatus, qint8 APriotity, const Jid &AToJid);
  virtual void presenceItem(IPresence *APresence, IPresenceItem *APresenceItem);
  virtual void presenceAboutToClose(IPresence *APresence, int AShow, const QString &AStatus);
  virtual void presenceClosed(IPresence *APresence);
  virtual void presenceRemoved(IPresence *APresence);
protected slots:
  void onPresenceOpened();
  void onSelfPresence(int AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid);
  void onPresenceItem(IPresenceItem *APresenceItem);
  void onPresenceAboutToClose(int AShow, const QString &AStatus);
  void onPresenceClosed();
  void onStreamAdded(IXmppStream *AXmppStream);
  void onStreamRemoved(IXmppStream *AXmppStream);
  void onPresenceDestroyed(QObject *AObject);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  QList<Presence *> FPresences;
  QObjectCleanupHandler FCleanupHandler;
  QHash<Jid, QSet<IPresence *> > FContactPresences;
};

#endif // PRESENCEPLUGIN_H
