#ifndef PRESENCEPLUGIN_H
#define PRESENCEPLUGIN_H

#include <QObjectCleanupHandler>
#include "interfaces/ipluginmanager.h"
#include "interfaces/ipresence.h"
#include "presence.h"

#define PRESENCE_UUID "{511A07C4-FFA4-43ce-93B0-8C50409AFC0E}"

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
  virtual QUuid getPluginUuid() const { return PRESENCE_UUID; }
  virtual void getPluginInfo(PluginInfo *APluginInfo);
  virtual bool initPlugin(IPluginManager *APluginManager);
  virtual bool startPlugin();

  //IPresencePlugin
  virtual IPresence *newPresence(IXmppStream *AStream);
  virtual IPresence *getPresence(const Jid &AStreamJid) const;
  virtual void removePresence(const Jid &AStreamJid);
signals:
  virtual void presenceAdded(IPresence *);
  virtual void presenceOpened(IPresence *);
  virtual void selfPresence(IPresence *, IPresence::Show, const QString &, qint8, const Jid &);
  virtual void presenceItem(IPresence *, IPresenceItem *);
  virtual void presenceClosed(IPresence *);
  virtual void presenceRemoved(IPresence *);
protected slots:
  void onStreamAdded(IXmppStream *AStream);
  void onStreamRemoved(IXmppStream *AStream);
  void onPresenceOpened();
  void onSelfPresence(IPresence::Show AShow, const QString &AStatus, qint8 APriority, const Jid &AToJid);
  void onPresenceItem(IPresenceItem *APresenceItem);
  void onPresenceClosed();
  void onPresenceDestroyed(QObject *APresence);
private:
  IStanzaProcessor *FStanzaProcessor;
private:
  QList<Presence *> FPresences;
  QObjectCleanupHandler FCleanupHandler;
};

#endif // PRESENCEPLUGIN_H
