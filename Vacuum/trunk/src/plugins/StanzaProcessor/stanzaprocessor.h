#ifndef STANZAPROCESSOR_H
#define STANZAPROCESSOR_H

#include <QDomDocument>
#include <QHash>
#include <QMultiMap>
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
  //IPlugin
  virtual QObject* instance() { return this; }
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
  virtual bool sendIqStanza(IIqStanzaOwner *AIqOwner, const Jid &AStreamJid, const Stanza &AStanza, int ATimeOut);
  virtual bool hasHandler(int AHandlerId) const { return FHandlerItems.contains(AHandlerId); }
  virtual int insertHandler(IStanzaHandler *AHandler, const QString &ACondition, 
    Direction ADirection, int APriority = SHP_DEFAULT, const Jid &AStreamJid = Jid()); 
  virtual Direction handlerDirection(int AHandlerId) const;
  virtual int handlerPriority(int AHandlerId) const;
  virtual Jid handlerStreamJid(int AHandlerId) const;
  virtual QStringList handlerConditions(int AHandlerId) const;
  virtual void appendCondition(int AHandlerId, const QString &ACondition);
  virtual void removeCondition(int AHandlerId, const QString &ACondition);
  virtual void removeHandler(int AHandlerId);
signals:
  virtual void handlerInserted(int AHandlerId, IStanzaHandler *AHandler);
  virtual void handlerRemoved(int AHandlerId);
protected:
  struct IqStanzaItem {
    QString stanzaId;
    IIqStanzaOwner *owner;
    Jid streamJid;
    QTimer *timer;
  };
  struct HandlerItem {
    int handlerId;
    IStanzaHandler *handler;
    QStringList conditions;
    Direction direction;
    int priority;
    Jid streamJid;
  };
protected:
  virtual bool checkCondition(const QDomElement &AElem, const QString &ACondition, const int APos = 0);
  virtual bool processStanzaIn(const Jid &AStreamJid, Stanza *AStanza);
  virtual bool processStanzaOut(const Jid &AStreamJid, Stanza *AStanza);
  virtual bool processIqStanza(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void removeIqStanzaItem(const QString &AStanzaId);
protected slots:
  void onStreamElement(IXmppStream *AStream, const QDomElement &AElem);
  void onStreamJidChanged(IXmppStream *AStream, const Jid &ABefour);
  void onIqStanzaTimeOut();
  void onHandlerOwnerDestroyed(QObject *AOwner);
  void onIqStanzaOwnerDestroyed(QObject *AOwner);
private:
  IXmppStreams *FXmppStreams;
private:
  QHash<QString, IqStanzaItem> FIqStanzaItems;
  QHash<int, HandlerItem> FHandlerItems;
  QMultiMap<int,int> FHandlerIdsByPriority;
};

#endif