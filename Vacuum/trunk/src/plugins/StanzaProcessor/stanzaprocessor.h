#ifndef STANZAPROCESSOR_H
#define STANZAPROCESSOR_H

#include <QDomDocument>
#include <QHash>
#include <QVector>
#include <QTimer>
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/ixmppstreams.h"

class StanzaProcessor :
  public QObject,
  public IPlugin,
  public IStanzaProcessor
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IStanzaProcessor);

public:
  StanzaProcessor();
  ~StanzaProcessor();

  virtual QObject* instance() { return this; }

  //IPlugin
  virtual QUuid pluginUuid() const { return STANZAPROCESSOR_UUID; }
  virtual void pluginInfo(PluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects() { return true; }
  virtual bool initSettings() { return true; }
  virtual bool startPlugin() { return true; }

  //IStanzaProcessor
  virtual QString newId() const;

  virtual bool sendStanzaIn(const Jid &AStreamJid, const Stanza &AStanza);
  virtual bool sendStanzaOut(const Jid &AStreamJid, const Stanza &AStanza);
  virtual bool sendIqStanza(IStanzaProcessorIqOwner *AIqOwner, const Jid &AStreamJid, 
    const Stanza &AStanza, qint32 ATimeOut);

  virtual PriorityId setPriority(PriorityLevel ALevel, qint32 AOffset, QObject *AOwner=0);
  virtual void removePriority(PriorityId APriorityId);

  virtual HandlerId setHandler(IStanzaProcessorHandler *, const QString &ACondition, 
    Direction ADirection, PriorityId APriorityId=0, const Jid &AStreamJid = Jid()); 
  virtual void addCondition(HandlerId AHandlerId, const QString &ACondition);
  virtual void removeHandler(HandlerId AHandlerId);
protected:
  struct IqStanzaItem {
    IStanzaProcessorIqOwner *owner;
    Jid streamJid;
    QObject *timer;
  };
  struct PriorityItem {
    QObject *owner;
    PriorityLevel level;
    qint32 offset;
    QStringList after;
    QStringList befour;
  };
  struct HandlerItem {
    IStanzaProcessorHandler *handler;
    QStringList conditions;
    Direction direction;
    PriorityId priority;
    Jid streamJid;
  };
  virtual bool checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos = 0);
  virtual bool processStanzaIn(const Jid &AStreamJid, Stanza *AStanza);
  virtual bool processStanzaOut(const Jid &AStreamJid, Stanza *AStanza);
  virtual bool processIqStanza(const Jid &AStreamJid, const Stanza &AStanza);
protected slots:
  void onAboutToQuit();
  void onStreamAdded(IXmppStream *AStream);
  void onStreamElement(IXmppStream *AStream, const QDomElement &AElem);
  void onStreamRemoved(IXmppStream *AStream);
  void onIqStanzaTimeOut();
  void onPriorityOwnerDestroyed(QObject *AOwner);
  void onHandlerOwnerDestroyed(QObject *AOwner);
  void onIqStanzaOwnerDestroyed(QObject *AOwner);
private:
  QHash<Jid, IXmppStream *> FStreams;
private:
  QHash<QString, IqStanzaItem> FIqStanzaItems;
  QVector<PriorityId> FPriorities;
  QHash<PriorityId, PriorityItem> FPriorityItems;
  QHash<PriorityId, QList<HandlerId>> FPriorityToHandlers;
  QHash<HandlerId, HandlerItem> FHandlerItems;
  QVector<HandlerId> FHandlerTurn;
};

#endif